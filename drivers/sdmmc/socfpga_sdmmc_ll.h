/*
 * SPDX-FileCopyrightText: Copyright (C) 2025-2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for SoC FPGA SDMMC low level driver
 */

/*
    File: SD/MMC Low-Level Driver

    Declares the SD/MMC low-level driver interface: 15 functions, 3 types
    and 8 constants.
*/

#ifndef __SOCFPGA_SDMMC_LL_H__
#define __SOCFPGA_SDMMC_LL_H__

#define PER0MODRST_ADDR    (0x10D11024U)

#define DESC_MAX_XFER_SIZE    (64U * 1024U)

/*
    Struct: cmd_parameters_t

    Parameters for the SD/MMC low-level driver, holding argument, command_index, data_xfer_present and 3 more. Passed to sdmmc_set_xfer_config and sdmmc_send_command.
*/
typedef struct
{
    uint64_t argument;
    uint8_t command_index;
    uint8_t data_xfer_present;
    uint8_t response_type;
    uint8_t id_check_enable;
    uint8_t crc_check_enable;

} cmd_parameters_t;

/*
    Struct: card_data_t

    Data for the SD/MMC low-level driver, holding relative_address and card_type. Passed to sd_read_response_rel_addr and sd_get_card_type.
*/
typedef struct
{
    uint64_t relative_address;
    uint8_t card_type;
} card_data_t;

typedef struct __attribute__((packed))
{
    uint8_t attribute;
    uint8_t reserved;
    uint16_t len;
    uint32_t addr_lo;
    uint32_t addr_hi;
} dma_descriptor_t;

#define CMD_SND_OK          0
#define CMD_ERR             1
#define XFER_CPT_OK         2
#define XFER_TIMOUT_ERR     3
#define CTRL_CONFIG_FAIL    4
#define CTRL_CONFIG_PASS    5

/*
    Function: sdmmc_disable_int

    Disables interrupts.

    Disables interrupts related to data and response triggers.
*/
void sdmmc_disable_int(void);

/*
    Function: sdmmc_set_xfer_config

    Configures the host for data transmission and reception.

    Configures the host based on read/write operation and the number of
    blocks.

    Parameters:

        params - The command parameter instance.
*/
void sdmmc_set_xfer_config(cmd_parameters_t const *params);

/*
    Function: sd_read_response_rel_addr

    Reads the Relative Card Address (RCA).

    Parameters:

        pxcard - Instance containing card-specific data.
*/
void sd_read_response_rel_addr(card_data_t *pxcard);

/*
    Function: sdmmc_set_up_xfer

    Sets the DMA attributes.

    Prepares the DMA for data transmission/reception with appropriate
    attributes.

    Parameters:

        pdesc - Pointer to dynamically allocated memory for descriptor
            preparation.
        buff - Memory buffer used for data transmission/reception.
        block_size - Size of each block in bytes.
        block_ct - Number of blocks to be transferred/received.
*/
void sdmmc_set_up_xfer(dma_descriptor_t *pdesc, uint64_t *buff, uint32_t
        block_size, uint32_t block_ct);

/*
    Function: sd_get_card_type

    Checks the card type based on its capacity.

    Parameters:

        pxcard - Reference to card-specific data.
*/
void sd_get_card_type(card_data_t *pxcard);

/*
    Function: sdmmc_send_command

    Configures send parameters and sends a command to the card.

    Parameters:

        params - Reference to the parameters loaded for sending the
            command.

    Returns:

        - CMD_SND_OK, on successfully sending the command.
        - CTRL_CONFIG_FAIL, on configuration failure.
*/
int32_t sdmmc_send_command(const cmd_parameters_t *params);

/*
    Function: sdmmc_is_card_detected

    Check if card is present.

    Returns:

        - 1 , if card is detected
        - 0, if card is not detected
*/
uint32_t sdmmc_is_card_detected(void);

/*
    Function: sdmmc_is_card_ready

    Check if card enable bit is set.

    Returns:

        - 1 , if enable bit not set.
        - 0 , if enable bit is set.
*/
uint32_t sdmmc_is_card_ready(void);

/*
    Function: sdmmc_get_int_status

    Read interrupt status register.

    Returns:

        Interrupt status.
*/
uint32_t sdmmc_get_int_status(void);

/*
    Function: sdmmc_read_sector_count

    Calculates the sector count of the sd card.

    Returns:

        Sector count of the sd card.
*/
uint64_t sdmmc_read_sector_count(void);

/*
    Function: sdmmc_reset_configs

    Resets all sdmmc host configurations.

    Returns:

         - CTRL_CONFIG_PASS , on reset success.
         - CTRL_CONFIG_FAIL , on reset fail.
*/
int32_t sdmmc_reset_configs(void);

/*
    Function: sdmmc_init_configs

    Initializes sdmmc host configuration.

    Parameters:

        emmc_bus_width - - setting the bus width to 8.
        def_speed - - is default speed supported.
*/
void sdmmc_init_configs(uint32_t emmc_bus_width, uint32_t def_speed);

/*
    Function: sdmmc_reset_per0

    Resets the sdmmc peripheral.

    Returns:

         - CTRL_CONFIG_PASS , on reset success.
         - CTRL_CONFIG_FAIL , on reset fail.
*/
int32_t sdmmc_reset_per0(void);

/*
    Function: sdmmc_init_phy

    Configures the sdmmc combo-phy.

    Returns:

         - CTRL_CONFIG_PASS , on reset success
         - CTRL_CONFIG_FAIL , on reset fail
*/
int32_t sdmmc_init_phy(void);

/*
    Function: sdmmc_clear_int

    Clears sdmmc interrupt flags.
*/
void sdmmc_clear_int(void);

#endif /*__SOCFPGA_SDMMC_LL_H__*/
