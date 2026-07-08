/**
    @file

    @brief
        Structures and defines for the CM35I2C driver module.


    $Id: cm35i2c_driver.h 150218 2025-10-13 15:58:55Z bkorpacz $
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

#ifndef __CM35I2C_DRIVER_H__
#define __CM35I2C_DRIVER_H__


#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/types.h>

/*=============================================================================
Constants
 =============================================================================*/

/**
 * @defgroup CM35I2C_Driver_Constants CM35I2C Driver Constants
 * @{
 */

/**
 * @brief
 * CM35I2C Maximum number of devices
 */
#define CM35I2C_MAX_NUM_DEVICES 	8

/**
 * @brief
 * CM35I2C Number of minors per device
 */
#define CM35I2C_NUM_MINORS_PER_DEVICE 1

/**
 * @brief
 * CM35I2C Max possible board name length.
 */
#define CM35I2C_NAME_LENGTH 200

/**
 * @brief
 * Number of standard PCI regions
 */

#define CM35I2C_PCI_NUM_REGIONS	  PCI_ROM_RESOURCE

/**
 * @brief
 * Number of interrupts to hold in a queue for processing
 */
 #define CM35I2C_INT_QUEUE_SIZE		256

/**
 * @} CM35I2C_Driver_Constants
 */


/*=============================================================================
Enumerations
 =============================================================================*/

/**
 * @defgroup CM35I2C_Driver_Enumerations CM35I2C Driver Enumerations
 * @{
 */

/**
 * @brief
 * Direction of access to standard PCI region
 */

enum cm35i2c_pci_region_access_dir {

	/**
	 * Read from the region
	 */

	CM35I2C_PCI_REGION_ACCESS_READ = 0,

	/**
	 * Write to the region
	 */

	CM35I2C_PCI_REGION_ACCESS_WRITE
};


/**
 * @} CM35I2C_Driver_Enumerations
 */



/*=============================================================================
Structures
 =============================================================================*/

/**
 * @defgroup CM35I2C_Driver_Structures CM35I2C Driver Structures
 * @{
 */

/**
 * @brief
 *	  CM35I2C PCI region descriptor.  This structure holds information about
 *	  one of a device's PCI memory regions.
 */

struct cm35i2c_pci_region {

	/**
	 * I/O port number if I/O mapped
	 */

	unsigned long io_addr;

	/**
	 * Length of region in bytes
	 */

	unsigned long length;

	/**
	 * Region's physical address if memory mapped or I/O port number if I/O
	 * mapped
	 */

	unsigned long phys_addr;

	/**
	 * Address at which region is mapped in kernel virtual address space if
	 * memory mapped
	 */

	void __iomem *virt_addr;

	/**
	 * Flag indicating whether or not the I/O-mapped memory ranged was
	 * allocated.  A value of zero means the memory range was not allocated.
	 * Any other value means the memory range was allocated.
	 */

	uint8_t allocated;
};

/**
 * @brief
 *	  CM35I2C DMA descriptor.  This structure holds information about
 *	  a single DMA buffer.
 */

struct cm35i2c_dma_descriptor {

	/**
	 * Function block number this DMA is associated with.
	 */
	uint32_t fb_num;

	/**
	 * DMA channel this buffer is in.
	 */
	int channel;

	/**
	 * DMA buffer number this descriptor represents.
	 */
	int buffer;

	/**
	 * System memory address for buffer
	 */
	void *virt_addr;


	/**
	 * Bus memory address for buffer.
	 */
	dma_addr_t bus_addr;


	/**
	 * Size of this allocated buffer
	 */
	unsigned int buffer_size;


	/**
	 * List head so that descriptors can be kept in a linked list.
	 */
	struct list_head list;


};


/**
 * @brief
 *	  CM35I2C Device Descriptor.  The identifying info for this
 *        particular board.
 */

struct cm35i2c_device_descriptor {

	/**
	 * Device name used when requesting resources; a NUL terminated string of
	 * the form rtd-cm35i2c-x where x is the device minor number.
	 */

	char name[CM35I2C_NAME_LENGTH];

	/**
	 * Device index
	 */
	int device_index;

	/**
	 * Information about each of the standard PCI regions
	 */

	struct cm35i2c_pci_region pci[PCI_ROM_RESOURCE];

	/**
	 * PCI device pointer
	 */
	struct pci_dev * pdev;

	/**
	 * Concurrency control
	 */

	spinlock_t device_lock;

	/**
	 * Character device
	 */
	struct cdev cdev;

	/**
	 * Number of entities which have the device file open.  Used to enforce
	 * single open semantics.
	 */

	uint8_t reference_count;

	/**
	 * IRQ line number
	 */

	unsigned int irq_number;


	/**
	* Used to assist poll in shutting down the thread waiting for interrupts
	*/

	uint8_t remove_isr_flag;

	/**
	* Queue of processes waiting to be woken up when an interrupt occurs
	*/

	wait_queue_head_t int_wait_queue;

	/**
	* Queue of processes waiting to be woken up when an interrupt occurs
	*/

	wait_queue_head_t dma_wait_queue;

	/**
	* Interrupt queue containing which functional blocks caused interrupts
	*/

	int interrupt_fb[CM35I2C_INT_QUEUE_SIZE];


	/**
	* Number of interrupts missed because of a full queue
	*/

	unsigned int int_queue_missed;

	/**
	* Number of interrupts currently in the queue
	*/

	unsigned int int_queue_count;

	/**
	* Where in the queue new entries are put
	*/

	unsigned int int_queue_in_marker;

	/**
	* Where in the queue entries are pulled from
	*/

	unsigned int int_queue_out_marker;

	/**
	 * A list of all allocated DMA buffers
	 */
	struct list_head dma_descr_list;

	/**
	 * 16-bit PCIe device ID
	 */
	unsigned int device_id;
};

/**
 * @brief
 *    Placeholder protoype for file ops struct
 */
static struct file_operations cm35i2c_file_ops;

/**
 * @} CM35I2C_Driver_Structures
 */


#endif

