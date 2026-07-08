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

#include "dm35mrm.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "cm35i2c_i2c3300.h"

#define GET_REGISTERS_READBACK_MESSAGE_SIZE 17
#define GET_VERSION_READBACK_MESSAGE_SIZE 4
#define BOARDNAME_SIZE 7

// Static function Prototypes -- not to be used outside of this file's scope.
static int DM35MRM_Set_Relay(dm35mrm_info_t* dm35mrm_information, int board, int row, int col, uint8_t value);


/**
 *  @brief 
 * 		Grabs all of the information from a specified microcontroller by sending i2c messages to the specified address.
 *  @param dm35mrm_information
 * 	 	used for holding the dm35mrm information. this struct is modified by the function
 *  @return 0 exit success
 *  
 *  @return 1 exit failure on getting relays from this board number -- may indicate the board is vacant on the system.
 *  @return 2 exit failure on second write command
 *  @return 3 exit failure on read back
 *  @return 4 exit failure on DM35MRM string comparison
 */
CM35I2CLIB_API
int DM35MRM_Get_Registers(dm35mrm_info_t* dm35mrm_information,
                    uint8_t board_id, uint8_t mc_number,dm35mrm_mc_register_set_t *mc_registers)
{
	// construct Read request starting at register 0x00
	uint32_t ReadLocationStartMsg[3] = { 
		DM35MRM_Construct_I2C_Address(board_id,mc_number,0), 
		CMD_READ, 
		0x00
	};  
	// construct Request read length 0x10 -- 16 bytes
 	uint32_t ReadRequestMsg[2] = { 
		DM35MRM_Construct_I2C_Address(board_id,mc_number,1),
		0x10
	};

	uint8_t readback[GET_REGISTERS_READBACK_MESSAGE_SIZE];
	char test_board_name[BOARDNAME_SIZE];
	int result, i;

	// Set name to nulls
	memset(test_board_name,0,sizeof(test_board_name));
	memset(readback,0,sizeof(readback));

	result = CM35I2C_I2C3300_Write_Execute_Bytes(dm35mrm_information->i2c_board,
					dm35mrm_information->func_block, ReadLocationStartMsg, 
					sizeof(ReadLocationStartMsg)/sizeof(ReadLocationStartMsg[0]));
	if (result) 
	{
		return 1;
	}
	
	result = CM35I2C_I2C3300_Write_Execute_Bytes(dm35mrm_information->i2c_board, 
				dm35mrm_information->func_block, ReadRequestMsg, 
				sizeof(ReadRequestMsg)/sizeof(ReadRequestMsg[0]));
	if (result)
	{
		return 2;
	}

	result = CM35I2C_I2C3300_Fifo_Read_All(dm35mrm_information->i2c_board, 
					dm35mrm_information->func_block, readback, 
					sizeof(readback)/sizeof(readback[0]));
	if (result) 
	{ 
		return 3; 
	}
	
	// Copy the readback into the MC_REGISTERS
	for(i=0; i< GET_REGISTERS_READBACK_MESSAGE_SIZE; i++)
	{
		mc_registers->byte_array[i] = readback[i];
	}

	
	
	// Duplicate the mc_registers board name to a char array
	for(i=0; i< BOARDNAME_SIZE; i++)
	{
		test_board_name[i] = (char) mc_registers->board_name[i];
	}

	//Verify that the readback was a DM35MRM board
	if(strncmp( test_board_name, "DM35MRM", BOARDNAME_SIZE) == 0)
	{
		return 0; // exit success
	}
	else
	{
		// Blast the registers read back if anything as it is not a DM35MRM
		memset(mc_registers,0,sizeof(*mc_registers));
		
		return 4; //exit failure
	}
}

/**
 *  @brief 
 * 		Grabs the version information from a specified microcontroller by sending i2c messages to the specified address.
 *  @param dm35mrm_information
 * 	 	used for holding the dm35mrm information. this struct is modified by the function
 *  @return 0 exit success
 *  
 *  @return 1 exit failure on getting relays from this board number -- may indicate the board is vacant on the system.
 *  @return 2 exit failure on second write command
 *  @return 3 exit failure on read back
 *  @return 4 exit failure on DM35MRM string comparison
 */
CM35I2CLIB_API
int DM35MRM_Get_Version(dm35mrm_info_t* dm35mrm_information,
                    uint8_t board_id, uint8_t mc_number, uint32_t* version)
{
	// construct Read request starting at register 0x00
	uint32_t ReadLocationStartMsg[3] = { 
		DM35MRM_Construct_I2C_Address(board_id,mc_number,0), 
		CMD_READ, 
		0x60
	};  
	// construct Request read length 0x10 -- 16 bytes
 	uint32_t ReadRequestMsg[2] = { 
		DM35MRM_Construct_I2C_Address(board_id,mc_number,1),
		0x4
	};

	uint8_t readback[GET_VERSION_READBACK_MESSAGE_SIZE];
	int result, i;

	memset(readback,0,sizeof(readback));

	result = CM35I2C_I2C3300_Write_Execute_Bytes(dm35mrm_information->i2c_board,
					dm35mrm_information->func_block, ReadLocationStartMsg, 
					sizeof(ReadLocationStartMsg)/sizeof(ReadLocationStartMsg[0]));
	if (result) 
	{
		return 1;
	}
	
	result = CM35I2C_I2C3300_Write_Execute_Bytes(dm35mrm_information->i2c_board, 
				dm35mrm_information->func_block, ReadRequestMsg, 
				sizeof(ReadRequestMsg)/sizeof(ReadRequestMsg[0]));
	if (result)
	{
		return 2;
	}

	result = CM35I2C_I2C3300_Fifo_Read_All(dm35mrm_information->i2c_board, 
					dm35mrm_information->func_block, readback, 
					sizeof(readback)/sizeof(readback[0]));
	if (result) 
	{ 
		return 3; 
	}
	
	// Copy the readback into the MC_REGISTERS
	for(i=0; i < GET_VERSION_READBACK_MESSAGE_SIZE; i++)
	{
		version[i] = readback[i];
	}

	return 0;
}


/**
 *  @brief 
 * 		Grabs all of the relays from the physical registers sets in all microcontrollers from a specified board
 * 		and fills the active relay information for it.
 *  @param dm35mrm_information
 * 	 	used for holding the dm35mrm information. this struct is modified by the function
 *  @return 0 exit success
 *  
 *  @return 1 exit failure on getting relays from this board number -- may indicate the board is vacant on the system.
 */
CM35I2CLIB_API
int DM35MRM_Get_Board_Relays(dm35mrm_info_t* dm35mrm_information, uint8_t board_id)
{
	dm35mrm_mc_register_set_t mc_registers[4];
	int result, i;

	memset(mc_registers,0,sizeof(mc_registers));
	
	for(i=0; i<MAX_MICRO_CONTROLLER; i++)
	{
		// number or'd into the 2nd and 3rd bits
		result = DM35MRM_Get_Registers(dm35mrm_information,board_id, i, &mc_registers[i]);
		// if this errors out in any way, leave this function not having changed the dm35mrm_board_relays_active array
		//printf("Get Board Relays id:%d Result:%d\r\n",board_id,result);
		if(result) return 1;
	}

	// Fill the dm35mrm_information active information for the given board with thesmc_register set
	DM35MRM_MC_Register_Ports_To_Active_Relay_Array(board_id, mc_registers, dm35mrm_information);

	return 0;
}


/**
 *  @brief This function fills the dm35mrm_information with all of the relevant fields
 *  @param dm35mrm_information
 * 	 	used for holding the dm35mrm information.
 *  @return 0 exit success -- doesn't indicate what if found check the 
 * 		information in the structs for this. could indicate no DM35MRMs found
 */
CM35I2CLIB_API
int DM35MRM_Get_All_Relays(dm35mrm_info_t* dm35mrm_information)
{
	uint8_t i;
	int result;
	
	// Reset the relay information to be refreshed
	memset(dm35mrm_information->dm35mrm_available_row_closings,MAX_ROW_CLOSINGS,
	    sizeof(dm35mrm_information->dm35mrm_available_row_closings)
	);
	memset(dm35mrm_information->dm35mrm_available_col_closings,MAX_COL_CLOSINGS,
		sizeof(dm35mrm_information->dm35mrm_available_col_closings)
	);
	memset(dm35mrm_information->dm35mrm_board_relays_active,RELAY_IGNORE,
		sizeof(dm35mrm_information->dm35mrm_board_relays_active)
	);
	memset(dm35mrm_information->dm35mrm_board_relays_virtual,
		RELAY_IGNORE,sizeof(dm35mrm_information->dm35mrm_board_relays_virtual)
	);
	dm35mrm_information->dm35mrm_board_mask = 0x00000000;
	
	// This function will fill the board mask. Each bit represents a single
	// DM35MRM's existance. Get the relays and populate the active relays.
	for(i=1; i<MAX_BOARD; i++)
	{
		result = DM35MRM_Get_Board_Relays(dm35mrm_information,i);
		if(!result)
		{
			dm35mrm_information->dm35mrm_board_mask |= (1<<i);
		}
	}
	
	// Populate the Virtual closing information from the active set -- if this returns non-zero
	// the curent configuration of the DM35MRM isn't allowed for your mode and set it to mode_invalid
	if(DM35MRM_Get_Virtual_Closing_Information(dm35mrm_information))
	{
		printf("DM35MRM_Initialize: WARN: Physical Relay State does not conform to the Loop Blocking Protection Mode cannot change relay states\r\n");
		dm35mrm_information->blocking_mode = mode_invalid;
	}
	return 0;
}

/**
 *  @brief prints all of the relays for the specified version
 *  @param dm35mrm_information
 * 	 	used for holding the dm35mrm information.
 *  @return 0 exit success
 */
CM35I2CLIB_API
int DM35MRM_Print_Relays(dm35mrm_info_t* dm35mrm_information, uint8_t board_id, uint8_t active_not_virtual)
{
	int i,j;
	for(i=0;i<MAX_ROW_NUMBER;i++)
	{	
		for(j=0; j<MAX_LOCAL_COL_NUMBER; j++)
		{
			// decide which information gets printed
			if(active_not_virtual ==1)
				printf("%x ", dm35mrm_information->dm35mrm_board_relays_active[board_id][i][j]);
			else
				printf("%x ", dm35mrm_information->dm35mrm_board_relays_virtual[board_id][i][j]);
		}
		printf("\r\n");
	}

	return 0;
}

/**
 * @brief prints all of the relay information for a quick debugging information
 * @param dm35mrm_information
 * 	used for holding the dm35mrm information.
 * @return 0 exit success
 */
CM35I2CLIB_API
int DM35MRM_Print_Everything(dm35mrm_info_t* dm35mrm_information)
{
	uint8_t i;

	// The board Mask for the DM35MRM
	printf("DM35MRM Board Mask: 0x%08x\r\n",dm35mrm_information->dm35mrm_board_mask);

	// Print out all of the board information to make sure that it has 
	//been retrieved right
	printf("Active\r\n");
	for(i=1; i<MAX_BOARD; i++)
	{
		if(!(dm35mrm_information->dm35mrm_board_mask & 1<<i))
			continue;
		printf("BOARD[%i]\r\n",i);
		DM35MRM_Print_Relays(dm35mrm_information, i, 1);
	}	
	printf("\r\nVirtual\r\n");
	for(i=1; i<MAX_BOARD; i++)
	{
		if(!(dm35mrm_information->dm35mrm_board_mask & 1<<i))
			continue;
		printf("BOARD[%i]\r\n",i);
		DM35MRM_Print_Relays(dm35mrm_information, i, 0);
	}	

	#ifdef DEBUG_COMPILE
	printf("Row available closings: \r\n");
	for(i=0; i<MAX_ROW_NUMBER; i++)
		printf("%d[%d] ",i,dm35mrm_information->dm35mrm_available_row_closings[i]);


	printf("\r\nCol Available CLosings: \r\n");
	for(i=0; i<MAX_BOARD; i++)
	{
		for(j=0;j<MAX_LOCAL_COL_NUMBER;j++)
		{
			printf("%d:%d[%d] ",i,j,dm35mrm_information->dm35mrm_available_col_closings[i][j]);
		}
		printf("\r\n");
	}

	#endif
	return 0;
}

/**
 * @brief
 * 		This function will setup the board handling descriptions for the I2C board and connector
 * 		for your board, and then pulls all of the relay information for use.
 * 
 * @param	dm35mrm_information
 *
 *   	The dm35mrm information struct that has been initialized to a CM35I2C
 *		board, with a I2C3300 Function block found.
 * 		
 * @retval
 * 		0 Exit Success
 * 
 * @retval
 * 		1 Exit Failure  -- couldn't get the relay state
 * 
 */
CM35I2CLIB_API
int DM35MRM_Initialize(dm35mrm_info_t* dm35mrm_information, struct CM35I2C_Board_Descriptor *board_descriptor_handle,
                    struct CM35I2C_Function_Block *fb_handle, blocking_mode_t mode_select)
{
	int result;

	// Initialize the board tracking information to be tracked by the
	// dm35mrm_info_t struct 
	dm35mrm_information->i2c_board = board_descriptor_handle;
	dm35mrm_information->func_block = fb_handle;
	dm35mrm_information->blocking_mode = mode_select;
	
	// Populate the Relays information into the dm35mrm_board_mask 
	// and dm35mrm_board_relays_active
	result = DM35MRM_Get_All_Relays(dm35mrm_information);
	if(result) return 1;

	return 0;
}


/**
 * @brief
 * 		This function will copy the physical relay boards and populate the closed relays
 * 		row counts and column counts. This is done to keep there from being two rows connected
 * 		and no more than 2 columns connected to a single row. 
 * 
 * @param	dm35mrm_information
 *
 *   	The dm35mrm information struct that has been initialized to a CM35I2C
 *		board, with a I2C3300 Function block found.
 * 		
 * @retval
 * 		0 Exit Success
 * 
 * @retval
 * 		1 Exit Failure -- Blocking algorithm would have prevented this physical layout
 * 
 */
CM35I2CLIB_API
int DM35MRM_Get_Virtual_Closing_Information(dm35mrm_info_t* dm35mrm_information)
{
	uint8_t board_id,row,col;
	int results;

	for(board_id=1;board_id<MAX_BOARD;board_id++) // Board Numbers Start with 1 not Zero
	{
		// Skip board number if it's not been masked in
		if(!(dm35mrm_information->dm35mrm_board_mask & 1<<board_id))
			continue;

		// this sets the boards virtual information to all 
		// zeros before looking for closings.
		memset(dm35mrm_information->dm35mrm_board_relays_virtual[board_id],0,
			sizeof(dm35mrm_information->dm35mrm_board_relays_virtual[board_id]));

		for(row=0;row<MAX_ROW_NUMBER;row++)
		for(col=0;col<MAX_LOCAL_COL_NUMBER;col++)
		{
			uint8_t test_relay;
			test_relay = dm35mrm_information->dm35mrm_board_relays_active[board_id][row][col];

			// Make sure to test agaisnt RELAY_CLOSE... RELAY_IGNORE is also a non-zero return
			if(test_relay == RELAY_CLOSE)
			{
				results = DM35MRM_Try_Close_Relay(dm35mrm_information,board_id, row, col);
				if(results)
				{
					return 1; // The blocking algorithm would have forbidden this physical configuration.
				}
			}
		}
	}
	return 0;
}

/**
 * @brief
 * 		Tries to close a virtual relay making sure that it follows the safety rules 
 * 
 * @param	dm35mrm_information
 *
 *   	The dm35mrm information struct that has been initialized to a CM35I2C
 *		board, with a I2C3300 Function block found.
 *
 * @param board
 * 		this is the board number that you wish to change the relay on determiend by the jumpers
 * 		set on the boards
 * 
 * @param  row
 * 		this is the desired relay row number
 * 
 * @param col
 * 		this is the desired relay column number
 * 		
 * @retval
 * 		0 Exit Success
 * 
 * @retval
 * 		1 -- Target Relay Board was not Detected at initialization
 * 		2 -- Target Relay could not be set
 * 		3 -- Invalid Configuration
 * 
 */
CM35I2CLIB_API
int DM35MRM_Try_Close_Relay(dm35mrm_info_t* dm35mrm_information, int board, int row, int col)
{
	int result;
	// Check if the board was detected on initialization
	if(!(dm35mrm_information->dm35mrm_board_mask & 1<<board)) {
		return 1;
	}

	switch(dm35mrm_information->blocking_mode) {
		case mode_loop_blocking_protection:
			// Rules for loop blocking mode:
			// 1: No more than 2 connections per row.
			// 2: No more than 1 connection per column.
			// Check availability by row and column	
			if(dm35mrm_information->dm35mrm_available_row_closings[row]>0 && 
				dm35mrm_information->dm35mrm_available_col_closings[board][col]>0)
			{
				// Update the blocking information if the DM35MRM only if the Relay setting is
				// Successful
				result = DM35MRM_Set_Relay(dm35mrm_information,board,row,col,RELAY_CLOSE);
				if(result == EXIT_SUCCESS)
				{
					dm35mrm_information->dm35mrm_available_row_closings[row]--;
					dm35mrm_information->dm35mrm_available_col_closings[board][col]--;
					return 0;
				}
			}
			// The closing relay algorithm is not allowed or wrong.
			return 2;

		case mode_no_blocking:
			// Rules for no blocking: None
			result = DM35MRM_Set_Relay(dm35mrm_information,board,row,col,RELAY_CLOSE);
			if(result)
				return 2;
			else
				return 0;

		case mode_invalid:
		default:
			// Any Mode that isn't specified is an error
			return 3;
	}
	// Getting here shouldn't be possible
	return 3;
}



/**
 * @brief
* 		Tries to open a virtual relay making sure that it follows the safety rules
 * 
 * @param	dm35mrm_information
 *
 *   	The dm35mrm information struct that has been initialized to a CM35I2C
 *		board, with a I2C3300 Function block found.
 *
 * @param board
 * 		this is the board number that you wish to change the relay on determiend by the jumpers
 * 		set on the boards
 * 
 * @param  row
 * 		this is the desired relay row number
 * 
 * @param col
 * 		this is the desired relay column number
 * 		
 * @retval
 * 		0 Exit Success
 * 
 * @retval
 * 		1 -- Selected board wasn't detected at initialization
 * 		2 -- Closing algorithm denied the Relay Change
 * 		3 -- Configuration error
 * 
 */
CM35I2CLIB_API
int DM35MRM_Try_Open_Relay(dm35mrm_info_t* dm35mrm_information, int board, int row, int col)
{
	int result=0;
	// Check if the board was detected on initialization
	if(!(dm35mrm_information->dm35mrm_board_mask & 1<<board)) {
		return 1;
	}

	switch(dm35mrm_information->blocking_mode)
	{
		case mode_loop_blocking_protection:
			// Rules for a loop blocking mode connection
			// 1: No more than 2 connections per row.
			// 2: No more than 1 connection per column.
			// Check availability by row and column	
			if(dm35mrm_information->dm35mrm_available_row_closings[row]<MAX_ROW_CLOSINGS && 
				dm35mrm_information->dm35mrm_available_col_closings[board][col]<MAX_COL_CLOSINGS)
			{
				// Update the blocking information if the DM35MRM only if the Relay setting is
				// Successful
				result = DM35MRM_Set_Relay(dm35mrm_information,board,row,col,RELAY_OPEN);
				if(result == EXIT_SUCCESS)
				{
					dm35mrm_information->dm35mrm_available_row_closings[row]++;
					dm35mrm_information->dm35mrm_available_col_closings[board][col]++;
					return 0;
				}
			}
			// The opening relay is not allowed or unexpected
			return 2;
		
		case mode_no_blocking:
			// Rules for no_blocking: None
			result = DM35MRM_Set_Relay(dm35mrm_information,board,row,col,RELAY_OPEN);
			if(result)
				return 2;
			else
				return 0;

		case mode_invalid:
		default:
			// Any Mode that isn't specified is an error
			return 3;
	}

	// Code shouldn't get here
	return 3;
}


/**
 * @brief
 * 		This function changes the value of a given relay to a new value
 * 
 * @param
 *   	relays_info
 *
 *   	The dm35mrm information struct that has been initialized to a CM35I2C
 *		board, with a I2C3300 Function block found.
 * 		
 * @param
 * 		new_value 
 *      
 *		The value that we want to set the speicified relay to
 *
 *
 * @retval
 * 		0 Exit Success
 * 
 * @retval
 * 		1 Exit Failure -- writing to a relay that is already the new_value or is marked with INVALID relay
 *			will fail.
 * 
 */
static int DM35MRM_Set_Relay(dm35mrm_info_t* dm35mrm_information, int board, int row, int col, uint8_t new_value)
{

	uint8_t existing_value = dm35mrm_information->dm35mrm_board_relays_virtual[board][row][col];
	// Do not change the value of an INVALID RELAY or if the New state will be the same as the old
	if(existing_value != RELAY_INVALID && existing_value != new_value)
	{
		dm35mrm_information->dm35mrm_board_relays_virtual[board][row][col] = new_value;
		return 0;
	}
	else
		return 1;
}

/**
 * @brief
 * 		This function is called to write all of the changes to the physical devices. effectively it creates the smallest
 * 		reasonable number of messages to write all of the changes to the board 
 * 
 * 
 * @param
 *   	relays_info
 *
 *   	The dm35mrm information struct that has been initialized to a CM35I2C
 *		board, with a I2C3300 Function block found.
 * 		
 * @retval
 * 		0 Exit Success
 * 
 * @retval
 * 		1 Exit Failure -- writing a register change returned a failure and board is in 
 * 		unknown state
 * 
 */
CM35I2CLIB_API
int DM35MRM_Registers_Write(dm35mrm_info_t* relay_info)
{
	// Relay Information
	uint8_t bid,mc,lo_row,hi_row,col;
	uint16_t test_word_lo, test_word_hi;
	dm35mrm_mc_register_set_t mc_register_sets[MAX_MICRO_CONTROLLER];
	uint8_t mc_modified;
	int result;

	memset(mc_register_sets,0,sizeof(mc_register_sets));

	for(bid=1; bid<MAX_BOARD;bid++)
	{
		//  check Board ID Mask for quick exit
		if(!(relay_info->dm35mrm_board_mask & 1<<bid))
		{
			continue;
		}
		// Clear the modified flags before the microcontroller loop
		mc_modified = 0;

		// Each Microcontroller
		for(mc=0;mc<MAX_MICRO_CONTROLLER;mc++)
		{
			// Set the row information
			lo_row = mc*2;
			hi_row = mc*2 + 1;

			for(col=0;col<MAX_LOCAL_COL_NUMBER; col++)
			{
				//Check the bits for each of the relays to see if they are different
				test_word_lo = (relay_info->dm35mrm_board_relays_virtual[bid][lo_row][col] == RELAY_CLOSE) ^ (relay_info->dm35mrm_board_relays_active[bid][lo_row][col] == RELAY_CLOSE);
				test_word_hi = (relay_info->dm35mrm_board_relays_virtual[bid][hi_row][col] == RELAY_CLOSE) ^ (relay_info->dm35mrm_board_relays_active[bid][hi_row][col] == RELAY_CLOSE);
			
				// if either the low row or the high row of the currently tested microcontroller 
				// is modified: flag the microcontroller as modified and start with the next MC don't waste time finding all differences
				if(test_word_lo || test_word_hi)
				{
					mc_modified |= 1<<mc;
					break;
				}
			}
		}

		// If nothing was modified for this board Skip board
		if(!mc_modified)
		{
			continue;
		}

		DM35MRM_Virtual_Relay_Array_To_Register_Ports(bid,mc_register_sets,relay_info);

		for(mc=0; mc<MAX_MICRO_CONTROLLER; mc++)
		{
			if(mc_modified & 1<<mc)
			{
				dm35mrm_write_ports_msg_t msg;
				msg.address = DM35MRM_Construct_I2C_Address(bid, mc, 0);
				msg.cmd = 0;
				msg.porta = mc_register_sets[mc].port_a;
				msg.portf = mc_register_sets[mc].port_f;
				msg.portd = mc_register_sets[mc].port_d;
				msg.portc = mc_register_sets[mc].port_c;

				result = CM35I2C_I2C3300_Write_Execute_Bytes(relay_info->i2c_board,relay_info->func_block,msg.data,sizeof(msg.data)/sizeof(msg.data[0]));
				if(result)
					return 1;
			}
		}

	}

	// If it got here the physical and shadow relays should match make it so
	memcpy(relay_info->dm35mrm_board_relays_active, relay_info->dm35mrm_board_relays_virtual, sizeof(relay_info->dm35mrm_board_relays_active));

	return 0;
}

/**
 * @brief
 * 		This is a helper function that is designed to pull the information from an MC_register set of size 4 into
 * 		A relay information structs physical information [board][row][column] information.
 * 
 * @param
 * 		board_id
 * 		the number of the board_id jumpers on the DM35MRM.
 * @param
 * 		mc_registers
 * 		sets up the microcontroller 
 * 
 * @param
 *   	relays_info
 *
 *   	The dm35mrm information struct that has been initialized to a CM35I2C
 *		board, with a I2C3300 Function block found.
 * 		
 * @retval
 * 		0 Exit Success
 * 
 */
CM35I2CLIB_API
int DM35MRM_MC_Register_Ports_To_Active_Relay_Array(uint8_t board_id, dm35mrm_mc_register_set_t* mc_registers, dm35mrm_info_t* relays_info)
{
	uint8_t i;

	// Bit hacking to shift mc port address to board row column info
	// Max number of local columns divided by 2 because it fills out two indexes
	// Per Loop, the even and odd bits of the ports filled into A/F/D/C
	for(i=0; i<(MAX_LOCAL_COL_NUMBER/2); i++)
	{
		
		relays_info->dm35mrm_board_relays_active[board_id][0][i]   = (mc_registers[0].port_a & (1 << i))? 1 : 0;
		relays_info->dm35mrm_board_relays_active[board_id][0][i+8] = (mc_registers[0].port_f & (1 << i))? 1 : 0;
		relays_info->dm35mrm_board_relays_active[board_id][1][i]   = (mc_registers[0].port_d & (1 << i))? 1 : 0;
		relays_info->dm35mrm_board_relays_active[board_id][1][i+8] = (mc_registers[0].port_c & (1 << i))? 1 : 0;
		
		relays_info->dm35mrm_board_relays_active[board_id][2][i]   = (mc_registers[1].port_a & (1 << i))? 1 : 0;
		relays_info->dm35mrm_board_relays_active[board_id][2][i+8] = (mc_registers[1].port_f & (1 << i))? 1 : 0;
		relays_info->dm35mrm_board_relays_active[board_id][3][i]   = (mc_registers[1].port_d & (1 << i))? 1 : 0;
		relays_info->dm35mrm_board_relays_active[board_id][3][i+8] = (mc_registers[1].port_c & (1 << i))? 1 : 0;

		relays_info->dm35mrm_board_relays_active[board_id][4][i]   = (mc_registers[2].port_a & (1 << i))? 1 : 0;
		relays_info->dm35mrm_board_relays_active[board_id][4][i+8] = (mc_registers[2].port_f & (1 << i))? 1 : 0;
		relays_info->dm35mrm_board_relays_active[board_id][5][i]   = (mc_registers[2].port_d & (1 << i))? 1 : 0;
		relays_info->dm35mrm_board_relays_active[board_id][5][i+8] = (mc_registers[2].port_c & (1 << i))? 1 : 0;

		relays_info->dm35mrm_board_relays_active[board_id][6][i]   = (mc_registers[3].port_a & (1 << i))? 1 : 0;
		relays_info->dm35mrm_board_relays_active[board_id][6][i+8] = (mc_registers[3].port_f & (1 << i))? 1 : 0;
		relays_info->dm35mrm_board_relays_active[board_id][7][i]   = (mc_registers[3].port_d & (1 << i))? 1 : 0;
		relays_info->dm35mrm_board_relays_active[board_id][7][i+8] = (mc_registers[3].port_c & (1 << i))? 1 : 0;
	}

	return 0;
}

/**
 * @brief
 * 		This is a helper function that is designed to pull the information from an virtual relay [board][row][column] 
 * 		into a size 4 set of MC register structs for message writing.
 * 
 * @param
 * 		board_id
 * 		the number of the board_id jumpers on the DM35MRM.
 * @param
 * 		mc_registers
 * 		sets up the microcontroller 
 * 
 * @param
 *   	relays_info
 *
 *   	The dm35mrm information struct that has been initialized to a CM35I2C
 *		board, with a I2C3300 Function block found.
 * 		
 * @retval
 * 		0 Exit Success
 * 
 */
CM35I2CLIB_API
int DM35MRM_Virtual_Relay_Array_To_Register_Ports(uint8_t board_id, dm35mrm_mc_register_set_t* mc_registers, dm35mrm_info_t* relays_info)
{
	uint8_t i;

	for(i=0; i<(MAX_LOCAL_COL_NUMBER/2); i++)
	{
		// Must check against RELAY_CLOSE to make sure RELAY_IGNORE doesn't flag as a set
		mc_registers[0].port_a |=  (relays_info->dm35mrm_board_relays_virtual[board_id][0][i]==RELAY_CLOSE)   ? (1<<i) : 0; //row 0 low byte
		mc_registers[0].port_f |=  (relays_info->dm35mrm_board_relays_virtual[board_id][0][i+8]==RELAY_CLOSE) ? (1<<i) : 0; //row 0 high byte
		mc_registers[0].port_d |=  (relays_info->dm35mrm_board_relays_virtual[board_id][1][i]==RELAY_CLOSE)   ? (1<<i) : 0; //row 1 low byte
		mc_registers[0].port_c |=  (relays_info->dm35mrm_board_relays_virtual[board_id][1][i+8]==RELAY_CLOSE) ? (1<<i) : 0; //row 1 hight byte

		mc_registers[1].port_a |=  (relays_info->dm35mrm_board_relays_virtual[board_id][2][i]==RELAY_CLOSE)   ? (1<<i) : 0; //row 2 low byte
		mc_registers[1].port_f |=  (relays_info->dm35mrm_board_relays_virtual[board_id][2][i+8]==RELAY_CLOSE) ? (1<<i) : 0; //row 2 high byte
		mc_registers[1].port_d |=  (relays_info->dm35mrm_board_relays_virtual[board_id][3][i]==RELAY_CLOSE)   ? (1<<i) : 0; //row 3 low byte
		mc_registers[1].port_c |=  (relays_info->dm35mrm_board_relays_virtual[board_id][3][i+8]==RELAY_CLOSE) ? (1<<i) : 0; //row 3 high byte

		mc_registers[2].port_a |=  (relays_info->dm35mrm_board_relays_virtual[board_id][4][i]==RELAY_CLOSE)   ? (1<<i) : 0; //row 4 low byte
		mc_registers[2].port_f |=  (relays_info->dm35mrm_board_relays_virtual[board_id][4][i+8]==RELAY_CLOSE) ? (1<<i) : 0; //row 4 high byte
		mc_registers[2].port_d |=  (relays_info->dm35mrm_board_relays_virtual[board_id][5][i]==RELAY_CLOSE)   ? (1<<i) : 0; //row 5 low byte
		mc_registers[2].port_c |=  (relays_info->dm35mrm_board_relays_virtual[board_id][5][i+8]==RELAY_CLOSE) ? (1<<i) : 0; //row 5 high byte

		mc_registers[3].port_a |=  (relays_info->dm35mrm_board_relays_virtual[board_id][6][i]==RELAY_CLOSE)   ? (1<<i) : 0; //row 6 low byte
		mc_registers[3].port_f |=  (relays_info->dm35mrm_board_relays_virtual[board_id][6][i+8]==RELAY_CLOSE) ? (1<<i) : 0; //row 6 high byte
		mc_registers[3].port_d |=  (relays_info->dm35mrm_board_relays_virtual[board_id][7][i]==RELAY_CLOSE)   ? (1<<i) : 0; //row 7 low byte
		mc_registers[3].port_c |=  (relays_info->dm35mrm_board_relays_virtual[board_id][7][i+8]==RELAY_CLOSE) ? (1<<i) : 0; //row 7 high byte
	}


	return 0;
}

/**
 *
 * @brief
 *
 *	dm35mrm construct i2c address from a row and microcontroller Read and Write
 *	
 * @param board_id
 * 	the value of the id jumpers on the target board
 * 
 * @param mc_num
 *  the microcontroller that you wish to send a message too
 * 
 * @param RnW
 * 	read not write bit, set if you are issuing a read command.
 *
 * @retval
 * 	the I2C address that is constructed from the byte field.
 *
 */
CM35I2CLIB_API
uint32_t DM35MRM_Construct_I2C_Address(uint8_t board_id, uint8_t mc_num,uint8_t RnW)
{
	// Construct The address using masking and bit shifting
	return (uint32_t) (0x100 | (board_id & 0x1f)<< 3 | (mc_num & 3)<< 1 | (RnW & 1));
}

/**
 *
 * @brief
 *
 *	Global Call open/clear all of the Relays 
 *  	
 * @param relays_info
 *   The dm35mrm information struct that has been initialized to a CM35I2C
 *	board, with a I2C3300 Function block found.
 *
 * @retval
 *   0
 *
 *   Success.
 *
 * @retval
 *   1 
 *
 *   Failure Writing Bytes
 *
 */
CM35I2CLIB_API
int DM35MRM_GC_Clear_Relays(dm35mrm_info_t* relays_info)
{
	
	// Construct message to send
	// Address zero is general call and board 0 mc 0 -- the command makes the difference.
	uint32_t msg[] = { 0x100, 0xFF, 0x00 };
	int result = 0;

	result = CM35I2C_I2C3300_Write_Execute_Bytes(relays_info->i2c_board, relays_info->func_block, msg, sizeof(msg)/sizeof(msg[0]));
	if(result) return 1;

	// Do not Naively Believe global Call -- check the relays again.
	// Reset the Relay Tracking information and refetch them with d
	DM35MRM_Get_All_Relays(relays_info);

	return 0;
}

/**
 * @brief
 * 
 * 	Global Call Reset Relay Microcontrollers
 * 	
 * @param
 *     relays_info
 * 
 *     The dm35mrm information struct that has been initialized to a CM35I2C
 * 	board, with a I2C3300 Function block found.
 * 
 * @retval
 *     0
 * 
 *     Success.
 * 
 * @retval
 *     1 
 * 
 *     Failure Writing
 *
 */
CM35I2CLIB_API
int DM35MRM_GC_Reset_MC(dm35mrm_info_t* relays_info)
{
	// Construct message to send
	// Address zero is general call and board 0 mc 0 -- the command makes the difference.
	uint32_t msg[] = { 0x100, 0xFD, 0x00 };
	int result = 0;

	result = CM35I2C_I2C3300_Write_Execute_Bytes(relays_info->i2c_board, relays_info->func_block, msg, sizeof(msg)/sizeof(msg[0]));
	if(result) 
	{
		return 1;
	}
	
	// Do not Naively Believe global Call -- check the relays again.
	// Reset the Relay Tracking information and refetch them with d
	DM35MRM_Get_All_Relays(relays_info);

	return 0;
}

/**
 * @brief
 * 
 * 	Global Call Manual Latch Push -- this will push the register changes when the auto
 * 	matic latch is disabled.
 * 	
 * @param
 *     relays_info
 * 
 *     The dm35mrm information struct that has been initialized to a CM35I2C
 * 	board, with a I2C3300 Function block found.
 * 
 * @retval
 *     0
 * 
 *     Success.
 * 
 * @retval
 *     1 
 * 
 *     Failure Writing
 *
 */
CM35I2CLIB_API
int DM35MRM_GC_Latch_Push(dm35mrm_info_t* relays_info)
{
	// Construct message to send
	// Address zero is general call and board 0 mc 0 -- the command makes the difference.
	uint32_t msg[] = { 0x100, 0xFE, 0x00 };
	int result = 0;

	result = CM35I2C_I2C3300_Write_Execute_Bytes(relays_info->i2c_board, relays_info->func_block, msg, sizeof(msg)/sizeof(msg[0]));
	if(result) return 1;

	return 0;
}
/**
 * @brief
 * 
 * 	Global Call auto latch disable -- this will disable automatic push feature of the relays
 * 	writing to the registers will not automatically set the relays, until 
 * 	the global push command.
 * 	
 * @param
 *     relays_info
 * 
 *     The dm35mrm information struct that has been initialized to a CM35I2C
 * 	board, with a I2C3300 Function block found.
 * 
 * @retval
 *     0
 * 
 *     Success.
 * 
 * @retval
 *     1 
 * 
 *     Failure Writing
 */
CM35I2CLIB_API
int DM35MRM_GC_Auto_Latch_Disable(dm35mrm_info_t* relays_info)
{
	// Construct message to send
	// Address zero is general call and board 0 mc 0 -- the command makes the difference.
	uint32_t msg[] = { 0x100, 0xFC, 0x00 };
	int result = 0;

	result = CM35I2C_I2C3300_Write_Execute_Bytes(relays_info->i2c_board, relays_info->func_block, msg, sizeof(msg)/sizeof(msg[0]));
	if(result) return 1;

	result = DM35MRM_GC_Zero_Readback_Call(relays_info);
	if(result) return 2;

	return 0;
}

/**
 * @brief
 * 
 * 	Global Call auto latch enable -- this will enable automatic push feature of the 
 * 	relays. Otherwise writing to the registers will not automatically set the 
 * 	relays, until the global push command.
 * 	
 * @param
 *     relays_info
 * 
 *     The dm35mrm information struct that has been initialized to a CM35I2C
 * 	board, with a I2C3300 Function block found.
 * 
 * @retval
 *     0
 * 
 *     Success.
 * 
 * @retval
 *     1 
 *     Failure Writing
 * 
 */
CM35I2CLIB_API
int DM35MRM_GC_Auto_Latch_Enable_And_Push(dm35mrm_info_t* relays_info)
{
	// Construct message to send
	// Address zero is general call and board 0 mc 0 -- the command makes the difference.
	uint32_t msg[] = { 0x100, 0xFB, 0x00 };
	int result = 0;

	result = CM35I2C_I2C3300_Write_Execute_Bytes(relays_info->i2c_board, relays_info->func_block, msg, sizeof(msg)/sizeof(msg[0]));
	if(result) return 1;


	return 0;
}

/**
 * @brief
 *
 *	Global Call Zero the readback register the call. This is used for the reading
 *	routine. this is used to set the readback register in every relay in the stack
 *	to start reading from address zero.
 *
 * @param
 *   relays_info
 *
 *   The dm35mrm information struct that has been initialized to a CM35I2C
 *	board, with a I2C3300 Function block found.
 *
 * @retval
 *   0
 *
 *   Success.
 * 
 * @retval
 *   1 
 *
 *   Failure Writing
 * 
 */
CM35I2CLIB_API
int DM35MRM_GC_Zero_Readback_Call(dm35mrm_info_t* relays_info)
{
	// Zero out the readback address
	uint32_t msg[] = { 0x100, 0xFA, 0x00 };
	int result = 0;

	result = CM35I2C_I2C3300_Write_Execute_Bytes(relays_info->i2c_board, relays_info->func_block, msg, sizeof(msg)/sizeof(msg[0]));
	if(result) return 1;

	return 0;

}

/**
 *
 * @brief
 *   import a relay setup from a csv file in the following format:
 *	
 *	board,row,col
 *	board,row,col
 * 	board,row,col
 *	...
 *
 *	import the relay setup from the export relay array.
 *	
 *
 * @param
 *   dmInfo
 *
 *  The DM35MRM information struct that has been initialized to a CM35I2C
 *	board, with a I2C3300 Function block found.
 *
 * @param
 *   filename
 *
 *   the name of the file to open
 *
 *  @retval
 *   0
 *
 *   Success.
 *
 *  @retval
 *    1 
 *
 *   Failure bad file name
 * 
 * @retval
 *   2 
 * 
 *   Failure improper file format, or specified relay is invalid
 * 
 * @retval
 *   3
 *
 *   Failure could not close relay specified
 *
 * @retval
 *	4
 *	
 *	Failure could not write to the relays.
 *
 *
 */
CM35I2CLIB_API
int DM35MRM_Set_Relays_With_CSV_Import(dm35mrm_info_t* dmInfo, char* filename)
{
	FILE* fp;
	int ret=0;
    char* token;
	char buffer[MAX_BUFFER];
	int bid, row, col;
    
	fp=fopen(filename,"r");

    if(!fp)
    {
        // Cannot Open the specified Filename
		return 1;
    }
	
	if(DM35MRM_GC_Reset_MC(dmInfo))
	{
		// Couldn't reset the relays before loading csv 
		return 1;
	}

	// Loop through the csv file to check all relay closes
	// file shouldn't be longer than 16 lines (i isn't used beyond this)
	while(1)
	{
		// if the fgets returns NULL there wasn't a line to grab -- this is how we break without error.
		if(fgets(buffer,MAX_BUFFER,fp)==NULL)
			break;

		// Get first token -- board
		token = strtok(buffer,",");
		bid = atoi(token);
		// Get second token -- row -- note you must use NULL as the pointer
		// to pull from the previous tokenized string
		token = strtok(NULL,",");
		row = atoi(token);
		// third token -- column
		token = strtok(NULL,",");
		col = atoi(token);

		if(bid<1||bid>=MAX_BOARD||row<0||row>=MAX_ROW_NUMBER||col<0||col>=MAX_LOCAL_COL_NUMBER)
		{
			ret = 2;
			break;
		}

		// try to Close the relay that matches. 
		if(DM35MRM_Try_Close_Relay(dmInfo,bid,row,col))
		{
			ret = 3;
			break;
		}
	}

	// Verify there wasn't any previous failures
	// before attempting to write new relay information.
	if(!ret)
	{
		// write registers and return 3 if fails
		if(DM35MRM_Registers_Write(dmInfo))
			ret = 4;
	}

	// close the file
	fclose(fp);
	return ret;
}



/**
 *
 * @brief
 *   Export a relay setup from a csv file in the following format:
 *	
 *	board,row,col
 *	board,row,col
 *	board,row,col
 *	...
 *
 *	This will only export relays that are closed from the current state.
 *	
 *
 * @param
 *    dmInfo
 *
 *  The dm35mrm information struct that has been initialized to a CM35I2C
 *	board, with a I2C3300 Function block found.
 *
 * @param
 *   filename
 *
 *	name of the file to export to
 *
 * @retval
 *   0
 *
 *   Success.
 *
 * @retval
 *   1 
 *
 *   Failure Could not open file
 *
 */
CM35I2CLIB_API
int DM35MRM_Relays_CSV_Export(dm35mrm_info_t* dmInfo, char* filename)
{
	FILE* fp;
	int ret=0;
	
	int bid, row, col;
    
	fp=fopen(filename,"w");

    if(!fp)
    {
		// Cannot open a file for writing... exit
		return 1;
    }

	// for every column in every row in every board...
	for(bid=1;bid<MAX_BOARD;bid++)
	for(row=0;row<MAX_ROW_NUMBER;row++)
	for(col=0;col<MAX_LOCAL_COL_NUMBER;col++)
	{
		// If the relay is closed export it
		if(dmInfo->dm35mrm_board_relays_active[bid][row][col] == RELAY_CLOSE)
		{
			fprintf(fp,"%d,%d,%d\r\n",bid,row,col);
		}
	}

	// close the file
	fclose(fp);
	return ret;


}
