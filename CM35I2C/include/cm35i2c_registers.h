/**
    @file

    @brief
        Defines for the CM35I2C Registers (Offsets)

    $Id: cm35i2c_registers.h 154807 2026-06-02 15:14:13Z asutton $
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

#ifndef __CM35I2C_REGISTERS_H__
#define __CM35I2C_REGISTERS_H__


 /**
  * @defgroup CM35I2C_Register_Offsets CM35I2C Register Offsets
  * @{
  */

/******************************************************************
 *  General Board Control (BAR0)
 ******************************************************************/
/**
 * @brief
 *     Offset to General Board Control (BAR0) Format ID register
 */
#define CM35I2C_OFFSET_GBC_FORMAT			0x00

/**
 * @brief
 *     Offset to General Board Control (BAR0) Format ID register
 */
#define CM35I2C_OFFSET_GBC_REV			0x01

/**
 * @brief
 *     Offset to General Board Control (BAR0) EOI (End of Interrupt) register
 */
#define CM35I2C_OFFSET_GBC_END_INTERRUPT		0x02

/**
 * @brief
 *     Offset to General Board Control (BAR0) Board Reset register
 */
#define CM35I2C_OFFSET_GBC_BOARD_RESET		0x03

/**
 * @brief
 *     Offset to General Board Control (BAR0) PDP Number register
 */
#define CM35I2C_OFFSET_GBC_PDP_NUMBER		0x04

/**
 * @brief
 *     Offset to General Board Control (BAR0) FPGA Build register
 */
#define CM35I2C_OFFSET_GBC_FPGA_BUILD		0x08

/**
 * @brief
 *     Offset to General Board Control (BAR0) System Clock register
 */
#define CM35I2C_OFFSET_GBC_SYS_CLK_FREQ		0x0c


/**
 * @brief
 *     Offset to General Board Control (BAR0) IRQ Status register.  Each bit
 *     corresponds to a function block.
 */
#define CM35I2C_OFFSET_GBC_IRQ_STATUS		0x10

/**
 * @brief
 *     Offset to General Board Control (BAR0) DMA IRQ Status register.  Each bit
 *     corresponds to a function block.
 */
#define CM35I2C_OFFSET_GBC_DMA_IRQ_STATUS		0x18

/**
 * @brief
 *     Offset to the beginning of the Function Blocks section of the GBC.
 */
#define CM35I2C_OFFSET_GBC_FB_START			0x20

/**
 * @brief
 *     Size of the function block entries in the GBC
 */
#define CM35I2C_GBC_FB_BLK_SIZE			0x10

/**
 * @brief
 *     Offset to Function Block ID, from the start of the function block
 *     section.
 */
#define CM35I2C_OFFSET_GBC_FB_ID			0x00

/**
 * @brief
 *     Bit mask for TYPE portion of FB ID
 */
#define CM35I2C_FB_ID_TYPE_MASK		0x0000FFFF

/**
 * @brief
 *     Bit mask for SUBTYPE portion of FB ID
 */
#define CM35I2C_FB_ID_SUBTYPE_MASK		0x00FF0000

/**
 * @brief
 *     Bit mask for TYPE REV portion of FB ID
 */
#define CM35I2C_FB_ID_TYPE_REV_MASK		0xFF000000

/**
 * @brief
 *     Offset to the FB Offset in the GBC, from the start of the
 *     FB data block.
 */
#define CM35I2C_OFFSET_GBC_FB_OFFSET		0x04

/**
 * @brief
 *     Offset to the FB DMA Offset in the GBC, from the start of the
 *     FB data block.
 */
#define CM35I2C_OFFSET_GBC_FB_DMA_OFFSET		0x08


/******************************************************************
 *  DMA Control (BAR2)
 ******************************************************************/
/**
 * @brief
 *     Offset to the DMA Action Register (BAR2)
 */
#define CM35I2C_OFFSET_DMA_ACTION		0x00

/**
 * @brief
 *     Offset to the DMA Setup Register (BAR2)
 */
#define CM35I2C_OFFSET_DMA_SETUP		0x01

/**
 * @brief
 *     Offset to the DMA Status (Overflow) Register (BAR2)
 */
#define CM35I2C_OFFSET_DMA_STAT_OVERFLOW	0x02

/**
 * @brief
 *     Offset to the DMA Status (Underflow) Register (BAR2)
 */
#define CM35I2C_OFFSET_DMA_STAT_UNDERFLOW	0x03

/**
 * @brief
 *     Offset to the DMA Current Count Register (BAR2)
 */
#define CM35I2C_OFFSET_DMA_CURRENT_COUNT	0x04

/**
 * @brief
 *     Offset to the DMA Current Buffer Register (BAR2)
 */
#define CM35I2C_OFFSET_DMA_CURRENT_BUFFER	0x07

/**
 * @brief
 *     Offset to the DMA Write FIFO Count Register (BAR2)
 */
#define CM35I2C_OFFSET_DMA_WR_FIFO_CNT	0x08

/**
 * @brief
 *     Offset to the DMA Read FIFO Count Register (BAR2)
 */
#define CM35I2C_OFFSET_DMA_RD_FIFO_CNT	0x0A

/**
 * @brief
 *     Offset to the DMA Status (Used) Register (BAR2)
 */
#define CM35I2C_OFFSET_DMA_STAT_USED		0x0C

/**
 * @brief
 *     Offset to the DMA Status (Invalid) Register (BAR2)
 */
#define CM35I2C_OFFSET_DMA_STAT_INVALID		0x0D

/**
 * @brief
 *     Offset to the DMA Status (Complete) Register (BAR2)
 */
#define CM35I2C_OFFSET_DMA_STAT_COMPLETE		0x0E

/**
 * @brief
 *     Offset to the DMA Last Action Register (BAR2)
 */
#define CM35I2C_OFFSET_DMA_LAST_ACTION		0x0F

/**
 * @brief
 *     Offset to the start of the buffer control section (BAR2)
 */
#define CM35I2C_OFFSET_DMA_BUFF_START		0x10

/**
 * @brief
 *     Offset to the buffer status register, from the start of the buffer
 *     control section (BAR2)
 */
#define CM35I2C_OFFSET_DMA_BUFFER_STAT		0x02

/**
 * @brief
 *     Offset to the buffer control register, from the start of the buffer
 *     control section (BAR2)
 */
#define CM35I2C_OFFSET_DMA_BUFFER_CTRL		0x03

/**
 * @brief
 *     Offset to the buffer size register, from the start of the buffer
 *     control section (BAR2)
 */
#define CM35I2C_OFFSET_DMA_BUFFER_SIZE		0x04

/**
 * @brief
 *     Offset to the buffer address register, from the start of the buffer
 *     control section (BAR2)
 */
#define CM35I2C_OFFSET_DMA_BUFFER_ADDRESS	0x08


/******************************************************************
 *  All Function Blocks (BAR2)
 ******************************************************************/
/**
 * @brief
 *     Offset to the DMA Channels count of the function block (BAR2)
 */
#define CM35I2C_OFFSET_FB_DMA_CHANNELS	0x06

/**
 * @brief
 *     Offset to the DMA buffers count of the function block (BAR2)
 */
#define CM35I2C_OFFSET_FB_DMA_BUFFERS	0x07

/**
 * @brief
 *     Offset to the beginning of the Function Block control section in BAR2.
 */
#define CM35I2C_OFFSET_FB_CTRL_START	0x08













/**
 * @brief
 *     Offset to the Output Value register, from the start of
 *     the Flash control section.
 */
#define CM35I2C_OFFSET_FLASH_CS_LEN			0x00
#define CM35I2C_OFFSET_FLASH_STATUS			0x01
#define CM35I2C_OFFSET_FLASH_START			0x03
#define CM35I2C_OFFSET_FLASH_READ			0x04
#define CM35I2C_OFFSET_FLASH_WRITE_LO		0x08
#define CM35I2C_OFFSET_FLASH_WRITE_HI		0x0c













/**
 * @brief
 *     Offset to the I2C Control Registers,
 *     from the start of the channel control section
 */
#define CM35I2C_I2C3300_OFFSET_I2C_BUS_CTRL 	0x08
#define CM35I2C_I2C3300_OFFSET_FB_RST 		0x0B
#define CM35I2C_I2C3300_OFFSET_CLOCK_RATE           0x0C
#define CM35I2C_I2C3300_OFFSET_INT_ENA	  	0x10
#define CM35I2C_I2C3300_OFFSET_INT_STAT	  	0x14
#define CM35I2C_I2C3300_OFFSET_WRITE 		0x18
#define CM35I2C_I2C3300_OFFSET_WRITE_COUNT 		0x1C
#define CM35I2C_I2C3300_OFFSET_READ 		0x20
#define CM35I2C_I2C3300_OFFSET_READ_COUNT 		0x24


// CM35I2C_I2C3300_OFFSET_I2C_BUS_CTRL Bit Definitions
#define CM35I2C_I2C3300_GO_BUSY_BITMASK       0x01
#define CM35I2C_I2C3300_BUS_RESET_BITMASK     0x02

// Function Block
#define CM35I2C_I2C3300_FUNCTION_BLOCK_CN3_4  0x00
#define CM35I2C_I2C3300_FUNCTION_BLOCK_CN5    0x01
#define CM35I2C_I2C3300_FUNCTION_BLOCK_CN6    0x02
#define CM35I2C_I2C3300_FUNCTION_BLOCK_CN9    0x03
#define CM35I2C_I2C3300_FUNCTION_BLOCK_CN10   0x04
#define CM35I2C_I2C3300_FUNCTION_BLOCK_CN11   0x05





/**
 * @} CM35I2C_Register_Offsets
 */

#endif
