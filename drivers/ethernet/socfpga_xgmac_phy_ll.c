/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Low level driver implementation for SoC FPGA PHY
 */

#include "socfpga_xgmac_phy_ll.h"
#include "socfpga_xgmac_reg.h"
#include "socfpga_xgmac_ll.h"

static inline bool check_mdio_busy(uint32_t *baseAddress){

    /* Read the mdio cmd control register */
    uint32_t status = RD_REG32(
            baseAddress + XGMAC_MDIO_SINGLE_COMMAND_CONTROL_DATA);

    if (status & XGMAC_MDIO_SINGLE_COMMAND_CONTROL_DATA_SBUSY_MASK) {
        /* mdio operation is busy */
        return 1;
    }
    else{
        /* MDIO is not busy */
        return 0;
    }
}

int16_t read_phy_reg(uint32_t *baseAddress, uint32_t phyAddress, uint8_t phyReg)
{

    uint32_t data;
    uint32_t addrData;
    uint8_t count = 0;
    uint8_t maxCnt = 100;

    /* Check MDIO sbusy bit status */
    while (check_mdio_busy(baseAddress)) {
        if (count++ > maxCnt) {
            return -1;
        }
        for (int i =0; i < maxCnt; i++)
            ;
    }

    /* Set clause 22 format for PHY address */
    data = RD_REG32(baseAddress + XGMAC_MDIO_CLAUSE_22_PORT);
    data |= (1 << phyAddress);

    /* selecting clause to given PHY address port. */
    WR_REG32(baseAddress + XGMAC_MDIO_CLAUSE_22_PORT, data);

    /* Read command address register and mask with our PHY address and PHY
     * register value.
     */
    addrData = RD_REG32(baseAddress + XGMAC_MDIO_SINGLE_COMMAND_ADDRESS);

    /*Mask PHY address and register */
    addrData =
            (phyAddress <<
            XGMAC_MDIO_SINGLE_COMMAND_ADDRESS_PA_POS) |
            (phyReg << XGMAC_MDIO_SINGLE_COMMAND_ADDRESS_RA_POS);

    WR_REG32(baseAddress + XGMAC_MDIO_SINGLE_COMMAND_ADDRESS, addrData);

    /* Read data from MDIO control data register. Mask with the
     * data which we want make.
     */

    /* 3'b010 corresponds to clk_csr_i: 250-300 MHz, with MDC clock: clk_csr_i/122 */
    data = (4 << XGMAC_MDIO_SINGLE_COMMAND_CONTROL_DATA_CR_POS);

    /* Mask with data var for read operation */
    data |= (XGMAC_MDIO_SINGLE_COMMAND_CONTROL_DATA_SAADR_MASK) |
            (XGMAC_MDIO_SINGLE_COMMAND_CONTROL_DATA_CMD_MASK) |
            (XGMAC_MDIO_SINGLE_COMMAND_CONTROL_DATA_SBUSY_MASK);

    /* Write masked data to cmd cntrl data register */
    WR_REG32(baseAddress + XGMAC_MDIO_SINGLE_COMMAND_CONTROL_DATA, data);

    /* wait for some */
    for (int i=0; i < 10000; i++)
        ;

    /* Verify MDIO sbusy bit status */
    count = 0;
    while (check_mdio_busy(baseAddress)) {
        if (count++ > maxCnt) {
            return -1;
        }
        for (int i=0; i < maxCnt; i++)
            ;
    }
    for (int i=0; i < 100000; i++)
        ;

    /* Read the data from the cmd control data register */
    data = RD_REG32(baseAddress + XGMAC_MDIO_SINGLE_COMMAND_CONTROL_DATA);
    uint16_t least_bits = (uint16_t)(data & 0xFFFF);

    return least_bits;

}

int8_t write_phy_reg(uint32_t *baseAddress, uint32_t phyAddress, uint8_t phyReg,
        uint16_t regVal)
{

    uint32_t data;
    uint32_t addrData;
    uint8_t count = 0;
    uint8_t maxCnt = 100;

    /* Check MDIO sbusy bit status */
    while (check_mdio_busy(baseAddress)) {
        if (count++ > maxCnt) {
            return -1;
        }
        for (int i =0; i < maxCnt; i++)
            ;
    }

    /* Set clause 22 format for PHY address */
    data = RD_REG32(baseAddress + XGMAC_MDIO_CLAUSE_22_PORT);
    data |= (1 << phyAddress);

    /* Selecting clause to given PHY address port. */
    WR_REG32(baseAddress + XGMAC_MDIO_CLAUSE_22_PORT, data);


    /* Read command address register and mask with our PHY address and PHY
     * register value.
     */
    addrData = RD_REG32(baseAddress + XGMAC_MDIO_SINGLE_COMMAND_ADDRESS);

    /* Mask the phy_address and phy_register to mdio_register */
    addrData =
            (phyAddress <<
            XGMAC_MDIO_SINGLE_COMMAND_ADDRESS_PA_POS) |
            (phyReg << XGMAC_MDIO_SINGLE_COMMAND_ADDRESS_RA_POS);

    WR_REG32(baseAddress + XGMAC_MDIO_SINGLE_COMMAND_ADDRESS, addrData);

    /* Read data MDIO control data register. Mask with the
     * data which we want make.
     */
    data = RD_REG32(baseAddress + XGMAC_MDIO_SINGLE_COMMAND_CONTROL_DATA);

    /* Set the application Clock Range 350-400 MHz */
    data = (4 << XGMAC_MDIO_SINGLE_COMMAND_CONTROL_DATA_CR_POS);

    data |= (XGMAC_MDIO_SINGLE_COMMAND_CONTROL_DATA_SAADR_MASK) |
            (1 <<
            XGMAC_MDIO_SINGLE_COMMAND_CONTROL_DATA_CMD_POS) |
            (XGMAC_MDIO_SINGLE_COMMAND_CONTROL_DATA_SBUSY_MASK) |
            (regVal << XGMAC_MDIO_SINGLE_COMMAND_CONTROL_DATA_SDATA_POS);

    /* Write masked data to MDIO cmd cntrl data register */
    WR_REG32(baseAddress + XGMAC_MDIO_SINGLE_COMMAND_CONTROL_DATA, data);

    for (int i=0; i < maxCnt * 10000; i++)
        ;
    count = 0;
    while (check_mdio_busy(baseAddress)) {
        if (count++ > maxCnt) {
            return -1;
        }
        for (int i=0; i < maxCnt; i++)
            ;
    }

    return 0;
}
