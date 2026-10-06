/*
    File: SMMU Configuration

    Build-time configuration for the SMMU: 11 definitions.
*/

/*
    Constants: SMMU constants

    ENABLE_SMMU_DMA0  - Value: 1.
    ENABLE_SMMU_DMA1  - Value: 1.
    ENABLE_SMMU_SDM   - Value: 1.
    ENABLE_SMMU_USB2  - Value: 0.
    ENABLE_SMMU_USB3  - Value: 0.
    ENABLE_SMMU_SDMMC - Value: 0.
    ENABLE_SMMU_NAND  - Value: 0.
    ENABLE_SMMU_ETR   - Value: 0.
    ENABLE_SMMU_TSN0  - Value: 0.
    ENABLE_SMMU_TSN1  - Value: 0.
    ENABLE_SMMU_TSN2  - Value: 0.
*/

#ifndef __SOCFPGA_SMMU_CONFIG__
#define __SOCFPGA_SMMU_CONFIG__

#define ENABLE_SMMU_DMA0    1
#define ENABLE_SMMU_DMA1    1
#define ENABLE_SMMU_SDM     1
#define ENABLE_SMMU_USB2    0
#define ENABLE_SMMU_USB3    0
#define ENABLE_SMMU_SDMMC   0
#define ENABLE_SMMU_NAND    0
#define ENABLE_SMMU_ETR     0
#define ENABLE_SMMU_TSN0    0
#define ENABLE_SMMU_TSN1    0
#define ENABLE_SMMU_TSN2    0

#endif
