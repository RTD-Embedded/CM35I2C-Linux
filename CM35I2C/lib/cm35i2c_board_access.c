/**
	@file

	@brief
		CM35I2C Board Access library source code

	$Id: cm35i2c_board_access.c 104885 2016-12-01 21:29:08Z rgroner $
*/

//----------------------------------------------------------------------------
//  COPYRIGHT (C) RTD EMBEDDED TECHNOLOGIES, INC.  ALL RIGHTS RESERVED.
//
//  This software package is dual-licensed.  Source code that is compiled for
//  kernel mode execution is licensed under the GNU General Public License
//  version 2.  For a copy of this license, refer to the file
//  LICENSE_GPLv2.TXT (which should be included with this software) or contact
//  the Free Software Foundation.  Source code that is compiled for user mode
//  execution is licensed under the RTD End-User Software License Agreement.
//  For a copy of this license, refer to LICENSE.TXT or contact RTD Embedded
//  Technologies, Inc.  Using this software indicates agreement with the
//  license terms listed above.
//----------------------------------------------------------------------------

#include <sys/ioctl.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


#include "cm35i2c_gbc_library.h"
#include "cm35i2c_ioctl.h"
#include "cm35i2c_board_access.h"

#define DEVICE_NAME_PATH_PREFIX "/dev/rtd-cm35i2c"

int
CM35I2C_Board_Open(uint8_t dev_num, struct CM35I2C_Board_Descriptor **handle)
{
	char device_name[25];
	int descriptor;

	/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	   Form the device file name and attempt to open the file
	   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

	(void)snprintf(&(device_name[0]), sizeof(device_name),
			   DEVICE_NAME_PATH_PREFIX "-%u", dev_num);

	descriptor = open(&(device_name[0]), O_RDWR);
	if (descriptor == -1) {
		if (errno != EBUSY) {
			*handle = NULL;
		}
		return -1;
	}

	/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	   Allocate and initialize memory for the library device descriptor
	   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

	*handle = (struct CM35I2C_Board_Descriptor *)
			   malloc(sizeof(struct CM35I2C_Board_Descriptor));
	if (*handle == NULL) {
		errno = ENOMEM;
		return -1;
	}

	(void)memset(*handle, 0x00, sizeof(struct CM35I2C_Board_Descriptor));

	(*handle)->file_descriptor = descriptor;
	(*handle)->isr = NULL;
	return 0;
}


int CM35I2C_Board_Close(struct CM35I2C_Board_Descriptor *handle)
{
	if (handle == NULL) {
		errno = ENODATA;
		return -1;
	}

	if (close(handle->file_descriptor) == -1) {
		free(handle);
		return -1;
	}

	free(handle);
	return 0;
}

int
CM35I2C_Read(struct CM35I2C_Board_Descriptor *handle,
	union cm35i2c_ioctl_argument *ioctl_request)
{

	return ioctl(handle->file_descriptor, CM35I2C_IOCTL_REGION_READ,
			ioctl_request);

}

int
CM35I2C_Write(struct CM35I2C_Board_Descriptor *handle,
	union cm35i2c_ioctl_argument *ioctl_request)
{
	return ioctl(handle->file_descriptor, CM35I2C_IOCTL_REGION_WRITE,
			ioctl_request);

}

int
CM35I2C_Modify(struct CM35I2C_Board_Descriptor *handle,
	union cm35i2c_ioctl_argument *ioctl_request)
{

	return ioctl(handle->file_descriptor, CM35I2C_IOCTL_REGION_MODIFY,
			ioctl_request);

}

int
CM35I2C_Dma(struct CM35I2C_Board_Descriptor *handle,
    union cm35i2c_ioctl_argument *ioctl_request)
{
    return ioctl(handle->file_descriptor, CM35I2C_IOCTL_DMA_FUNCTION,
        ioctl_request);
}

