/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Low level driver implementation for I3C
 */

#include "socfpga_defines.h"
#include "socfpga_interrupt.h"
#include "osal.h"
#include "socfpga_i3c_regs.h"
#include "socfpga_i3c.h"
#include "socfpga_i3c_ll.h"

#define MHZ                   (1000000U)
#define NANO_SEC              (1000000000U)

#define I3C_XFER_WAIT_TIME    pdMS_TO_TICKS(10000U)                                /* 1000ms wait for xfer complete,*/
#define WAIT_FOR_XFER_COMPLETE(instance)    ((osal_semaphore_wait(i3c_Obj[ \
            (instance)].xferComplete, \
        (uint64_t) I3C_XFER_WAIT_TIME)) ? (I3C_OK) : (I3C_TIMEDOUT))

/* I3C Controller Instance object  */
struct i3c_DriverObj i3c_Obj[I3C_NUM_INSTANCES] = {0};

void i3c_Isr( void *param );

static uint32_t GetNextFreePosition( uint8_t instance )
{
    uint32_t datFreeMask  = i3c_Obj[instance].datFreeMask;
    uint16_t totaNumdev   = i3c_Obj[instance].numDev;
    uint32_t i;

    for (i = 0U; i < totaNumdev; i++)
    {
        if ((datFreeMask & 0x1U) != 0U)
        {
            break;
        }
        datFreeMask >>= 1U;
    }
    return ((i > (sizeof(uint32_t) * 8U)) ? 0U: (i));
}

static uint16_t PushDataToTXFifo( uint8_t instance, uint8_t *data,
        uint16_t length )
{
    uint32_t tx_empty_loc, write_len;
    uint16_t bytes_written = 0U;
    uint16_t numWords;
    uint16_t numBytes;

    uint32_t word, i;
    uint8_t *buf = data;

    if (buf == NULL)
    {
        return 0U;
    }

    /*Find the number of empty locations within the FIFO*/
    tx_empty_loc =
            RD_REG32(
            (i3c_Obj[instance].regBase + I3C_DATA_BUFFER_STATUS_LEVEL));
    tx_empty_loc =
            (tx_empty_loc &
            I3C_DATA_BUFFER_STATUS_LEVEL_TX_BUF_EMPTY_LOC_MASK) >>
            I3C_DATA_BUFFER_STATUS_LEVEL_TX_BUF_EMPTY_LOC_POS;

    /*The writable data is the minimum of the data length
     * or the tx empty locations
     */
    write_len = (uint32_t) length < tx_empty_loc? (uint32_t) length: tx_empty_loc;
    numWords = (uint16_t) (write_len / RX_TX_DATA_PORT_SIZE);
    numBytes = (uint16_t) (write_len & (RX_TX_DATA_PORT_SIZE - 1U));


    for (i = 0; i < numWords; i++)
    {
        word = ((uint32_t) buf[0]) | ((uint32_t) buf[1] << 8U) | ((uint32_t) buf [2] << 16U) | ((uint32_t) buf[3] << 24U);
        WR_REG32((i3c_Obj[instance].regBase + I3C_TX_DATA_PORT), word);

        buf += RX_TX_DATA_PORT_SIZE;
    }
    bytes_written = numWords * 4U;

    if (numBytes != 0U) {
        word = 0;
        for (i = 0; i < numBytes; i++)
        {
            word |= (uint32_t)(*buf) << (8U * i);
            buf++;
            bytes_written++;
        }

        WR_REG32((i3c_Obj[instance].regBase + I3C_TX_DATA_PORT), word);
    }
    return bytes_written;
}

static uint16_t ReadDataFromRXFifo( uint8_t instance, uint8_t *data,
        uint16_t length )
{
    uint16_t numWords;
    uint16_t numBytes, i, j;

    uint32_t word;
    uint8_t *buf = data;
    uint16_t bytes_read = 0U;
    uint32_t rx_fill_level, read_len;

    rx_fill_level =
            RD_REG32(
            (i3c_Obj[instance].regBase + I3C_DATA_BUFFER_STATUS_LEVEL));
    rx_fill_level =
            ((rx_fill_level & I3C_DATA_BUFFER_STATUS_LEVEL_RX_BUF_BLR_MASK) >>
            I3C_DATA_BUFFER_STATUS_LEVEL_RX_BUF_BLR_POS);
    rx_fill_level = rx_fill_level * 4U;

    read_len = (uint32_t) length < rx_fill_level? (uint32_t) length: rx_fill_level;
    numWords = (uint16_t) (read_len / RX_TX_DATA_PORT_SIZE);
    numBytes = (uint16_t) (read_len & (RX_TX_DATA_PORT_SIZE - 1U));

    for (i = 0U; i < numWords; i++)
    {
        word = RD_REG32((i3c_Obj[instance].regBase + I3C_TX_DATA_PORT));
        for (j = 0U; j < RX_TX_DATA_PORT_SIZE; j++)
        {
            buf[j] = (uint8_t) (word & 0xFFU);
            word >>= 8U;
        }
        buf += RX_TX_DATA_PORT_SIZE;
    }
    bytes_read = numWords * 4U;
    if (numBytes > 0U) {
        word = RD_REG32((i3c_Obj[instance].regBase + I3C_TX_DATA_PORT));
        for (j = 0U; j < numBytes; j++)
        {
            buf[j] = (uint8_t) (word & 0xFFU);
            word >>= 8U;
            bytes_read++;
        }
    }

    return bytes_read;
}


/* Function to read the response for the xfer requests, will be called from ISR for I3C controller.*/
static int32_t ReadXferResponse(uint8_t instance)
{
    union i3c_xferResponse status;
    struct i3c_CmdObj *pCmdObj;

    uint8_t i, numResponse,idx;
    int32_t error = I3C_OK;

    /* read the number of response in the response queue*/
    numResponse =
            (uint8_t) HAL_REG_READ_FIELD((i3c_Obj[instance].regBase +
            I3C_QUEUE_STATUS_LEVEL),
            I3C_QUEUE_STATUS_LEVEL_RESP_BUF_BLR_POS,
            I3C_QUEUE_STATUS_LEVEL_RESP_BUF_BLR_MASK);

    for (i = 0; i < numResponse; i++)
    {
        status.value =
                RD_REG32((i3c_Obj[instance].regBase + I3C_RESPONSE_QUEUE_PORT));

        /* get the transaction ID,this would be the index in the
           internal command object array
         */
        idx = status.field.tid;

        pCmdObj = &i3c_Obj[instance].cmdObj[idx];

        pCmdObj->rx_length = status.field.dl;
        pCmdObj->status    = status.field.err_sts;

        switch (pCmdObj->status)
        {
        case I3C_RESPONSE_OK:
            break;

        case I3C_RESPONSE_CRC_ERROR:
        case I3C_RESPONSE_PARITY_ERROR:
        case I3C_RESPONSE_FRAME_ERROR:
        case I3C_RESPONSE_BRAODCAST_NAK:
        case I3C_RESPONSE_XFER_ABORT:
            error = I3C_IO;
            break;

        case I3C_RESPONSE_BUF_OVERFLOW:
            error = I3C_NOMEM;
            break;

        case I3C_RESPONSE_ADDRESS_NAK:
        case I3C_RESPONSE_SLAVE_WRITE_NACK:
        default:
            error = I3C_INVALID;
            break;
        }
    }

    if (error != I3C_OK)
    {
        /* RESET THE Rx/Tx , COMMAND AND RESPONSE Queue */
        WR_REG32((i3c_Obj[instance].regBase + I3C_RESET_CTRL),
                (I3C_RESET_CTRL_RX_FIFO_RST_MASK |
                I3C_RESET_CTRL_TX_FIFO_RST_MASK |
                I3C_RESET_CTRL_RESP_QUEUE_RST_MASK |
                I3C_RESET_CTRL_CMD_QUEUE_RST_MASK));

        /* Resume the I3C controller instance after error */
        HAL_REG_WRITE_FIELD((i3c_Obj[instance].regBase + I3C_DEVICE_CTRL),
                I3C_DEVICE_CTRL_RESUME_POS, I3C_DEVICE_CTRL_RESUME_MASK,
                1U);

    }

    if (osal_semaphore_post(i3c_Obj[(instance)].xferComplete) == false)
    {
        return I3C_INVALID;
    }

    return error;
}

/* sends the actual transfer request on to the i3C bus
   by writing onto the command queue port of the controller
 */
static void  RequestTransfer(uint8_t instance)
{
    uint8_t i;
    uint16_t xBytes = 0;

    for (i = 0; i < i3c_Obj[instance].numXfers; i++)
    {
        if (i3c_Obj[instance].cmdObj[i].cmd.xfer.field.rnw == 0U)
        {
            i3c_Obj[instance].cmdObj[i].usWriteBytesLeft =
                    i3c_Obj[instance].cmdObj[i].tx_length;
            i3c_Obj[instance].cmdObj[i].puWriteBuffer    =
                    i3c_Obj[instance].cmdObj[i].data;
        }
        /* Push the data onto the TX data FIFO*/
        xBytes = PushDataToTXFifo(instance,
                i3c_Obj[instance].cmdObj[i].data,
                i3c_Obj[instance].cmdObj[i].tx_length);
        /*Find the remaining amount of data
         * that has to be written to
         * the FIFO for next transfer
         */
        if (i3c_Obj[instance].cmdObj[i].cmd.xfer.field.rnw == 0U)
        {
            i3c_Obj[instance].cmdObj[i].usWriteBytesLeft -= xBytes;
            if (i3c_Obj[instance].cmdObj[i].usWriteBytesLeft > 0U)
            {
                i3c_Obj[instance].cmdObj[i].puWriteBuffer    += xBytes;

                uint32_t interruptSignalEnable =
                        RD_REG32(
                        (i3c_Obj[instance].regBase + I3C_INTR_SIGNAL_EN));
                interruptSignalEnable |= TX_THLD_INTR;
                WR_REG32(
                        (i3c_Obj[instance].regBase + I3C_INTR_SIGNAL_EN),
                        interruptSignalEnable);
            }
        }
    }
    /* update the response threshold*/
    HAL_REG_WRITE_FIELD((i3c_Obj[instance].regBase + I3C_QUEUE_THLD_CTRL),
            I3C_QUEUE_THLD_CTRL_RESP_BUF_THLD_POS,
            I3C_QUEUE_THLD_CTRL_RESP_BUF_THLD_MASK,
            (i3c_Obj[instance].numXfers - 1U));

    for (i = 0; i < i3c_Obj[instance].numXfers; i++)
    {
        /* enque the command and argument*/
        WR_REG32((i3c_Obj[instance].regBase + I3C_COMMAND_QUEUE_PORT),
                i3c_Obj[instance].cmdObj[i].arg.value);
        WR_REG32((i3c_Obj[instance].regBase + I3C_COMMAND_QUEUE_PORT),
                i3c_Obj[instance].cmdObj[i].cmd.xfer.value);
    }
    return;
}

/**
 * @brief Interrupt Service Routine for the I3C controller.
 *
 * @param[in] param  Pointer to the I3C controller instance.
 * @return None.
 */
void i3c_Isr( void *param )
{
    struct i3c_DriverObj *i3cDriverObj = (struct i3c_DriverObj *)param;
    uint8_t instance =
            (i3cDriverObj->isPrimary == true) ? I3C_INSTANCE1 : I3C_INSTANCE2;
    uint32_t status;
    uint16_t xBytes;

    status = RD_REG32(i3cDriverObj->regBase + I3C_INTR_STATUS);

    /*Interrupt due to the Tx FIFO being empty
     * beyond the threshold
     */
    if ((status & I3C_INTR_STATUS_TX_THLD_STS_MASK) != 0U)
    {
        for (uint32_t i = 0; i < i3cDriverObj->numXfers; i++)
        {
            if (i3cDriverObj->cmdObj[i].usWriteBytesLeft > 0U)
            {
                /*Once there are free slots in Tx FIFO
                 * start filling data to it
                 */
                xBytes = PushDataToTXFifo(instance,
                        i3cDriverObj->cmdObj[i].puWriteBuffer,
                        i3cDriverObj->cmdObj[i].usWriteBytesLeft);

                /*Update bytes left and the buffer*/
                i3cDriverObj->cmdObj[i].usWriteBytesLeft -= xBytes;
                i3cDriverObj->cmdObj[i].puWriteBuffer    += xBytes;
            }
        }
    }
    /*Interrupt due to Rx FIFO being full
     * beyond the threshold
     */
    if ((status & I3C_INTR_STATUS_RX_THLD_STS_MASK) != 0U)
    {
        for (uint32_t i = 0; i < i3cDriverObj->numXfers; i++)
        {
            if (i3cDriverObj->cmdObj[i].usReadBytesLeft > 0U)
            {
                /*Once the Rx FIFO is full
                 * start reading from it
                 */
                xBytes =  ReadDataFromRXFifo(instance,
                        i3cDriverObj->cmdObj[i].puReadBuffer,
                        i3cDriverObj->cmdObj[i].usReadBytesLeft);
                i3cDriverObj->cmdObj[i].usReadBytesLeft -= xBytes;
                i3cDriverObj->cmdObj[i].puReadBuffer    += xBytes;
            }
        }
    }
    /*Interrupt received due to response from slave*/
    if ((status &
            (I3C_INTR_STATUS_TRANSFER_ERR_STS_MASK |
            I3C_INTR_STATUS_RESP_READY_STS_MASK)) != 0U)
    {
        (void) ReadXferResponse(instance);

        /* clear the error status*/
        if ((status & I3C_INTR_STATUS_TRANSFER_ERR_STS_MASK) != 0U) {
            WR_REG32((i3cDriverObj->regBase + I3C_INTR_STATUS),
                    I3C_INTR_STATUS_TRANSFER_ERR_STS_MASK);
        }
    }

}


/**
 * @brief Configure the SCL timing parameters for the I3C controller.
 *
 * @param[in] instance  Instance of the I3C controller.
 * @return None.
 */
static void i3c_ll_configureSCL(uint8_t instance)
{
    uint32_t coreClk, coreRate, corePeriod;
    uint32_t hcnt, lcnt;

    coreClk    = I3C_CORE_CLOCK;
    coreRate   = coreClk;

    if (coreClk == 0U)
    {
        return;
    }
    corePeriod = ((NANO_SEC + (coreClk - 1U)) / coreClk);  /* round up and in nano seconds */

    if (corePeriod == 0U)
    {
        return;
    }
    /* I3C_PP */
    hcnt = (I3C_BUS_THIGH_MAX_NS + (corePeriod - 1U)) / corePeriod;
    hcnt -= 1U;
    if (hcnt < SCL_I3C_TIMING_CNT_MIN) {
        hcnt = SCL_I3C_TIMING_CNT_MIN;
    }

    lcnt = (coreRate + (I3C_BUS_TYP_I3C_SCL_RATE - 1U)) /
            I3C_BUS_TYP_I3C_SCL_RATE;
    lcnt -= hcnt;
    if (lcnt < SCL_I3C_TIMING_CNT_MIN) {
        lcnt = SCL_I3C_TIMING_CNT_MIN;
    }

    /* i3c scl Push pull (PP) timing*/
    HAL_REG_WRITE_FIELD((i3c_Obj[instance].regBase + I3C_SCL_I3C_PP_TIMING),
            I3C_SCL_I3C_PP_TIMING_I3C_PP_LCNT_POS,
            I3C_SCL_I3C_PP_TIMING_I3C_PP_LCNT_MASK, lcnt);
    HAL_REG_WRITE_FIELD((i3c_Obj[instance].regBase + I3C_SCL_I3C_PP_TIMING),
            I3C_SCL_I3C_PP_TIMING_I3C_PP_HCNT_POS,
            I3C_SCL_I3C_PP_TIMING_I3C_PP_HCNT_MASK, hcnt);

    /* update BUS free timing*/
    WR_REG32((i3c_Obj[instance].regBase + I3C_BUS_FREE_AVAIL_TIMING),
            lcnt);

    /* i3c scl open-drain (OD) timing*/
    lcnt = (I3C_BUS_TLOW_OD_MIN_NS + (corePeriod - 1U)) / corePeriod;
    HAL_REG_WRITE_FIELD((i3c_Obj[instance].regBase + I3C_SCL_I3C_OD_TIMING),
            I3C_SCL_I3C_OD_TIMING_I3C_OD_LCNT_POS,
            I3C_SCL_I3C_OD_TIMING_I3C_OD_LCNT_MASK, lcnt);

    HAL_REG_WRITE_FIELD((i3c_Obj[instance].regBase + I3C_SCL_I3C_OD_TIMING),
            I3C_SCL_I3C_OD_TIMING_I3C_OD_HCNT_POS,
            I3C_SCL_I3C_OD_TIMING_I3C_OD_HCNT_MASK, hcnt);


    /* I3C SCL extended low periods for I3C Transfers (EXT_LCNT) timings*/
    lcnt = ((coreRate + (I3C_BUS_SDR1_SCL_RATE - 1U)) / I3C_BUS_SDR1_SCL_RATE) -
            hcnt;
    HAL_REG_WRITE_FIELD((i3c_Obj[instance].regBase + I3C_SCL_EXT_LCNT_TIMING),
            I3C_SCL_EXT_LCNT_TIMING_I3C_EXT_LCNT_1_POS,
            I3C_SCL_EXT_LCNT_TIMING_I3C_EXT_LCNT_1_MASK, lcnt);

    lcnt = ((coreRate + (I3C_BUS_SDR2_SCL_RATE - 1U)) / I3C_BUS_SDR2_SCL_RATE) -
            hcnt;
    HAL_REG_WRITE_FIELD((i3c_Obj[instance].regBase + I3C_SCL_EXT_LCNT_TIMING),
            I3C_SCL_EXT_LCNT_TIMING_I3C_EXT_LCNT_2_POS,
            I3C_SCL_EXT_LCNT_TIMING_I3C_EXT_LCNT_2_MASK, lcnt);

    lcnt = ((coreRate + (I3C_BUS_SDR3_SCL_RATE - 1U)) / I3C_BUS_SDR3_SCL_RATE) -
            hcnt;
    HAL_REG_WRITE_FIELD((i3c_Obj[instance].regBase + I3C_SCL_EXT_LCNT_TIMING),
            I3C_SCL_EXT_LCNT_TIMING_I3C_EXT_LCNT_3_POS,
            I3C_SCL_EXT_LCNT_TIMING_I3C_EXT_LCNT_3_MASK, lcnt);

    lcnt = ((coreRate + (I3C_BUS_SDR4_SCL_RATE - 1U)) / I3C_BUS_SDR4_SCL_RATE) -
            hcnt;
    HAL_REG_WRITE_FIELD((i3c_Obj[instance].regBase + I3C_SCL_EXT_LCNT_TIMING),
            I3C_SCL_EXT_LCNT_TIMING_I3C_EXT_LCNT_4_POS,
            I3C_SCL_EXT_LCNT_TIMING_I3C_EXT_LCNT_4_MASK, lcnt);

    /* I2C Fast mode timing */
    lcnt = (I3C_BUS_I2C_FM_TLOW_MIN_NS + (corePeriod - 1U)) / corePeriod;
    hcnt =
            ((coreRate + (I3C_BUS_I2C_FM_SCL_RATE - 1U)) /
            I3C_BUS_I2C_FM_SCL_RATE) - lcnt;
    HAL_REG_WRITE_FIELD((i3c_Obj[instance].regBase + I3C_SCL_I2C_FM_TIMING),
            I3C_SCL_I2C_FM_TIMING_I2C_FM_LCNT_POS,
            I3C_SCL_I2C_FM_TIMING_I2C_FM_LCNT_MASK, lcnt);

    HAL_REG_WRITE_FIELD((i3c_Obj[instance].regBase + I3C_SCL_I2C_FM_TIMING),
            I3C_SCL_I2C_FM_TIMING_I2C_FM_HCNT_POS,
            I3C_SCL_I2C_FM_TIMING_I2C_FM_HCNT_MASK, hcnt);

    /* I2C Fast mode Plus timing */
    lcnt = (I3C_BUS_I2C_FMP_TLOW_MIN_NS + (corePeriod - 1U)) / corePeriod;
    hcnt =
            ((coreRate + (I3C_BUS_I2C_FM_PLUS_SCL_RATE - 1U)) /
            I3C_BUS_I2C_FM_PLUS_SCL_RATE) - lcnt;
    HAL_REG_WRITE_FIELD((i3c_Obj[instance].regBase + I3C_SCL_I2C_FMP_TIMING),
            I3C_SCL_I2C_FMP_TIMING_I2C_FMP_LCNT_POS,
            I3C_SCL_I2C_FMP_TIMING_I2C_FMP_LCNT_MASK, lcnt);

    HAL_REG_WRITE_FIELD((i3c_Obj[instance].regBase + I3C_SCL_I2C_FMP_TIMING),
            I3C_SCL_I2C_FMP_TIMING_I2C_FMP_HCNT_POS,
            I3C_SCL_I2C_FMP_TIMING_I2C_FMP_HCNT_MASK, hcnt);

    return;
}

/**
 * @brief Initialize I3C controller hardware and configure GPIO pins and timing parameters.
 *
 * @param[in] instance  Instance of the I3C controller.
 * @return None.
 */
void i3c_ll_init(uint8_t instance)
{
    uint32_t role;
    uint32_t interruptStatus = 0, interruptSignal = 0;
    uint8_t bitpos;
    uint32_t mask;
    SocfpgaHpuInterrupt_t xIntID;
    SocfpgaInterruptErr_t xIntRet;

    /* take the I3C module instance out of reset in the reset manager*/
    bitpos = PER1MODRST_I3C0_POS + instance;
    mask   = (1U << (uint32_t) bitpos);

    /* clearing the bit in the mod reset register will take the
       module out of reset */
    HAL_REG_WRITE_FIELD(PER1MODRST, bitpos, mask, 0U);

    /* delay for clearing the mod reset*/
    for (int i = 0; i < 1000000; i++)
    {

    }

    /* set the register base address */
    i3c_Obj[instance].regBase = I3C_CONTROLLER_REGISTER_BASE(instance);
    /* configure SCL Timing*/
    i3c_ll_configureSCL(instance);

    /* update the role of the controller instance*/
    role = HAL_REG_READ_FIELD((i3c_Obj[instance].regBase + I3C_HW_CAPABILITY),
            I3C_HW_CAPABILITY_DEVICE_ROLE_CONFIG_POS,
            I3C_HW_CAPABILITY_DEVICE_ROLE_CONFIG_MASK);

    i3c_Obj[instance].isPrimary = (role == I3C_CONTROLLER_MASTER);

    /* HDR is currently no supported*/
    i3c_Obj[instance].supportHDR  = false;
    i3c_Obj[instance].datFreeMask = ((uint32_t) 1U << 31U) - 1U; /* ideally it should be (1<< total Slaves)- 1*/

    i3c_Obj[instance].devAddressTable =
            (uint32_t *)(uintptr_t) HAL_REG_READ_FIELD((i3c_Obj[instance].
            regBase + I3C_DEVICE_ADDR_TABLE_POINTER),
            I3C_DEVICE_ADDR_TABLE_POINTER_P_DEV_ADDR_TABLE_START_ADDR_POS,
            I3C_DEVICE_ADDR_TABLE_POINTER_P_DEV_ADDR_TABLE_START_ADDR_MASK);
    i3c_Obj[instance].devCharTable    =
            (uint32_t *)(uintptr_t) HAL_REG_READ_FIELD((i3c_Obj[instance].
            regBase + I3C_DEV_CHAR_TABLE_POINTER),
            I3C_DEV_CHAR_TABLE_POINTER_P_DEV_CHAR_TABLE_START_ADDR_POS,
            I3C_DEV_CHAR_TABLE_POINTER_P_DEV_CHAR_TABLE_START_ADDR_MASK);

    /* update the own Dynamic Address*/
    HAL_REG_WRITE_FIELD((i3c_Obj[instance].regBase + I3C_DEVICE_ADDR),
            I3C_DEVICE_ADDR_DYNAMIC_ADDR_VALID_POS,
            I3C_DEVICE_ADDR_DYNAMIC_ADDR_VALID_MASK, 1U);
    HAL_REG_WRITE_FIELD((i3c_Obj[instance].regBase + I3C_DEVICE_ADDR),
            I3C_DEVICE_ADDR_DYNAMIC_ADDR_POS,
            I3C_DEVICE_ADDR_DYNAMIC_ADDR_MASK, i3c_Obj[instance].ownDA);


    /* Enable the GIC interrupts and attach the interrupt service routine */
    /*Setup and enable interrupts in GIC*/
    xIntID = GET_I3C_INTERRUPT_ID(instance);
    xIntRet = interrupt_register_isr(xIntID, i3c_Isr,
            &i3c_Obj[instance]);
    if (xIntRet != eErrOk)
    {
        return;
    }

    xIntRet = interrupt_enable(xIntID, GIC_INTERRUPT_PRIORITY_I3C);
    if (xIntRet != eErrOk)
    {
        return;
    }

    /*Set the Rx thresholds to 16*/
    uint32_t rx_thld =
            RD_REG32((i3c_Obj[instance].regBase + I3C_DATA_BUFFER_THLD_CTRL));
    rx_thld |=
            ((uint32_t) I3C_DATA_BUFFER_THLD_CTRL_LVL16 <<
            I3C_DATA_BUFFER_THLD_CTRL_RX_BUF_THLD_POS);
    WR_REG32((i3c_Obj[instance].regBase + I3C_DATA_BUFFER_THLD_CTRL),
            rx_thld);

    uint32_t tx_thld =
            RD_REG32((i3c_Obj[instance].regBase + I3C_DATA_BUFFER_THLD_CTRL));
    tx_thld |=
            (I3C_DATA_BUFFER_THLD_CTRL_LVL16 <<
            I3C_DATA_BUFFER_THLD_CTRL_TX_EMPTY_BUF_THLD_POS);
    WR_REG32((i3c_Obj[instance].regBase + I3C_DATA_BUFFER_THLD_CTRL),
            tx_thld);

    /* Enable transfer err, response ready and IBI threhold interrupt signals*/
    interruptStatus = TRANSFER_ERR_INTR | RESP_READY_INTR | RX_THLD_INTR |
            TX_THLD_INTR;                                                               //| IBI_THLD_INTR;
    interruptSignal = TRANSFER_ERR_INTR | RESP_READY_INTR | RX_THLD_INTR;

    /* status enable and signal enable has same bitfields*/
    WR_REG32((i3c_Obj[instance].regBase + I3C_INTR_STATUS_EN),
            interruptStatus);
    WR_REG32((i3c_Obj[instance].regBase + I3C_INTR_SIGNAL_EN),
            interruptSignal);

    /* clear any previous interrupts if any */
    WR_REG32((i3c_Obj[instance].regBase + I3C_INTR_STATUS),
            interruptStatus);

    /* disable in-band master request and Slave interrupt request*/
    {
        WR_REG32((i3c_Obj[instance].regBase + I3C_IBI_SIR_REQ_REJECT),
                SIR_REQ_REJECT_MASK);
        WR_REG32((i3c_Obj[instance].regBase + I3C_IBI_MR_REQ_REJECT),
                MR_REQ_REJECT_MASK);

        /* notify if  sir and ibi are rejected    */
        WR_REG32((i3c_Obj[instance].regBase + I3C_IBI_QUEUE_CTRL),
                0xB /*1011b*/);
    }

    /* clear response and ibi threshold levels */
    mask = RD_REG32((i3c_Obj[instance].regBase + I3C_QUEUE_THLD_CTRL));
    mask &=
            ((~I3C_QUEUE_THLD_CTRL_IBI_STATUS_THLD_MASK) &
            (~I3C_QUEUE_THLD_CTRL_RESP_BUF_THLD_MASK));
    WR_REG32((i3c_Obj[instance].regBase + I3C_QUEUE_THLD_CTRL), mask);

    HAL_REG_WRITE_FIELD((i3c_Obj[instance].regBase + I3C_DATA_BUFFER_THLD_CTRL),
            I3C_DATA_BUFFER_THLD_CTRL_RX_BUF_THLD_POS,
            I3C_DATA_BUFFER_THLD_CTRL_RX_BUF_THLD_MASK, 0U);

    /* read the command and data fifo depths */
    i3c_Obj[instance].cmdFIFODepth =
            HAL_REG_READ_FIELD((i3c_Obj[instance].regBase +
            I3C_QUEUE_STATUS_LEVEL),
            I3C_QUEUE_STATUS_LEVEL_CMD_QUEUE_EMPTY_LOC_POS,
            I3C_QUEUE_STATUS_LEVEL_CMD_QUEUE_EMPTY_LOC_MASK);

    i3c_Obj[instance].dataFIFODepth =
            HAL_REG_READ_FIELD(
            (i3c_Obj[instance].regBase + I3C_DATA_BUFFER_STATUS_LEVEL),
            I3C_DATA_BUFFER_STATUS_LEVEL_TX_BUF_EMPTY_LOC_POS,
            I3C_DATA_BUFFER_STATUS_LEVEL_TX_BUF_EMPTY_LOC_MASK);

    /* Enable I3C controller */
    HAL_REG_WRITE_FIELD((i3c_Obj[instance].regBase + I3C_DEVICE_CTRL),
            I3C_DEVICE_CTRL_ENABLE_POS, I3C_DEVICE_CTRL_ENABLE_MASK,
            1U);

    return;
}

/**
 * @brief Send CCC commands, supporting both direct and broadcast, and block until response.
 *
 * @param[in] instance     Instance of the I3C controller.
 * @param[in] pCmdPayload  Pointer to the command payload list.
 * @param[in] numCmds      Number of commands.
 * @param[in] isI2c        Indicates if the transfer is for an I2C device.
 * @return int32_t         I3C_OK if the operation was successful.
 */
int32_t i3c_ll_SendXferCommand(uint8_t instance,
        struct i3c_CmdPayload *pCmdPayload,
        uint8_t numCmds, bool isI2c)
{
    struct i3c_CmdObj *pCmdObj;
    struct i3c_CmdPayload *pCmd = pCmdPayload;
    uint8_t idx = 0, i;
    int32_t ret = I3C_OK;
    uint16_t xBytes = 0;

    if ((instance >= I3C_NUM_INSTANCES) || (pCmdPayload == NULL))
    {
        return I3C_PARAM;
    }

    for (i = 0U; i < numCmds; i++)
    {
        pCmdObj = &i3c_Obj[instance].cmdObj[i];
        if (pCmd->targetAddress == 0U)
        {
            /* Clear bit 7 to make it a broadcast command*/
            pCmd->cmdId &= (uint8_t) (~(1U << 7U));
        }
        else
        {
            for (idx = 0U; idx < i3c_Obj[instance].numAttachedDev; idx++)
            {
                if (pCmd->targetAddress ==
                        i3c_Obj[instance].attachedDevAddrList[idx])
                {
                    break;
                }
            }
        }

        /* initialize the command object */
        (void) memset(pCmdObj, 0, sizeof(*pCmdObj));

        /* fill up the command argument */
        pCmdObj->arg.field.dl          = pCmd->dataLength;
        pCmdObj->arg.field.cmd_attr    = I3C_CCC_TRANSFER_ARG;

        /* fill up the transfer command if specified*/
        if (pCmd->cmdId != 0U) {
            pCmdObj->cmd.xfer.field.cp        = 1;                              /* command present */
            pCmdObj->cmd.xfer.field.cmd       = pCmd->cmdId;
        }
        pCmdObj->cmd.xfer.field.cmd_attr  = I3C_CCC_TRANSFER_CMD;
        pCmdObj->cmd.xfer.field.tid       = i;
        pCmdObj->cmd.xfer.field.dev_index = idx;
        pCmdObj->cmd.xfer.field.rnw       = (pCmd->read == true) ? 1U : 0U;
        pCmdObj->cmd.xfer.field.roc       = 1;                                  /* request response */
        pCmdObj->cmd.xfer.field.toc       = (uint32_t)(i == (numCmds - 1U));               /* terminate on completion of the last command*/


        if (isI2c) {
            pCmdObj->cmd.xfer.field.speed = 1;
        }

        pCmdObj->data   = pCmd->data;
        if (pCmd->read == true)
        {
            pCmdObj->rx_length = pCmd->dataLength;
            pCmdObj->usReadBytesLeft     =  pCmd->dataLength;
            pCmdObj->puReadBuffer        =  pCmd->data;
        }
        else
        {
            pCmdObj->tx_length = pCmd->dataLength;
        }

        /* get the next command in the list*/
        pCmd++;
    }
    i3c_Obj[instance].numXfers = numCmds;

    /*Initiate the command transfer*/
    RequestTransfer(instance);

    /* wait for the command transfer to complete, signaled by the xfer semaphore*/
    ret = WAIT_FOR_XFER_COMPLETE(instance);

    /*Disable the TX THRESHOLD INTERRUPT*/
    uint32_t interruptSignalEnable =
            RD_REG32((i3c_Obj[instance].regBase + I3C_INTR_SIGNAL_EN));
    interruptSignalEnable &= ~TX_THLD_INTR;
    WR_REG32((i3c_Obj[instance].regBase + I3C_INTR_SIGNAL_EN),
            interruptSignalEnable);

    if (ret == I3C_OK)
    {
        pCmd = pCmdPayload;
        for (i = 0; i < i3c_Obj[instance].numXfers; i++)
        {
            /*Read last response data from the RX data fifo*/
            if ((i3c_Obj[instance].cmdObj[i].rx_length > 0U) &&
                    (i3c_Obj[instance].cmdObj[i].status == I3C_RESPONSE_OK)) {
                if (i3c_Obj[instance].cmdObj[i].usReadBytesLeft > 0U)
                {
                    xBytes =  ReadDataFromRXFifo(instance,
                            i3c_Obj[instance].cmdObj[i].puReadBuffer,
                            i3c_Obj[instance].cmdObj[i].usReadBytesLeft);

                    i3c_Obj[instance].cmdObj[i].usReadBytesLeft -= xBytes;
                    i3c_Obj[instance].cmdObj[i].puReadBuffer    += xBytes;
                }
                pCmd->dataLength = i3c_Obj[instance].cmdObj[i].rx_length;
            }
            else
            {
                ret |= i3c_Obj[instance].cmdObj[i].status;
            }
            pCmd++;
        }
    }
    return ret;
}

/**
 * @brief Send address assignment command (ENTDAA or ENTDASA) and block until response.
 *
 * @param[in] instance     Instance of the I3C controller.
 * @param[in] pCmdPayload  Pointer to the command payload.
 * @param[in] startIdx     Start index in the DAT which contains the dynamic address that should be
 *                         assigned during the DAA process.
 * @return int32_t         I3C_OK if the operation was successful.
 */
int32_t i3c_ll_SendAddrAssignmentCommand(uint8_t instance,
        struct i3c_CmdPayload *pCmdPayload,
        uint32_t startIdx)
{
    struct i3c_CmdObj *pCmdObj;
    int32_t ret = I3C_OK;
    uint8_t i;

    if ((instance >= I3C_NUM_INSTANCES) || (pCmdPayload == NULL))
    {
        return I3C_INVALID;
    }

    pCmdObj = &i3c_Obj[instance].cmdObj[0];

    /* initialize the command object */
    (void) memset(pCmdObj, 0, sizeof(*pCmdObj));

    /* fill up the command argument */
    pCmdObj->arg.field.dl          = 0U;
    pCmdObj->arg.field.cmd_attr    = I3C_CCC_TRANSFER_ARG;

    /* fill up the address assignment command*/
    pCmdObj->cmd.addr.field.cmd_attr  = I3C_CCC_ADDRESS_ASSIGN_CMD;
    pCmdObj->cmd.addr.field.cmd       = pCmdPayload->cmdId;
    pCmdObj->cmd.xfer.field.tid       = 0U;
    pCmdObj->cmd.addr.field.dev_index = startIdx;
    pCmdObj->cmd.addr.field.dev_count = pCmdPayload->dataLength;    /* for address assignment datalength indicates number of devices*/
    pCmdObj->cmd.addr.field.toc       = 1;                          /* terminate on completion*/
    pCmdObj->cmd.addr.field.roc       = 1;                          /* request response */

    pCmdObj->data      = NULL;
    pCmdObj->tx_length = 0;
    pCmdObj->rx_length = 0;

    i3c_Obj[instance].numXfers = 1;

    /* initiate the command transfer*/
    {
        /* update the response threshold*/
        HAL_REG_WRITE_FIELD((i3c_Obj[instance].regBase + I3C_QUEUE_THLD_CTRL),
                I3C_QUEUE_THLD_CTRL_RESP_BUF_THLD_POS,
                I3C_QUEUE_THLD_CTRL_RESP_BUF_THLD_MASK,
                (i3c_Obj[instance].numXfers - 1U));

        for (i = 0U; i < i3c_Obj[instance].numXfers; i++)
        {
            /* enque the command and argument*/
            WR_REG32(
                    (i3c_Obj[instance].regBase + I3C_COMMAND_QUEUE_PORT),
                    i3c_Obj[instance].cmdObj[i].arg.value);
            WR_REG32(
                    (i3c_Obj[instance].regBase + I3C_COMMAND_QUEUE_PORT),
                    i3c_Obj[instance].cmdObj[i].cmd.addr.value);
        }
    }

    /* wait for the command tranfer to complete, signaled by the xfer semaphore*/
    ret = WAIT_FOR_XFER_COMPLETE(instance);

    for (i = 0; (ret == I3C_OK) && (i < i3c_Obj[instance].numXfers); i++)
    {
        if ((i3c_Obj[instance].cmdObj[i].rx_length > 0U) &&
                (i3c_Obj[instance].cmdObj[i].status == I3C_RESPONSE_OK)) {
            /* read the response data from the RX data fifo*/
            if (ReadDataFromRXFifo(instance,
                    i3c_Obj[instance].cmdObj[i].data,
                    i3c_Obj[instance].cmdObj[i].rx_length) == 0U)
            {
                return I3C_INVALID;
            }
        }
        else
        {
            ret |= i3c_Obj[instance].cmdObj[i].status;
        }
    }

    return ret;
}

/**
 * @brief Attach an I2C device to the I3C controller and update relevant data structures.
 *
 * @param[in] instance     Instance of the I3C controller.
 * @param[in] pDeviceDesc  Pointer to the device descriptor.
 * @param[in] address      Address of the device.
 * @return int32_t         I3C_OK if the operation was successful.
 */
int32_t i3c_ll_AttachI2cDevice(uint8_t instance,
        struct i3c_DeviceDesc *pDeviceDesc,
        uint8_t address){

    uint32_t devIdx;
    int32_t ret = I3C_OK;
    uint32_t addrEntry;
    uint32_t addrEntry_val;

    devIdx = GetNextFreePosition(instance);

    /*Get the table entry to which the index points to*/
    addrEntry = (uint32_t)(uintptr_t)(i3c_Obj[instance].devAddressTable) +
            ((devIdx * sizeof(uint32_t)));

    addrEntry_val = 0U;

    /*Set the LEGACY_I2C_BIT*/
    addrEntry_val = ((uint32_t) 1 << LEGACY_I2C_MASK);

    /*Store the static address of the device*/
    addrEntry_val |=
            (((uint32_t) address <<
            I3C_DEV_ADDR_TABLE1_LOC1_STATIC_ADDRESS_POS) &
            (I3C_DEV_ADDR_TABLE1_LOC1_STATIC_ADDRESS_MASK));

    WR_REG32((i3c_Obj[instance].regBase + addrEntry), addrEntry_val);


    /* store the index of dat in the device structure for later references*/
    pDeviceDesc->datIndex = devIdx;

    /* clear the bit value in datFreeMask for the position*/
    i3c_Obj[instance].datFreeMask &= ~(1U << devIdx);

    /* add the device address to the attached devices list. */
    i3c_Obj[instance].attachedDevAddrList[devIdx] = address;

    /*Increment the count of the attached I2C devices*/
    i3c_Obj[instance].numAttachedDev++;

    return ret;
}

/**
 * @brief Attach a device to the I3C controller and update relevant data structures.
 *
 * @param[in] instance     Instance of the I3C controller.
 * @param[in] pDeviceDesc  Pointer to the device descriptor.
 * @param[in] address      Address of the device.
 * @return int32_t         I3C_OK if the operation was successful.
 */
int32_t i3c_ll_AttachDevice(uint8_t instance,
        struct i3c_DeviceDesc *pDeviceDesc, uint8_t address)
{
    uint32_t devIdx;
    int32_t ret = I3C_OK;
    uint32_t addrEntry;
    uint8_t tempAddress, p;

    devIdx = GetNextFreePosition(instance);

    /* add the address to the device table address at index devIdx */
    addrEntry = (uint32_t)(uintptr_t)(i3c_Obj[instance].devAddressTable) +
            ((devIdx * sizeof(uint32_t)));                                                      /* LOC register offsets are 4 bytes apart*/

    p = (address ^ (address >> 4U)) & 0xFU;
    p = ((uint8_t)(0x9669U >> p)) & 0x1U;
    tempAddress = address | (p << 7);
    WR_REG32((i3c_Obj[instance].regBase + addrEntry),
            (uint32_t) (((uint32_t) tempAddress <<
                I3C_DEV_ADDR_TABLE1_LOC1_DEV_DYNAMIC_ADDR_POS) &
            I3C_DEV_ADDR_TABLE1_LOC1_DEV_DYNAMIC_ADDR_MASK));

    /* store the index of dat in the device structure for later references*/
    pDeviceDesc->datIndex = devIdx;

    /* clear the bit value in datFreeMask for the position*/
    i3c_Obj[instance].datFreeMask &= ~(1U << devIdx);

    /* add the device address to the attached devices list. */
    i3c_Obj[instance].attachedDevAddrList[devIdx] = address;

    /* increment the number of attached devices*/
    i3c_Obj[instance].numAttachedDev++;

    return ret;
}

/**
 * @brief Remove device dynamic address from DAT and update relevant data structures.
 *
 * @param[in] instance     Instance of the I3C controller.
 * @param[in] pDeviceDesc  Pointer to the device descriptor.
 * @param[in] address      Address of the device.
 * @return int32_t         I3C_OK if the operation was successful.
 */
int32_t i3c_ll_DetachDevice(uint8_t instance,
        struct i3c_DeviceDesc *pDeviceDesc, uint8_t address)
{
    uint8_t curAddr;
    uint32_t addrEntry;
    int32_t ret = I3C_OK;
    uint32_t idx = pDeviceDesc->datIndex; /* current index of the device in the DAT*/

    /* find the address entry in the device table address */
    addrEntry = (uint32_t)(uintptr_t)(i3c_Obj[instance].devAddressTable) +
            ((idx * sizeof(uint32_t)) << 2);                                                      /* LOC register offsets are 4 bytes apart*/

    curAddr = (uint8_t) HAL_REG_READ_FIELD((i3c_Obj[instance].regBase + addrEntry),
            I3C_DEV_ADDR_TABLE1_LOC1_DEV_DYNAMIC_ADDR_POS,
            I3C_DEV_ADDR_TABLE1_LOC1_DEV_DYNAMIC_ADDR_MASK);

    /* confirm the index corresponds to the same device */
    if (curAddr == address)
    {
        /* set the address to 0 in the index corresponding the address*/
        HAL_REG_WRITE_FIELD((i3c_Obj[instance].regBase + addrEntry),
                I3C_DEV_ADDR_TABLE1_LOC1_DEV_DYNAMIC_ADDR_POS,
                I3C_DEV_ADDR_TABLE1_LOC1_DEV_DYNAMIC_ADDR_MASK,
                0U);

        /* set the bit in th dat free mask*/
        i3c_Obj[instance].datFreeMask |= (1U << idx);

        /* remove from the attached dev list*/
        i3c_Obj[instance].attachedDevAddrList[idx] = 0U;

        /* reduce the number of attached devices*/
        i3c_Obj[instance].numAttachedDev--;
    }
    else
    {
        ret = I3C_INVALID;
    }

    return ret;
}

/* end of File*/
