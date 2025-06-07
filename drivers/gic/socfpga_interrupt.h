/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for interrupt related APIs for SoC FGPA 
 */


#ifndef __SOCFPGA_INTERRUPT_H__
#define __SOCFPGA_INTERRUPT_H__

/**
 * @file socfpga_interrupt.h
 * @brief File for the HAL APIs of Interrupt controller.
 */

/**
 * @defgroup intr GIC
 * @ingroup drivers
 * @{
 */

#include <stdint.h>
#include "socfpga_interrupt_priority.h"

#define interruptMIN_INTERRUPT_PRIORITY    14

typedef enum {
    /*System PPIs*/
    ePpiStart = 22,
    eEl1PhyTmrIntr = 30,
    ePpiMax = 30,

    /*SDM Block interrupts*/
    eSdmApsMailboxIntr = 32,
    eSdmI2cIntr0,
    eSdmI2cIntr1,
    eSdmQspiIntr,

    eSdmPwrAlertIntr = 38,
    eSdmHpsSpareIntr0,
    eSdmHpsSpareIntr1,
    eSdmHpsSpareIntr2,

    /*Secure manager block interrupts*/
    eSecmgrTripleRedunError = 46,
    eSerrGlobal,

    /*CCU block interrupts*/
    eInterruptCcu,

    /*FPGA to HPS interrupts*/
    eFpga2hpsInterrupt0,
    eFpga2hpsInterrupt1,
    eFpga2hpsInterrupt2,
    eFpga2hpsInterrupt3,
    eFpga2hpsInterrupt4,
    eFpga2hpsInterrupt5,
    eFpga2hpsInterrupt6,
    eFpga2hpsInterrupt7,
    eFpga2hpsInterrupt8,
    eFpga2hpsInterrupt9,
    eFpga2hpsInterrupt10,
    eFpga2hpsInterrupt11,
    eFpga2hpsInterrupt12,
    eFpga2hpsInterrupt13,
    eFpga2hpsInterrupt14,
    eFpga2hpsInterrupt15,
    eFpga2hpsInterrupt16,
    eFpga2hpsInterrupt17,
    eFpga2hpsInterrupt18,
    eFpga2hpsInterrupt19,
    eFpga2hpsInterrupt20,
    eFpga2hpsInterrupt21,
    eFpga2hpsInterrupt22,
    eFpga2hpsInterrupt23,
    eFpga2hpsInterrupt24,
    eFpga2hpsInterrupt25,
    eFpga2hpsInterrupt26,
    eFpga2hpsInterrupt27,
    eFpga2hpsInterrupt28,
    eFpga2hpsInterrupt29,
    eFpga2hpsInterrupt30,
    eFpga2hpsInterrupt31,
    eFpga2hpsInterrupt32,
    eFpga2hpsInterrupt33,
    eFpga2hpsInterrupt34,
    eFpga2hpsInterrupt35,
    eFpga2hpsInterrupt36,
    eFpga2hpsInterrupt37,
    eFpga2hpsInterrupt38,
    eFpga2hpsInterrupt39,
    eFpga2hpsInterrupt40,
    eFpga2hpsInterrupt41,
    eFpga2hpsInterrupt42,
    eFpga2hpsInterrupt43,
    eFpga2hpsInterrupt44,
    eFpga2hpsInterrupt45,
    eFpga2hpsInterrupt46,
    eFpga2hpsInterrupt47,
    eFpga2hpsInterrupt48,
    eFpga2hpsInterrupt49,
    eFpga2hpsInterrupt50,
    eFpga2hpsInterrupt51,
    eFpga2hpsInterrupt52,
    eFpga2hpsInterrupt53,
    eFpga2hpsInterrupt54,
    eFpga2hpsInterrupt55,
    eFpga2hpsInterrupt56,
    eFpga2hpsInterrupt57,
    eFpga2hpsInterrupt58,
    eFpga2hpsInterrupt59,
    eFpga2hpsInterrupt60,
    eFpga2hpsInterrupt61,
    eFpga2hpsInterrupt62,
    eFpga2hpsInterrupt63,

    /*DMA0 interrupts*/
    eDmaIrq0,
    eDmaIrq1,
    eDmaIrq2,
    eDmaIrq3,
    eDma0CommonIrq,
    eDma0CombinedIrq,

    /*USB interrupts*/
    eUsbHostSystemErrIrq = 124,
    eUsb0Irq,
    eUsb1Irq,

    /*MPFE interrupt*/
    eIo96b0_dbeIrq,

    /*SDMMC/NAND interrupts*/
    eSdmmcIrq,
    eNandIrq,
    eNandSysWakeIrq,

    /*SPI interrupts*/
    eSpi0Irq,
    eSpi1Irq,
    eSpi2Irq,
    eSpi3Irq,

    /*I2C interrupts*/
    eI2c0Irq,
    eI2c1Irq,
    eI2c2Irq,
    eI2c3Irq,
    eI2c4Irq,

    /*UART interrupts*/
    eUart0Irq,
    eUart1Irq,

    /*GPIO interrupts*/
    eGpio0Irq,
    eGpio1Irq,

    /*Timer interrupts*/
    eTimerL4sp0irq = 145,
    eTimerL4sp1irq,
    eTimerOsc10irq,
    eTimerOsc11irq,

    /*Watchdog 0 interrupts*/
    eWdog0Irq,
    eWdog1Irq,

    /*Clock manager interrupt*/
    eClkmgrIrq,

    /*MPFE block interrupts*/
    eIo96b1dbeIrq,

    /*Watchdog 2/3 interrupts*/
    eWdog2Irq = 157,
    eWdog3Irq,

    /*SMMU interrupts*/
    eSysTcuGlobalIrptS = 160,
    eSysTcuGlobalIrptNs,
    eSysTcuCmdSyncIrptS,
    eesystcucmdsyncirptns,
    eSysTcuPriQ_irptNs,
    eSysTcuEventQ_irptS,
    eSysTcuEventQ_irptNs,
    eSysTcuRasIrpt,
    eSysTcuPmuIrpt,
    eF2socTbuRasIrpt,
    eF2socTbuPmuIrpt,
    eTsnTbuRasIrpt,
    eTsnTbuPmuIrpt,
    eIoTbuRasIrpt,
    eIoTbuPmuIrpt,
    eDmaTbuRasIrpt,
    eDmaTbuPmuIrpt,
    eSdmTbuRasIrpt,
    eSdmTbuPmuIrpt,
    eF2sdramTbuRasIrpt,
    eF2sdramTbuPmuIrpt,

    /*MPU block interrupt*/
    eEtrBufintr = 195,

    /*I3C interrupts*/
    eI3c0Irq,
    eI3c1Irq,

    /*DMA interrupts*/
    eDma1Irq0,
    eDma1Irq1,
    eDma1Irq2,
    eDma1Irq3,
    eDma1CommonIrq,
    eDma1CombinedIrq,

    /*Watchdog 4 interrupt*/
    eWdog4Irq = 207,

    /*MPU block interrupt*/
    eNclusterpmuirq,
    eNfaultirq0,
    eNfaultirq1,
    eNfaultirq2,
    eNfaultirq3,
    eNfaultirq4,
    eNerrirq0,
    eNerrirq1,
    eNerrirq2,
    eNerrirq3,
    eNerrirq4,

    /*EMAC block interrupts*/
    eEmac0Irq = 222,
    eEmac0TxIrq0,
    eEmac0TxIrq1,
    eEmac0TxIrq2,
    eEmac0TxIrq3,
    eEmac0TxIrq4,
    eEmac0TxIrq5,
    eEmac0TxIrq6,
    eEmac0TxIrq7,
    eEmac0RxIrq0,
    eEmac0RxIrq1,
    eEmac0RxIrq2,
    eEmac0RxIrq3,
    eEmac0RxIrq4,
    eEmac0RxIrq5,
    eEmac0RxIrq6,
    eEmac0RxIrq7,
    eEmac1Irq,
    eEmac1TxIrq0,
    eEmac1TxIrq1,
    eEmac1TxIrq2,
    eEmac1TxIrq3,
    eEmac1TxIrq4,
    eEmac1TxIrq5,
    eEmac1TxIrq6,
    eEmac1TxIrq7,
    eEmac1RxIrq0,
    eEmac1RxIrq1,
    eEmac1RxIrq2,
    eEmac1RxIrq3,
    eEmac1RxIrq4,
    eEmac1RxIrq5,
    eEmac1RxIrq6,
    eEmac1RxIrq7,
    eEmac2Irq,
    eEmac2TxIrq0,
    eEmac2TxIrq1,
    eEmac2TxIrq2,
    eEmac2TxIrq3,
    eEmac2TxIrq4,
    eEmac2TxIrq5,
    eEmac2TxIrq6,
    eEmac2TxIrq7,
    eEmac2RxIrq0,
    eEmac2RxIrq1,
    eEmac2RxIrq2,
    eEmac2RxIrq3,
    eEmac2RxIrq4,
    eEmac2RxIrq5,
    eEmac2RxIrq6,
    eEmac2RxIrq7,

    /*ECC block interrupt*/
    eEccDerrIntrN,

    /*Maximum number of interrupts*/
    eMaxHpuSpiInterrupt
} SocfpgaHpuInterrupt_t;

#define MAX_SPI_HPU_INTERRUPT    274U

typedef void (*SocfpgaInterruptCallback_t )(void*data);

typedef enum {
    eSpiInterruptTypeLevel = 0,
    eSpiInterruptTypeEdge = 2
} SocfpgaHpuInterruptType_t;

typedef enum {
    eSpiInterruptModeAny = 0,
    eSpiInterruptModeTarget = 2
} SocfpgaHpuSpiInterruptMode_t;

typedef enum {
    eErrOk = 0,
    eErrSpiId, //1
    eErrSpiType, //2
    eErrSpiMode, //3
    eErrSpiTarget, //4
    eErrInterruptCallback, //5
    eErrPpiId //6
} SocfpgaInterruptErr_t;

void interrupt_init_gic(void);
void gic_default_interrupt_handler(void *data);

SocfpgaInterruptErr_t interrupt_spi_enable(SocfpgaHpuInterrupt_t xId,
        SocfpgaHpuInterruptType_t xInterruptType,
        SocfpgaHpuSpiInterruptMode_t xInterruptMode, uint8_t uPriority);
SocfpgaInterruptErr_t interrupt_spi_disable(SocfpgaHpuInterrupt_t xId);
SocfpgaInterruptErr_t interrupt_register_isr(SocfpgaHpuInterrupt_t xId,
        SocfpgaInterruptCallback_t xCallback, void *vUserData);

SocfpgaInterruptErr_t interrupt_ppi_enable(SocfpgaHpuInterrupt_t xId,
        SocfpgaHpuInterruptType_t xInterruptType, uint8_t uPriority,
        uint32_t uGicRedisId);

SocfpgaInterruptErr_t interrupt_enable(SocfpgaHpuInterrupt_t xId,
        uint8_t uPriority);

void interrupt_spi_test(void);

/** @} */

#endif /*__SOCFPGA_INTERRUPT_H__*/
