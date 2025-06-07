/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Implementation of interrupt APIs for SoC FPGA 
 */


#include "socfpga_interrupt.h"
#include "socfpga_GIC.h"
#include "socfpga_gic_reg.h"
#include "osal_log.h"

#define AGX5_DIST_BASE_ADDR    (0x1D000000)
#define AGX5_RD_BASE_ADDR      (0x1D060000)

static uint32_t ulGICRedisID;

#define SOCFPGA_DEFAULT_INTERRUPT_SPIN

#define SOCFPGA_PPI_START    22
#define SOCFPGA_MAX_PPI      30
#define SOCFPGA_SPI_START    eSdmApsMailboxIntr
#define SOCFPGA_MAX_SPI      eMaxHpuSpiInterrupt

typedef struct {
    SocfpgaInterruptCallback_t callback;
    void *data;
} InterruptHandler_t;

static InterruptHandler_t xInterruptCallbacks[MAX_SPI_HPU_INTERRUPT] = {
    [0 ... MAX_SPI_HPU_INTERRUPT - 1U] = { gic_default_interrupt_handler, NULL }
};

void interrupt_irq_handler(unsigned int ulInterruptID);
void interrupt_spi_test_isr(void *data);

void gic_default_interrupt_handler(void*data) {
    (void)data;
#ifdef SOCFPGA_DEFAULT_INTERRUPT_SPIN
    while (1 == 1) {
    }
#else
    return;
#endif
}

/**
 * @brief    Initializes the GIC600 interrupt controller.
 */
void interrupt_init_gic(void)
{
    /* Enable GIC. */
    if (gic_enable_gic() != interruptRETURN_SUCCESS)
    {
        return;
    }

    /* Get the ID of the Redistributor connected to this PE. */
    ulGICRedisID = (uint32_t) gic_get_redist_id(
            (uint32_t) gic_reg_get_cpu_affinity());

    /* Mark this core as being active. */
    if (gic_wakeup_redist(ulGICRedisID) != interruptRETURN_SUCCESS)
    {
        return;
    }

    /* Set the interrupt mask. */
    gic_reg_write_group1_end_of_interrupt(0xFF);

    /* Enable interrupt groups. */
    gic_reg_enable_group0_interrupts();
    gic_reg_enable_group1_interrupts();
}

SocfpgaInterruptErr_t interrupt_ppi_enable(SocfpgaHpuInterrupt_t xId,
        SocfpgaHpuInterruptType_t xInterruptType,
        uint8_t uPriority, uint32_t uGicRedisId) {
    uint32_t uType = GICV3_CONFIG_LEVEL;
    if ((xId > ePpiMax) || (xId < ePpiStart))
    {
        return eErrPpiId;
    }

    if (xInterruptType == eSpiInterruptTypeEdge) {
        uType = GICV3_CONFIG_EDGE;
    }

    if (gic_enable_int((uint32_t) xId, uGicRedisId) != interruptRETURN_SUCCESS)
    {
        return eErrPpiId;
    }
    if (gic_set_int_group((uint32_t) xId, uGicRedisId,
            GICV3_GROUP1_NON_SECURE) != interruptRETURN_SUCCESS)
    {
        return eErrPpiId;
    }
    if (gic_set_int_type((uint32_t) xId, 0U, uType) != interruptRETURN_SUCCESS)
    {
        return eErrPpiId;
    }
    if (gic_set_int_priority((uint32_t) xId, uGicRedisId,
            uPriority) != interruptRETURN_SUCCESS)
    {
        return eErrPpiId;
    }
    return eErrOk;
}

SocfpgaInterruptErr_t interrupt_spi_enable(SocfpgaHpuInterrupt_t xId,
        SocfpgaHpuInterruptType_t xInterruptType,
        SocfpgaHpuSpiInterruptMode_t xInterruptMode,
        uint8_t uPriority) {
    uint32_t uMode = GICV3_ROUTE_MODE_ANY;
    uint32_t uType = GICV3_CONFIG_LEVEL;

    if ((xId > SOCFPGA_MAX_SPI) || (xId < SOCFPGA_SPI_START))
    {
        return eErrSpiId;
    }

    if (xInterruptType == eSpiInterruptTypeEdge) {
        uType = GICV3_CONFIG_EDGE;
    }
    if (xInterruptMode == eSpiInterruptModeTarget) {
        uMode = GICV3_ROUTE_MODE_COORDINATE;
    }

    if (gic_set_int_priority((uint32_t) xId, 0U,
            uPriority) != interruptRETURN_SUCCESS)
    {
        return eErrSpiId;
    }
    if (gic_set_int_group((uint32_t) xId, 0U,
            GICV3_GROUP1_NON_SECURE) != interruptRETURN_SUCCESS)
    {
        return eErrSpiId;
    }
    if (gic_set_int_route((uint32_t) xId, uMode, 0U) != interruptRETURN_SUCCESS) 
    {
        return eErrSpiId;
    }
    if (gic_set_int_type((uint32_t) xId, 0U, uType) != interruptRETURN_SUCCESS)
    {
        return eErrSpiId;
    }
    if (gic_enable_int((uint32_t) xId, 0U) != interruptRETURN_SUCCESS)
    {
        return eErrSpiId;
    }
    return eErrOk;
}

SocfpgaInterruptErr_t interrupt_enable(SocfpgaHpuInterrupt_t xId,
        uint8_t uPriority) {

    SocfpgaInterruptErr_t Error = eErrOk;
    uint32_t uGicRedisributorId;
    if (xId < SOCFPGA_SPI_START) {
        uGicRedisributorId = (uint32_t) gic_get_redist_id(
                (uint32_t) gic_reg_get_cpu_affinity());
        Error = interrupt_ppi_enable(xId, eSpiInterruptTypeLevel, uPriority,
                uGicRedisributorId);
    } else  {
        Error = interrupt_spi_enable(xId, eSpiInterruptTypeLevel,
                eSpiInterruptModeTarget, uPriority);
    }

    return Error;
}

SocfpgaInterruptErr_t interrupt_spi_disable(SocfpgaHpuInterrupt_t xId) {
    if (gic_disable_int((uint32_t) xId, 0) != interruptRETURN_SUCCESS)
    {
        return eErrSpiId;
    }
    return eErrOk;
}

SocfpgaInterruptErr_t interrupt_register_isr(SocfpgaHpuInterrupt_t xId,
        SocfpgaInterruptCallback_t xCallback,
        void *vUserData) {
    if (xId > eMaxHpuSpiInterrupt) {
        return eErrSpiId;
    }
    if (xCallback == 0) {
        return eErrInterruptCallback;
    }
    xInterruptCallbacks[xId].callback = xCallback;
    xInterruptCallbacks[xId].data = vUserData;
    return eErrOk;
}

void interrupt_spi_test_isr(void *data) {
    (void)data;
    INFO("From Test ISR.");
}

void interrupt_spi_test(void) {
    if (interrupt_register_isr(eGpio0Irq, interrupt_spi_test_isr,
            NULL) != eErrOk)
    {
        return;
    }
    if (interrupt_spi_enable(eGpio0Irq, eSpiInterruptTypeLevel,
            eSpiInterruptModeAny, interruptMIN_INTERRUPT_PRIORITY) != eErrOk)
    {
        return;
    }
    if (gic_set_int_pending((uint32_t) eGpio0Irq,
            0U) != interruptRETURN_SUCCESS)
    {
        return;
    }
}


/*
 * @func  : vInterruptIRQHandler
   @brief : The IRQ interrupt handler. This function will determine the IRQ handler based on the interrupt ID.
   @param : ulInterruptID -> interruptID
 */

void interrupt_irq_handler(unsigned int ulInterruptID)
{
    /* Clear pending interrupts. */
    ulGICRedisID = (uint32_t) gic_get_redist_id(
            (uint32_t) gic_reg_get_cpu_affinity());
    if (gic_clear_int_pending(ulInterruptID,
            ulGICRedisID) != interruptRETURN_SUCCESS)
    {
        return;
    }
    //  agx_printf("FIQ: Received INTID %d\n", ID);

    /*This is the Max ID for PPI and SPI*/
    if (ulInterruptID < MAX_SPI_HPU_INTERRUPT) {
        xInterruptCallbacks[ulInterruptID].callback(xInterruptCallbacks[
                    ulInterruptID].data);
    } else if (ulInterruptID == 1023U) {
        INFO("FIQ: Interrupt was spurious");
        while (1 == 1)
        {

        }
    } else {
        INFO("FIQ: Panic, unexpected INTID");
    }

    /* Write EOIR to deactivate interrupt. */
    gic_reg_enable_group0_interrupts();
    gic_enable_interrupts();

    return;
}

