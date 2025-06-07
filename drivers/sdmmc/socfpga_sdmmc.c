/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * HAL driver implementation for SDMMC
 */
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include "osal.h"
#include "osal_log.h"
#include "socfpga_cache.h"
#include "socfpga_interrupt.h"
#include "socfpga_sdmmc_ll.h"
#include "socfpga_sdmmc.h"

#define DEF_SPEED_EN            1
#define DEF_SPEED_DI            0

#define DEV_TYPE_SD             0
#define DEV_TYPE_EMMC           1

/*specify max descriptor count here
 * 1 descriptor can handle up to 64KB of data
 */
#define SDMMC_MAX_DESCRIPTOR    160U
/* specify your device here */
#define DEV_TYPE                DEV_TYPE_SD

/*INFO:
 * Driver supports High speed[speed upto 25 MBps at 50 MHz] and
 * Default speed[speed upto 12.5 MBps at 25 MHz] mode.
 **/

#define SUPPORT_DEF_SPEED    DEF_SPEED_DI

#if SDMMC_MAX_DESCRIPTOR < 1
#error Invalid descriptor count
#endif

#if (DEV_TYPE ==  DEV_TYPE_SD)
#define BUS_PARAM    0
static eErrorState sd_mmc_init( uint64_t *SecNum );
static eErrorState sd_go_idle( CmdParameters_t *pxCmd );
static eErrorState sd_snd_if_cond( CmdParameters_t *pxCmd );
static eErrorState sd_snd_app_cmd( CmdParameters_t *pxCmd );
static eErrorState sd_check_ocr( CmdParameters_t *pxCmd );
static eErrorState sd_snd_all_cid( CmdParameters_t *pxCmd );
static eErrorState sd_snd_rel_add( CmdParameters_t *pxCmd );
static eErrorState sd_sel_card( CmdParameters_t *pxCmd );
static eErrorState sd_snd_csd( CmdParameters_t *pxCmd );
static eErrorState sd_en_card_ready( CmdParameters_t *pxCmdHandle );
static eErrorState sd_snd_app_bus_cmd( CmdParameters_t *pxCmd );
static eErrorState sd_set_bus_width( CmdParameters_t *pxCmdHandle );
static eErrorState sd_bus_width_4( CmdParameters_t *pxCmd );

#elif (DEV_TYPE ==  DEV_TYPE_EMMC)
#define BUS_PARAM    1
static eErrorState sd_mmc_init( uint64_t *SecNum );
static eErrorState mmc_go_idle( CmdParameters_t *pxCmd );
static eErrorState mmc_snd_all_cid( CmdParameters_t *pxCmd );
static eErrorState mmc_set_rel_add( CmdParameters_t *pxCmd );
static eErrorState mmc_check_ocr( CmdParameters_t *pxCmd );
static eErrorState mmc_sel_card( CmdParameters_t *pxCmd );
static eErrorState mmc_snd_csd( CmdParameters_t *pxCmd );
static eErrorState mmc_send_ext_csd( CmdParameters_t *pxCmd,
        uint64_t*sector_count_ref);
static eErrorState mmc_switch_bus_width( CmdParameters_t *pxCmd );

static uint8_t ext_csd_buff[512];

#else
#error "Device not supported"
#endif

static eErrorState sdmmc_setup_host( void );

static void sdmmc_wait_xfer_done( void );
void sdmmc_irq_handler( void *data );
static void sdmmc_wait_cmd_done( void );

static CardData_t *pxCardSpecificData;
static CardData_t xCardData;

static osal_semaphore_def_t osal_def_xfer;
static osal_semaphore_def_t osal_def_cmd;

struct SdmmcContext
{
    bool is_api_sync;
    eErrorState StatusCode;
    osal_semaphore_t xSemaphoreXfer;
    osal_semaphore_t xSemaphoreCmd;
    sdmmc_cb_fun xfer_call_back;
    DmaDescriptor_t dma_descriptor[ SDMMC_MAX_DESCRIPTOR ];
    uint32_t is_def_speed_supported;
    uint32_t dev_type;
};

static struct SdmmcContext sdmmc_descriptor;

int32_t sdmmc_read_block_async( uint64_t *pReadBuffer, uint64_t ulReadAddr,
        uint32_t ulBlockSize, uint32_t ulNumberOfBlocks,
        sdmmc_cb_fun xfer_done_call_back)
{
    CmdParameters_t *pxCmd;
    CmdParameters_t xCommandConfig;
    pxCmd = &xCommandConfig;
    uint32_t req_desc_count;
    eErrorState state;
    sdmmc_descriptor.StatusCode = 0;

    sdmmc_descriptor.is_api_sync = false;
    sdmmc_descriptor.xfer_call_back = xfer_done_call_back;

    if (ulBlockSize == 0U)
    {
        return -EINVAL;
    }

    /*convert address into block number*/
    ulReadAddr /= ulBlockSize;

    if ((pReadBuffer == NULL) || (ulNumberOfBlocks == 0U)
            || (ulBlockSize == 0U))
    {
        return -EINVAL;
    }
    /*send cmd to set block size*/
    pxCmd->ulArgument = ulBlockSize;
    pxCmd->ucCommandIndex = SDMMC_CMD_SET_BLOCK_LEN;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_NOT_PST;
    pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_EN;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_EN;
    state = sdmmc_send_command(pxCmd);

    if (state != 0)
    {
        return -EIO;
    }

    sdmmc_wait_cmd_done();

    if (sdmmc_descriptor.StatusCode != 0)
    {
        return -EIO;
    }
    /*argument preparation for data read*/
    if (ulNumberOfBlocks > 1U)
    {
        pxCmd->ulArgument = ulReadAddr;
        pxCmd->ucCommandIndex = SDMMC_CMD_READ_MULT_BLOCK;
        pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_PST;
        pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE;
        pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_EN;
        pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_EN;
    }
    else
    {
        pxCmd->ulArgument = ulReadAddr;
        pxCmd->ucCommandIndex = SDMMC_CMD_READ_SINGLE_BLOCK;
        pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_PST;
        pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE;
        pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_EN;
        pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_EN;
    }
    /*calculate no of descriptors required*/
    req_desc_count = ((ulBlockSize * ulNumberOfBlocks) + (DESC_MAX_XFER_SIZE)
            -1U) / DESC_MAX_XFER_SIZE;

    if (req_desc_count > SDMMC_MAX_DESCRIPTOR)
    {
        return -EIO;
    }

    /*send cmd to request data from the card*/
    else
    {
        sdmmc_set_up_xfer(sdmmc_descriptor.dma_descriptor, pReadBuffer,
                ulBlockSize, ulNumberOfBlocks);
        sdmmc_set_xfer_config(pxCmd);
        DEBUG("Initiating sdmmc data read");
        state = sdmmc_send_command(pxCmd);
        if (state != 0)
        {
            return -EIO;
        }

        return 0;
    }
}

int32_t sdmmc_read_block_sync( uint64_t *pReadBuffer, uint64_t ulReadAddr,
        uint32_t ulBlockSize, uint32_t ulNumberOfBlocks )
{
    sdmmc_descriptor.is_api_sync = false;
    CmdParameters_t *pxCmd;
    CmdParameters_t xCommandConfig;
    pxCmd = &xCommandConfig;
    uint32_t req_desc_count;
    eErrorState state;

    sdmmc_descriptor.is_api_sync = true;
    if (ulBlockSize == 0U)
    {
        return -EINVAL;
    }

    /*convert address into block number*/
    ulReadAddr /= ulBlockSize;

    if ((pReadBuffer == NULL) || (ulNumberOfBlocks == 0U)
            || (ulBlockSize == 0U))
    {
        return -EINVAL;
    }
    /*send cmd to set block size*/
    pxCmd->ulArgument = ulBlockSize;
    pxCmd->ucCommandIndex = SDMMC_CMD_SET_BLOCK_LEN;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_NOT_PST;
    pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_EN;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_EN;
    state = sdmmc_send_command(pxCmd);

    if (state != 0)
    {
        return -EIO;
    }

    sdmmc_wait_cmd_done();

    if (sdmmc_descriptor.StatusCode != 0)
    {
        return -EIO;
    }
    /*argument preparation for data read*/
    if (ulNumberOfBlocks > 1U)
    {
        pxCmd->ulArgument = ulReadAddr;
        pxCmd->ucCommandIndex = SDMMC_CMD_READ_MULT_BLOCK;
        pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_PST;
        pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE;
        pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_EN;
        pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_EN;
    }
    else
    {
        pxCmd->ulArgument = ulReadAddr;
        pxCmd->ucCommandIndex = SDMMC_CMD_READ_SINGLE_BLOCK;
        pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_PST;
        pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE;
        pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_EN;
        pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_EN;
    }
    /*calculate no of descriptors required*/
    req_desc_count = ((ulBlockSize * ulNumberOfBlocks) + (DESC_MAX_XFER_SIZE)
            -1U) / DESC_MAX_XFER_SIZE;

    if (req_desc_count > SDMMC_MAX_DESCRIPTOR)
    {
        return -EIO;
    }

    /*send cmd to request data from the card*/
    else
    {
        sdmmc_set_up_xfer(sdmmc_descriptor.dma_descriptor, pReadBuffer,
                ulBlockSize, ulNumberOfBlocks);
        sdmmc_set_xfer_config(pxCmd);
        DEBUG("Initiating sdmmc data read");
        state = sdmmc_send_command(pxCmd);
        if (state != 0)
        {
            return -EIO;
        }

        sdmmc_wait_xfer_done();

        if (sdmmc_descriptor.StatusCode == 0)
        {
            cache_force_invalidate((uint64_t*) pReadBuffer,
                    ulBlockSize * ulNumberOfBlocks);
            DEBUG("Read %x blocks",ulNumberOfBlocks);

            return 0;
        }
        else
        {
            return -EIO;
        }
    }
}

int32_t sdmmc_write_block_async( uint64_t *pWriteBuffer,
        uint64_t ulWriteAddr,
        uint32_t ulBlockSize, uint32_t ulNumberOfBlocks,
        sdmmc_cb_fun xfer_done_call_back)
{
    CmdParameters_t *pxCmd;
    CmdParameters_t xCommandConfig;
    pxCmd = &xCommandConfig;
    uint32_t req_desc_count;
    eErrorState state;

    sdmmc_descriptor.is_api_sync = false;
    sdmmc_descriptor.xfer_call_back = xfer_done_call_back;

    if (ulBlockSize == 0U)
    {
        return -EINVAL;
    }

    /*converts address into block number*/
    ulWriteAddr /= ulBlockSize;

    if ((pWriteBuffer == NULL) || (ulNumberOfBlocks == 0U)
            || (ulBlockSize == 0U))
    {
        return -EINVAL;
    }
    /*set cmd to set block size*/
    pxCmd->ulArgument = ulBlockSize;
    pxCmd->ucCommandIndex = SDMMC_CMD_SET_BLOCK_LEN;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_NOT_PST;
    pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_EN;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_EN;
    state = sdmmc_send_command(pxCmd);

    if (state != 0)
    {
        return -EIO;
    }

    sdmmc_wait_cmd_done();

    if (sdmmc_descriptor.StatusCode != 0)
    {
        return -EIO;
    }
    /*argument preparation for data transfer*/
    if (ulNumberOfBlocks > 1U)
    {
        pxCmd->ulArgument = ulWriteAddr;
        pxCmd->ucCommandIndex = SDMMC_CMD_WRITE_MULT_BLOCK;
        pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_PST;
        pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE;
        pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_EN;
        pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_EN;
    }
    else
    {
        pxCmd->ulArgument = ulWriteAddr;
        pxCmd->ucCommandIndex = SDMMC_CMD_WRITE_MULT_BLOCK;
        pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_PST;
        pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE;
        pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_EN;
        pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_EN;
    }
    /*calculate no of descriptors required*/
    req_desc_count = ((ulBlockSize * ulNumberOfBlocks) + (DESC_MAX_XFER_SIZE)
            -1U) / DESC_MAX_XFER_SIZE;
    if (req_desc_count > SDMMC_MAX_DESCRIPTOR)
    {
        return -EIO;
    }

    /*send cmd to transfer data to the card*/
    else
    {
        cache_force_write_back((uint64_t*) pWriteBuffer,
                ulBlockSize * ulNumberOfBlocks);
        sdmmc_set_up_xfer(sdmmc_descriptor.dma_descriptor, pWriteBuffer,
                ulBlockSize, ulNumberOfBlocks);
        sdmmc_set_xfer_config(pxCmd);

        state = sdmmc_send_command(pxCmd);
        INFO("Initiating sdmmc data write");
        if (state != 0)
        {
            return -EIO;
        }
    }
    return 0;
}

int32_t sdmmc_write_block_sync( uint64_t *pWriteBuffer,
        uint64_t ulWriteAddr,
        uint32_t ulBlockSize, uint32_t ulNumberOfBlocks )
{
    CmdParameters_t *pxCmd;
    CmdParameters_t xCommandConfig;
    pxCmd = &xCommandConfig;
    uint32_t req_desc_count;
    eErrorState state;

    sdmmc_descriptor.is_api_sync = true;
    if (ulBlockSize == 0U)
    {
        return -EINVAL;
    }

    /*converts address into block number*/
    ulWriteAddr /= ulBlockSize;

    if ((pWriteBuffer == NULL) || (ulNumberOfBlocks == 0U)
            || (ulBlockSize == 0U))
    {
        return -EINVAL;
    }
    /*set cmd to set block size*/
    pxCmd->ulArgument = ulBlockSize;
    pxCmd->ucCommandIndex = SDMMC_CMD_SET_BLOCK_LEN;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_NOT_PST;
    pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_EN;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_EN;
    state = sdmmc_send_command(pxCmd);

    if (state != 0)
    {
        return -EIO;
    }

    sdmmc_wait_cmd_done();

    if (sdmmc_descriptor.StatusCode != 0)
    {
        return -EIO;
    }
    /*argument preparation for data transfer*/
    if (ulNumberOfBlocks > 1U)
    {
        pxCmd->ulArgument = ulWriteAddr;
        pxCmd->ucCommandIndex = SDMMC_CMD_WRITE_MULT_BLOCK;
        pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_PST;
        pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE;
        pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_EN;
        pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_EN;
    }
    else
    {
        pxCmd->ulArgument = ulWriteAddr;
        pxCmd->ucCommandIndex = SDMMC_CMD_WRITE_MULT_BLOCK;
        pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_PST;
        pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE;
        pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_EN;
        pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_EN;
    }
    /*calculate no of descriptors required*/
    req_desc_count = ((ulBlockSize * ulNumberOfBlocks) + (DESC_MAX_XFER_SIZE)
            -1U) / DESC_MAX_XFER_SIZE;

    if (req_desc_count > SDMMC_MAX_DESCRIPTOR)
    {
        return -EIO;
    }
    /*send cmd to transfer data to the card*/
    else
    {
        cache_force_write_back((uint64_t*) pWriteBuffer,
                ulBlockSize * ulNumberOfBlocks);
        sdmmc_set_up_xfer(sdmmc_descriptor.dma_descriptor, pWriteBuffer,
                ulBlockSize, ulNumberOfBlocks);
        sdmmc_set_xfer_config(pxCmd);

        state = sdmmc_send_command(pxCmd);
        INFO("Initiating sdmmc data write");
        if (state != 0)
        {
            return -EIO;
        }

        sdmmc_wait_xfer_done();

        if (sdmmc_descriptor.StatusCode == 0)
        {
            INFO("Written %x blocks",ulNumberOfBlocks);
            return 0;
        }
        else
        {
            return -EIO;
        }
    }
}


int32_t sdmmc_init_card( uint64_t *ptrSecNum )
{
    eErrorState ret;
    eErrorState cmd_status;
    SocfpgaInterruptErr_t intr_ret;

    pxCardSpecificData = &xCardData;
    if (pxCardSpecificData == NULL)
    {
        return -EIO;
    }

    if (ptrSecNum == NULL)
    {
        return -EINVAL;
    }

    if (sdmmc_is_card_present() != SDMMC_IS_CARD_DET)
    {
        ERROR("Device detection failed");
        return -EIO;
    }

    sdmmc_descriptor.is_def_speed_supported = SUPPORT_DEF_SPEED;
    sdmmc_descriptor.dev_type = DEV_TYPE;

    sdmmc_descriptor.xSemaphoreXfer = osal_semaphore_create(&osal_def_xfer);
    sdmmc_descriptor.xSemaphoreCmd = osal_semaphore_create(&osal_def_cmd);
    intr_ret = interrupt_register_isr(eSdmmcIrq, sdmmc_irq_handler, NULL);
    if (intr_ret != eErrOk)
    {
        return -EIO;
    }
    intr_ret = interrupt_enable(eSdmmcIrq, GIC_INTERRUPT_PRIORITY_SDMMC);
    if (intr_ret != eErrOk)
    {
        return -EIO;
    }

    ret = sdmmc_setup_host();

    if (ret != CTRL_CONFIG_PASS)
    {
        return -EIO;
    }
    cmd_status = sd_mmc_init(ptrSecNum);

    if (cmd_status == 0)
    {
        return 0;
    }
    else
    {
        return -EIO;
    }
}
#if (DEV_TYPE ==  DEV_TYPE_SD)
/*follows SD association standard*/
static eErrorState sd_mmc_init( uint64_t *SecNum )
{
    CmdParameters_t *pxCmdHandle;
    CmdParameters_t xCommandConfig;
    eErrorState state;

    pxCmdHandle = &xCommandConfig;
    /*send cmd to set the card idle*/
    state = sd_go_idle(pxCmdHandle);
    if (state != 0)
    {
        return -EIO;
    }
    /*send cmd to echo back the argument*/
    state = sd_snd_if_cond(pxCmdHandle);
    if (state != 0)
    {
        return -EIO;
    }
    /*delay required when the combo phy clock -> 200 MHz*/
    osal_task_delay(10);
    /*send cmd to put card into ready state*/
    state = sd_en_card_ready(pxCmdHandle);
    if (state != 0)
    {
        return -EIO;
    }
    sd_get_card_type(pxCardSpecificData);
    /*send cmd to request cid of the card*/
    state = sd_snd_all_cid(pxCmdHandle);
    if (state != 0)
    {
        return -EIO;
    }
    /*send cmd to req the rel card addr of the card*/
    state = sd_snd_rel_add(pxCmdHandle);
    if (state != 0)
    {
        return -EIO;
    }
    sd_read_response_rel_addr(pxCardSpecificData);
    /*send cmd to request csd of the card*/
    state = sd_snd_csd(pxCmdHandle);
    if (state != 0)
    {
        return -EIO;
    }
    *SecNum = sdmmc_read_sector_count();
    /*send cmd to select the card*/
    state = sd_sel_card(pxCmdHandle);
    if (state != 0)
    {
        return -EIO;
    }
    state = sd_set_bus_width(pxCmdHandle);

    return state;
}
static eErrorState sd_go_idle( CmdParameters_t *pxCmd )
{
    eErrorState state;
    pxCmd->ulArgument = SDMMC_NO_CMD_ARG;
    pxCmd->ucCommandIndex = SDMMC_CMD_GO_IDLE_STATE;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_NOT_PST;
    pxCmd->ucResponseType = SDMMC_NO_RESPONSE;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_DI;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_DI;
    state = sdmmc_send_command(pxCmd);
    if (state != 0)
    {
        return state;
    }
    sdmmc_wait_cmd_done();
    return sdmmc_descriptor.StatusCode;
}

static eErrorState sd_snd_if_cond( CmdParameters_t *pxCmd )
{
    eErrorState state;
    pxCmd->ulArgument = SDMMC_ARG_CHECK_PATTERN;
    pxCmd->ucCommandIndex = SDMMC_CMD_SEND_IF_COND;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_NOT_PST;
    pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_EN;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_EN;
    state = sdmmc_send_command(pxCmd);
    if (state != 0)
    {
        return state;
    }
    sdmmc_wait_cmd_done();
    return sdmmc_descriptor.StatusCode;
}

static eErrorState sd_snd_app_cmd( CmdParameters_t *pxCmd )
{
    eErrorState state;
    pxCmd->ulArgument = SDMMC_NO_CMD_ARG;
    pxCmd->ucCommandIndex = SDMMC_CMD_SEND_APP;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_NOT_PST;
    pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_DI;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_DI;
    state = sdmmc_send_command(pxCmd);
    if (state != 0)
    {
        return state;
    }
    sdmmc_wait_cmd_done();
    return sdmmc_descriptor.StatusCode;
}

static eErrorState sd_check_ocr( CmdParameters_t *pxCmd )
{
    eErrorState state;
    pxCmd->ulArgument = SDMMC_ARG_SDHC_OCR;
    pxCmd->ucCommandIndex = SDMMC_CMD_READ_OCR;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_NOT_PST;
    pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_DI;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_DI;
    state = sdmmc_send_command(pxCmd);
    if (state != 0)
    {
        return state;
    }
    sdmmc_wait_cmd_done();
    return sdmmc_descriptor.StatusCode;
}

static eErrorState sd_snd_all_cid( CmdParameters_t *pxCmd )
{
    eErrorState state;
    pxCmd->ulArgument = SDMMC_NO_CMD_ARG;
    pxCmd->ucCommandIndex = SDMMC_CMD_ALL_SEND_CID;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_NOT_PST;
    pxCmd->ucResponseType = SDMMC_LONG_RESPONSE;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_DI;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_DI;
    state = sdmmc_send_command(pxCmd);
    if (state != 0)
    {
        return state;
    }
    sdmmc_wait_cmd_done();
    return sdmmc_descriptor.StatusCode;
}

static eErrorState sd_snd_rel_add( CmdParameters_t *pxCmd )
{
    eErrorState state;
    pxCmd->ulArgument = SDMMC_NO_CMD_ARG;
    pxCmd->ucCommandIndex = SDMMC_CMD_SND_REL_ADDR;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_NOT_PST;
    pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_EN;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_EN;
    state = sdmmc_send_command(pxCmd);
    if (state != 0)
    {
        return state;
    }
    sdmmc_wait_cmd_done();
    return sdmmc_descriptor.StatusCode;

}
static eErrorState sd_en_card_ready( CmdParameters_t *pxCmdHandle )
{
    eErrorState state;

    while (sdmmc_is_card_ready() == 0U)
    {
        state = sd_snd_app_cmd(pxCmdHandle);
        if (state != 0)
        {
            return -EIO;
        }
        state = sd_check_ocr(pxCmdHandle);
        if (state != 0)
        {
            return -EIO;
        }
    }
    return 0;
}

static eErrorState sd_sel_card( CmdParameters_t *pxCmd )
{
    eErrorState state;
    pxCmd->ulArgument = (pxCardSpecificData->usRelativeAddress)
            << SDMMC_ARG_MASK_REL_ADD;
    pxCmd->ucCommandIndex = SDMMC_CMD_SELECT_CARD;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_NOT_PST;
    pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE_BUSY;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_EN;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_EN;
    sdmmc_descriptor.is_api_sync = true;
    state = sdmmc_send_command(pxCmd);
    if (state != 0)
    {
        return state;
    }
    sdmmc_wait_xfer_done();
    return sdmmc_descriptor.StatusCode;
}

static eErrorState sd_snd_csd( CmdParameters_t *pxCmd )
{
    eErrorState state;
    pxCmd->ulArgument = (pxCardSpecificData->usRelativeAddress)
            << SDMMC_ARG_MASK_REL_ADD;
    pxCmd->ucCommandIndex = SDMMC_CMD_SEND_CSD;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_NOT_PST;
    pxCmd->ucResponseType = SDMMC_LONG_RESPONSE;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_DI;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_DI;
    state = sdmmc_send_command(pxCmd);
    if (state != 0)
    {
        return state;
    }
    sdmmc_wait_cmd_done();
    return sdmmc_descriptor.StatusCode;
}

static eErrorState sd_set_bus_width( CmdParameters_t *pxCmdHandle )
{
    eErrorState state;
    state = sd_snd_app_bus_cmd(pxCmdHandle);

    if (state != 0)
    {
        return -EIO;
    }

    state = sd_bus_width_4(pxCmdHandle);
    if (state != 0)
    {
        return -EIO;
    }

    return 0;
}

static eErrorState sd_bus_width_4( CmdParameters_t *pxCmd )
{
    eErrorState state;
    pxCmd->ulArgument = SDMMC_ARG_BUS_WIDTH;
    pxCmd->ucCommandIndex = SDMMC_CMD_SWITCH;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_NOT_PST;
    pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_DI;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_DI;
    state = sdmmc_send_command(pxCmd);
    if (state != 0)
    {
        return state;
    }
    sdmmc_wait_cmd_done();
    return sdmmc_descriptor.StatusCode;
}
static eErrorState sd_snd_app_bus_cmd( CmdParameters_t *pxCmd )
{
    eErrorState state;
    pxCmd->ulArgument = (pxCardSpecificData->usRelativeAddress)
            << SDMMC_ARG_MASK_REL_ADD;
    pxCmd->ucCommandIndex = SDMMC_CMD_SEND_APP;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_NOT_PST;
    pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_DI;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_DI;
    state = sdmmc_send_command(pxCmd);
    if (state != 0)
    {
        return state;
    }
    sdmmc_wait_cmd_done();
    return sdmmc_descriptor.StatusCode;
}
#endif

#if (DEV_TYPE ==  DEV_TYPE_EMMC)
/*follows e.MMC standard: JESD84-B51A: Embedded MultiMediaCard (e.MMC),
 * Electrical Standard (5.1A)
 */
static eErrorState sd_mmc_init( uint64_t *SecNum )
{
    CmdParameters_t *pxCmdHandle;
    CmdParameters_t xCommandConfig;
    eErrorState state;

    pxCmdHandle = &xCommandConfig;
    /*required delay for re-init*/
    osal_task_delay(10);
    /*send cmd to set the card idle*/
    state = mmc_go_idle(pxCmdHandle);
    if (state != 0)
    {
        return -EIO;
    }
    /*send command to set the card into ready state*/
    state = mmc_check_ocr(pxCmdHandle);
    if (state != 0)
    {
        return -EIO;
    }
    /*send cmd to request cid of the card*/
    state = mmc_snd_all_cid(pxCmdHandle);
    if (state != 0)
    {
        return -EIO;
    }
    /*send cmd to set the rel addr of the card*/
    state = mmc_set_rel_add(pxCmdHandle);
    if (state != 0)
    {
        return -EIO;
    }
    sd_read_response_rel_addr(pxCardSpecificData);
    /*send command to req csd of the card*/
    state = mmc_snd_csd(pxCmdHandle);
    if (state != 0)
    {
        return -EIO;
    }
    /*send cmd to select the card*/
    state = mmc_sel_card(pxCmdHandle);
    if (state != 0)
    {
        return -EIO;
    }
    /*send cmd to set ext_csd */
    state = mmc_switch_bus_width( pxCmdHandle );
    if (state != 0)
    {
        return -EIO;
    }
    /*send cmd to request extended csd of the card*/
    state = mmc_send_ext_csd( pxCmdHandle, SecNum );
    return state;
}


static eErrorState mmc_go_idle( CmdParameters_t *pxCmd )
{
    eErrorState state;
    pxCmd->ulArgument = SDMMC_NO_CMD_ARG;
    pxCmd->ucCommandIndex = SDMMC_CMD_GO_IDLE_STATE;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_NOT_PST;
    pxCmd->ucResponseType = SDMMC_NO_RESPONSE;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_DI;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_DI;
    state = sdmmc_send_command(pxCmd);
    if (state != 0)
    {
        return state;
    }
    sdmmc_wait_cmd_done();
    return sdmmc_descriptor.StatusCode;
}
static eErrorState mmc_sel_card( CmdParameters_t *pxCmd )
{
    eErrorState state;
    pxCmd->ulArgument = (SDMMC_SET_ADDR << SDMMC_ARG_MASK_REL_ADD);
    pxCmd->ucCommandIndex = SDMMC_CMD_SELECT_CARD;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_NOT_PST;
    pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE_BUSY;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_EN;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_EN;
    state = sdmmc_send_command(pxCmd);
    if (state != 0)
    {
        return state;
    }
    sdmmc_wait_xfer_done();
    return sdmmc_descriptor.StatusCode;
}

static eErrorState mmc_snd_csd( CmdParameters_t *pxCmd )
{
    eErrorState state;
    pxCmd->ulArgument = (SDMMC_REL_CARD_ADDRESS << SDMMC_ARG_MASK_REL_ADD);
    pxCmd->ucCommandIndex = SDMMC_CMD_SEND_CSD;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_NOT_PST;
    pxCmd->ucResponseType = SDMMC_LONG_RESPONSE;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_DI;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_DI;
    state = sdmmc_send_command(pxCmd);
    if (state != 0)
    {
        return state;
    }
    sdmmc_wait_cmd_done();
    return sdmmc_descriptor.StatusCode;
}

static eErrorState mmc_set_rel_add( CmdParameters_t *pxCmd )
{
    eErrorState state;
    pxCmd->ulArgument = (SDMMC_REL_CARD_ADDRESS << SDMMC_ARG_MASK_REL_ADD);
    pxCmd->ucCommandIndex = SDMMC_CMD_SET_REL_ADD;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_NOT_PST;
    pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_DI;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_DI;
    sdmmc_descriptor.is_api_sync = true;
    state = sdmmc_send_command(pxCmd);
    if (state != 0)
    {
        return state;
    }
    sdmmc_wait_cmd_done();
    return sdmmc_descriptor.StatusCode;
}

static eErrorState mmc_snd_all_cid( CmdParameters_t *pxCmd )
{
    eErrorState state;
    pxCmd->ulArgument = SDMMC_NO_CMD_ARG;
    pxCmd->ucCommandIndex = SDMMC_CMD_ALL_SEND_CID;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_NOT_PST;
    pxCmd->ucResponseType = SDMMC_LONG_RESPONSE;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_DI;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_DI;
    state = sdmmc_send_command(pxCmd);
    if (state != 0)
    {
        return state;
    }
    sdmmc_wait_cmd_done();
    return sdmmc_descriptor.StatusCode;
}

static eErrorState mmc_check_ocr( CmdParameters_t *pxCmd )
{
    eErrorState state;
    pxCmd->ulArgument = SDMMC_ARG_SDHC_OCR;
    pxCmd->ucCommandIndex = SDMMC_CMD_CHECK_OCR;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_NOT_PST;
    pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_DI;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_DI;

    while (sdmmc_is_card_ready() == 0U)
    {
        state = sdmmc_send_command(pxCmd);
        if (state != 0)
        {
            return state;
        }
        sdmmc_wait_cmd_done();
    }
    return sdmmc_descriptor.StatusCode;
}

static eErrorState mmc_send_ext_csd( CmdParameters_t *pxCmd,
        uint64_t*sector_count_ref )
{
    eErrorState state;
    pxCmd->ulArgument = SDMMC_NO_CMD_ARG;
    pxCmd->ucCommandIndex = SDMMC_CMD_SEND_EXT_CSD;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_PST;
    pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_EN;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_EN;

    sdmmc_descriptor.is_api_sync = true;

    sdmmc_set_up_xfer(sdmmc_descriptor.dma_descriptor,(uint64_t*)ext_csd_buff,
            SDMMC_BLOCK_SIZE,
            SDMMC_SINGLE_BLOCK);
    sdmmc_set_xfer_config(pxCmd);

    state = sdmmc_send_command(pxCmd);
    if (state != 0)
    {
        return -EIO;
    }
    sdmmc_wait_xfer_done();

    if (sdmmc_descriptor.StatusCode == 0)
    {
        cache_force_invalidate(ext_csd_buff, SDMMC_BLOCK_SIZE );
        *sector_count_ref =
                *(uint32_t*)(ext_csd_buff + SDMMC_EXT_CSD_SEC_NUM);
    }
    return sdmmc_descriptor.StatusCode;
}
static eErrorState mmc_switch_bus_width( CmdParameters_t *pxCmd )
{
    eErrorState state;
    pxCmd->ulArgument = SDMMC_SET_EXT_BUS_WIDTH;
    pxCmd->ucCommandIndex = SDMMC_CMD_SWITCH;
    pxCmd->ucDataXferPresent = SDMMC_DATA_XFER_NOT_PST;
    pxCmd->ucResponseType = SDMMC_SHORT_RESPONSE_BUSY;
    pxCmd->ucIdCheckEnable = SDMMC_CMD_ID_CHECK_DI;
    pxCmd->ucCrcCheckEnable = SDMMC_CMD_CRC_CHECK_DI;
    sdmmc_descriptor.is_api_sync = true;

    state = sdmmc_send_command(pxCmd);
    if (state != 0)
    {
        return state;
    }
    sdmmc_wait_xfer_done();
    return sdmmc_descriptor.StatusCode;
}
#endif

static eErrorState sdmmc_setup_host( void )
{
    /*
       the default mux for shared combo phy is configured
       for the nand ,if combophy is not enabled for
       sdmmc at ATF configure dfi_interface_cfg to 1 through smc
     */
    eErrorState ret = CTRL_CONFIG_PASS;
    ret = sdmmc_reset_per0();
    if (ret != CTRL_CONFIG_PASS)
    {
        return ret;
    }
    ret = sdmmc_reset_configs();
    if (ret != CTRL_CONFIG_PASS)
    {
        return ret;
    }
    ret = sdmmc_init_phy();

    if (ret != CTRL_CONFIG_PASS)
    {
        return ret;
    }
    sdmmc_init_configs(sdmmc_descriptor.dev_type,
            sdmmc_descriptor.is_def_speed_supported);
    return ret;
}

uint32_t sdmmc_is_card_present( void )
{
    return sdmmc_is_card_detected();
}

static void sdmmc_wait_xfer_done( void )
{
    (void) osal_semaphore_wait(sdmmc_descriptor.xSemaphoreXfer,
            OSAL_TIMEOUT_WAIT_FOREVER);
}

static void sdmmc_wait_cmd_done( void )
{
    (void) osal_semaphore_wait(sdmmc_descriptor.xSemaphoreCmd,
            OSAL_TIMEOUT_WAIT_FOREVER);
}

void sdmmc_irq_handler( void *data )
{
    (void) data;
    uint32_t volatile int_status = sdmmc_get_int_status();
    sdmmc_disable_int();
    sdmmc_clear_int();

    switch (int_status)
    {
    case SDMMC_CMD_CPT_INT_LOG:
        sdmmc_descriptor.StatusCode = 0;
        (void) osal_semaphore_post(sdmmc_descriptor.xSemaphoreCmd);
        break;
    case SDMMC_XFER_CPT_INT_LOG:
        if (sdmmc_descriptor.is_api_sync == true)
        {
            sdmmc_descriptor.StatusCode = 0;
            (void) osal_semaphore_post(sdmmc_descriptor.xSemaphoreXfer);
        }
        else
        {
            sdmmc_descriptor.StatusCode = 0;
            if (sdmmc_descriptor.xfer_call_back != NULL)
            {
                sdmmc_descriptor.xfer_call_back( 0 );
            }
        }
        break;

    case SDMMC_CMD_TIMOUT_INT_LOG:
        sdmmc_descriptor.StatusCode = -EIO;
        (void) osal_semaphore_post(sdmmc_descriptor.xSemaphoreCmd);
        break;

    case SDMMC_XFER_TIMOUT_INT_LOG:
        if (sdmmc_descriptor.is_api_sync == true)
        {
            sdmmc_descriptor.StatusCode = XFER_TIMOUT_ERR;
            (void) osal_semaphore_post(sdmmc_descriptor.xSemaphoreXfer);
        }
        else
        {
            sdmmc_descriptor.StatusCode = XFER_TIMOUT_ERR;
            if (sdmmc_descriptor.xfer_call_back != NULL)
            {
                sdmmc_descriptor.xfer_call_back( -EIO );
            }
        }
        break;
    default:
        /*Do Nothing*/
        break;
    }
}


