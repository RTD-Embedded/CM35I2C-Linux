/**
    @file

    @brief
	Gets the version of the microcontrollers on the specified board.	
    @verbatim
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
#define NUM_OF_MICROCONTROLLERS 4

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
	uint32_t version[4];
	int connector;
	int num_function_block = CM35I2C_I2C3300_FUNCTION_BLOCK_CN3_4;

	memset(version, 0, sizeof(version));


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


	// TODO: Library calls to get register space 0x60-0x63

	for (int i = 1; i < MAX_BOARD; i++){
		if (!(relay_info.dm35mrm_board_mask & 1 << i)) continue;
		for (int j = 0; j < NUM_OF_MICROCONTROLLERS; j++){
			DM35MRM_Get_Version(&relay_info, i, j, version);
			printf("Version number for board %d microcontroller %d: %02x %02x %02x %02x\n", i, j, version[0], version[1], version[2], version[3]);
		}
	}

	printf("\nClosing Board\n");
	result = CM35I2C_Board_Close(i2c_board);
	CM35I2C_Check_Result(result, "Error closing board.");
	printf("Example program completed.\n");
	return 0;
}
