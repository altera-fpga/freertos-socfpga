/*
 * SPDX-FileCopyrightText: Copyright (C) 2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Documentation-only header: FreeRTOS 26.3 release feature summary.
 * This file contains no code; it exists so the release information is rendered
 * by "make naturaldocs".
 */

#ifndef __RELEASE_NOTES_H__
#define __RELEASE_NOTES_H__

/*
    File: Release Notes

    Feature support summary for the SoC FPGA FreeRTOS BSP on Agilex 3 and
    Agilex 5 devices. Content is taken from the FreeRTOS 26.3 release
    documentation.

    Layer: examples

    Group: Release Information

    (start code)
    Stable Version    : v26.3
    Quartus Version   : 26.1
    Supported Devices : Agilex 3 and Agilex 5
    Branch/Tag        : test/all_peripherals
    (end code)

    Group: Feature Support Matrix

    Legend: Yes = supported, No = not supported, NA = not applicable,
    NT = not tested. Device columns are Agilex3 / Agilex5 / Agilex3 SMP /
    Agilex5 SMP.

    (start code)
    Feature              A3   A5   A3-SMP  A5-SMP  Supported Features / Limitations
    -------------------  ---  ---  ------  ------  --------------------------------------------------
    A55 boot             Yes  Yes  Yes     Yes     Single-core boot, Dual-core SMP, Quad-core SMP (A5)
    A76 boot             NA   Yes  NA      Yes     Single-core boot, Dual-core SMP, Quad-core SMP
    QSPI boot            Yes  Yes  Yes     Yes     -
    eMMC boot            Yes  Yes  Yes     Yes     -
    SD boot              Yes  Yes  Yes     Yes     -
    NAND boot            No   No   No      No      -
    Clock Manager        Yes  Yes  Yes     Yes     API to get clock speed of different blocks
    Reset Manager        Yes  Yes  Yes     Yes     Peripheral reset assert/de-assert
    DMA driver           Yes  Yes  Yes     Yes     Mem-to-Mem, Mem-to-Peripheral, Peripheral-to-Mem
    GPIO driver          Yes  Yes  Yes     Yes     Write, read and interrupt support
    Timer driver         Yes  Yes  Yes     Yes     User-defined and free-running modes
    UART driver          Yes  Yes  Yes     Yes     Full-duplex TX and RX; DMA not supported (planned)
    I2C driver           Yes  Yes  Yes     Yes     Master/Slave mode, standard and fast modes
    QSPI driver          Yes  Yes  Yes     Yes     QSPI flash read/write/erase
    I3C driver           Yes  Yes  Yes     Yes     Master mode, I3C and legacy I2C; IBI not supported
    SPI driver           Yes  Yes  Yes     Yes     Master/Slave mode write and read
    NAND driver          No   No   No      No      -
    SDM Mailbox driver   Yes  Yes  Yes     Yes     SDM commands with SIP_SVC
    SMMU support         Yes  Yes  Yes     Yes     Static identity mapping for cache coherency
    SDMMC driver         Yes  Yes  Yes     Yes     Std and HS speeds, SDMMC and eMMC, FATFS support
    Ethernet stack       Yes  Yes  Yes     Yes     TCP/IP, UDP, ICMP, DHCP, IPv4 and IPv6
    USB 2.0 stack        NT   Yes  NT      Yes     USB mass storage class; tested w/ custom board+SOF
    USB 3.1 stack        NT   Yes  NT      Yes     USB mass storage operation
    WDT driver           Yes  Yes  Yes     Yes     Interrupt or reset on timer expiry
    EDAC support         Yes  Yes  Yes     Yes     Inject/detect for EMAC, USB, QSPI; OCRAM excluded
    IOSSM driver         Yes  Yes  Yes     Yes     Error injection and detection
    Bridge driver        Yes  Yes  Yes     Yes     Enable/Disable bridges
    Reboot Manager       Yes  Yes  Yes     Yes     Warm/Cold reboot
    FPGA Manager         Yes  Yes  Yes     Yes     FPGA configuration (Full and Partial)
    (end code)
*/

#endif /* __RELEASE_NOTES_H__ */
