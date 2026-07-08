/**
	@file

	@brief
		CM35I2C Board library source code


	$Id: librtd-cm35i2c_gbc.c 104760 2016-11-28 20:17:52Z rgroner $
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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

#include "cm35i2c_registers.h"
#include "cm35i2c_board_access.h"
#include "cm35i2c_gbc_library.h"
#include "cm35i2c_util_library.h"
#include "cm35i2c_dma_library.h"
#include "cm35i2c_board_access_structs.h"
#include "cm35i2c_i2c3300.h"


#define CM35I2C_RESET_DELAY_MILLI_SEC	1000
#define CM35I2C_MAX_CHECK_READY_ATTEMPTS 100
#define CM35I2C_MAX_WRITE_FIFO_SIZE		2048
#define CM35I2C_I2C3300_FB_RESET		0xAA

#define CM35I2C_I2C3300_STDCLK_100K 	0x1F3
#define CM35I2C_I2C3300_STDCLK_400K 	0x7C
#define CM35I2C_I2C3300_STDCLK_1M 		0x31


CM35I2CLIB_API
int CM35I2C_I2C3300_Fifo_Write(struct CM35I2C_Board_Descriptor *handle,
                                        const struct CM35I2C_Function_Block *func_block,
                                        uint32_t value)
{
	union cm35i2c_ioctl_argument ioctl_request;
	int return_code = 0;
        
	// Set up the IOCTL struct
        ioctl_request.readwrite.access.offset = func_block->fb_offset + CM35I2C_I2C3300_OFFSET_WRITE;
        ioctl_request.readwrite.access.region = CM35I2C_PCI_REGION_FB;
        ioctl_request.readwrite.access.size = CM35I2C_PCI_REGION_ACCESS_32;
        ioctl_request.readwrite.access.data.data32 = value;

        // Perform the write
        return_code = CM35I2C_Write(handle, &ioctl_request);
        
	return return_code;
}

CM35I2CLIB_API
int CM35I2C_I2C3300_Bus_Control_Write(struct CM35I2C_Board_Descriptor *handle,
                                        const struct CM35I2C_Function_Block *func_block,
                                        uint32_t value)
{
	union cm35i2c_ioctl_argument ioctl_request;
	int return_code = 0;
        
	// Set up the IOCTL struct
        ioctl_request.readwrite.access.offset = func_block->fb_offset + CM35I2C_I2C3300_OFFSET_I2C_BUS_CTRL;
        ioctl_request.readwrite.access.region = CM35I2C_PCI_REGION_FB;
        ioctl_request.readwrite.access.size = CM35I2C_PCI_REGION_ACCESS_32;
        ioctl_request.readwrite.access.data.data32 = value;

        // Perform the write
        return_code = CM35I2C_Write(handle, &ioctl_request);
        
	return return_code;
}

CM35I2CLIB_API
int CM35I2C_I2C3300_Fifo_Read(struct CM35I2C_Board_Descriptor *handle,
                                        const struct CM35I2C_Function_Block *func_block,
                                        uint8_t *value)
{
	union cm35i2c_ioctl_argument ioctl_request;
	int return_val;

	ioctl_request.readwrite.access.offset = func_block->fb_offset + CM35I2C_I2C3300_OFFSET_READ;
	ioctl_request.readwrite.access.region = CM35I2C_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = CM35I2C_PCI_REGION_ACCESS_8;

	return_val = CM35I2C_Read(handle, &ioctl_request);

	*value = ioctl_request.readwrite.access.data.data8;

	return return_val;
}


CM35I2CLIB_API
int CM35I2C_I2C3300_Bus_Control_Read(struct CM35I2C_Board_Descriptor *handle,
				const struct CM35I2C_Function_Block *func_block,
				uint8_t *go_busy_return)
{

	union cm35i2c_ioctl_argument ioctl_request;
	int return_val;

	ioctl_request.readwrite.access.offset = func_block->fb_offset + CM35I2C_I2C3300_OFFSET_I2C_BUS_CTRL;
	ioctl_request.readwrite.access.region = CM35I2C_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = CM35I2C_PCI_REGION_ACCESS_8;

	return_val = CM35I2C_Read(handle, &ioctl_request);

	*go_busy_return = ioctl_request.readwrite.access.data.data8;

	return return_val;
}



CM35I2CLIB_API
int CM35I2C_I2C3300_Get_Fifo_Write_Count(struct CM35I2C_Board_Descriptor *handle,
				const struct CM35I2C_Function_Block *func_block,
				uint8_t *write_count_out)
{

	union cm35i2c_ioctl_argument ioctl_request;
	int return_val;

	ioctl_request.readwrite.access.offset = func_block->fb_offset + CM35I2C_I2C3300_OFFSET_WRITE_COUNT;
	ioctl_request.readwrite.access.region = CM35I2C_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = CM35I2C_PCI_REGION_ACCESS_8;

	return_val = CM35I2C_Read(handle, &ioctl_request);

	*write_count_out = ioctl_request.readwrite.access.data.data8;

	return return_val;
}

CM35I2CLIB_API
int CM35I2C_I2C3300_Get_Fifo_Read_Count(struct CM35I2C_Board_Descriptor *handle,
				const struct CM35I2C_Function_Block *func_block,
				uint8_t *read_fifo_count_out)
{

	union cm35i2c_ioctl_argument ioctl_request;
	int return_val;

	ioctl_request.readwrite.access.offset = func_block->fb_offset + CM35I2C_I2C3300_OFFSET_READ_COUNT;
	ioctl_request.readwrite.access.region = CM35I2C_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = CM35I2C_PCI_REGION_ACCESS_8;

	return_val = CM35I2C_Read(handle, &ioctl_request);

	*read_fifo_count_out = ioctl_request.readwrite.access.data.data8;

	return return_val;
}

CM35I2CLIB_API
int CM35I2C_I2C3300_Function_Block_Reset(struct CM35I2C_Board_Descriptor *handle,
				const struct CM35I2C_Function_Block *func_block)
{

	union cm35i2c_ioctl_argument ioctl_request;
	int return_code = 0;
        
	// Set up the IOCTL struct
    ioctl_request.readwrite.access.offset = func_block->fb_offset + CM35I2C_I2C3300_OFFSET_I2C_BUS_CTRL;
    ioctl_request.readwrite.access.region = CM35I2C_PCI_REGION_FB;
    ioctl_request.readwrite.access.size = CM35I2C_PCI_REGION_ACCESS_32;
    ioctl_request.readwrite.access.data.data32 = CM35I2C_I2C3300_FB_RESET;

    // Perform the write
    return_code = CM35I2C_Write(handle, &ioctl_request);

	return return_code;
}

CM35I2CLIB_API
int CM35I2C_I2C3300_Get_Status(struct CM35I2C_Board_Descriptor *handle,
				const struct CM35I2C_Function_Block *func_block,
				uint32_t *read_fifo_count_out)
{

	union cm35i2c_ioctl_argument ioctl_request;
	int return_val;

	ioctl_request.readwrite.access.offset = func_block->fb_offset + CM35I2C_I2C3300_OFFSET_INT_STAT;
	ioctl_request.readwrite.access.region = CM35I2C_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = CM35I2C_PCI_REGION_ACCESS_32;

	return_val = CM35I2C_Read(handle, &ioctl_request);
	*read_fifo_count_out = ioctl_request.readwrite.access.data.data32;

	return return_val;
}

CM35I2CLIB_API
int CM35I2C_I2C3300_Set_Clock_Register(struct CM35I2C_Board_Descriptor *handle,
                                        const struct CM35I2C_Function_Block *func_block,
                                        uint32_t value)
{
	union cm35i2c_ioctl_argument ioctl_request;
	int return_code = 0;
        
	// Set up the IOCTL struct
        ioctl_request.readwrite.access.offset = func_block->fb_offset + CM35I2C_I2C3300_OFFSET_CLOCK_RATE;
        ioctl_request.readwrite.access.region = CM35I2C_PCI_REGION_FB;
        ioctl_request.readwrite.access.size = CM35I2C_PCI_REGION_ACCESS_32;
        ioctl_request.readwrite.access.data.data32 = value;

        // Perform the write
        return_code = CM35I2C_Write(handle, &ioctl_request);
        
	return return_code;
}

CM35I2CLIB_API
int CM35I2C_I2C3300_Get_Clock_Register(struct CM35I2C_Board_Descriptor *handle,
				const struct CM35I2C_Function_Block *func_block,
				uint32_t *kHz_clock_rate_out)
{

	union cm35i2c_ioctl_argument ioctl_request;
	int return_val;

	ioctl_request.readwrite.access.offset = func_block->fb_offset + CM35I2C_I2C3300_OFFSET_CLOCK_RATE;
	ioctl_request.readwrite.access.region = CM35I2C_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = CM35I2C_PCI_REGION_ACCESS_32;

	return_val = CM35I2C_Read(handle, &ioctl_request);
	*kHz_clock_rate_out = ioctl_request.readwrite.access.data.data32;

	return return_val;
}


// All clock rates for input should be 1 == 1kHz
CM35I2CLIB_API
int CM35I2C_I2C3300_Clock_To_Register_Value(uint32_t kHz_clock_rate,uint32_t * register_value)
{
	if(kHz_clock_rate <= 0 && kHz_clock_rate >= 50000 )
	{
		return 1;
	}

	*register_value = (uint32_t)round(50000.0/kHz_clock_rate - 1);

	return 0;
}

CM35I2CLIB_API
int CM35I2C_I2C3300_Clock_From_Register_Value(uint32_t register_value, uint32_t * kHz_clock_rate)
{
	if(register_value < 1)
	{
		return 1;
	}

	*kHz_clock_rate = (uint32_t)50000/(register_value+1);
	
	return 0;
}


// takes an array started with an address, and loads the values into the fifo
int CM35I2C_I2C3300_Fifo_Write_Bytes(struct CM35I2C_Board_Descriptor *handle,
                                        const struct CM35I2C_Function_Block *func_block,
                                        uint32_t *bytes, size_t bytes_length)
{
	uint32_t value;
	uint32_t i;
	uint8_t write_count_out;
	int result;
	
	// Check Parameters for invalid cases
	if(	handle == NULL || 
		bytes == NULL ||
		func_block == NULL || 
		bytes_length == 0 || 
		bytes_length > CM35I2C_MAX_WRITE_FIFO_SIZE)
	{
		return 1;
	}

	result = CM35I2C_I2C3300_Check_Ready(handle,func_block);
	CM35I2C_I2C3300_Get_Fifo_Write_Count(handle,func_block,&write_count_out);

	if(result)
	{
		return 2; 
	}

	// Check the write fifo to make sure it isn't or won't be full
	if(write_count_out + bytes_length > CM35I2C_MAX_WRITE_FIFO_SIZE)
	{
		return 3;
	}

	for(i=0; i<bytes_length; i++)
	{
		value = bytes[i];
		CM35I2C_I2C3300_Fifo_Write(handle,func_block,value);
	}

	return 0;
}

CM35I2CLIB_API
int CM35I2C_I2C3300_Bus_Control_Reset(struct CM35I2C_Board_Descriptor *handle, 
											const struct CM35I2C_Function_Block *func_block)
{
	uint8_t bus_control_register;
	int attempt_number;

	// Write the reset command to the register -- this write a 1 to the bus reset bit to start
	// the sequence. if the reset sequence is started, it will read back as a zero.
	// if the readback is ever set back to 1 the reset was successful.
	CM35I2C_I2C3300_Bus_Control_Write(handle,func_block,CM35I2C_I2C3300_BUS_RESET_BITMASK);	 
	
	for(attempt_number = 0; attempt_number < CM35I2C_MAX_CHECK_READY_ATTEMPTS; attempt_number++)
	{
		CM35I2C_I2C3300_Bus_Control_Read(handle,func_block,&bus_control_register);

		if((bus_control_register & CM35I2C_I2C3300_BUS_RESET_BITMASK))
		{
			// The function block is not reseting anymore
			return 0;
		}
	}
	// The bus control bit never comes out of reset
	return 1;
	
}

CM35I2CLIB_API
int CM35I2C_I2C3300_Check_Ready(struct CM35I2C_Board_Descriptor *handle, 
									const struct CM35I2C_Function_Block *func_block)
{
	int attempt_number;
	uint8_t isReady = 0;

	for(attempt_number = 0; attempt_number < CM35I2C_MAX_CHECK_READY_ATTEMPTS; attempt_number++)
	{
		CM35I2C_I2C3300_Bus_Control_Read(handle,func_block,&isReady);

		if((isReady & CM35I2C_I2C3300_GO_BUSY_BITMASK) == 0x01)
		{
			// The function block is not busy and is ready
			return EXIT_SUCCESS;
		}
		CM35I2C_Micro_Sleep(CM35I2C_RESET_DELAY_MILLI_SEC);
	}
	// The Board is still busy
	return 1;
}

CM35I2CLIB_API
// takes an array started with an address, and loads the values into the fifo
int CM35I2C_I2C3300_Fifo_Read_All(struct CM35I2C_Board_Descriptor *handle,
                                        const struct CM35I2C_Function_Block *func_block,
                                        uint8_t *bytes, size_t bytes_length)
{
	int i, result;
	uint8_t read_fifo_count_out;
	uint8_t fifo_out=0;

	if(handle == NULL || func_block == NULL)
	{
		return 1;
	}

	// Check for a read count
	result = CM35I2C_I2C3300_Get_Fifo_Read_Count(handle,func_block,&read_fifo_count_out);
	if(result)
	{		
		return 1;
	}	

	if(read_fifo_count_out > bytes_length || read_fifo_count_out == 0)
	{
		return 2;
	}

	for(i=0; i<read_fifo_count_out; i++)
	{
		
		result = CM35I2C_I2C3300_Fifo_Read(handle,func_block,&fifo_out);
		bytes[i] = fifo_out;

	}

	return 0;
}


int CM35I2C_I2C3300_Write_Execute_Bytes(
							struct CM35I2C_Board_Descriptor *handle,
                            const struct CM35I2C_Function_Block *func_block,
                            uint32_t *bytes, size_t bytes_length)
{
	int result;

	// Verify Valid Inputs
	if(handle == NULL || func_block == NULL || bytes == NULL || bytes_length == 0)
	{
		return 1;
	}

	// Write Bytes to the Fifo -- this checks to see if the I2C port is busy
	result = CM35I2C_I2C3300_Fifo_Write_Bytes(handle, func_block, bytes,
					bytes_length);
	if(result != EXIT_SUCCESS)
	{
		return 2;
	}

	// Send the execute command
	result = CM35I2C_I2C3300_Bus_Control_Write(handle,func_block,
					CM35I2C_I2C3300_GO_BUSY_BITMASK);
	if(result != EXIT_SUCCESS)
	{
		return 3;
	}

	// Check that the board is ready again to verify that the sending is complete.
	result = CM35I2C_I2C3300_Check_Ready(handle,func_block);
	// 1 indicates board is ready not zero
	if(result)
	{
		return 4;
	}

	// Message was sent successfully and is ready for a new message
	return EXIT_SUCCESS;
}
