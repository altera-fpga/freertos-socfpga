/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Driver implementation for GIC
 */

#include <string.h>
#include "osal_log.h"
#include "osal.h"
#include "socfpga_gic_registers.h"

#include "socfpga_GIC.h"

#define interruptDCTRL_ENG0      (1U << 0U)
#define interruptDCTRL_ENG1NS    (1U << 1U)
#define interruptDCTRL_ENG1S     (1U << 2U)
#define interruptDCTRL_ARE_S     (1U << 4U)
#define interruptDCTRL_ARE_NS    (1U << 5U)
#define interruptDCTRL_DS        (1U << 6U)
#define interruptDCTRL_E1NWF     (1U << 7U)

#define interruptMAKE_PRIORITY(x)    (((uint32_t) (x) << \
            portPRIORITY_SHIFT) & 0xFFU)

static struct GICv3_dist_if *xGicDist;
static struct GICv3_rdist_if *xGicRdist;

static uint32_t uGicMaxRd = 0U;

/*
 * @func: gic_enable_gic
 * @brief : Function to enable the GIC and configure the base address of GIC controller
 */
int32_t gic_enable_gic( void )
{

    uint32_t uIndex = 0U;

    xGicDist = (struct GICv3_dist_if*) ((void*) registerSOCFPGA_DIST_BASE_ADDR);
    xGicRdist = (struct GICv3_rdist_if*) ((void*) registerSOCFPGA_RD_BASE_ADDR);

    if (xGicDist == NULL)
    {
        return interruptRETURN_ERROR;
    }

    while ((xGicRdist[ uIndex ].lpis.GICR_TYPER[ 0 ] & (1U << 4U)) == 0U)   // Keep incrementing until GICR_TYPER.Last reports no more RDs in block
    {
        uIndex++;
    }

    uGicMaxRd = uIndex;

    // First set the ARE bits
    xGicDist->GICD_CTLR = interruptDCTRL_ARE_S | interruptDCTRL_ARE_NS |
            interruptDCTRL_DS;

    // The split here is because the register layout is different once ARE==1

    // Now set the rest of the options
    xGicDist->GICD_CTLR = interruptDCTRL_ENG0 | interruptDCTRL_ENG1NS |
            interruptDCTRL_ENG1S | interruptDCTRL_ARE_S |
            interruptDCTRL_ARE_NS | interruptDCTRL_DS;

    return interruptRETURN_SUCCESS;
}

/*
 * @func :
 * @brief :
 */
int32_t gic_get_redist_id( uint32_t uAffinity )
{
    int32_t lIndex = 0;

    if (xGicRdist == NULL)
    {
        return interruptRETURN_ERROR;
    }

    do
    {
        if (xGicRdist[ lIndex ].lpis.GICR_TYPER[ 1 ] == uAffinity)
        {
            return lIndex;
        }
        lIndex++;
    } while ((uint32_t) lIndex <= uGicMaxRd);

    return interruptRETURN_SUCCESS; // return -1 to signal not RD found
}

// ------------------------------------------------------------

/*
 * @func :
 * @brief :
 */
int32_t gic_wakeup_redist( uint32_t uRd )
{
    uint32_t uTmp;

    if (xGicRdist == NULL)
    {
        return interruptRETURN_ERROR;
    }
    // Tell the Redistributor to wake-up by clearing ProcessorSleep bit
    uTmp = xGicRdist[ uRd ].lpis.GICR_WAKER;
    uTmp = uTmp & ~0x2U;
    xGicRdist[ uRd ].lpis.GICR_WAKER = uTmp;

    // Poll ChildrenAsleep bit until Redistributor wakes
    do
    {
        uTmp = xGicRdist[ uRd ].lpis.GICR_WAKER;
    } while ((uTmp & 0x4U) != 0U);

    return interruptRETURN_SUCCESS;
}

// Returns 1 if the specified INTuID is within the implemented Extended SPI range

/*
 * @func :
 * @brief :
 */
static uint32_t gic_is_valid_ext_spi( uint32_t uID )
{
    uint32_t uMax_spi;

    // Check Ext SPI implemented
    if (((xGicDist->GICD_TYPER >> 8U) & 0x1U) == 0U)
    {
#ifdef DEBUG
        // put debug print here
#endif
        return 0U;  // GICD_TYPER.ESPI==0: Extended SPI range not present
    }
    else
    {
        uMax_spi = ((xGicDist->GICD_TYPER >> 27U) & 0x1FU); // Get field which reports the number ESPIs in blocks of 32, minus 1
        uMax_spi = (uMax_spi + 1U) * 32U;          // Convert into number of ESPIs
        uMax_spi = uMax_spi + 4096U;                      // Range starts at 4096

        if (!(uID < uMax_spi))
        {
#ifdef DEBUG
            // put debug print here
#endif
            return 0U;
        }
    }

    return 1U;
}

// ------------------------------------------------------------

/*
 * @func :
 * @brief :
 */
#define gicPRIORITY_MASK(offset)                 (uint32_t)(0xFFU << (offset))
#define gicPRIORITY_SHIFT_OFFSET(val, offset)    (uint32_t)((val) << (offset))
#define gicCLEAR_PRIORITY(dest, \
            offset)                              ((dest) &= \
    (~gicPRIORITY_MASK(offset)))
#define gicSET_PRIORITY(dest, val, offset)       gicCLEAR_PRIORITY((dest), \
        (offset)); (dest) |= gicPRIORITY_SHIFT_OFFSET((val), (offset))

int32_t gic_set_int_priority( uint32_t uID, uint32_t uRd, uint8_t priority )
{
    //Check if the priority is within the bounds
    if ((priority & 0xF0U) == 0xF0U)
    {
        return interruptRETURN_INVALID_PRIORITY;
    }

    if ((xGicRdist == NULL) || (xGicDist == NULL))
    {
        return interruptRETURN_ERROR;
    }

    if (uID < 31U)
    {
        // Check uRd in range
        if (uRd > uGicMaxRd)
        {
            return interruptRETURN_INVALID_RDIST;
        }
        // SGI or PPI
        xGicRdist[ uRd ].sgis.GICR_IPRIORITYR[ uID ] =
                (uint8_t) interruptMAKE_PRIORITY(
                priority);
    }
    else if (uID < 1020U)
    {
        // SPI
        /* similar checks are to avoid numeric overflow violation in static analysis */
        if (uID == 31U)
        {
            return interruptRETURN_INVALID_SPI;
        }
        uID -= 32U; // Adjust ID for Distributor registers

        uint32_t Index = uID / 4U;
        uint32_t Offset = (uID & 0x03U) * 8U;
        uint32_t ullPriorityTemp = xGicDist->GICD_IPRIORITYR[ Index ];
        gicSET_PRIORITY(ullPriorityTemp, interruptMAKE_PRIORITY(priority),
                Offset);
        xGicDist->GICD_IPRIORITYR[ Index ] = ullPriorityTemp;
    }
    else if ((uID > 1055U) && (uID < 1120U))
    {
        return interruptRETURN_INVALID_ID;
    }
    else if ((uID > 4095U) && (uID < 5120U))
    {
        return interruptRETURN_INVALID_ID;
    }
    else
    {
        return interruptRETURN_INVALID_ID;
    }

    return interruptRETURN_SUCCESS;
}

// ------------------------------------------------------------

int32_t gic_set_int_group( uint32_t uID, uint32_t uRd, uint32_t uSecurity )
{
    uint32_t uBank, uGroup, uMod, uRet = 0U;

#ifdef DEBUG
    //put debug print here
#endif

    if ((xGicRdist == NULL) || (xGicDist == NULL))
    {
        return interruptRETURN_ERROR;
    }

    if (uID < 31U)
    {
        // Check uRd in range
        if (uRd > uGicMaxRd)
        {
            return interruptRETURN_INVALID_RDIST;
        }
        // SGI or PPI
        uID = 1U << uID;      // Move a '1' into the correct bit position

        // Read current values
        uGroup = xGicRdist[ uRd ].sgis.GICR_IGROUPR0;
        uMod = xGicRdist[ uRd ].sgis.GICR_IGRPMODR0;

        // Update required bits
        switch (uSecurity)
        {
        case GICV3_GROUP0:
            uGroup = (uGroup & ~uID);
            uMod = (uMod & ~uID);
            break;

        case GICV3_GROUP1_SECURE:
            uGroup = (uGroup & ~uID);
            uMod = (uMod | uID);
            break;

        case GICV3_GROUP1_NON_SECURE:
            uGroup = (uGroup | uID);
            uMod = (uMod & ~uID);
            break;

        default:
            uRet = 1U;
            break;
        }
        if (uRet == 1U)
        {
            return interruptRETURN_INVALID_GROUP;
        }

        // Write modified version back
        xGicRdist[ uRd ].sgis.GICR_IGROUPR0 = uGroup;
        xGicRdist[ uRd ].sgis.GICR_IGRPMODR0 = uMod;

    }
    else if (uID < 1020U)
    {
        if (uID == 31U)
        {
            return interruptRETURN_INVALID_SPI;
        }
        uID -= 32U; // Adjust ID for Distributor registers
        // SPI
        uBank = uID / 32U; // There are 32 uIDs per register, need to work out which register to access
        uID = uID & 0x1fU;    // ... and which bit within the register

        uID = 1U << uID;      // Move a '1' into the correct bit position

        uGroup = xGicDist->GICD_IGROUPR[ uBank ];
        uMod = xGicDist->GICD_IGRPMODR[ uBank ];

        switch (uSecurity)
        {
        case GICV3_GROUP0:
            uGroup = (uGroup & ~uID);
            uMod = (uMod & ~uID);
            break;

        case GICV3_GROUP1_SECURE:
            uGroup = (uGroup & ~uID);
            uMod = (uMod | uID);
            break;

        case GICV3_GROUP1_NON_SECURE:
            uGroup = (uGroup | uID);
            uMod = (uMod & ~uID);
            break;

        default:
            uRet = 2U;
            break;
        }
        if (uRet == 2U)
        {
            return interruptRETURN_INVALID_GROUP;
        }

        xGicDist->GICD_IGROUPR[ uBank ] = uGroup;
        xGicDist->GICD_IGRPMODR[ uBank ] = uMod;
    }
    else
    {
        // Unknown or unsupported uID.
        return interruptRETURN_INVALID_ID;
    }

    return interruptRETURN_SUCCESS;
}
// ------------------------------------------------------------

#define GICV3_ROUTE_AFF3_SHIFT    (8)

// Sets the target CPUs of the specified uID
// For 'target' use one of the above defines
// uID       = INTuID of interrupt (uID must be less than 1020)
// mode     = Routing mode
// aff<n>   = Affinity co-ordinate of target
int32_t gic_set_int_route( uint32_t uID, uint32_t mode, uint32_t affinity )
{
    uint64_t ulTmp;

#ifdef DEBUG
    //put debug print here
#endif

    if (xGicDist == NULL)
    {
        return interruptRETURN_ERROR;
    }
    // Check for SPI ranges
    if (!((uID > 31U) && (uID < 1020U)))
    {
        // Not a GICv3.0 SPI

        if (!((uID > 4095U) && (uID < 5120U)))
        {
            // Not a GICv3.1 SPI either
            return interruptRETURN_INVALID_SPI;
        }

        // Check Ext SPI implemented
        if (gic_is_valid_ext_spi(uID) == 0U)
        {
            return interruptRETURN_INVALID_SPI;
        }
    }

    uID -= 32U; // Adjust ID for Distributor registers
    // Combine routing in
    ulTmp = (uint64_t) (affinity & 0x00FFFFFFU) |
            (((uint64_t) affinity & 0xFF000000U) <<
            GICV3_ROUTE_AFF3_SHIFT) | (uint64_t) mode;

    if ((uID > 31U) && (uID < 1020U))
    {
        xGicDist->GICD_IROUTER[ uID ] = ulTmp;
    }

    return interruptRETURN_SUCCESS;
}

// ------------------------------------------------------------
// Interrupt configuration
// ------------------------------------------------------------

int32_t gic_enable_int( uint32_t uID, uint32_t uRd )
{
    uint32_t uBank;

    if ((xGicRdist == NULL) || (xGicDist == NULL))
    {
        return interruptRETURN_ERROR;
    }

    if (uID < 31U)
    {
        // Check uRd in range
        if (uRd > uGicMaxRd)
        {
            return interruptRETURN_INVALID_RDIST;
        }

        xGicRdist[ uRd ].sgis.GICR_ISENABLER0 = (1U << uID);
    }
    else if (uID < 1020U)
    {
        if (uID == 31U)
        {
            return interruptRETURN_INVALID_SPI;
        }
        uID -= 32U; // Adjust ID for Distributor registers
        // SPI
        uBank = uID / 32U; // There are 32 uIDs per register, need to work out which register to access
        uID = uID & 0x1fU;    // ... and which bit within the register

        uID = 1U << uID;      // Move a '1' into the correct bit position

        xGicDist->GICD_ISENABLER[ uBank ] = uID;
    }
    else
    {
#ifdef DEBUG
        ERROR("enableInt:: ERROR - Invalid or unsupported interrupt.");
#endif
        return interruptRETURN_ERROR;
    }

    return interruptRETURN_SUCCESS;
}

// ------------------------------------------------------------

int32_t gic_disable_int( uint32_t uID, uint32_t uRd )
{
    uint32_t uBank;

    if ((xGicRdist == NULL) || (xGicDist == NULL))
    {
        return interruptRETURN_ERROR;
    }

    if (uID < 31U)
    {
        // Check uRd in range
        if (uRd > uGicMaxRd)
        {
            return 1;
        }
        // SGI or PPI
        uID = uID & 0x1fU;    // ... and which bit within the register
        uID = 1U << uID;      // Move a '1' into the correct bit position

        xGicRdist[ uRd ].sgis.GICR_ICENABLER0 = uID;
    }
    else if (uID < 1020U)
    {
        if (uID == 31U)
        {
            return interruptRETURN_INVALID_SPI;
        }
        uID -= 32U; // Adjust ID for Distributor registers
        // SPI
        uBank = uID / 32U; // There are 32 uIDs per register, need to work out which register to access
        uID = uID & 0x1fU;    // ... and which bit within the register

        uID = 1U << uID;      // Move a '1' into the correct bit position

        xGicDist->GICD_ICENABLER[ uBank ] = uID;
    }
    else
    {
#ifdef DEBUG
        //put debug print here
#endif
        return interruptRETURN_ERROR;
    }

    return interruptRETURN_SUCCESS;
}

int32_t gic_set_int_type( uint32_t uID, uint32_t uRd, uint32_t type )
{
    uint32_t uBank, uTmp, conf;

    if ((xGicDist == NULL) || (xGicRdist == NULL))
    {
        return interruptRETURN_ERROR;
    }

#ifdef DEBUG
    //put debug print here
#endif

    if (uID < 31U)
    {
        if (uID < 16U)
        {
            return 1;
        }
        else
        {
            xGicRdist[ uRd ].sgis.GICR_ICFGR[ 1 ] = (type & 0x3U)
                    << ((uID - 16U) << 1U);
        }
    }
    else if (uID < 1020U)
    {
        if (uID == 31U)
        {
            return interruptRETURN_INVALID_SPI;
        }
        uID -= 32U; // Adjust ID for Distributor registers
        // SPI
        type = type & 0x3U;            // Mask out unused bits

        uBank = uID / 16U; // There are 16 uIDs per register, need to work out which register to access
        uID = uID & 0xFU;              // ... and which field within the register
        uID = uID * 2U; // Convert from which field to a bit offset (2-bits per field)

        conf = type << uID; // Move configuration value into correct bit position

        uTmp = xGicDist->GICD_ICFGR[ uBank ];     // Read current value
        uTmp = uTmp & ~(0x3U << uID);   // Clear the bits for the specified field
        uTmp = uTmp | conf;                     // OR in new configuration
        xGicDist->GICD_ICFGR[ uBank ] = uTmp;     // Write updated value back
    }
    else
    {
        return interruptRETURN_ERROR;
    }

    return interruptRETURN_SUCCESS;
}

int32_t gic_clear_int_pending( uint32_t uID, uint32_t uRd )
{
    uint32_t uBank;

    if ((xGicDist == NULL) || (xGicRdist == NULL))
    {
        return interruptRETURN_ERROR;
    }

    if (uID < 31U)
    {
        // Check uRd in range
        if (uRd > uGicMaxRd)
        {
            return 1;
        }

        xGicRdist[ uRd ].sgis.GICR_ICPENDR0 |= (1U << uID);
    }
    else if (uID < 1020U)
    {
        if (uID == 31U)
        {
            return interruptRETURN_INVALID_SPI;
        }
        uID -= 32U; // Adjust ID for Distributor registers
        // SPI
        uBank = uID / 32U; // There are 32 uIDs per register, need to work out which register to access
        uID = uID & 0x1fU;    // ... and which bit within the register

        uID = 1U << uID;      // Move a '1' into the correct bit position

        xGicDist->GICD_ICPENDR[ uBank ] = uID;
    }
    else
    {
        return interruptRETURN_ERROR;
    }

    return interruptRETURN_SUCCESS;
}

// ----------------------------------------------------------
// Interrupt state
// ------------------------------------------------------------

int32_t gic_set_int_pending( uint32_t uID, uint32_t uRd )
{
    uint32_t uBank;

    //Adjust for SPI
    uID -= 32U;

    if ((xGicDist == NULL) || (xGicRdist == NULL))
    {
        return interruptRETURN_ERROR;
    }

    if (uID < 31U)
    {
        // Check uRd in range
        if (uRd > uGicMaxRd)
        {
            return 1;
        }

        uID = uID & 0x1fU;    // Find which bit within the register
        uID = 1U << uID;      // Move a '1' into the correct bit position

        xGicRdist[ uRd ].sgis.GICR_ISPENDR0 |= uID;

    }
    else if (uID < 1020U)
    {
        if (uID == 31U)
        {
            return interruptRETURN_INVALID_SPI;
        }
        uID -= 32U; // Adjust ID for Distributor registers
        // SPI
        uBank = uID / 32U; // There are 32 uIDs per register, need to work out which register to access
        uID = uID & 0x1fU;    // ... and which bit within the register

        uID = 1U << uID;      // Move a '1' into the correct bit position

        xGicDist->GICD_ISPENDR[ uBank ] |= uID;
    }
    else
    {
        return interruptRETURN_ERROR;
    }

    return interruptRETURN_SUCCESS;
}

void gic_enable_interrupts( void )
{
    __asm__ volatile ("msr DAIFClr, #0xf" ::: "memory");
}
