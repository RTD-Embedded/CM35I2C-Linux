#include "cm35i2c_board_access.h"

CM35I2CLIB_API
int CM35I2C_I2C3300_Fifo_Write(struct CM35I2C_Board_Descriptor *handle,
                                        const struct CM35I2C_Function_Block *func_block,
                                        uint32_t value);

CM35I2CLIB_API
int CM35I2C_I2C3300_Fifo_Read(struct CM35I2C_Board_Descriptor *handle,
                                        const struct CM35I2C_Function_Block *func_block,
                                        uint8_t *value);

CM35I2CLIB_API
int CM35I2C_I2C3300_Bus_Control_Write(struct CM35I2C_Board_Descriptor *handle,
                                        const struct CM35I2C_Function_Block *func_block,
                                        uint32_t value);

CM35I2CLIB_API
int CM35I2C_I2C3300_Bus_Control_Read(struct CM35I2C_Board_Descriptor *handle,
				                        const struct CM35I2C_Function_Block *func_block,
				                        uint8_t *go_busy_return);

CM35I2CLIB_API
int CM35I2C_I2C3300_Get_Fifo_Write_Count(struct CM35I2C_Board_Descriptor *handle,
				                        const struct CM35I2C_Function_Block *func_block,
				                        uint8_t *write_count_out);

CM35I2CLIB_API
int CM35I2C_I2C3300_Get_Fifo_Read_Count(struct CM35I2C_Board_Descriptor *handle,
				                        const struct CM35I2C_Function_Block *func_block,
				                        uint8_t *read_fifo_count_out);

CM35I2CLIB_API
int CM35I2C_I2C3300_Fifo_Write_Bytes(struct CM35I2C_Board_Descriptor *handle,
                                        const struct CM35I2C_Function_Block *func_block,
                                        uint32_t *bytes, size_t bytes_length);

CM35I2CLIB_API
int CM35I2C_I2C3300_Check_Ready(struct CM35I2C_Board_Descriptor *handle, 
									const struct CM35I2C_Function_Block *func_block);

CM35I2CLIB_API
int CM35I2C_I2C3300_Function_Block_Reset(struct CM35I2C_Board_Descriptor *handle,
				const struct CM35I2C_Function_Block *func_block);

CM35I2CLIB_API
int CM35I2C_I2C3300_Fifo_Read_All(struct CM35I2C_Board_Descriptor *handle,
                                        const struct CM35I2C_Function_Block *func_block,
                                        uint8_t *bytes, size_t bytes_length);

CM35I2CLIB_API
int CM35I2C_I2C3300_Write_Execute_Bytes(struct CM35I2C_Board_Descriptor *handle,
                                        const struct CM35I2C_Function_Block *func_block,
                                        uint32_t *bytes, size_t bytes_length);

CM35I2CLIB_API
int CM35I2C_I2C3300_Get_Status(struct CM35I2C_Board_Descriptor *handle,
				const struct CM35I2C_Function_Block *func_block,
				uint32_t *read_fifo_count_out);

CM35I2CLIB_API
int CM35I2C_I2C3300_Get_Clock_Register(struct CM35I2C_Board_Descriptor *handle,
				const struct CM35I2C_Function_Block *func_block,
				uint32_t *register_value);

CM35I2CLIB_API
int CM35I2C_I2C3300_Set_Clock_Register(struct CM35I2C_Board_Descriptor *handle,
                                        const struct CM35I2C_Function_Block *func_block,
                                        uint32_t register_value);

CM35I2CLIB_API
int CM35I2C_I2C3300_Clock_To_Register_Value(uint32_t kHz_clock_rate,uint32_t * register_value);

CM35I2CLIB_API
int CM35I2C_I2C3300_Clock_From_Register_Value(uint32_t register_value, uint32_t * kHz_clock_rate);

CM35I2CLIB_API
int CM35I2C_I2C3300_Bus_Control_Reset(struct CM35I2C_Board_Descriptor *handle, 
                                        const struct CM35I2C_Function_Block *func_block);
