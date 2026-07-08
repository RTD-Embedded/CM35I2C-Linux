/**
    @file

    @brief
        This program demonstrates how to write to a specific relay immediately.
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
	fprintf(stderr, "\t\tSpecify the minor number (>= 0) of the board to open.  When not specified,\n");
	fprintf(stderr, "\t\tthe device file with minor 0 is opened.\n");
	fprintf(stderr, "\t--open\n");
	fprintf(stderr, "\t\tGoing to attempt to open the specified relay.\n");
	fprintf(stderr, "\t--close\n");
	fprintf(stderr, "\t\tGoing to attempt to close the specified relay.\n");
	fprintf(stderr, "\t--board NUM\n");
	fprintf(stderr, "\t\tSpecify which DM35MRM board ID to change\n");
	fprintf(stderr, "\t--row NUM\n");
	fprintf(stderr, "\t\tThe target Relay's Row\n");
	fprintf(stderr, "\t--col NUM\n");
	fprintf(stderr, "\t\tThe target Relay's column\n");
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
	struct CM35I2C_Board_Descriptor *i2c_board;
	struct CM35I2C_Function_Block my_func_block;
	int help_option_given = 0;
	int reset_option_given = 0;
	int status;
	int bid;
	int row, col, close_not_open;
	dm35mrm_info_t relay_info;
	uint32_t readback[18];
	int connector;
	int num_function_block = CM35I2C_I2C3300_FUNCTION_BLOCK_CN3_4;

	// Make sure that the readback variable isn't 
	// initialized with Non-zero data
	memset(readback, 0, sizeof(readback));

	// Set these variables to invalid they are required.
	row = -1;
	col = -1;
	close_not_open = -1;
	bid = -1;


	char *invalid_char_p;
	struct option options[] = {
		{"help", 0, 0, 1},
		{"minor", 1, 0, 2},
		{"close", 0, 0, 3},
		{"open", 0, 0, 4},
		{"board", 1, 0, 5},
		{"row", 1, 0, 6},
		{"col", 1, 0, 7},
		{"reset",1, 0, 8},
		{"connector",1,0,9},
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
			if ((*invalid_char_p != '\0')
			    || (invalid_char_p == optarg)) {
				error(0, 0,
				      "ERROR: Non-decimal device minor number");
				help_option_given = OPTION_GIVEN;
			}

			break;

		/*#################################################################
			User entered '--close'
		################################################################# */
		case 3:
			close_not_open = 1;
			break;
		/*#################################################################
			User entered '--open'
		################################################################# */
		case 4:
			close_not_open = 0;
			break;

		/*#################################################################
			User entered '--board'
		################################################################# */
		case 5:
			/*
			 * Convert option argument string to unsigned long integer
			 */
			errno = 0;
			bid = strtoul(optarg, &invalid_char_p, 10);

			/*
			 * Catch unsigned long int overflow
			 */
			if (bid < 0 || bid > 31)
			{
				error(0, 0, "ERROR: Board Number Out of Bounds");
				help_option_given = OPTION_GIVEN;
			}

			break;

		/*#################################################################
			User entered '--row'
		################################################################# */
		case 6:
			/*
			 * Convert option argument string to unsigned long integer
			 */
			errno = 0;
			row = strtoul(optarg, &invalid_char_p, 10);

			/*
			 * Catch unsigned long int overflow
			 */
			if (row < 0 || row >=MAX_ROW_NUMBER)
			{
				error(0, 0, "ERROR: Row Number Out of Bounds");
				help_option_given = OPTION_GIVEN;
			}

			break;

		/*#################################################################
			User entered '--col'
		################################################################# */
		case 7:
			/*
			 * Convert option argument string to unsigned long integer
			 */
			errno = 0;
			col = strtoul(optarg, &invalid_char_p, 10);

			/*
			 * Catch unsigned long int overflow
			 */
			if (col < 0 || col >=MAX_LOCAL_COL_NUMBER)
			{
				error(0, 0, "ERROR: Column Number Out of Bounds");
				help_option_given = OPTION_GIVEN;
			}
			break;

		/*#################################################################
			User entered '--reset'
		################################################################# */
		case 8:
			reset_option_given = OPTION_GIVEN;
			break;

		/*#################################################################
			User entered '--connector'
		################################################################# */
        case 9:
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

	// Verify that we have received all of the proper input flags
	if( row == -1 || col == -1 || bid == -1 || close_not_open == -1) {
		help_option_given = OPTION_GIVEN;
	}

	if(help_option_given == OPTION_GIVEN){
		usage();
		return 1;
	}


	printf("Opening board.....");
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


	DM35MRM_Initialize(&relay_info, i2c_board, &my_func_block, mode_loop_blocking_protection);

	// Print everything before the change.
	DM35MRM_Print_Everything(&relay_info);

	if(close_not_open)
	{
		printf("CLOSING: [%i][%i][%i]\r\n",bid,row,col);
		result = DM35MRM_Try_Close_Relay(&relay_info, (uint8_t) bid, (uint8_t)row, (uint8_t)col);
	}
	else
	{
		printf("OPENING: [%i][%i][%i]\r\n",bid,row,col);
		result = DM35MRM_Try_Open_Relay(&relay_info, (uint8_t)bid, (uint8_t)row, (uint8_t)col);
	}

	// Print everything Again
	if(!result)
	{
		DM35MRM_Print_Everything(&relay_info);
		// This function will attempt to write the board information using the blocking registers algorithm.
		// if successful it will close and open the appropriate relays by issueing the I2C commands
		if(DM35MRM_Registers_Write(&relay_info))
		{
			printf("ERROR: Could not write registers to board!");
		}
	}
	else
	{
		printf("Write Failed with ERROR:%d\r\n",result);
	}
	
	printf("\nClosing Board\n");
	result = CM35I2C_Board_Close(i2c_board);
	CM35I2C_Check_Result(result, "Error closing board.");
	printf("Example program successfully completed.\n");
	return 0;
}
