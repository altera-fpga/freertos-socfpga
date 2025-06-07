#ifndef _TIMER_TEST_H_
#define _TIMER_TEST_H_

#include "socfpga_dma.h"

/*Api*/
#define iot_dma_open                              dma_open
#define iot_dma_config 							  dma_config
#define iot_dma_setup                             dma_setup_transfer
#define iot_dma_start                             dma_start_tranfer
#define iot_dma_stop                              dma_stop_transfer
#define iot_dma_suspend                           dma_suspend_transfer
#define iot_dma_resume                            dma_resume_transfer
#define iot_dma_close                             dma_close

#define IotDmaTestChannel                          DMA_CHANNEL_1
#define IotDmaTestChannel2                         DMA_CHANNEL_2
#define IotDmaTestChannel3                         DMA_CHANNEL_3
#define IotDmaTestChannel4                         DMA_CHANNEL_4
#define IotDmaTestInstance                         DMA_INSTANCE_0
#define IotDmaTestInstance2                        DMA_INSTANCE_1
#define IotDmaTestLength                          4096
#define IotDmaTestLength10M                       (1024 * 1024 * 10)

/*Structure and enums*/
#define IotDmaConfig                              DmaConfig
#define IotDmaBlockTransferCfg                    DmaBlkTrnsfrCfg
#define IotDmaInstance_t                          DmaInstance_t
#define IotDmaChannel_t                           DmaChannels_t
#define IotDmaHandle_t                            xDmaHandle_t 
#define IotDmaCallback_t                          DmaCallback_t

#endif
