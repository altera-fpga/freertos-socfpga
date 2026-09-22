#ifndef LIBFCS_UTILS_H
#define LIBFCS_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "libfcs_osal.h"

/*
    File: Portable Layer FCS Library Utilities

    Declares the Portable Layer FCS Library utilities interface: 2
    functions.
*/

/*
    Function: fat_get_size

    Gets the file size

    Parameters:

        file - Name of the file. It shall contain the source device
            also.(eg: /usb/file)
*/
uint32_t fat_get_size( const char *file );
/*
    Function: fat_read

    Reads contents of the file

    Parameters:

        file - Name of the file. It shall contain the source device
            also.(eg: /usb/file)
        buffer - Buffer to store contents
*/

uint32_t fat_read( const char *FileName, void *buffer );

#ifdef __cplusplus
}
#endif /* __cplusplus */

// #endif
#endif
