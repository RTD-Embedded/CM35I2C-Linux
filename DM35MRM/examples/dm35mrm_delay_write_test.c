/**
    @file

    @brief
        This program demonstrates how to write a delayed change to the
		DM35MRM. Example program is designed to show how to interact with 
		the	General call (GC) functions of the DM35MRM library. GC commands
		send a query out to all DM35MRM boards listening on an I2C bus.
		
    @verbatim

		This code demonstrates how disabling the auto latch feature works
		for more control over when boards will close and open relays.The 
		DM35MRM_GC Commands are used for more strictly synchronous changes 
		to the relays, as well as configuration changes for all DM35MRM 
		boards on an I2C.

		The General Calls Used Here are:
		DM35MRM_GC_Auto_Latch_Disable(...)
		DM35MRM_GC_Latch_Push(...) 
		DM35MRM_GC_Auto_Latch_Enable_And_Push(...)
		
		These are typically used in this order. This program uses the DM35MRM
		Printout to demonstrate this, but it is also reccomended to test
		Continuity between row and column. 

		This program requires a DM35MRM to be set to Board Address 1 (only a jumper 
		on pin 0 of JP1) and assumes the use of the Stacking I2C connector.		

    @endverbatim

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

#define OPTION_GIVEN 0xFF

static char *program_name;


static void usage(void)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "NAME\n\n\t%s\n\n", program_name);
	fprintf(stderr, "USAGE\n\n\t%s [OPTIONS]\n\n", program_name);
	fprintf(stderr, "OPTIONS\n\n");
	fprintf(stderr, "\t--help\n");
	fprintf(stderr, "\t\tShow this help screen and exit.\n\n");
	fprintf(stderr, "\t--minor NUM\n");
	fprintf(stderr, "\t\tSpecify the minor number (>= 0) of the board to open. When not specified,\n");
	fprintf(stderr, "\t\tthe device file with minor 0 is opened.\n");
	fprintf(stderr, "\t--reset\n");
	fprintf(stderr, "\t\tIf this flag is used to reset the CM35I2C board before the operation is started.\n");
	fprintf(stderr, "\t--connector NUM\n");
	fprintf(stderr, "\t\tThe Number of the Connector (CN6 -> --connector 6)\n");
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}



int main(int argument_count, char **arguments)
{
	unsigned long int minor = 0;
	int result;
	int help_option_given = 0;
	int reset_option_given = 0;
	int status;
	struct CM35I2C_Board_Descriptor *i2c_board;
	struct CM35I2C_Function_Block my_func_block;
	dm35mrm_info_t relay_info;
	uint32_t readback[18];
	int connector;
	int num_function_block = CM35I2C_I2C3300_FUNCTION_BLOCK_CN3_4;

	memset(readback, 0, sizeof(readback));


	char *invalid_char_p;
	struct option options[] = {
		{"help", 0, 0, 1},
		{"minor", 1, 0, 2},
		{"reset", 0, 0, 3},
		{"connector", 1, 0, 4},
		{0, 0, 0, 0}
	};

	program_name = arguments[0];

	// Show usage, parse arguments
	while (1) {
		
		/*
		 * Parse the next command line option and any arguments it may require
		 */
		status = getopt_long(argument_count,
				     arguments, "", options, NULL);
		/*
		 * If getopt_long() returned -1, then all options have been processed
		 */
		if (status == -1) {
			break;
		}

		/*
		 * Figure out what getopt_long() found
		 */
		switch (status) {

		/*#################################################################
			  User entered '--help'
		################################################################# */
		case HELP_OPTION:
			help_option_given = OPTION_GIVEN;
			break;

		/*#################################################################
			User entered '--minor'
		################################################################# */
		case MINOR_OPTION:
			/*
			 * Convert option argument string to unsigned long integer
			 */
			errno = 0;
			minor = strtoul(optarg, &invalid_char_p, 10);

			/*
			 * Catch unsigned long int overflow
			 */
			if ((minor == ULONG_MAX)
			    && (errno == ERANGE)) {
				error(0, 0,
				      "ERROR: Device minor number caused numeric overflow");
				help_option_given = OPTION_GIVEN;
			}

			/*
			 * Catch argument strings with valid decimal prefixes, for
			 * example "1q", and argument strings which cannot be converted,
			 * for example "abc1"
			 */
			if ((*invalid_char_p != '\0') || (invalid_char_p == optarg)) {
				error(0, 0, 
					"ERROR: Non-decimal device minor number");
				help_option_given = OPTION_GIVEN;
			}

			break;
		/*#################################################################
			  User entered '--reset'
		################################################################# */
		case 3:
			reset_option_given = OPTION_GIVEN;
			break;

		/*#################################################################
			User entered '--connector'
		################################################################# */
        case 4:
            errno = 0;
            connector = strtoul(optarg, &invalid_char_p,10);
            
            if((minor == ULONG_MAX)
                && (errno == ERANGE)) {
                error(0, 0, "ERROR: Connection Number Out of Bounds");
                help_option_given = OPTION_GIVEN;
            }
            
            if ((*invalid_char_p != '\0') || (invalid_char_p == optarg)) {
				error(0, 0, 
					"ERROR: Invalid Character");
				help_option_given = OPTION_GIVEN;
			}

            // Connector number is not a function block number
            // Function Block Number is used for all operations
            if(connector == 3 || connector == 4)
                num_function_block = CM35I2C_I2C3300_FUNCTION_BLOCK_CN3_4;
            else if(connector == 5)
                num_function_block = CM35I2C_I2C3300_FUNCTION_BLOCK_CN5;
            else if(connector == 6)
                num_function_block = CM35I2C_I2C3300_FUNCTION_BLOCK_CN6;
            else
                help_option_given = OPTION_GIVEN;

            break;
		/*#################################################################
		   User entered unsupported option
		   ################################################################# */
		case '?':
			help_option_given = OPTION_GIVEN;
			break;

		/*#################################################################
		   getopt_long() returned unexpected value
		   ################################################################# */
		default:
			error(EXIT_FAILURE,
			      0,
			      "ERROR: getopt_long() returned unexpected value %#x",
			      status);
			break;
		}
	}

	// If an input is incorrect or a help argument is called display usage and halt program
	if(help_option_given == OPTION_GIVEN){
		usage();
		return 1;
	}

	printf("Opening board...");
	result = CM35I2C_Board_Open(minor, &i2c_board);
	CM35I2C_Check_Result(result, "Could not open board");
	printf("success.\r\n");

	if(reset_option_given == OPTION_GIVEN){
		printf("Resetting board.....");
		result = CM35I2C_Gbc_Board_Reset(i2c_board);
		CM35I2C_Check_Result(result, "Could not reset board");
		printf("success.\n\n");
	}

    result = CM35I2C_Function_Block_Open(i2c_board, num_function_block, &my_func_block);
    CM35I2C_Check_Result(result, "Could not open fb");
	printf("Function Block success.\n\n");

	printf("INITIALIZE DATA...\r\n");
	result = DM35MRM_Initialize(&relay_info, i2c_board, &my_func_block, mode_loop_blocking_protection);
	CM35I2C_Check_Result(result,"Failed to DM35MRM initialize.");
	DM35MRM_Print_Everything(&relay_info);
	
	// Begin Demonstrating the Auto Latching Behaviour
	// DM35MRM_GC_Auto_Latch_Disable Should Be re enabled with
	// DM35MRM_GC_Auto_Latch_Enable_And_Push(...) which will
	// Both Enable and Begin pushing the latch registers before
	// the end of the program.
	printf("disable latch autopush...\r\n");
	result = DM35MRM_GC_Auto_Latch_Disable(&relay_info);
	if(result) {
		printf("Could not disable latch autopush... error %d\r\n",result);
	}

	// Display the Delay Write Behaviour with changing the Relay
	// on Board 1, Row 0, Column 0
	printf("Toggle Relay [1 0 0]...\r\n");
	if(relay_info.dm35mrm_board_relays_active[1][0][0] != RELAY_CLOSE) {
		result = DM35MRM_Try_Close_Relay(&relay_info,1,0,0);
	} else {
		result = DM35MRM_Try_Open_Relay(&relay_info,1,0,0);
	}

	if(result) {
		printf("Could not toggle relay... error %d\r\n",result);
	}

	// This sends the I2C commands to write out the DM35MRM
	// Registers it should not change the Relay immediately.
	printf("Write Registers\r\n");
	result = DM35MRM_Registers_Write(&relay_info);
	if(result) {
		printf("Could not write registers... error %d\r\n",result);
	}

	// Refresh Data To Prove that the Changes Haven't
	// Been Pushed Yet
	DM35MRM_Get_All_Relays(&relay_info);

	printf("vvvvvv CONFIRM NO CHANGES YET vvvvv\r\n");
	DM35MRM_Print_Everything(&relay_info);
	printf("^^^^^ CONFIRM NO CHANGES YET ^^^^^\r\n");

	// Wait an arbitrary time before pushing the register 
	// changes to prove the concept of a non-latching output.
	sleep(1);
	
	printf("\r\n\r\n Pushing registers...\r\n");
	result = DM35MRM_GC_Latch_Push(&relay_info);
	if(result) {
		printf("Could not GLOBAL PUSH... error %d\r\n",result);
	}
	
	// Refresh Data to Prove that the Changes now Exist
	DM35MRM_Get_All_Relays(&relay_info);

	printf("vvvvvv CONFIRM RELAY CHANGES vvvvv\r\n");
	DM35MRM_Print_Everything(&relay_info);
	printf("^^^^^ CONFIRM RELAY CHANGES ^^^^^\r\n");
	
	// Send the latching enable command to set it back, this is recomended to
	// be used whenever you are done using DM35MRM_GC_Auto_Latch_Disable(...)
	DM35MRM_GC_Auto_Latch_Enable_And_Push(&relay_info);

	printf("\nClosing Board\n");
	result = CM35I2C_Board_Close(i2c_board);
	CM35I2C_Check_Result(result, "Error closing board.");
	printf("Example program completed.\n");
	return 0;
}
