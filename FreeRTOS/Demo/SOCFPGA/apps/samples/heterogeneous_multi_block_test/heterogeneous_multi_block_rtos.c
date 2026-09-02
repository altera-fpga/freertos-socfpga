/*
 * SPDX-FileCopyrightText: Copyright (C) 2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * FreeRTOS constructs multi block tasks
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include "stream_buffer.h"
#include "message_buffer.h"

#include "osal.h"
#include "osal_log.h"

#define RTOS_QUEUE_LENGTH        16U
#define RTOS_QUEUE_WAIT_TICKS    portMAX_DELAY
#define RTOS_MUTEX_WAIT_TICKS    pdMS_TO_TICKS(100U)
#define RTOS_SUCCESS_DELAY_MS    10U

#define RTOS_TEST_ENABLE_QUEUE   1
#define RTOS_TEST_ENABLE_STREAM  1
#define RTOS_TEST_ENABLE_EVENT   1
#define RTOS_TEST_ENABLE_MUTEX   1
#define RTOS_TEST_ENABLE_MESSAGE 1

#define RTOS_EVENT_BIT_PING      (1U << 0)
#define RTOS_EVENT_BIT_PONG      (1U << 1)
#define RTOS_STREAM_ITEM_SIZE    sizeof(uint32_t)
#define RTOS_STREAM_BUFFER_SIZE  (RTOS_STREAM_ITEM_SIZE * 16U)
#define RTOS_MESSAGE_ITEM_SIZE   sizeof(uint32_t)
#define RTOS_MESSAGE_BUFFER_SIZE (RTOS_MESSAGE_ITEM_SIZE * 8U)

static QueueHandle_t rtos_queue;
static EventGroupHandle_t rtos_events;
static SemaphoreHandle_t rtos_mutex;
static StreamBufferHandle_t rtos_stream;
static MessageBufferHandle_t rtos_message;
static volatile uint32_t rtos_shared_counter = 0;

static volatile bool rtos_queue_stopped = false;
static volatile bool rtos_event_stopped = false;
static volatile bool rtos_mutex_stopped = false;
static volatile bool rtos_stream_tx_stopped = false;
static volatile bool rtos_stream_rx_stopped = false;
static volatile bool rtos_msg_tx_stopped = false;
static volatile bool rtos_msg_rx_stopped = false;

static volatile uint32_t hetero_rtos_success_count = 0;
static volatile uint32_t hetero_rtos_error_count = 0;
static volatile uint32_t hetero_rtos_current_core = 0;
static volatile bool hetero_rtos_stop_requested = false;
static volatile bool hetero_rtos_stopped = false;

extern uint32_t ulGetPhysicalCoreMpidr(void);

static void update_rtos_stopped(void)
{
    if (
#if RTOS_TEST_ENABLE_QUEUE
        rtos_queue_stopped &&
#endif
#if RTOS_TEST_ENABLE_EVENT
        rtos_event_stopped &&
#endif
#if RTOS_TEST_ENABLE_MUTEX
        rtos_mutex_stopped &&
#endif
#if RTOS_TEST_ENABLE_STREAM
        rtos_stream_tx_stopped && rtos_stream_rx_stopped &&
#endif
#if RTOS_TEST_ENABLE_MESSAGE
        rtos_msg_tx_stopped && rtos_msg_rx_stopped &&
#endif
        true)
    {
        hetero_rtos_stopped = true;
    }
}

#if RTOS_TEST_ENABLE_QUEUE
#if RTOS_TEST_ENABLE_QUEUE
static void rtos_queue_producer(void *arg)
{
    uint32_t value = 0;

    (void)arg;

    while (!hetero_rtos_stop_requested)
    {
        hetero_rtos_current_core = ulGetPhysicalCoreMpidr();
        if (xQueueSend(rtos_queue, &value, RTOS_QUEUE_WAIT_TICKS) == pdPASS)
        {
            hetero_rtos_success_count++;
            value++;
        }
        osal_task_delay(10);
    }

    rtos_queue_stopped = true;
    update_rtos_stopped();
    vTaskDelete(NULL);
}

static void rtos_queue_consumer(void *arg)
{
    uint32_t value = 0;
    uint32_t expected = 0;

    (void)arg;

    while (!hetero_rtos_stop_requested)
    {
        hetero_rtos_current_core = ulGetPhysicalCoreMpidr();
        if (xQueueReceive(rtos_queue, &value, RTOS_QUEUE_WAIT_TICKS) == pdPASS)
        {
            if (value != expected)
            {
                hetero_rtos_error_count++;
                expected = value + 1U;
            }
            else
            {
                expected++;
                hetero_rtos_success_count++;
            }
        }
        osal_task_delay(10);
    }

    rtos_queue_stopped = true;
    update_rtos_stopped();
    vTaskDelete(NULL);
}

#endif
#endif

#if RTOS_TEST_ENABLE_EVENT
#if RTOS_TEST_ENABLE_EVENT
static void rtos_event_ping(void *arg)
{
    (void)arg;

    while (!hetero_rtos_stop_requested)
    {
        hetero_rtos_current_core = ulGetPhysicalCoreMpidr();
        xEventGroupSetBits(rtos_events, RTOS_EVENT_BIT_PING);
        if ((xEventGroupWaitBits(rtos_events, RTOS_EVENT_BIT_PONG,
                pdTRUE, pdTRUE, RTOS_QUEUE_WAIT_TICKS) & RTOS_EVENT_BIT_PONG) != 0U)
        {
            hetero_rtos_success_count++;
        }
        else
        {
            hetero_rtos_error_count++;
        }
        osal_task_delay(10);
    }

    rtos_event_stopped = true;
    update_rtos_stopped();
    vTaskDelete(NULL);
}

static void rtos_event_pong(void *arg)
{
    (void)arg;

    while (!hetero_rtos_stop_requested)
    {
        hetero_rtos_current_core = ulGetPhysicalCoreMpidr();
        if ((xEventGroupWaitBits(rtos_events, RTOS_EVENT_BIT_PING,
                pdTRUE, pdTRUE, RTOS_QUEUE_WAIT_TICKS) & RTOS_EVENT_BIT_PING) != 0U)
        {
            xEventGroupSetBits(rtos_events, RTOS_EVENT_BIT_PONG);
            hetero_rtos_success_count++;
        }
        else
        {
            hetero_rtos_error_count++;
        }
        osal_task_delay(10);
    }

    rtos_event_stopped = true;
    update_rtos_stopped();
    vTaskDelete(NULL);
}

#endif
#endif

#if RTOS_TEST_ENABLE_MUTEX
#if RTOS_TEST_ENABLE_MUTEX
static void rtos_mutex_task(void *arg)
{
    (void)arg;

    while (!hetero_rtos_stop_requested)
    {
        hetero_rtos_current_core = ulGetPhysicalCoreMpidr();
        if (xSemaphoreTake(rtos_mutex, RTOS_MUTEX_WAIT_TICKS) == pdPASS)
        {
            rtos_shared_counter++;
            xSemaphoreGive(rtos_mutex);
            hetero_rtos_success_count++;
        }
        else
        {
            hetero_rtos_error_count++;
        }
        osal_task_delay(10);
    }

    rtos_mutex_stopped = true;
    update_rtos_stopped();
    vTaskDelete(NULL);
}

#endif
#endif

#if RTOS_TEST_ENABLE_STREAM
#if RTOS_TEST_ENABLE_STREAM
static void rtos_stream_sender(void *arg)
{
    uint32_t value = 0;

    (void)arg;

    while (!hetero_rtos_stop_requested)
    {
        hetero_rtos_current_core = ulGetPhysicalCoreMpidr();
        size_t sent_total = 0;
        const uint8_t *src = (const uint8_t *)&value;

        while (sent_total < RTOS_STREAM_ITEM_SIZE)
        {
            size_t sent = xStreamBufferSend(rtos_stream,
                    src + sent_total,
                    RTOS_STREAM_ITEM_SIZE - sent_total,
                    RTOS_QUEUE_WAIT_TICKS);
            if (sent == 0U)
            {
                break;
            }
            sent_total += sent;
        }

        if (sent_total == RTOS_STREAM_ITEM_SIZE)
        {
            hetero_rtos_success_count++;
            value++;
        }
        osal_task_delay(10);
    }

    rtos_stream_tx_stopped = true;
    update_rtos_stopped();
    vTaskDelete(NULL);
}

static void rtos_stream_receiver(void *arg)
{
    uint32_t value = 0;
    uint32_t expected = 0;

    (void)arg;

    while (!hetero_rtos_stop_requested)
    {
        hetero_rtos_current_core = ulGetPhysicalCoreMpidr();
        size_t received_total = 0;
        uint8_t *dst = (uint8_t *)&value;

        while (received_total < RTOS_STREAM_ITEM_SIZE)
        {
            size_t received = xStreamBufferReceive(rtos_stream,
                    dst + received_total,
                    RTOS_STREAM_ITEM_SIZE - received_total,
                    RTOS_QUEUE_WAIT_TICKS);
            if (received == 0U)
            {
                break;
            }
            received_total += received;
        }

        if (received_total == RTOS_STREAM_ITEM_SIZE)
        {
            if (value != expected)
            {
                hetero_rtos_error_count++;
                expected = value + 1U;
            }
            else
            {
                expected++;
                hetero_rtos_success_count++;
            }
        }
        osal_task_delay(10);
    }

    rtos_stream_rx_stopped = true;
    update_rtos_stopped();
    vTaskDelete(NULL);
}
#endif
#endif

#if RTOS_TEST_ENABLE_MESSAGE
#if RTOS_TEST_ENABLE_MESSAGE
static void rtos_message_sender(void *arg)
{
    uint32_t value = 0;

    (void)arg;

    while (!hetero_rtos_stop_requested)
    {
        hetero_rtos_current_core = ulGetPhysicalCoreMpidr();
        if (xMessageBufferSend(rtos_message, &value, RTOS_MESSAGE_ITEM_SIZE,
                RTOS_QUEUE_WAIT_TICKS) == RTOS_MESSAGE_ITEM_SIZE)
        {
            hetero_rtos_success_count++;
            value++;
        }
        else
        {
            hetero_rtos_error_count++;
        }
        osal_task_delay(10);
    }

    rtos_msg_tx_stopped = true;
    update_rtos_stopped();
    vTaskDelete(NULL);
}

static void rtos_message_receiver(void *arg)
{
    uint32_t value = 0;
    uint32_t expected = 0;

    (void)arg;

    while (!hetero_rtos_stop_requested)
    {
        hetero_rtos_current_core = ulGetPhysicalCoreMpidr();
        if (xMessageBufferReceive(rtos_message, &value, RTOS_MESSAGE_ITEM_SIZE,
                RTOS_QUEUE_WAIT_TICKS) == RTOS_MESSAGE_ITEM_SIZE)
        {
            if (value != expected)
            {
                hetero_rtos_error_count++;
                expected = value + 1U;
            }
            else
            {
                expected++;
                hetero_rtos_success_count++;
            }
        }
        else
        {
            hetero_rtos_error_count++;
        }
        osal_task_delay(10);
    }

    rtos_msg_rx_stopped = true;
    update_rtos_stopped();
    vTaskDelete(NULL);
}
#endif
#endif

void hetero_rtos_task(void *arg)
{
    (void)arg;

    rtos_queue = xQueueCreate(RTOS_QUEUE_LENGTH, sizeof(uint32_t));
    rtos_events = xEventGroupCreate();
    rtos_mutex = xSemaphoreCreateMutex();
    rtos_stream = xStreamBufferCreate(RTOS_STREAM_BUFFER_SIZE, RTOS_STREAM_ITEM_SIZE);
    rtos_message = xMessageBufferCreate(RTOS_MESSAGE_BUFFER_SIZE);

    if (
#if RTOS_TEST_ENABLE_QUEUE
        (rtos_queue == NULL) ||
#endif
#if RTOS_TEST_ENABLE_EVENT
        (rtos_events == NULL) ||
#endif
#if RTOS_TEST_ENABLE_MUTEX
        (rtos_mutex == NULL) ||
#endif
#if RTOS_TEST_ENABLE_STREAM
        (rtos_stream == NULL) ||
#endif
#if RTOS_TEST_ENABLE_MESSAGE
        (rtos_message == NULL) ||
#endif
        false)
    {
        ERROR("Failed to create RTOS constructs");
        hetero_rtos_error_count++;
        vTaskDelete(NULL);
    }

#if RTOS_TEST_ENABLE_QUEUE
    (void)xTaskCreate(rtos_queue_producer, "RTOS_Q_Prod",
            configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    (void)xTaskCreate(rtos_queue_consumer, "RTOS_Q_Cons",
            configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
#endif
#if RTOS_TEST_ENABLE_EVENT
    (void)xTaskCreate(rtos_event_ping, "RTOS_E_Ping",
            configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    (void)xTaskCreate(rtos_event_pong, "RTOS_E_Pong",
            configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
#endif
#if RTOS_TEST_ENABLE_MUTEX
    (void)xTaskCreate(rtos_mutex_task, "RTOS_Mutex",
            configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
#endif
#if RTOS_TEST_ENABLE_STREAM
    (void)xTaskCreate(rtos_stream_sender, "RTOS_SB_Tx",
            configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    (void)xTaskCreate(rtos_stream_receiver, "RTOS_SB_Rx",
            configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
#endif
#if RTOS_TEST_ENABLE_MESSAGE
    (void)xTaskCreate(rtos_message_sender, "RTOS_MB_Tx",
            configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    (void)xTaskCreate(rtos_message_receiver, "RTOS_MB_Rx",
            configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
#endif

    while (!hetero_rtos_stop_requested)
    {
        osal_task_delay(10);
    }

    while (!hetero_rtos_stopped)
    {
        osal_task_delay(10);
    }

    vTaskDelete(NULL);
}

void hetero_rtos_request_stop(void)
{
    hetero_rtos_stop_requested = true;
}

void hetero_rtos_print_status(void)
{
    PRINT("RTOS: physical core 0x%lx ok %lu errors %lu",
          (unsigned long)hetero_rtos_current_core,
          (unsigned long)hetero_rtos_success_count,
          (unsigned long)hetero_rtos_error_count);
}
