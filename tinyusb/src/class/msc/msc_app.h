#ifndef __USB_MSCH__
#define __USB_MSCH__

bool usb_disk_read(void *buffer, uint32_t lba, uint16_t count);

bool usb_disk_write(void *buffer, uint32_t lba, uint16_t count);

/*
 * @func  : is_msc_mount_complete
 * @brief : function to check whether the MSC mount process is completed or not
 * @return 
 *      1 if MSC mount is completed
 *      0 otherwise
 */
int is_msc_mount_complete(void);

#endif
