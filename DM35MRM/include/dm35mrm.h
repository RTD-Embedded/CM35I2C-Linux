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

#include <stdint.h>
#include "cm35i2c_board_access.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CUR_TAR_ADDR 0x0100





// Board address range is 0-31 
#define MAX_BOARD 32
// Microcontroller address range is 0-3
#define MAX_MICRO_CONTROLLER 4
// Max Closings values are used in loop blocking mode.
#define MAX_ROW_CLOSINGS 2
#define MAX_COL_CLOSINGS 1
#define MAX_ROW_NUMBER 8
#define MAX_LOCAL_COL_NUMBER 16
#define MAX_LINEAR_COL_NUMBER MAX_LOCAL_COL_NUMBER*MAX_BOARD
#define MAX_BUFFER 1024
#define MAX_LENGTH_FILENAME 100


/**
 * These are the four states that a relay can be set to, by default when the DM35MRM logic
 * is initialized it will set all of the virtual and active relays on the board to RELAY_IGNORE,
 * then it will populate the RELAY_OPEN and RELAY_CLOSE as the readback messages come back from 
 * populated board IDs.
*/
#define RELAY_CLOSE 1
#define RELAY_OPEN 0
#define RELAY_IGNORE 0x0F
#define RELAY_INVALID 0xFF


/**
 * This union describes the Microcontroller Register set with the name of the
 * registers in the anonymous struct. It then unifies it with a uint8_t 
 * byte_array for ease of communication for i2c read commands. This byte
 * array will be read back by our DM35MRM_Get_Registers(...)
*/
typedef union {
	struct {
		uint8_t num_write;
		uint8_t num_read;
		uint8_t board_id;
		uint8_t board_name[7];
		uint8_t flag_status_register;
		uint8_t rd_back_addr;
		uint8_t port_a;
		uint8_t port_f;
		uint8_t port_d;
		uint8_t port_c;
		uint8_t config;
	};
	uint8_t byte_array[17];
} dm35mrm_mc_register_set_t;



typedef enum { 
	mode_loop_blocking_protection, 
	mode_no_blocking, 
	mode_invalid 
} blocking_mode_t; 

/**
 * DM35MRM information construct
 * This is used for tracking all information of a DM35MRM stackup.
 * Almost all functions that relate to the DM35MRM pass and modify this 
 * structure to keep track of the active state of the relays.
*/
typedef struct DM35MRM_information_construct {
	struct CM35I2C_Board_Descriptor* i2c_board;
	struct CM35I2C_Function_Block* func_block;
	uint8_t dm35mrm_board_relays_active[MAX_BOARD][MAX_ROW_NUMBER][MAX_LOCAL_COL_NUMBER];
	uint8_t dm35mrm_board_relays_virtual[MAX_BOARD][MAX_ROW_NUMBER][MAX_LOCAL_COL_NUMBER];
	uint32_t dm35mrm_board_mask;
	blocking_mode_t blocking_mode;
	uint8_t dm35mrm_available_row_closings[MAX_ROW_NUMBER];
	uint8_t dm35mrm_available_col_closings[MAX_BOARD][MAX_LOCAL_COL_NUMBER];
} dm35mrm_info_t;

/**
 * This union is used with an anonymous struct to name specific bytes to 
 * construct an i2c write payload
*/
typedef union {
	struct {
		uint32_t address;
		uint32_t cmd;
		uint32_t porta;
		uint32_t portf;
		uint32_t portd;
		uint32_t portc;
	};
	uint32_t data[6];
} dm35mrm_write_ports_msg_t;


// These are the valid commands for use with the 
// DM35MRM Microcontrollers on DATA OUT
#define CMD_PORT_A 0x00
#define CMD_PORT_F 0x01
#define CMD_PORT_D 0x02
#define CMD_PORT_C 0x03
#define CMD_CONFIG 0x04
#define CMD_READ   0x05



CM35I2CLIB_API
int DM35MRM_Get_Registers(dm35mrm_info_t* dm35mrm_information,
                    uint8_t board_id, uint8_t mc_number,dm35mrm_mc_register_set_t *mc_registers);

CM35I2CLIB_API
int DM35MRM_Get_Version(dm35mrm_info_t* dm35mrm_information,
                    uint8_t board_id, uint8_t mc_number, uint32_t* version);

CM35I2CLIB_API
int DM35MRM_Get_Board_Relays(dm35mrm_info_t* dm35mrm_information,
					uint8_t board_id);

CM35I2CLIB_API
int DM35MRM_Print_Relays(dm35mrm_info_t* dm35mrm_information, uint8_t board_id, uint8_t activeNotVirtual);

CM35I2CLIB_API
int DM35MRM_Get_All_Relays(dm35mrm_info_t* dm35mrm_information);

CM35I2CLIB_API
int DM35MRM_Try_Close_Relay(dm35mrm_info_t* dm35mrm_information,int board, int row, int col);

CM35I2CLIB_API
int DM35MRM_Initialize(dm35mrm_info_t* dm35mrm_information, struct CM35I2C_Board_Descriptor *board_descriptor_handle,
                    struct CM35I2C_Function_Block *fb_handle, blocking_mode_t mode_select);

CM35I2CLIB_API
int DM35MRM_Get_Virtual_Closing_Information(dm35mrm_info_t* dm35mrm_information);

CM35I2CLIB_API
int DM35MRM_Try_Open_Relay(dm35mrm_info_t* dm35mrm_information, int board, int row, int col);

CM35I2CLIB_API
int DM35MRM_Virtual_Relay_Array_To_Register_Ports(uint8_t board_id, dm35mrm_mc_register_set_t* mc_registers, dm35mrm_info_t* relays_info);

CM35I2CLIB_API
int DM35MRM_MC_Register_Ports_To_Active_Relay_Array(uint8_t board_id, dm35mrm_mc_register_set_t* mc_registers, dm35mrm_info_t* relays_info);

CM35I2CLIB_API
uint32_t DM35MRM_Construct_I2C_Address(uint8_t board_id, uint8_t mc_num,uint8_t RnW);

CM35I2CLIB_API
int DM35MRM_Registers_Write(dm35mrm_info_t* relay_info);

CM35I2CLIB_API
int DM35MRM_Print_Everything(dm35mrm_info_t* dm35mrm_information);

CM35I2CLIB_API
int DM35MRM_GC_Reset_MC(dm35mrm_info_t* relays_info);

CM35I2CLIB_API
int DM35MRM_GC_Clear_Relays(dm35mrm_info_t* relays_info);

CM35I2CLIB_API
int DM35MRM_GC_Auto_Latch_Disable(dm35mrm_info_t* relays_info);

CM35I2CLIB_API
int DM35MRM_GC_Auto_Latch_Enable_And_Push(dm35mrm_info_t* relays_info);

CM35I2CLIB_API
int DM35MRM_GC_Latch_Push(dm35mrm_info_t* relays_info);

CM35I2CLIB_API
int DM35MRM_GC_Zero_Readback_Call(dm35mrm_info_t* relays_info);

CM35I2CLIB_API
int DM35MRM_Set_Relays_With_CSV_Import(dm35mrm_info_t* dmInfo, char* filename);

CM35I2CLIB_API
int DM35MRM_Relays_CSV_Export(dm35mrm_info_t* dmInfo, char* filename);


#ifdef __cplusplus
}
#endif
