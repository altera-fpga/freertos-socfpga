/*
 * SPDX-FileCopyrightText: Copyright (C) 2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * Ethernet multi block task
 */

#include "FreeRTOS.h"
#include "task.h"
#include "SocfpgaNetworkInterface.h"
#include "osal.h"
#include "osal_log.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define TCP_RCV_SERVER_PORT 9640
#define TCP_SND_SERVER_PORT 9650
#define EMAC_INSTANCE_2     2
#define HET_STRESS_ETHERNET_BUF_SIZE 256
#define HET_STRESS_ETH_TASK_PRIORITY (configMAX_PRIORITIES - 2)

TaskHandle_t RcvTaskHandle = NULL;

static uint8_t ucBuffer[3000];

static const uint8_t ucMAC[6] = { 0xAA, 0xDA, 0xAF, 0xB1, 0x6A, 0xC0 };

// static const uint8_t ucIPAddress[4] = { 192, 168, 1, 6 };
// static const uint8_t ucNetMask[4] = { 255, 255, 255, 0 };
// static const uint8_t ucGateway[4] = { 192, 168, 1, 1 };
// static const uint8_t ucDNSServer[ 4 ] = { 192, 168, 1, 1 };

static const uint8_t ucIPAddress[4] = { 192, 168, 9, 101 };
static const uint8_t ucNetMask[4] = { 255, 255, 255, 0 };
static const uint8_t ucGateway[4] = { 192, 168, 9, 100 };
static const uint8_t ucDNSServer[ 4 ] = { 192, 168, 9, 100 };

static NetworkInterface_t xInterfaces[1];
static NetworkEndPoint_t xEndPoints[1];

static const TickType_t receive_timeout = portMAX_DELAY;
static volatile bool eth_network_up = false;

extern uint32_t ulGetPhysicalCoreMpidr(void);

struct EthernetXferState
{
    struct freertos_sockaddr rcv_server_address, rcv_client_address;
    Socket_t rcv_listening_socket, rcv_connected_socket;

    struct freertos_sockaddr snd_server_address, snd_client_address;
    Socket_t snd_listening_socket, snd_connected_socket;

    volatile bool stopRequested;
    int rdErrCnt;
    int wrErrCnt;
    int wrCnt;
    int rdCnt;
};

uint8_t ethernet_wr_buf[HET_STRESS_ETHERNET_BUF_SIZE + 128] __attribute__((aligned(64)));
uint8_t ethernet_rd_buf[HET_STRESS_ETHERNET_BUF_SIZE + 128] __attribute__((aligned(64)));
uint8_t rd_data_buffer[HET_STRESS_ETHERNET_BUF_SIZE + 128] __attribute__((aligned(64)));
uint8_t wr_data_buffer[HET_STRESS_ETHERNET_BUF_SIZE + 128] __attribute__((aligned(64)));

static struct EthernetXferState ethernetState;
static volatile bool eth_rx_stopped = false;

static volatile uint32_t hetero_eth_error_count = 0;
static volatile uint32_t hetero_eth_rx_error_count = 0;
static volatile uint32_t hetero_eth_short_rx_count = 0;
static volatile uint32_t hetero_eth_verify_error_count = 0;
static volatile uint32_t hetero_eth_tx_error_count = 0;
static volatile uint32_t hetero_eth_success_count = 0;
static volatile uint32_t hetero_eth_current_core = 0;
static volatile bool hetero_eth_stop_requested = false;
static volatile bool hetero_eth_stopped = false;

static void setup_static_demo(void)
{
    (void)pxFillInterfaceDescriptor(EMAC_INSTANCE_2, &(xInterfaces[0]));

    FreeRTOS_FillEndPoint(
            &(xInterfaces[0]),
            &(xEndPoints[0]),
            ucIPAddress,
            ucNetMask,
            ucGateway,
            ucDNSServer,
            ucMAC);

    xEndPoints[0].bits.bIPv6 = pdFALSE_UNSIGNED;
    (void)FreeRTOS_IPInit_Multi();
}

eDHCPCallbackAnswer_t xApplicationDHCPHook_Multi(
        eDHCPCallbackPhase_t eDHCPPhase,
        struct xNetworkEndPoint *pxEndPoint,
        IP_Address_t *pxIPAddress)
{
    (void)eDHCPPhase;
    (void)pxEndPoint;
    (void)pxIPAddress;
    return eDHCPContinue;
}

uint32_t ulApplicationTimeHook(void)
{
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void vApplicationIPNetworkEventHook_Multi(eIPCallbackEvent_t eNetworkEvent,
        struct xNetworkEndPoint *pxEndPoint)
{
    if (eNetworkEvent == eNetworkUp)
    {
        uint32_t pulIPAddress, pulNetMask, pulGatewayAddress,
                pulDNSServerAddress;

        if ((pxEndPoint->bits.bIPv6 == 0) &&
                (pxEndPoint->bits.bEndPointUp == 1))
        {
            FreeRTOS_GetEndPointConfiguration(&pulIPAddress, &pulNetMask,
                    &pulGatewayAddress, &pulDNSServerAddress, pxEndPoint);

            printf("\r\nIPv4 Network interface test is up");

            FreeRTOS_inet_ntoa(pulIPAddress, (char *)ucBuffer);
            printf("\r\nIPv4 Address: %s", ucBuffer);
            FreeRTOS_inet_ntoa(pulNetMask, (char *)ucBuffer);
            printf("\r\nIPv4 Subnet Mask: %s", ucBuffer);
            FreeRTOS_inet_ntoa(pulGatewayAddress, (char *)ucBuffer);
            printf("\r\nIPv4 Gateway Address: %s", ucBuffer);
            FreeRTOS_inet_ntoa(pulDNSServerAddress, (char *)ucBuffer);

            eth_network_up = true;
            if (RcvTaskHandle != NULL)
            {
                xTaskNotifyGive(RcvTaskHandle);
            }
        }
    }
}

uint32_t ulApplicationGetNextSequenceNumber(uint32_t ulSourceAddress,
        uint16_t usSourcePort, uint32_t ulDestinationAddress,
        uint16_t usDestinationPort)
{
    uint32_t ulReturn;
    uint32_t ulRandomNumber;
    ulReturn = ulSourceAddress ^ ulDestinationAddress;
    ulReturn += (uint32_t)usSourcePort + (uint32_t)usDestinationPort;
    if (xApplicationGetRandomNumber(&ulRandomNumber) == pdPASS)
        ulReturn ^= ulRandomNumber;

    return ulRandomNumber;
}

BaseType_t xApplicationGetRandomNumber(uint32_t *pulNumber)
{
    if (pulNumber == NULL)
        return pdFAIL;

    *pulNumber = (uint32_t)rand();

    return pdPASS;
}

const char *pcApplicationHostnameHook(void)
{
    return "AGLEX5_device";
}

void vApplicationPingReplyHook(ePingReplyStatus_t eStatus,
        uint16_t usIdentifier)
{
    (void)eStatus;
    (void)usIdentifier;
}

eDHCPCallbackAnswer_t xApplicationDHCPHook(eDHCPCallbackPhase_t eDHCPPhase,
        uint32_t ulIPAddress)
{
    (void)ulIPAddress;
    (void)eDHCPPhase;

    return eDHCPContinue;
}

static void ethernet_receiver_thread(void *arg)
{
    (void)arg;
    const BaseType_t xBacklog = 20;
    socklen_t xClientSize = sizeof(ethernetState.rcv_client_address);
    BaseType_t xReceivedBytes = 0;

    if (!eth_network_up)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }

    ethernetState.rcv_listening_socket = FreeRTOS_socket(FREERTOS_AF_INET,
            FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
    configASSERT(ethernetState.rcv_listening_socket != FREERTOS_INVALID_SOCKET);

    FreeRTOS_setsockopt(ethernetState.rcv_listening_socket, 0,
            FREERTOS_SO_RCVTIMEO, &receive_timeout, sizeof(receive_timeout));

    ethernetState.rcv_server_address.sin_port = FreeRTOS_htons(TCP_RCV_SERVER_PORT);
    ethernetState.rcv_server_address.sin_family = FREERTOS_AF_INET;
    ethernetState.rcv_server_address.sin_address.ulIP_IPv4 = FreeRTOS_GetIPAddress();
    FreeRTOS_bind(ethernetState.rcv_listening_socket,
            &ethernetState.rcv_server_address,
            sizeof(ethernetState.rcv_server_address));

    FreeRTOS_listen(ethernetState.rcv_listening_socket, xBacklog);

    ethernetState.rcv_connected_socket = FreeRTOS_accept(ethernetState.rcv_listening_socket,
            &ethernetState.rcv_client_address, &xClientSize);
    configASSERT(ethernetState.rcv_connected_socket != FREERTOS_INVALID_SOCKET);

    if (ethernetState.rcv_connected_socket != FREERTOS_INVALID_SOCKET)
    {
        int total_bytes = 0;
        uint64_t remaining_bytes = HET_STRESS_ETHERNET_BUF_SIZE;

        while (ethernetState.stopRequested != 1)
        {
            memset(ethernet_rd_buf, 0x0, HET_STRESS_ETHERNET_BUF_SIZE);
            memset(rd_data_buffer, 0x55, HET_STRESS_ETHERNET_BUF_SIZE);
            remaining_bytes = HET_STRESS_ETHERNET_BUF_SIZE;
            total_bytes = 0;
            while (total_bytes < HET_STRESS_ETHERNET_BUF_SIZE)
            {
                xReceivedBytes = FreeRTOS_recv(ethernetState.rcv_connected_socket,
                        rd_data_buffer, remaining_bytes, 0);
                if (xReceivedBytes <= 0)
                {
                    ethernetState.rdErrCnt++;
                    hetero_eth_error_count++;
                    hetero_eth_rx_error_count++;
                    PRINT("eth rx err got %d exp %d",
                          total_bytes, HET_STRESS_ETHERNET_BUF_SIZE);
                    break;
                }
                for (int i = 0; i < xReceivedBytes; i++)
                    ethernet_rd_buf[i + total_bytes] = rd_data_buffer[i];
                remaining_bytes -= xReceivedBytes;
                total_bytes += xReceivedBytes;
            }
            if (total_bytes != HET_STRESS_ETHERNET_BUF_SIZE)
            {
                ethernetState.rdErrCnt++;
                hetero_eth_error_count++;
                hetero_eth_short_rx_count++;
                osal_task_delay(1);
                continue;
            }
            int total_sent = 0;
            while (total_sent < HET_STRESS_ETHERNET_BUF_SIZE)
            {
                BaseType_t xBytesWritten = FreeRTOS_send(
                        ethernetState.rcv_connected_socket,
                        ethernet_rd_buf + total_sent,
                        HET_STRESS_ETHERNET_BUF_SIZE - total_sent, 0);
                if (xBytesWritten <= 0)
                {
                    ethernetState.wrErrCnt++;
                    hetero_eth_error_count++;
                    hetero_eth_tx_error_count++;
                    PRINT("eth tx err sent %d exp %d",
                          total_sent, HET_STRESS_ETHERNET_BUF_SIZE);
                    osal_task_delay(1);
                    break;
                }
                total_sent += xBytesWritten;
            }
            if (total_sent < HET_STRESS_ETHERNET_BUF_SIZE)
            {
                ethernetState.wrErrCnt++;
                hetero_eth_error_count++;
                hetero_eth_tx_error_count++;
                PRINT("eth tx short sent %d exp %d",
                      total_sent, HET_STRESS_ETHERNET_BUF_SIZE);
                osal_task_delay(1);
                continue;
            }
            ethernetState.rdCnt++;
            hetero_eth_success_count++;
            osal_task_delay(5);
        }
    }
    FreeRTOS_closesocket(ethernetState.rcv_connected_socket);
    eth_rx_stopped = true;
    vTaskDelete(NULL);
}

void hetero_ethernet_task(void *arg)
{
    BaseType_t xReturn;

    (void)arg;

    ethernetState.stopRequested = false;
    ethernetState.rdErrCnt = 0;
    ethernetState.wrErrCnt = 0;
    ethernetState.wrCnt = 0;
    ethernetState.rdCnt = 0;
    eth_rx_stopped = false;

    setup_static_demo();

    xReturn = xTaskCreate(ethernet_receiver_thread, "Het_Eth_Rx",
            configMINIMAL_STACK_SIZE, NULL,
            HET_STRESS_ETH_TASK_PRIORITY, &RcvTaskHandle);
    if (xReturn != pdPASS)
    {
        ERROR("Failed to create Ethernet receiver task");
        vTaskDelete(NULL);
    }
    else if (eth_network_up)
    {
        xTaskNotifyGive(RcvTaskHandle);
    }

    while (!hetero_eth_stop_requested)
    {
        hetero_eth_current_core = ulGetPhysicalCoreMpidr();
        osal_task_delay(100);
    }

    ethernetState.stopRequested = true;
    while (!eth_rx_stopped)
    {
        osal_task_delay(100);
    }
    hetero_eth_stopped = true;
    vTaskDelete(NULL);
}

void hetero_ethernet_request_stop(void)
{
    hetero_eth_stop_requested = true;
}

void hetero_ethernet_print_status(void)
{
    PRINT("Ethernet: physical core 0x%lx ok %lu errors %lu",
          (unsigned long)hetero_eth_current_core,
          (unsigned long)hetero_eth_success_count,
          (unsigned long)hetero_eth_error_count);
}
