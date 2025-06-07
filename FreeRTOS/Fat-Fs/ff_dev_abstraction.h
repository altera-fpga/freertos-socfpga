#ifndef __FF_DEV_ABSTRACTION__
#define __FF_DEV_ABSTRACTION__

int32_t FFWriteSdmmc( uint8_t *pucBuffer, uint32_t ulSectorNumber,
        uint32_t ulSectorCount, FF_Disk_t *pxDisk );

int32_t FFReadSdmmc( uint8_t *pucBuffer, uint32_t ulSectorNumber,
        uint32_t ulSectorCount, FF_Disk_t *pxDisk );

int32_t FatReadAbsLayer( uint8_t *pucBuffer, uint32_t ulSectorNumber,
        uint32_t ulSectorCount, FF_Disk_t *pxDisk );
int32_t FatWriteAbsLayer( uint8_t *pucBuffer, uint32_t ulSectorNumber,
        uint32_t ulSectorCount, FF_Disk_t *pxDisk );

#endif
