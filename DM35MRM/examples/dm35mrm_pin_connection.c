/**
    @file

    @brief
        This program demonstrates how to connect and disconnect two pins on DM35MRM boards.
		It is designed for use in scripts.

    @verbatim
    --------------------------------------------------------------------------
    This file and its contents are copyright (C) RTD Embedded Technologies,
    Inc.  All Rights Reserved.

    This software is licensed as described in the RTD End-User Software License
    Agreement.  For a copy of this agreement, refer to the file LICENSE.TXT
    (which should be included with this software) or contact RTD Embedded
    Technologies, Inc.
    --------------------------------------------------------------------------
    @endverbatim

*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <limits.h>
#include <getopt.h>
#include <string.h>

#include "cm35i2c_gbc_library.h"
#include "cm35i2c_ioctl.h"
#include "cm35i2c_examples.h"
#include "cm35i2c_util_library.h"
#include "cm35i2c_i2c3300.h"
#include "cm35i2c_registers.h"
#include "dm35mrm.h"

char* program_name;

static void usage(void){
	fprintf(stderr, "\n");
	fprintf(stderr, "NAME\n\n\t%s\n\n", program_name);
	fprintf(stderr, "USAGE\n\n\t%s [OPTIONS]\n\n", program_name);
	fprintf(stderr, "OPTIONS\n\n");
	fprintf(stderr, "\t--help\n");
	fprintf(stderr, "\t\tShow this help screen and exit.\n\n");
	fprintf(stderr, "\t--minor NUM\n");
	fprintf(stderr, "\t\tSpecify the minor number (>= 0) of the board to open.\n");
	fprintf(stderr, "\t--connector NUM\n");
    	fprintf(stderr, "\t\tThe Number of the Connector (CN6 -> --connector 6)\n");
	fprintf(stderr, "\t--pin NUM\n");
	fprintf(stderr, "\t\tSpecify one of the pins you want connected/disconnected\n");
	fprintf(stderr, "\t--connect\n");
	fprintf(stderr, "\t\tConnect the two pins specified\n");
	fprintf(stderr, "\t--disconnect\n");
	fprintf(stderr, "\t\tDisconnect the two pins specified\n");
	fprintf(stderr, "\t--clear\n");
	fprintf(stderr, "\t\tClear all pin connections by opening all relays\n");
	fprintf(stderr, "\t--print\n");
	fprintf(stderr, "\t\tPrint out the status of all relays\n");
}

// This function will attempt to create a connection between two pins, returns 0 if successful 
int connect_pins(int start_pin, int target_pin, unsigned long int minor, int connector){	
	struct CM35I2C_Board_Descriptor *i2c_board;
	struct CM35I2C_Function_Block my_func_block;
	dm35mrm_info_t relay_info;
	int result;
	int num_function_block = 0;
	int START_COL = (start_pin - 1) % MAX_LOCAL_COL_NUMBER;
	int TARGET_COL = (target_pin - 1) % MAX_LOCAL_COL_NUMBER;
	int START_BOARD = (start_pin - 1) / MAX_LOCAL_COL_NUMBER + 1;
	int TARGET_BOARD = (target_pin - 1) / MAX_LOCAL_COL_NUMBER + 1;
	int empty_row;
	int row_to_use = -1;
	
	if(connector == 3 || connector == 4)
		num_function_block = CM35I2C_I2C3300_FUNCTION_BLOCK_CN3_4;
	else if(connector == 5)
		num_function_block = CM35I2C_I2C3300_FUNCTION_BLOCK_CN5;
	else if(connector == 6)
                num_function_block = CM35I2C_I2C3300_FUNCTION_BLOCK_CN6;

	if (start_pin == target_pin){
		printf("ERROR: The pins provided must be distinct\n");
		return 1;
	}

	// open and initialize the boards
	printf("Opening CM35I2C board...\n");
	result = CM35I2C_Board_Open(minor, &i2c_board);
	
	CM35I2C_Check_Result(result, "Could not open board");

	printf("Opening CM35I2C function block...\n");
	result = CM35I2C_Function_Block_Open(i2c_board, num_function_block, &my_func_block);
	CM35I2C_Check_Result(result, "Could not open fb");

	printf("Initializing DM35MRM...\n");
	result = DM35MRM_Initialize(&relay_info, i2c_board, &my_func_block, mode_loop_blocking_protection);
	
	if (START_BOARD < 1 || START_BOARD >= MAX_BOARD || !(relay_info.dm35mrm_board_mask & 1 << START_BOARD)){
		printf("ERROR: Start pin does not exist on current board setup\n");
		result = CM35I2C_Board_Close(i2c_board);
		CM35I2C_Check_Result(result, "Error closing board.");
		return 1;
	}
	
	if (TARGET_BOARD < 1 || TARGET_BOARD >= MAX_BOARD || !(relay_info.dm35mrm_board_mask & 1 << TARGET_BOARD)){
		printf("ERROR: Target pin does not exist on current board setup\n");
		result = CM35I2C_Board_Close(i2c_board);
		CM35I2C_Check_Result(result, "Error closing board.");
		return 1;
	}

	for (int i = 0; i < MAX_ROW_NUMBER; i++){

		// make sure that the start/target pins aren't already allocated to another connection
		if (relay_info.dm35mrm_board_relays_active[START_BOARD][i][START_COL]){
			printf("ERROR: Start pin relay already has active relay in row\n");
			result = CM35I2C_Board_Close(i2c_board);
			CM35I2C_Check_Result(result, "Error closing board.");
			return 1;
		}
		if (relay_info.dm35mrm_board_relays_active[TARGET_BOARD][i][TARGET_COL]){
			printf("ERROR: Target pin relay already has active relay in row\n");
			result = CM35I2C_Board_Close(i2c_board);
			CM35I2C_Check_Result(result, "Error closing board.");
			return 1;
		}


		if (row_to_use != -1) continue;

		// check if the row is empty, if it is we will use it for our connection
		empty_row = 1;
		for (int j = 1; j < MAX_BOARD; j++){
			if (!(relay_info.dm35mrm_board_mask & 1 << j)) continue;
			for (int k = 0; k < MAX_LOCAL_COL_NUMBER; k++){
				if (relay_info.dm35mrm_board_relays_active[j][i][k]) empty_row = 0;	
			}
		}
		if (empty_row) row_to_use = i;
	}

	if (row_to_use == -1){ // all rows are being used for connections
		printf("ERROR: No available empty rows are available for use\n");
		result = CM35I2C_Board_Close(i2c_board);
		CM35I2C_Check_Result(result, "Error closing board.");
		return 1;
	}
	
	// close the necessary relays
	printf("Attempting to close relay at (%d, %d, %d)...\n", START_BOARD, row_to_use, START_COL);
	result = DM35MRM_Try_Close_Relay(&relay_info, START_BOARD, row_to_use, START_COL);	

	if (result) {
		printf("ERROR: Start relay close failed\n");
		return result;
	}

	printf("Writing registers to board...\n");
	if (DM35MRM_Registers_Write(&relay_info)){
		printf("ERROR: Could not write registers to board!\n");
		return 1;
	}

	printf("Attempting to close relay at (%d, %d, %d)...\n", TARGET_BOARD, row_to_use, TARGET_COL);
	result = DM35MRM_Try_Close_Relay(&relay_info, TARGET_BOARD, row_to_use, TARGET_COL);	
	
	if (result){
		printf("ERROR: Target relay close failed\n");
		return result;
	}
	
	printf("Writing registers to board...\n");
	if (DM35MRM_Registers_Write(&relay_info)){
		printf("ERROR: Could not write registers to board!\n");
		return 1;
	}

	printf("Closing CM35I2C board...\n");
	result = CM35I2C_Board_Close(i2c_board);
	CM35I2C_Check_Result(result, "Error closing board.");
	return 0;
}

// This function will attempt to destroy a connection between two pins if one exists, returns 0 if successful 
int disconnect_pins(int start_pin, int target_pin, unsigned long int minor, int connector){
	struct CM35I2C_Board_Descriptor *i2c_board;
	struct CM35I2C_Function_Block my_func_block;
	dm35mrm_info_t relay_info;
	int result;
	int num_function_block = 0;
	int START_COL = (start_pin - 1) % MAX_LOCAL_COL_NUMBER;
	int TARGET_COL = (target_pin - 1) % MAX_LOCAL_COL_NUMBER;
	int START_BOARD = (start_pin - 1) / MAX_LOCAL_COL_NUMBER + 1;
	int TARGET_BOARD = (target_pin - 1) / MAX_LOCAL_COL_NUMBER + 1;
	
	if(connector == 3 || connector == 4)
		num_function_block = CM35I2C_I2C3300_FUNCTION_BLOCK_CN3_4;
	else if(connector == 5)
		num_function_block = CM35I2C_I2C3300_FUNCTION_BLOCK_CN5;
	else if(connector == 6)
                num_function_block = CM35I2C_I2C3300_FUNCTION_BLOCK_CN6;

	if (start_pin == target_pin){
		printf("ERROR: The pins provided must be distinct\n");
		return 1;
	}

	// open and initialize the boards
	printf("Opening CM35I2C board...\n");
	result = CM35I2C_Board_Open(minor, &i2c_board);
	
	CM35I2C_Check_Result(result, "Could not open board");

	printf("Opening CM35I2C function block...\n");
	result = CM35I2C_Function_Block_Open(i2c_board, num_function_block, &my_func_block);
	CM35I2C_Check_Result(result, "Could not open fb");

	printf("Initializing DM35MRM...\n");
	DM35MRM_Initialize(&relay_info, i2c_board, &my_func_block, mode_loop_blocking_protection);

	if (START_BOARD < 1 || START_BOARD >= MAX_BOARD || !(relay_info.dm35mrm_board_mask & 1 << START_BOARD)){
		printf("ERROR: Start pin does not exist on current board setup\n");
		result = CM35I2C_Board_Close(i2c_board);
		CM35I2C_Check_Result(result, "Error closing board.");
		return 1;
	}
	
	if (TARGET_BOARD < 1 || TARGET_BOARD >= MAX_BOARD || !(relay_info.dm35mrm_board_mask & 1 << TARGET_BOARD)){
		printf("ERROR: Target pin does not exist on current board setup\n");
		result = CM35I2C_Board_Close(i2c_board);
		CM35I2C_Check_Result(result, "Error closing board.");
		return 1;
	}

	// find which row the two pins are connected on, if at all
	for (int i = 0; i < MAX_ROW_NUMBER; i++){
		if (relay_info.dm35mrm_board_relays_active[START_BOARD][i][START_COL] == 1 &&
		relay_info.dm35mrm_board_relays_active[TARGET_BOARD][i][TARGET_COL] == 1){

			// found the connection, open the relays
			printf("Attempting to open relay at (%d, %d, %d)...\n", START_BOARD, i, START_COL);
			
			result = DM35MRM_Try_Open_Relay(&relay_info, START_BOARD, i, START_COL);	
			
			if (result) {
				printf("ERROR: Start relay open failed\n");
				return result;
			}

			printf("Writing registers to board...\n");
			if (DM35MRM_Registers_Write(&relay_info)){
				printf("ERROR: Could not write registers to board!\n");
				return 1;
			}

			printf("Attempting to open relay at (%d, %d, %d)...\n", TARGET_BOARD, i, TARGET_COL);

			result = DM35MRM_Try_Open_Relay(&relay_info, TARGET_BOARD, i, TARGET_COL);	
			
			if (result) {
				printf("ERROR: Target relay open failed\n");
				return result;
			}

			printf("Writing registers to board...\n");
			if (DM35MRM_Registers_Write(&relay_info)){
				printf("ERROR: Could not write registers to board!\n");
				return 1;
			}
			
			printf("Closing CM35I2C board...\n");
			result = CM35I2C_Board_Close(i2c_board);
			CM35I2C_Check_Result(result, "Error closing board.");
			return 0;
		}
	}

	printf("ERROR: Pins are not connected\n");

	printf("Closing CM35I2C board...\n");
	result = CM35I2C_Board_Close(i2c_board);
	CM35I2C_Check_Result(result, "Error closing board.");
	return 1;
}

// This function opens all the relays, hence clearing all connections
int clear_all_connections(unsigned long int minor, int connector){
	struct CM35I2C_Board_Descriptor *i2c_board;
	struct CM35I2C_Function_Block my_func_block;
	dm35mrm_info_t relay_info;
	int result;
	int num_function_block = 0;
	
	if(connector == 3 || connector == 4)
		num_function_block = CM35I2C_I2C3300_FUNCTION_BLOCK_CN3_4;
	else if(connector == 5)
		num_function_block = CM35I2C_I2C3300_FUNCTION_BLOCK_CN5;
	else if(connector == 6)
                num_function_block = CM35I2C_I2C3300_FUNCTION_BLOCK_CN6;

	// open and initialize the boards
	printf("Opening CM35I2C board...\n");
	result = CM35I2C_Board_Open(minor, &i2c_board);
	
	CM35I2C_Check_Result(result, "Could not open board");

	printf("Opening CM35I2C function block...\n");
	result = CM35I2C_Function_Block_Open(i2c_board, num_function_block, &my_func_block);
	CM35I2C_Check_Result(result, "Could not open fb");

	printf("Initializing DM35MRM...\n");
	DM35MRM_Initialize(&relay_info, i2c_board, &my_func_block, mode_loop_blocking_protection);

	for (int i = 1; i < MAX_BOARD; i++){
		if (!(relay_info.dm35mrm_board_mask & 1 << i)) continue;
		for (int j = 0; j < MAX_ROW_NUMBER; j++){
			for (int k = 0; k < MAX_LOCAL_COL_NUMBER; k++){
				if (relay_info.dm35mrm_board_relays_active[i][j][k] == 1){
					printf("Attempting to open relay at (%d, %d, %d)...\n", i, j, k);
					result = DM35MRM_Try_Open_Relay(&relay_info, i, j, k);	
					if (result){
						printf("ERROR: relay open failed\n");
						return result;
					}
					printf("Writing registers to board...\n");
					if (DM35MRM_Registers_Write(&relay_info)){
						printf("ERROR: Could not write registers to board!\n");
						return 1;
					}
				}
			}
		}
	}

	printf("Closing CM35I2C board...\n");
	result = CM35I2C_Board_Close(i2c_board);
	CM35I2C_Check_Result(result, "Error closing board.");
	return 0;
}

int print_everything(unsigned long int minor, int connector){
	struct CM35I2C_Board_Descriptor *i2c_board;
	struct CM35I2C_Function_Block my_func_block;
	dm35mrm_info_t relay_info;
	int result;
	int num_function_block = 0;
	
	if(connector == 3 || connector == 4)
		num_function_block = CM35I2C_I2C3300_FUNCTION_BLOCK_CN3_4;
	else if(connector == 5)
		num_function_block = CM35I2C_I2C3300_FUNCTION_BLOCK_CN5;
	else if(connector == 6)
                num_function_block = CM35I2C_I2C3300_FUNCTION_BLOCK_CN6;

	// open and initialize the boards
	printf("Opening CM35I2C board...\n");
	result = CM35I2C_Board_Open(minor, &i2c_board);
	
	CM35I2C_Check_Result(result, "Could not open board");

	printf("Opening CM35I2C function block...\n");
	result = CM35I2C_Function_Block_Open(i2c_board, num_function_block, &my_func_block);
	CM35I2C_Check_Result(result, "Could not open fb");

	printf("Initializing DM35MRM...\n");
	DM35MRM_Initialize(&relay_info, i2c_board, &my_func_block, mode_loop_blocking_protection);
	
	result = DM35MRM_Print_Everything(&relay_info);

	printf("Closing CM35I2C board...\n");
	result = CM35I2C_Board_Close(i2c_board);
	CM35I2C_Check_Result(result, "Error closing board.");
	return 0;
}

int main(int argc, char** argv){
	int status;
	int show_help = 0;
	int do_clear = 0;
	int do_print = 0;

	int start = -1;
	int target = -1;
	int connector = 3;
	unsigned long int minor = 0;
	unsigned long int ularg;
	int connect_not_disconnect = -1;

	int pin_error_found = 0;
	int connect_disconnect_error = 0;

	char* invalid_char_p;

	struct option options[] = {
		{"help", 0, 0, 1},
		{"minor", 1, 0, 2},
		{"connector", 1, 0, 3},
		{"pin", 1, 0, 4},
		{"connect", 0, 0, 5},
		{"disconnect", 0, 0, 6},
		{"clear", 0, 0, 7},
		{"print", 0, 0, 8},
		{0, 0, 0, 0}
	};
	
	program_name = argv[0];

	while (1){
		
		status = getopt_long(argc, argv, "", options, NULL);

		if (status == -1) break;

		switch (status){
			case 1: // help option
				
				show_help = 1;
				break;

			case 2: // minor option
				
				//parse minor number
				errno = 0;
				minor = strtoul(optarg, &invalid_char_p, 10);

				if ((minor == ULONG_MAX) && (errno == ERANGE)){
					error(0, 0, "ERROR: Device minor number caused numeric overflow");
					show_help = 1;	
				}
				
				//verify the whole argument was a valid decimal (checks for trailing non-decimal chars)
				if ((*invalid_char_p != '\0') || (invalid_char_p == optarg)){
					error(0, 0, "ERROR: Non-decimal device minor number");
					show_help = 1;
				}

				break;

			case 3: // connector option
				
				//parse connector number
				errno = 0;
				ularg = strtoul(optarg, &invalid_char_p, 10);

				if ((ularg == ULONG_MAX) && (errno == ERANGE)){
					error(0, 0, "ERROR: Connector number caused numeric overflow");
					show_help = 1;
				}
				
				//verify the whole argument was a valid decimal (checks for trailing non-decimal chars)
				if ((*invalid_char_p != '\0') || (invalid_char_p == optarg)){
					error(0, 0, "ERROR: Non-decimal connector number");
					show_help = 1;
				}
				
				connector = (int) ularg;

				break;

			case 4: // pin option
				
				//parse pin number
				errno = 0;
				ularg = strtoul(optarg, &invalid_char_p, 10);

				if ((ularg == ULONG_MAX) && (errno == ERANGE)){
					error(0, 0, "ERROR: Pin number caused numeric overflow");
					show_help = 1;
				}
				
				//verify the whole argument was a valid decimal (checks for trailing non-decimal chars)
				if ((*invalid_char_p != '\0') || (invalid_char_p == optarg)){
					error(0, 0, "ERROR: Non-decimal pin number");
					show_help = 1;
				}
				
				if (start == -1) {
					start = (int) ularg;
				} else if (target == -1) {
					target = (int) ularg;
				} else if (!pin_error_found) {
					error(0, 0, "Too many pin arguements were given");
					pin_error_found = 1;
					show_help = 1;
				}

				break;

			case 5: // connect option
				
				if (connect_not_disconnect != -1 && connect_disconnect_error == 0){
					error(0, 0, "ERROR: too many --connect and --disconnect options");
					connect_disconnect_error = 1;
					show_help = 1;
				} else {
					connect_not_disconnect = 1;
				}

				break;

			case 6: // connect option
				
				if (connect_not_disconnect != -1 && connect_disconnect_error == 0){
					error(0, 0, "ERROR: too many --connect and --disconnect options");
					connect_disconnect_error = 1;
					show_help = 1;
				} else {
					connect_not_disconnect = 0;
				}

				break;

			case 7: // clear option

				do_clear = 1;
				break;

			case 8: // print option

				do_print = 1;
				break;

		}
	}

	if (((start == -1 || target == -1 || connect_not_disconnect == -1) && do_clear == 0 && do_print == 0) || connector < 3 || connector > 6){
		show_help = 1;
	}

	if (show_help){
		usage();
		status = 0;
	} else if (do_clear){
		status = clear_all_connections(minor, connector);
	} else if (do_print){
		status = print_everything(minor, connector);
	} else if (connect_not_disconnect){
		status = connect_pins(start, target, minor, connector);
	} else {
		status = disconnect_pins(start, target, minor, connector);
	}

	if (!status) printf("Program completed successfully\n");

	return status;
}
