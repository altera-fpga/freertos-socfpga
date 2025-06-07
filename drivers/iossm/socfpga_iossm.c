/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * HAL driver implementation for iossm
 */

#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include "socfpga_iossm.h"
#include "socfpga_iossm_ll.h"
#include "socfpga_interrupt.h"
#include "socfpga_cache.h"
#include "osal_log.h"
#include "osal.h"

#define USE_DUAL_INSTANCE     0
#define IOSSM_NUM_INSTANCE    2

static void* error_inj_mem_sb = NULL;
static void* error_inj_mem_db = NULL;

static xiossm_context iossm_var[IOSSM_NUM_INSTANCE];

void iossm_irq_handler(void *pvParam);

xiossm_context*iossm_open(uint32_t instance)
{
    xResponseData RespData;
    iossm_type iossm_desc;
    SocfpgaInterruptErr_t xIntRet;
    xiossm_context *iossm_handle;


    if (instance > IOSSM_INSTANCE_1)
    {
        ERROR("Invalid instance");
        return NULL;
    }

    iossm_handle = &iossm_var[instance];

    if (iossm_handle->iossm_instance_is_open == true)
    {
        ERROR("Instance opened previously");
        return NULL;
    }
    iossm_handle->iossm_base_addr = IOSSM0_BASE_ADDR +
            (instance * IOSSM_INSTANCE_OFFSET);
    iossm_handle->iossm_instance_is_open = true;
    xIntRet = interrupt_register_isr(eIo96b0_dbeIrq, iossm_irq_handler,
            iossm_handle);
    if (xIntRet != eErrOk)
    {
        return NULL;
    }
    xIntRet = interrupt_spi_enable(eIo96b0_dbeIrq, eSpiInterruptTypeEdge,
                eSpiInterruptModeTarget, GIC_INTERRUPT_PRIORITY_IOSSM);
    if (xIntRet != eErrOk)
    {
        return NULL;
    }

    iossm_desc.base_addr = iossm_handle->iossm_base_addr;
    iossm_desc.ip_type = 0U;
    iossm_desc.instance_id = 0U;
    /*send command to get the interface id and ip_type*/
    RespData = iossm_send_command( &iossm_desc, GET_SYS_INFO, NO_PARAM );

 #if USE_DUAL_INSTANCE
    if ((RespData.ulResp0 & 0x1FFFFFFFU) != 1U)
    {
        if ((RespData.ulResp1 & 0x1FFFFFFFU) == 1U)
        {
            iossm_handle->iossm_instance_id = RespData.ulResp1 & 0xE1FFFFFFU;
            iossm_handle->iossm_ip_type = 1U;
        }

        else
        {
            /*No instance found*/
            iossm_handle->iossm_ip_type = 0U;
        }
    }
    else
    {
        iossm_handle->iossm_instance_id = RespData.ulResp0 & 0xE1FFFFFFU;
        iossm_handle->iossm_ip_type = 1U;
    }
#else
    /*check if the recieved response is valid*/
    if ((RespData.ulResp0 & ~(0x1FFFFFFFU)) != 0U)
    {
        iossm_handle->iossm_ip_type = 0x1U;
        iossm_handle->iossm_instance_id = 0U;
    }
    else
    {
        return NULL;
    }
#endif

    return iossm_handle;
}

uint32_t iossm_read_ecc_status(const xiossm_context*xhandle)
{
    uint32_t status;

    if (xhandle == NULL)
    {
        ERROR("Invalid handle");
        return 0U;
    }

    if (xhandle->iossm_instance_is_open == false)
    {
        ERROR("Instance is not opened");
        return 0U;
    }

    status = iossm_read_register(xhandle->iossm_base_addr , 
            IOSSM_ECC_ERROR_STATUS);
    status = status & IOSSM_ECC_COUNT_MASK;

    return status;
}


uint32_t iossm_clear_ecc_buffer(const xiossm_context*xhandle)
{
    xResponseData RespData;
    iossm_type iossm_desc;
    uint32_t status;

    if (xhandle == NULL)
    {
        ERROR("Invalid handle");
        return 0U;
    }

    if (xhandle->iossm_instance_is_open == false)
    {
        ERROR("Instance is not opened");
        return 0U;
    }

    iossm_desc.base_addr = xhandle->iossm_base_addr;
    iossm_desc.ip_type = xhandle->iossm_ip_type;
    iossm_desc.instance_id = xhandle->iossm_instance_id;
    /*send command to read the ecc status */
    RespData = iossm_send_command( &iossm_desc, ECC_CLEAR_ERR, NO_PARAM  );

    status = ((RespData.status >> 16U) & 0x7U);
    return status;
}

int32_t iossm_inject_sbit_err(const xiossm_context*xhandle )
{
    xResponseData RespData;
    iossm_type iossm_desc;

    if (xhandle == NULL)
    {
        ERROR("Invalid handle");
        return -EINVAL;
    }
    if (xhandle->iossm_instance_is_open == false)
    {
        ERROR("Instance is not opened");
        return -EIO;
    }

    if(error_inj_mem_sb == NULL)
    {
        error_inj_mem_sb = pvPortMallocCoherent(10);
    }

    iossm_desc.base_addr = xhandle->iossm_base_addr;
    iossm_desc.ip_type = xhandle->iossm_ip_type;
    iossm_desc.instance_id = xhandle->iossm_instance_id;
    /*send command to inject single bit error*/
    osal_enter_critical();
    RespData =
            iossm_err_inject_command( &iossm_desc, error_inj_mem_sb,  SING_BIT_ERR_PARAM);
    osal_exit_critical();

    cache_force_invalidate(error_inj_mem_sb, 8);

    if ((RespData.status & 1U) != 1U)
    {
        return -EIO;
    }

    return 0;
}

int32_t iossm_inject_dbit_err(const xiossm_context*xhandle )
{
    xResponseData RespData;
    iossm_type iossm_desc;

    if (xhandle == NULL)
    {
        ERROR("Invalid handle");
        return -EINVAL;
    }
    if (xhandle->iossm_instance_is_open == false)
    {
        ERROR("Instance is not opened");
        return -EIO;
    }

    if(error_inj_mem_db == NULL)
    {
        error_inj_mem_db = pvPortMallocCoherent(10);
    }

    iossm_desc.base_addr = xhandle->iossm_base_addr;
    iossm_desc.ip_type = xhandle->iossm_ip_type;
    iossm_desc.instance_id = xhandle->iossm_instance_id;
    /*send command to inject double bit error*/
    osal_enter_critical();
    RespData =
            iossm_err_inject_command( &iossm_desc, error_inj_mem_db,  DBL_BIT_ERR_PARAM);
    osal_exit_critical();

    cache_force_invalidate(error_inj_mem_db, 8);

    if ((RespData.status & 1U) != 1U)
    {
        return -EIO;
    }

    return 0;
}

int32_t iossm_mask_int(const xiossm_context*xhandle,  uint32_t ulInterrupts)
{
    xResponseData RespData;
    iossm_type iossm_desc;

    if (xhandle == NULL)
    {
        ERROR("Invalid handle");
        return -EINVAL;
    }

    if (xhandle->iossm_instance_is_open == false)
    {
        ERROR("Instance is not opened");
        return -EIO;
    }

    iossm_desc.base_addr = xhandle->iossm_base_addr;
    iossm_desc.ip_type = xhandle->iossm_ip_type;
    iossm_desc.instance_id = xhandle->iossm_instance_id;
    /*send command to mask the interrupts*/
    RespData = iossm_send_command( &iossm_desc, ECC_INT_MASK, ulInterrupts );

    if ((RespData.status & 1U) != 1U)
    {
        return -EIO;
    }

    return 0;
}

int32_t iossm_ack_int( const xiossm_context*xhandle,  uint32_t ulInterrupts )
{
    xResponseData RespData;
    iossm_type iossm_desc;

    if (xhandle == NULL)
    {
        ERROR("Invalid handle");
        return -EINVAL;
    }
    if (xhandle->iossm_instance_is_open == false)
    {
        ERROR("Instance is not opened");
        return -EIO;
    }

    iossm_desc.base_addr = xhandle->iossm_base_addr;
    iossm_desc.ip_type = xhandle->iossm_ip_type;
    iossm_desc.instance_id = xhandle->iossm_instance_id;
    /*send command to acknowledge the ecc interrupts*/
    RespData = iossm_send_command( &iossm_desc, ECC_INT_ACK, ulInterrupts );

    if ((RespData.status & 1U) != 1U)
    {
        return -EIO;
    }

    return 0;
}

uint32_t iossm_get_err_type( const xiossm_context*xhandle, uint32_t index )
{
    uint32_t status;

    if (xhandle == NULL)
    {
        ERROR("Invalid handle");
        return 0U;
    }

    if (xhandle->iossm_instance_is_open == false)
    {
        ERROR("Instance is not opened");
        return 0U;
    }

    status = iossm_read_register(xhandle->iossm_base_addr,
            IOSSM_ECC_GET_ERR_TYPE(index));
    return IOSSM_ECC_ERR_TYP_FIELD(status);
}

uint32_t iossm_get_err_addr_offset( const xiossm_context*xhandle, uint32_t index )
{
    uint32_t offset;

    if (xhandle == NULL)
    {
        ERROR("Invalid handle");
        return 0U;
    }

    if (xhandle->iossm_instance_is_open == false)
    {
        ERROR("Instance is not opened");
        return 0U;
    }

    offset = iossm_read_register(xhandle->iossm_base_addr,
            IOSSM_ECC_GET_ERR_OFFSET(index));

    return offset;
}

void iossm_irq_handler(void *pvParam)
{
    xiossm_context *iossm_handle = (xiossm_context*)pvParam;

    if (iossm_handle->iossm_cb_fun != NULL)
    {
        iossm_handle->iossm_cb_fun();
    }
}

void iossm_set_callback( xiossm_context*xhandle,iossm_cb_handler call_back_fun)
{
    xhandle->iossm_cb_fun = call_back_fun;
}

int32_t iossm_close( uint32_t instance )
{
    if (instance > 1U)
    {
        ERROR("Invalid instance");
        return -EINVAL;
    }
    if (iossm_var[instance].iossm_instance_is_open == false)
    {
        ERROR("Instance is not opened");
        return -EIO;
    }
    iossm_var[instance].iossm_instance_is_open = false;
    return 0;
}
