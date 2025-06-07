#ifndef __SOCFPGA_SMMU__
#define __SOCFPGA_SMMU__

#define SMMU_MAX_STREAM_ID      0xA

#define SMMU_STREAM_ID_TSN0     0x1
#define SMMU_STREAM_ID_TSN1     0x2
#define SMMU_STREAM_ID_TSN2     0x3
#define SMMU_STREAM_ID_NAND     0x4
#define SMMU_STREAM_ID_SDMMC    0x5
#define SMMU_STREAM_ID_USB0     0x6
#define SMMU_STREAM_ID_USB1     0x7
#define SMMU_STREAM_ID_DMA0     0x8
#define SMMU_STREAM_ID_DMA1     0x9
#define SMMU_STREAM_ID_SDM      0xA



typedef struct __attribute__((aligned(64), packed))
{
    uint32_t T0SZ : 6;
    uint32_t TG0 : 2;
    uint32_t IR0 : 2;
    uint32_t OR0 : 2;
    uint32_t SH0 : 2;
    uint32_t EPD0 : 1;
    uint32_t ENDI : 1;
    uint32_t T1SZ : 6;
    uint32_t TG1 : 2;
    uint32_t IR1 : 2;
    uint32_t OR1 : 2;
    uint32_t SH1 : 2;
    uint32_t EPD1 : 1;
    uint32_t V : 1;
    uint32_t IPS : 3;
    uint32_t AFFD : 1;
    uint32_t WXN : 1;
    uint32_t UWXN : 1;
    uint32_t TBI0 : 1;
    uint32_t TBI1 : 1;
    uint32_t PAN : 1;
    uint32_t AA64 : 1;
    uint32_t HD : 1;
    uint32_t HA : 1;
    uint32_t S : 1;
    uint32_t R : 1;
    uint32_t A : 1;
    uint32_t ASET : 1;
    uint32_t ASID : 16;
    uint32_t NSCFG0 : 1;
    uint32_t DisCH0 : 1;
    uint32_t E0PD0 : 1;
    uint32_t HAFT : 1;
    uint64_t TTB0 : 52;
    uint32_t RES0_0 : 2;
    uint32_t PnCH : 1;
    uint32_t EPAN : 1;
    uint32_t HWU059 : 1;
    uint32_t HWU060 : 1;
    uint32_t SKL0 : 2;
    uint32_t NSCFG1 : 1;
    uint32_t DisCH1 : 1;
    uint32_t E0PD1 : 1;
    uint32_t AIE : 1;
    uint64_t TTB1 : 52;
    uint32_t RES0_1 : 2;
    uint32_t DS : 1;
    uint32_t PIE : 1;
    uint32_t HWU159 : 1;
    uint32_t HWU160 : 1;
    uint32_t SKL1 : 2;
    uint32_t MAIR0 : 32;
    uint32_t MAIR1 : 32;
    uint32_t AMAIR0 : 32;
    uint32_t AMAIR1 : 32;
    uint32_t IMPL_DEFINED : 32;
    uint32_t PARTID : 16;
    uint32_t PMG : 8;
    uint32_t RES0_2 : 8;
    uint64_t PI1 : 64;
    uint64_t PI2 : 64;
} context_desc_t;

typedef struct __attribute__((aligned(64), packed))
{
    uint64_t V : 1;
    uint64_t Config : 3;
    uint64_t S1Fmt : 2;
    uint64_t S1ContextPtr : 50;
    uint64_t RES0   : 3;
    uint64_t S1CDMax    : 5;
    uint64_t S1DSS  : 2;
    uint64_t S1CIR : 2;
    uint64_t S1COR : 2;
    uint64_t S1CSH : 2;
    uint64_t S2HWU59 : 1;
    uint64_t S2HWU60 : 1;
    uint64_t S2HWU61 : 1;
    uint64_t S2HWU62 : 1;
    uint64_t DRE : 1;
    uint64_t CONT : 4;
    uint64_t DCP : 1;
    uint64_t PPAR : 1;
    uint64_t MEV : 1;
    uint64_t SW_RESERVED : 4;
    uint64_t S1PIE : 1;
    uint64_t S2FWB : 1;
    uint64_t S1MPAM : 1;
    uint64_t S1STALLD : 1;
    uint64_t EATS : 2;
    uint64_t STRW : 2;
    uint64_t MemAttr : 4;
    uint64_t MTCFG : 1;
    uint64_t ALLOCCFG : 4;
    uint64_t RES0_1 : 3;
    uint64_t SHCFG : 2;
    uint64_t NSCFG : 2;
    uint64_t PRIVCFG : 2;
    uint64_t INSTCFG : 2;
    uint64_t IMP_DEFINED_0 : 12;
    uint64_t S2VMID : 16;
    uint64_t IMP_DEFINED_1 : 16;
    uint32_t S2_CONFIGS_0;
    uint64_t S2_CONFIGS_1;
    uint64_t S2_CONFIGS_2;
    uint64_t S2_CONFIGS_3;
    uint64_t S2_CONFIGS_4;
    uint64_t S2_CONFIGS_5;
} stream_table_t;

typedef struct __attribute__((aligned(8), packed))
{
    uint32_t W0;
    uint32_t W1;
    uint32_t W2;
    uint32_t W3;
} command_desc_t;

typedef struct
{
    stream_table_t stream_table[ SMMU_MAX_STREAM_ID +
            1 ] __attribute__((aligned(64)));
    context_desc_t context_desc_1tb;
    context_desc_t context_desc_512mb;
    command_desc_t command_desc[5*sizeof(command_desc_t)] __attribute__((aligned(1024*1024)));;
    uint64_t *event_queue;

    uint64_t lvl0_page_table_512mb[256] __attribute__((aligned(4096)));
    uint64_t lvl1_page_table_512mb[512][512] __attribute__((aligned(4096)));

    uint64_t lvl0_page_table_1tb[2] __attribute__((aligned(4096)));
    uint64_t lvl1_page_table_1tb[512] __attribute__((aligned(4096)));
    uint64_t lvl2_page_table_1tb[512] __attribute__((aligned(4096)));
}smmu_descriptors_t;

int32_t smmu_enable();

#endif
