/*
 * FreeRTOS Common IO V0.1.3
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */
/*******************************************************************************
 * @file dma_test.c
 * @brief Functional Unit Test - DMA
 *******************************************************************************
 */
/* Test includes */
#include <stdint.h>
#include <errno.h>
#include "dma_test.h"
#include "socfpga_dma.h"
#include "socfpga_cache.h"
#include "unity.h"
#include "unity_fixture.h"
#include "osal.h"
#include "string.h"

/* Driver includes */
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "string.h"

#define DMA_TEST_PATTERN_64BIT 0xDEADBEEF
#define MAX_CHANNEL_NUM   (4U)
/*-----------------------------------------------------------*/
/** Static globals */
/*-----------------------------------------------------------*/
static uint8_t ucPatternData[ 4 ] =
{ 0xDE, 0xAD, 0xBE, 0xEF };
static osal_semaphore_t xDmaTransferSemaphore = NULL;
static osal_semaphore_t xtestIotDmaSemaphore = NULL;
static osal_semaphore_t xtestIotDmaSemThread2 = NULL;
static osal_semaphore_t xtestIotDmaSemThread3 = NULL;
static osal_semaphore_t xtestIotDmaSemThread4 = NULL;
static osal_semaphore_t xtestIotDmaSemThreadTest = NULL;
static osal_semaphore_t xDmaReaderDoneSemaphore = NULL;
static IotDmaHandle_t pxGlobalDmaHandle = NULL;
static void *pvDestinationAddress = NULL;
static IotDmaHandle_t xDmaHandle = NULL;

/*-----------------------------------------------------------*/
typedef struct
{
    uint64_t src;
    uint64_t dest;
    uint16_t len;
} DMAParams_t;

DMAParams_t table[ 4 ];
IotDmaHandle_t xMCDmaHandle[4];
uint32_t threadCntFinish = 0;
/*-----------------------------------------------------------*/

static void dma_ch_thread_1(void *pvParameters);
static void dma_ch_thread_2(void *pvParameters);
static void dma_ch_thread_3(void *pvParameters);
static void dma_ch_thread_4(void *pvParameters);

static void prvDmarCallback( void *pvUserContext )
{
    (void) pvUserContext;
    osal_semaphore_post(xtestIotDmaSemaphore);
}

static void prvDmaMultithreadCallback2( void *pvUserContext )
{
    (void)pvUserContext;
    osal_semaphore_post(xtestIotDmaSemThread2);
}
static void prvDmaMultithreadCallback3( void *pvUserContext )
{
    (void)pvUserContext;
    osal_semaphore_post(xtestIotDmaSemThread3);
}
static void prvDmaMultithreadCallback4( void *pvUserContext )
{
    (void)pvUserContext;
    osal_semaphore_post(xtestIotDmaSemThread4);
}

static void *pvMakeSourceBuffer(uint32_t ulLength)
{
    uint8_t *pucBuffer = (uint8_t*) malloc(ulLength);

    if ( pucBuffer )
    {
        for ( uint32_t idx = 0; idx < ulLength; idx++ )
        {
            pucBuffer[ idx ] = ucPatternData[ idx % 4 ];
        }
        cache_force_write_back(pucBuffer, ulLength);
    }
    return (void*) pucBuffer;
}
static void* pvMakeDestinationBuffer( uint32_t ulLength )
{
    void *pvBuffer = calloc(ulLength, 1);
    if ( pvBuffer )
    {
        pvDestinationAddress = pvBuffer;
        cache_force_write_back(pvBuffer, ulLength);
    }
    return pvBuffer;
}

static struct DmaBlkTrnsfrCfg get_blk_transfer_cfg(uint32_t transfer_length)
{
    struct DmaBlkTrnsfrCfg xDmaBlkTrnsfr;
    uint8_t *pucSrc;
    uint8_t *pucDst;

    pucDst = (uint8_t*) pvMakeDestinationBuffer(transfer_length);
    pucSrc = (uint8_t*) pvMakeSourceBuffer(transfer_length);

    xDmaBlkTrnsfr.ulSrcAddr = (uint64_t) (pucSrc);
    xDmaBlkTrnsfr.ulDestAddr = (uint64_t) (pucDst);
    xDmaBlkTrnsfr.ulBlockSize = transfer_length;
    xDmaBlkTrnsfr.xNextTrnsfrCfg = NULL;

    return xDmaBlkTrnsfr;
}

static void tableInit(uint64_t *src, uint64_t *dest, int len, uint16_t startNum)
{
    uint16_t i;
    uint8_t *srcBuf = (uint8_t*)malloc(len);
    uint8_t *destBuf = (uint8_t*)malloc(len);

    for ( i = 0 ; i < len; i++)
    {
        *(srcBuf + i) = startNum++;
        if(startNum == 255)
        {
            startNum = 0;
        }
    }
    memset(destBuf,0,len);
    cache_force_write_back(srcBuf, len);
    cache_force_write_back(destBuf, len);
    *src = (uint64_t) &srcBuf[0];
    *dest = (uint64_t) &destBuf[0];
}

static int32_t do_dma_transfer(uint8_t *src, uint8_t *dest, uint32_t ulTransferLength)
{
    /*TODO: Write code to initiate a transfer*/

    struct IotDmaConfig xDmaConfig;
    struct IotDmaBlockTransferCfg xDmaBlkTrnsfr;

    xDmaConfig.uDmaChannelDirection = eDmaMemToMemDmac;
    xDmaConfig.uDmaChannelPriority = 0;
    xDmaConfig.ulDmaLinkedListCount = 1;
    xDmaConfig.pxDmaCallback = (IotDmaCallback_t) prvDmarCallback;

    xDmaBlkTrnsfr.ulSrcAddr = (uint64_t) (src);
    xDmaBlkTrnsfr.ulDestAddr = (uint64_t) (dest);
    xDmaBlkTrnsfr.ulBlockSize = ulTransferLength; //Max size 4 * 2^21
    xDmaBlkTrnsfr.xNextTrnsfrCfg = NULL;

    if ( pxGlobalDmaHandle == NULL )
    {
        return -EINVAL;
    }

    iot_dma_config(pxGlobalDmaHandle, &xDmaConfig);
    iot_dma_setup(pxGlobalDmaHandle, &xDmaBlkTrnsfr);
    iot_dma_start(pxGlobalDmaHandle);

    /*****************************************/

	xSemaphoreTake(xtestIotDmaSemaphore, portMAX_DELAY);

	/* Omit invalidate cache function for OCRAM memory */
	if( xDmaBlkTrnsfr.ulDestAddr > 0x80000 )
	{
    	cache_force_invalidate(dest, ulTransferLength);
	}
    for ( uint32_t idx = 0; idx < ulTransferLength; idx++ )
    {
        if ( dest[ idx ] != ucPatternData[ idx % 4 ] )
        {
            return -EFAULT;
        }
    }

    return 0;
}

static int32_t ulDmaDoTransfer( uint32_t ulTransferLength )
{
    uint8_t *pucSrc;
    uint8_t *pucDst;
    pucSrc = (uint8_t*) pvMakeSourceBuffer(ulTransferLength);
    pucDst = (uint8_t*) pvMakeDestinationBuffer(ulTransferLength);
    /*TODO: Write code to initiate a transfer*/
    struct IotDmaConfig xDmaConfig;
    struct IotDmaBlockTransferCfg xDmaBlkTrnsfr;
    xDmaConfig.uDmaChannelDirection = eDmaMemToMemDmac;
    xDmaConfig.uDmaChannelPriority = 0;
    xDmaConfig.ulDmaLinkedListCount = 1;
    xDmaConfig.pxDmaCallback = (IotDmaCallback_t) prvDmarCallback;
    xDmaBlkTrnsfr.ulSrcAddr = (uint64_t) (pucSrc);
    xDmaBlkTrnsfr.ulDestAddr = (uint64_t) (pucDst);
    xDmaBlkTrnsfr.ulBlockSize = ulTransferLength; //Max size 4 * 2^21
    xDmaBlkTrnsfr.xNextTrnsfrCfg = NULL;

    if ( pxGlobalDmaHandle == NULL )
    {
        return -EINVAL;
    }

    iot_dma_config(pxGlobalDmaHandle, &xDmaConfig);
    iot_dma_setup(pxGlobalDmaHandle, &xDmaBlkTrnsfr);
    iot_dma_start(pxGlobalDmaHandle);

    /*****************************************/
    osal_semaphore_wait(xtestIotDmaSemaphore, portMAX_DELAY);

    cache_force_invalidate(pucDst, ulTransferLength);
    for ( uint32_t idx = 0; idx < ulTransferLength; idx++ )
    {
        if ( pucDst[ idx ] != ucPatternData[ idx % 4 ] )
        {
            free(pucDst);
            free(pucSrc);
            return -EFAULT;
        }
    }
    free(pucDst);
    free(pucSrc);
    return 0;
}

static int32_t ulDmaTestTransferStatus( uint32_t ulTransferLength )
{
    uint8_t *pucDst = (uint8_t*) pvDestinationAddress;

    cache_force_invalidate(pucDst, ulTransferLength);
    for ( uint32_t idx = 0; idx < ulTransferLength; idx++ )
    {
        if ( pucDst[ idx ] != ucPatternData[ idx % 4 ] )
        {
            return -EFAULT;
        }
    }
    return 0;
}

static int32_t ulDmaDoTransferAsync( uint32_t ulTransferLength )
{
    uint8_t *pucSrc;
    uint8_t *pucDst;
    pucSrc = (uint8_t*) pvMakeSourceBuffer(ulTransferLength);
    pucDst = (uint8_t*) pvMakeDestinationBuffer(ulTransferLength);
    /*TODO: Write code to initiate a transfer*/
    struct IotDmaConfig xDmaConfig;
    struct IotDmaBlockTransferCfg xDmaBlkTrnsfr;
    xDmaConfig.uDmaChannelDirection = eDmaMemToMemDmac;
    xDmaConfig.uDmaChannelPriority = 0;
    xDmaConfig.ulDmaLinkedListCount = 1;
    xDmaConfig.pxDmaCallback = (IotDmaCallback_t) prvDmarCallback;
    xDmaBlkTrnsfr.ulSrcAddr = (uint64_t) (pucSrc);
    xDmaBlkTrnsfr.ulDestAddr = (uint64_t) (pucDst);
    xDmaBlkTrnsfr.ulBlockSize = ulTransferLength; //Max size 4 * 2^21
    xDmaBlkTrnsfr.xNextTrnsfrCfg = NULL;
    if ( pxGlobalDmaHandle == NULL )
    {
        return -EINVAL;
    }
    iot_dma_config(pxGlobalDmaHandle, &xDmaConfig);
    iot_dma_setup(pxGlobalDmaHandle, &xDmaBlkTrnsfr);
    iot_dma_start(pxGlobalDmaHandle);
    return 0;
}

void dma_reader_thread( void *pvParameters )
{
    xDmaHandle = (IotDmaHandle_t) pvParameters;
    uint32_t ulRetVal;
    (void) xDmaHandle;
    /* Wait for the writer to signal that the transfer is done */
    osal_semaphore_wait(xDmaTransferSemaphore, portMAX_DELAY);
    /* Check transfer status */
    ulRetVal = ulDmaTestTransferStatus(IotDmaTestLength);
    TEST_ASSERT_EQUAL(0, ulRetVal);
    /* Notify the writer that verification is done */
    osal_semaphore_post(xDmaReaderDoneSemaphore);
    /* Delete the thread after completion */
    vTaskDelete(NULL);
}
/*-----------------------------------------------------------*/
/* Define Test Group. */
TEST_GROUP(TEST_IOT_DMA);
/*-----------------------------------------------------------*/
/**
 * @brief Setup function called before each test in this group is executed.
 */
TEST_SETUP( TEST_IOT_DMA)
{

    xtestIotDmaSemaphore = osal_semaphore_create(NULL);
    TEST_ASSERT_NOT_EQUAL(NULL, xtestIotDmaSemaphore);

    xtestIotDmaSemThread2 = osal_semaphore_create(NULL);
    TEST_ASSERT_NOT_EQUAL(NULL, xtestIotDmaSemThread2);

    xtestIotDmaSemThread3 = osal_semaphore_create(NULL);
    TEST_ASSERT_NOT_EQUAL(NULL, xtestIotDmaSemThread3);

    xtestIotDmaSemThread4 = osal_semaphore_create(NULL);
    TEST_ASSERT_NOT_EQUAL(NULL, xtestIotDmaSemThread4);

    xtestIotDmaSemThreadTest = osal_semaphore_create(NULL);
    TEST_ASSERT_NOT_EQUAL(NULL, xtestIotDmaSemThreadTest);

    xDmaReaderDoneSemaphore = osal_semaphore_create(NULL);
    TEST_ASSERT_NOT_EQUAL(NULL, xDmaReaderDoneSemaphore);

    xDmaTransferSemaphore = osal_semaphore_create(NULL);
    TEST_ASSERT_NOT_EQUAL(NULL, xDmaTransferSemaphore);

}

/*-----------------------------------------------------------*/
/**
 * @brief Tear down function called after each test in this group is executed.
 */
TEST_TEAR_DOWN( TEST_IOT_DMA)
{
}

/**
 * @brief Function to define which tests to execute as part of this group.
 */
TEST_GROUP_RUNNER( TEST_IOT_DMA )
{
    RUN_TEST_CASE(TEST_IOT_DMA, AFWP_IotDma_OpenCloseOpen);
    RUN_TEST_CASE(TEST_IOT_DMA, AFWP_IotDma_OpenOpen);
    RUN_TEST_CASE(TEST_IOT_DMA, AFWP_IotDma_Transfer);
    RUN_TEST_CASE(TEST_IOT_DMA, AFWP_IotDma_MultiTransfer);
    RUN_TEST_CASE(TEST_IOT_DMA, AFWP_IotDma_MultiChannelTransfer);
    RUN_TEST_CASE(TEST_IOT_DMA, AFWP_IotDma_MultiChannelOpenClose);
    RUN_TEST_CASE(TEST_IOT_DMA, AFWP_IotDma_OCRAM_Transfer);
    RUN_TEST_CASE(TEST_IOT_DMA, AFWP_IotDma_WriteCloseRead);
    RUN_TEST_CASE(TEST_IOT_DMA, AFWP_IotDma_AsyncMultiThreadTransfer);

    RUN_TEST_CASE(TEST_IOT_DMA, test_dma_open_valid);
    RUN_TEST_CASE(TEST_IOT_DMA, test_dma_open_invalid_instance);
    RUN_TEST_CASE(TEST_IOT_DMA, test_dma_open_invalid_channel);
    RUN_TEST_CASE(TEST_IOT_DMA, test_dma_open_same_instance_twice);
    RUN_TEST_CASE(TEST_IOT_DMA, test_dma_config_success);
    RUN_TEST_CASE(TEST_IOT_DMA, test_dma_config_null_handle);
    RUN_TEST_CASE(TEST_IOT_DMA, test_dma_config_busy);
    RUN_TEST_CASE(TEST_IOT_DMA, test_dma_setup_transfer_invalid_value);
    RUN_TEST_CASE(TEST_IOT_DMA, test_dma_setup_transfer_busy);
    RUN_TEST_CASE(TEST_IOT_DMA, test_dma_start_transfer_success);

    RUN_TEST_CASE(TEST_IOT_DMA, test_dma_stop_transfer_null_handle);
    RUN_TEST_CASE(TEST_IOT_DMA, test_dma_stop_transfer_no_transfer_in_progress);
    RUN_TEST_CASE(TEST_IOT_DMA, test_dma_suspend_transfer_null_handle);
    RUN_TEST_CASE(TEST_IOT_DMA, test_dma_suspend_transfer_no_transfer_in_progress);
    RUN_TEST_CASE(TEST_IOT_DMA, test_dma_suspend_transfer_failure);
    RUN_TEST_CASE(TEST_IOT_DMA, test_dma_resume_transfer_null_handle);
    RUN_TEST_CASE(TEST_IOT_DMA, test_dma_close_valid_handle);
    RUN_TEST_CASE(TEST_IOT_DMA, test_dma_close_null_handle);

}

/*-----------------------------------------------------------*/

/*Main function to invoke the tests*/
void dma_test()
{
    UNITY_BEGIN();

    TEST_SETUP(TEST_IOT_DMA);
    RUN_TEST_GROUP(TEST_IOT_DMA);

    UNITY_END();
}

/**
 * @brief Test Function to test iot_dma_open and iot_dma_close.
 *
 */
TEST( TEST_IOT_DMA, AFWP_IotDma_OpenCloseOpen )
{
    int32_t ulRetVal;
    xDmaHandle = iot_dma_open(IotDmaTestInstance, IotDmaTestChannel);
    TEST_ASSERT_NOT_EQUAL(NULL, xDmaHandle);
    ulRetVal = iot_dma_close(xDmaHandle);
    TEST_ASSERT_EQUAL(0, ulRetVal);
    xDmaHandle = iot_dma_open(IotDmaTestInstance, IotDmaTestChannel);
    TEST_ASSERT_NOT_EQUAL(NULL, xDmaHandle);
    ulRetVal = iot_dma_close(xDmaHandle);
    TEST_ASSERT_EQUAL(0, ulRetVal);
}
/**
 * @brief Test Function to test iot_dma_open.
 *
 */
TEST( TEST_IOT_DMA, AFWP_IotDma_OpenOpen)
{
    IotDmaHandle_t xDmaHandle2;
    int32_t ulRetVal;
    xDmaHandle = iot_dma_open(IotDmaTestInstance, IotDmaTestChannel);
    TEST_ASSERT_NOT_EQUAL(NULL, xDmaHandle);
    xDmaHandle2 = iot_dma_open(IotDmaTestInstance, IotDmaTestChannel);
    TEST_ASSERT_EQUAL(NULL, xDmaHandle2);
    ulRetVal = iot_dma_close(xDmaHandle);
    TEST_ASSERT_EQUAL(0, ulRetVal);
}


/**
 * @brief Test Function to test dma transfer using OCRAM memory
 *
 */
TEST( TEST_IOT_DMA, AFWP_IotDma_OCRAM_Transfer)
{
    int32_t ulRetVal;

    xDmaHandle = iot_dma_open(IotDmaTestInstance, IotDmaTestChannel);
    TEST_ASSERT_NOT_EQUAL(NULL, xDmaHandle);

    pxGlobalDmaHandle = xDmaHandle;

	/**********************************************************************/
	/*					 OCRAM to OCRAM transfer                          */
	/**********************************************************************/
	uint8_t *buffer1 = (uint8_t *)0x00000010;
	uint8_t *buffer2 = (uint8_t *)0x00002000;

	for(int i = 0; i < IotDmaTestLength; i++)
	{
		buffer1[i] = 0;
	}	
	for(int i = 0; i < IotDmaTestLength; i++)
	{
		buffer2[i] = 0;
	}	

	if(buffer1)
    {
        for ( uint32_t idx = 0; idx < IotDmaTestLength; idx++ )
        {
            buffer1[ idx ] = ucPatternData[ idx % 4 ];
        }
    }

    ulRetVal = do_dma_transfer(buffer1, buffer2, IotDmaTestLength);
    TEST_ASSERT_EQUAL(0, ulRetVal);

	/**********************************************************************/
	/*					 OCRAM to DRAM transfer                           */
	/**********************************************************************/
	buffer1 = (uint8_t *)0x00000010;
	for(int i = 0; i < IotDmaTestLength; i++)
	{
		buffer1[i] = 0;
	}	

 	buffer2 = (uint8_t *)pvPortMalloc(IotDmaTestLength);
	if( buffer2 != NULL )
	{
		bzero(buffer2, IotDmaTestLength);
		cache_force_write_back(buffer2, IotDmaTestLength);
	}

	if(buffer1)
    {
        for ( uint32_t idx = 0; idx < IotDmaTestLength; idx++ )
        {
            buffer1[ idx ] = ucPatternData[ idx % 4 ];
        }
    }

    ulRetVal = do_dma_transfer(buffer1, buffer2, IotDmaTestLength);
    TEST_ASSERT_EQUAL(0, ulRetVal);

	vPortFree(buffer2);
	/**********************************************************************/
	/*					 DRAM to OCRAM transfer                           */
	/**********************************************************************/

	buffer1 = (uint8_t *)pvPortMalloc(IotDmaTestLength);
	if( buffer1 != NULL )
	{
		bzero(buffer2, IotDmaTestLength);
        for ( uint32_t idx = 0; idx < IotDmaTestLength; idx++ )
        {
            buffer1[ idx ] = ucPatternData[ idx % 4 ];
        }
        cache_force_write_back(buffer1, IotDmaTestLength);
	}

	buffer2 = (uint8_t *)0x00000010;
	for(int i = 0; i < IotDmaTestLength; i++)
	{
		buffer1[i] = 0;
	}	

    ulRetVal = do_dma_transfer(buffer1, buffer2, IotDmaTestLength);
    TEST_ASSERT_EQUAL(0, ulRetVal);

	vPortFree(buffer1);
	/*********************************************************************/

    ulRetVal = iot_dma_close(xDmaHandle);
    TEST_ASSERT_EQUAL(0, ulRetVal);

    pxGlobalDmaHandle = NULL;
}

/**
 * @brief Test Function to test dma transfer.
 *
 */
TEST( TEST_IOT_DMA, AFWP_IotDma_Transfer)
{
    int32_t ulRetVal;
    xDmaHandle = iot_dma_open(IotDmaTestInstance, IotDmaTestChannel);
    TEST_ASSERT_NOT_EQUAL(NULL, xDmaHandle);
    pxGlobalDmaHandle = xDmaHandle;
    ulRetVal = ulDmaDoTransfer(IotDmaTestLength);
    TEST_ASSERT_EQUAL(0, ulRetVal);
    ulRetVal = iot_dma_close(xDmaHandle);
    TEST_ASSERT_EQUAL(0, ulRetVal);
    pxGlobalDmaHandle = NULL;
}

/**
 * @brief Test Function to test whether mutiple channels can be used simultaneously.
 *
 */
TEST( TEST_IOT_DMA, AFWP_IotDma_MultiChannelOpenClose )
{
    IotDmaHandle_t xDmaHandle1, xDmaHandle2, xDmaHandle3, xDmaHandle4;
    int32_t ulRetVal;

    xDmaHandle1 = iot_dma_open(IotDmaTestInstance, IotDmaTestChannel);
    TEST_ASSERT_NOT_EQUAL(NULL, xDmaHandle1);

    xDmaHandle2 = iot_dma_open(IotDmaTestInstance, IotDmaTestChannel2);
    TEST_ASSERT_NOT_EQUAL(NULL, xDmaHandle2);

    xDmaHandle3 = iot_dma_open(IotDmaTestInstance, IotDmaTestChannel3);
    TEST_ASSERT_NOT_EQUAL(NULL, xDmaHandle3);

    xDmaHandle4 = iot_dma_open(IotDmaTestInstance, IotDmaTestChannel4);
    TEST_ASSERT_NOT_EQUAL(NULL, xDmaHandle4);

    ulRetVal = iot_dma_close(xDmaHandle1);
    TEST_ASSERT_EQUAL(0, ulRetVal);

    ulRetVal = iot_dma_close(xDmaHandle2);
    TEST_ASSERT_EQUAL(0, ulRetVal);

    ulRetVal = iot_dma_close(xDmaHandle3);
    TEST_ASSERT_EQUAL(0, ulRetVal);

    ulRetVal = iot_dma_close(xDmaHandle4);
    TEST_ASSERT_EQUAL(0, ulRetVal);
}

/**
 * @brief Test Function to test multi block dma transfer.
 *
 */
TEST( TEST_IOT_DMA, AFWP_IotDma_MultiTransfer)
{
    int32_t ulRetVal, count;

    xDmaHandle = iot_dma_open(IotDmaTestInstance, IotDmaTestChannel);
    TEST_ASSERT_NOT_EQUAL(NULL, xDmaHandle);
    pxGlobalDmaHandle = xDmaHandle;

    for ( count = 0; count < 10; count++ )
    {
        ulRetVal = ulDmaDoTransfer(IotDmaTestLength);
        TEST_ASSERT_EQUAL(0, ulRetVal);
    }
    ulRetVal = iot_dma_close(xDmaHandle);
    TEST_ASSERT_EQUAL(0, ulRetVal);

    pxGlobalDmaHandle = NULL;
}

/*
* @brief Test Function to test dma with multiple entries transfer.
*
*/
TEST( TEST_IOT_DMA, AFWP_IotDma_MultiChannelTransfer)
{
   uint32_t ulRetVal;
   uint32_t i;
   uint32_t dmaCh;
   int length[] ={16000, 1500, 1500, 80};
  /* TODO : Only 3 threads are running successfull as the 4 thread always fail due to partial data transfer */
   for(i=0;i<(MAX_CHANNEL_NUM-1);i++)
   {
       tableInit(&(table[i].src), &(table[i].dest), length[i], i);
       table[i].len = length[i];

   }

   void (*taskPtr[])(void*) = {dma_ch_thread_1, dma_ch_thread_2, dma_ch_thread_3, dma_ch_thread_4};
   for(dmaCh=0;dmaCh<(MAX_CHANNEL_NUM-1);dmaCh++)
   {
       xMCDmaHandle[dmaCh] = iot_dma_open(IotDmaTestInstance, dmaCh);
       TEST_ASSERT_NOT_EQUAL(NULL, xMCDmaHandle[dmaCh]);
       xTaskCreate(taskPtr[dmaCh], "TransferThread", configMINIMAL_STACK_SIZE, (void*)&table[dmaCh], configMAX_PRIORITIES - 2, NULL);
   }
   while(threadCntFinish < (MAX_CHANNEL_NUM-1));
   for(dmaCh=0;dmaCh<(MAX_CHANNEL_NUM-1);dmaCh++)
   {
       ulRetVal = iot_dma_close(xMCDmaHandle[dmaCh]);
       TEST_ASSERT_EQUAL(0, ulRetVal);
   }
   /* Set the global Dma handle to NULL as we closed the handle */
   pxGlobalDmaHandle = NULL;
}

static void dma_ch_thread_1(void *pvParameters)
{
    IotDmaHandle_t xDmaHandle;
    uint8_t *pucSrc;
    uint8_t *pucDst;
    int res = 0;
    DMAParams_t *params;
    xDmaHandle = xMCDmaHandle[0];
    params = (DMAParams_t*)pvParameters;
    pucSrc = (uint8_t *)params->src;
    pucDst = (uint8_t *)params->dest;

    struct IotDmaConfig xDmaConfig;
    struct IotDmaBlockTransferCfg xDmaBlkTrnsfr;

    xDmaConfig.uDmaChannelDirection = eDmaMemToMemDmac;
    xDmaConfig.uDmaChannelPriority = 0;
    xDmaConfig.ulDmaLinkedListCount = 1;
    xDmaConfig.pxDmaCallback = (IotDmaCallback_t)prvDmarCallback;

    xDmaBlkTrnsfr.ulSrcAddr = (uint64_t)(pucSrc);
    xDmaBlkTrnsfr.ulDestAddr = (uint64_t)(pucDst);
    xDmaBlkTrnsfr.ulBlockSize = params->len+2; //Max size 4 * 2^21
    xDmaBlkTrnsfr.xNextTrnsfrCfg = NULL;

    if(xDmaHandle == NULL)
    {
        return;
    }

    iot_dma_config(xDmaHandle, &xDmaConfig);
    iot_dma_setup(xDmaHandle, &xDmaBlkTrnsfr);
    iot_dma_start(xDmaHandle);

    /*****************************************/

    osal_semaphore_wait(xtestIotDmaSemaphore, portMAX_DELAY);

    cache_force_invalidate(pucDst, params->len);
    for(uint32_t idx = 0; idx < params->len; idx++) {
        if(pucDst[idx] != pucSrc[idx])
        {
            res = -EFAULT;
        }
    }
    TEST_ASSERT_EQUAL(0, res);
    threadCntFinish++;
    for(;;);
}

static void dma_ch_thread_2(void *pvParameters)
{
    IotDmaHandle_t xDmaHandle;
    uint8_t *pucSrc;
    uint8_t *pucDst;
    int res = 0;
    DMAParams_t *params;
    xDmaHandle = xMCDmaHandle[1];
    params = (DMAParams_t*)pvParameters;
    pucSrc = (uint8_t *)params->src;
    pucDst = (uint8_t *)params->dest;

    struct IotDmaConfig xDmaConfig;
    struct IotDmaBlockTransferCfg xDmaBlkTrnsfr;

    xDmaConfig.uDmaChannelDirection = eDmaMemToMemDmac;
    xDmaConfig.uDmaChannelPriority = 0;
    xDmaConfig.ulDmaLinkedListCount = 1;
    xDmaConfig.pxDmaCallback = (IotDmaCallback_t)prvDmaMultithreadCallback2;

    xDmaBlkTrnsfr.ulSrcAddr = (uint64_t)(pucSrc);
    xDmaBlkTrnsfr.ulDestAddr = (uint64_t)(pucDst);
    xDmaBlkTrnsfr.ulBlockSize = params->len+2; //Max size 4 * 2^21
    xDmaBlkTrnsfr.xNextTrnsfrCfg = NULL;

    if(xDmaHandle == NULL)
    {
        return;
    }

    iot_dma_config(xDmaHandle, &xDmaConfig);
    iot_dma_setup(xDmaHandle, &xDmaBlkTrnsfr);
    iot_dma_start(xDmaHandle);

    /*****************************************/

    osal_semaphore_wait(xtestIotDmaSemThread2, portMAX_DELAY);

    cache_force_invalidate(pucDst, params->len);
    for(uint32_t idx = 0; idx < params->len; idx++) {
        if(pucDst[idx] != pucSrc[idx])
        {
            res = -EFAULT;
        }
    }
    TEST_ASSERT_EQUAL(0, res);
    threadCntFinish++;
    for(;;);
}

static void dma_ch_thread_3(void *pvParameters)
{
    IotDmaHandle_t xDmaHandle;
    uint8_t *pucSrc;
    uint8_t *pucDst;
    int res = 0;
    DMAParams_t *params;
    xDmaHandle = xMCDmaHandle[2];
    params = (DMAParams_t*)pvParameters;
    pucSrc = (uint8_t *)params->src;
    pucDst = (uint8_t *)params->dest;

    struct IotDmaConfig xDmaConfig;
    struct IotDmaBlockTransferCfg xDmaBlkTrnsfr;

    xDmaConfig.uDmaChannelDirection = eDmaMemToMemDmac;
    xDmaConfig.uDmaChannelPriority = 0;
    xDmaConfig.ulDmaLinkedListCount = 1;
    xDmaConfig.pxDmaCallback = (IotDmaCallback_t)prvDmaMultithreadCallback3;

    xDmaBlkTrnsfr.ulSrcAddr = (uint64_t)(pucSrc);
    xDmaBlkTrnsfr.ulDestAddr = (uint64_t)(pucDst);
    xDmaBlkTrnsfr.ulBlockSize = params->len+4; //Max size 4 * 2^21
    xDmaBlkTrnsfr.xNextTrnsfrCfg = NULL;

    if(xDmaHandle == NULL)
    {
        return;
    }

    iot_dma_config(xDmaHandle, &xDmaConfig);
    iot_dma_setup(xDmaHandle, &xDmaBlkTrnsfr);
    iot_dma_start(xDmaHandle);

    /*****************************************/

    osal_semaphore_wait(xtestIotDmaSemThread3, portMAX_DELAY);
    cache_force_invalidate(pucDst, params->len);
    for(uint32_t idx = 0; idx < params->len; idx++) {
        if(pucDst[idx] != pucSrc[idx])
        {
            res = -EFAULT;
        }
    }
    TEST_ASSERT_EQUAL(0, res);
    threadCntFinish++;
    for(;;);
}

static void dma_ch_thread_4(void *pvParameters)
{
    IotDmaHandle_t xDmaHandle;
    uint8_t *pucSrc;
    uint8_t *pucDst;
    int res = 0;
    DMAParams_t *params;
    xDmaHandle = xMCDmaHandle[3];
    params = (DMAParams_t*)pvParameters;
    pucSrc = (uint8_t *)params->src;
    pucDst = (uint8_t *)params->dest;

    struct IotDmaConfig xDmaConfig;
    struct IotDmaBlockTransferCfg xDmaBlkTrnsfr;

    xDmaConfig.uDmaChannelDirection = eDmaMemToMemDmac;
    xDmaConfig.uDmaChannelPriority = 0;
    xDmaConfig.ulDmaLinkedListCount = 1;
    xDmaConfig.pxDmaCallback = (IotDmaCallback_t)prvDmaMultithreadCallback4;

    xDmaBlkTrnsfr.ulSrcAddr = (uint64_t)(pucSrc);
    xDmaBlkTrnsfr.ulDestAddr = (uint64_t)(pucDst);
    xDmaBlkTrnsfr.ulBlockSize = params->len+2; //Max size 4 * 2^21
    xDmaBlkTrnsfr.xNextTrnsfrCfg = NULL;

    if(xDmaHandle == NULL)
    {
        return;
    }

    iot_dma_config(xDmaHandle, &xDmaConfig);
    iot_dma_setup(xDmaHandle, &xDmaBlkTrnsfr);
    iot_dma_start(xDmaHandle);

    /*****************************************/

    osal_semaphore_wait(xtestIotDmaSemThread4, portMAX_DELAY);
    osal_delay_ms(10000);
    cache_force_invalidate(pucDst, params->len);
    for(uint32_t idx = 0; idx < params->len; idx++) {
        if(pucDst[idx] != pucSrc[idx])
        {
            res = -EFAULT;
        }
    }
    TEST_ASSERT_EQUAL(0, res);
    threadCntFinish++;
    for(;;);
}


/**
 * @brief Test Function to write data in one channel.
 *
 */
TEST( TEST_IOT_DMA, AFWP_IotDma_WriteCloseRead)
{
    int32_t ulRetVal;
    xDmaHandle = iot_dma_open(IotDmaTestInstance, IotDmaTestChannel);
    TEST_ASSERT_NOT_EQUAL(NULL, xDmaHandle);
    pxGlobalDmaHandle = xDmaHandle;
    ulRetVal = ulDmaDoTransferAsync(IotDmaTestLength);
    TEST_ASSERT_EQUAL(0, ulRetVal);
    osal_semaphore_wait(xtestIotDmaSemaphore, portMAX_DELAY);
    ulRetVal = iot_dma_close(xDmaHandle);
    TEST_ASSERT_EQUAL(0, ulRetVal);
    ulRetVal = ulDmaTestTransferStatus(IotDmaTestLength);
    TEST_ASSERT_EQUAL(0, ulRetVal);
}
TEST(TEST_IOT_DMA, AFWP_IotDma_AsyncMultiThreadTransfer)
{
    int32_t ulRetVal;
    xDmaHandle = iot_dma_open(IotDmaTestInstance, IotDmaTestChannel);
    TEST_ASSERT_NOT_EQUAL(NULL, xDmaHandle);
    pxGlobalDmaHandle = xDmaHandle;
    xTaskCreate(dma_reader_thread, "DmaReaderThread",
    configMINIMAL_STACK_SIZE, (void*) xDmaHandle, 3, NULL);
    ulRetVal = ulDmaDoTransferAsync(IotDmaTestLength);
    TEST_ASSERT_EQUAL(0, ulRetVal);
    osal_semaphore_wait(xtestIotDmaSemaphore, portMAX_DELAY);
    osal_semaphore_post(xDmaTransferSemaphore);
    osal_semaphore_wait(xDmaReaderDoneSemaphore, portMAX_DELAY);

    ulRetVal = iot_dma_close(xDmaHandle);
    TEST_ASSERT_EQUAL(0, ulRetVal);
}

TEST(TEST_IOT_DMA, test_dma_open_valid) {
    xDmaHandle_t handle;

    handle = dma_open(IotDmaTestInstance, IotDmaTestChannel);
    TEST_ASSERT_NOT_NULL(handle);
    dma_close(handle);

    handle = dma_open(IotDmaTestInstance2, IotDmaTestChannel2);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = dma_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_DMA, test_dma_open_invalid_instance) {
    xDmaHandle_t handle;

    handle = dma_open(DMA_INSTANCE_1 + 'A', IotDmaTestChannel);
    TEST_ASSERT_NULL(handle);
}

TEST(TEST_IOT_DMA, test_dma_open_invalid_channel) {
    xDmaHandle_t handle;

    handle = dma_open(IotDmaTestInstance, DMA_CHANNEL_4 + 'A');
    TEST_ASSERT_NULL(handle);
}

TEST(TEST_IOT_DMA, test_dma_open_same_instance_twice) {
    xDmaHandle_t handle1, handle2;

    handle1 = dma_open(IotDmaTestInstance, IotDmaTestChannel);
    TEST_ASSERT_NOT_NULL(handle1);

    handle2 = dma_open(IotDmaTestInstance, IotDmaTestChannel);
    TEST_ASSERT_NULL(handle2);

    int32_t result = dma_close(handle1);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_DMA, test_dma_config_success) {
    xDmaHandle_t handle = dma_open(IotDmaTestInstance, IotDmaTestChannel);
    TEST_ASSERT_NOT_NULL(handle);

    DmaConfig_t config = {
        .ulDmaInstance = IotDmaTestInstance,
        .uDmaChannelDirection = eDmaMemToMemDmac,
        .uDmaChannelPriority = 1,
        .ulPeripheralId = eDmaFpgaPeripheral0,
        .ulDmaLinkedListCount = 0,
        .pxDmaCallback = NULL
    };

    int32_t result = dma_config(handle, &config);
    TEST_ASSERT_EQUAL(0, result);

    result = dma_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_DMA, test_dma_config_null_handle) {
    DmaConfig_t config = {
        .ulDmaInstance = IotDmaTestInstance,
        .uDmaChannelDirection = eDmaMemToMemDmac,
        .uDmaChannelPriority = 1,
        .ulPeripheralId = eDmaFpgaPeripheral0,
        .ulDmaLinkedListCount = 0,
        .pxDmaCallback = NULL
    };

    int32_t result = dma_config(NULL, &config);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST(TEST_IOT_DMA, test_dma_config_busy) {
    xDmaHandle_t handle = dma_open(IotDmaTestInstance, IotDmaTestChannel);
    TEST_ASSERT_NOT_NULL(handle);

    DmaConfig_t config = {
        .ulDmaInstance = IotDmaTestInstance,
        .uDmaChannelDirection = eDmaMemToMemDmac,
        .uDmaChannelPriority = 1,
        .ulPeripheralId = eDmaFpgaPeripheral0,
        .ulDmaLinkedListCount = 0,
        .pxDmaCallback = NULL
    };

    // Simulate a busy state
    int32_t result = dma_start_tranfer(handle);
    TEST_ASSERT_EQUAL(0, result);

    result = dma_config(handle, &config);
    TEST_ASSERT_EQUAL(-EBUSY, result);

//    result = dma_stop_transfer(handle);
//    TEST_ASSERT_EQUAL(0, result);

    result = dma_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_DMA, test_dma_setup_transfer_invalid_value) {
    DmaBlkTrnsfrCfg_t blkTrnsfrCfg = {0x1000, 0x2000, 1024, NULL};

    int32_t result = dma_setup_transfer(NULL, &blkTrnsfrCfg);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST(TEST_IOT_DMA, test_dma_setup_transfer_busy) {
    xDmaHandle_t dmaHandle = dma_open(IotDmaTestInstance2, IotDmaTestChannel2);
    TEST_ASSERT_NOT_NULL(dmaHandle);

    DmaBlkTrnsfrCfg_t blkTrnsfrCfg = {0x1000, 0x2000, 1024, NULL};

    // Simulate ongoing transfer
    int32_t result = dma_start_tranfer(dmaHandle);
    TEST_ASSERT_EQUAL(0, result);

    result = dma_setup_transfer(dmaHandle, &blkTrnsfrCfg);
    TEST_ASSERT_EQUAL(-EBUSY, result);

//    result = dma_stop_transfer(dmaHandle);
//    TEST_ASSERT_EQUAL(0, result);

    result = dma_close(dmaHandle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_DMA, test_dma_start_transfer_success) {
    xDmaHandle = dma_open(IotDmaTestInstance, IotDmaTestChannel); // Initialize and return a valid handle
    TEST_ASSERT_NOT_NULL(xDmaHandle);
    int32_t result;

    result = dma_start_tranfer(xDmaHandle);
    TEST_ASSERT_EQUAL_UINT32(0, result);

    result = dma_close(xDmaHandle); // Clean up the handle
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_DMA, test_dma_start_transfer_invalid_value) {
    int32_t result = dma_start_tranfer(NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

//test_dma_stop_transfer_null_handle
TEST(TEST_IOT_DMA, test_dma_stop_transfer_null_handle) {
    int32_t result = dma_stop_transfer(NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}
//test_dma_stop_transfer_no_transfer_in_progress
TEST(TEST_IOT_DMA, test_dma_stop_transfer_no_transfer_in_progress) {
    xDmaHandle_t handle = dma_open(IotDmaTestInstance, IotDmaTestChannel);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = dma_stop_transfer(handle);
    TEST_ASSERT_EQUAL(-EIO, result);

    result = dma_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

//test_dma_suspend_transfer_null_handle
TEST(TEST_IOT_DMA, test_dma_suspend_transfer_null_handle) {
    int32_t result = dma_suspend_transfer(NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}
//test_dma_suspend_transfer_no_transfer_in_progress
TEST(TEST_IOT_DMA, test_dma_suspend_transfer_no_transfer_in_progress) {
    xDmaHandle_t handle = dma_open(IotDmaTestInstance, IotDmaTestChannel);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = dma_suspend_transfer(handle);
    TEST_ASSERT_EQUAL(-EIO, result);

    result = dma_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}
//test_dma_suspend_transfer_failure
TEST(TEST_IOT_DMA, test_dma_suspend_transfer_failure) {
    xDmaHandle_t handle = dma_open(IotDmaTestInstance, IotDmaTestChannel);
    TEST_ASSERT_NOT_NULL(handle);

    DmaBlkTrnsfrCfg_t blkTrnsfrCfg = get_blk_transfer_cfg(IotDmaTestLength);

    int32_t result = dma_setup_transfer(handle, &blkTrnsfrCfg); // Assuming this sets up a transfer
    TEST_ASSERT_EQUAL(0, result);

    result = dma_start_tranfer(handle); // Assuming this starts the transfer
    TEST_ASSERT_EQUAL(0, result);

    // Simulate a condition where suspension fails
    // This part depends on the implementation details of dma_suspend_transfer

    result = dma_suspend_transfer(handle);
    TEST_ASSERT_EQUAL(-EBUSY, result);

    result = dma_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

//test_dma_resume_transfer_null_handle
TEST(TEST_IOT_DMA, test_dma_resume_transfer_null_handle) {
    int32_t result = dma_resume_transfer(NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

// test_dma_close_valid_handle
TEST(TEST_IOT_DMA, test_dma_close_valid_handle) {
    xDmaHandle = iot_dma_open(IotDmaTestInstance, IotDmaTestChannel);
    TEST_ASSERT_NOT_EQUAL(NULL, xDmaHandle);

    int32_t result = dma_close(xDmaHandle);
    TEST_ASSERT_EQUAL_UINT32(0, result);
}
//test_dma_close_null_handle
TEST(TEST_IOT_DMA, test_dma_close_null_handle) {
    int32_t result = dma_close(NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}
