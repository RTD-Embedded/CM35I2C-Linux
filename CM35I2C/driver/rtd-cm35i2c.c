/**
    @file

    @brief
        CM35I2C driver source code

    $Id: rtd-cm35i2c.c 154351 2026-05-14 12:16:17Z asutton $
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


#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/version.h>


#include "cm35i2c_driver.h"
#include "cm35i2c_ioctl.h"
#include "cm35i2c_board_access_structs.h"
#include "cm35i2c_registers.h"
#include "cm35i2c.h"
#include "cm35i2c_types.h"

/*===============================================================
Driver identification
 ===============================================================*/

#define DRIVER_NAME "rtd-cm35i2c"
#define DRIVER_VERSION "05.00.02"
#define DRIVER_DESCRIPTION "Device driver for the CM35I2C"
#define DRIVER_COPYRIGHT "Copyright (C), RTD Embedded Technologies, Inc.  All Rights Reserved."

#define DEVICE_PREFIX "cm35i2c"
#define RESET_ON_CLOSE

/*===============================================================
Debug Flags.
 ===============================================================*/
#if defined(DEBUG_INT) || defined(DEBUG_ALL)
	#define CM35I2C_DEBUG_INTERRUPTS
#endif

#if defined(DEBUG_DMA) || defined(DEBUG_ALL)
	#define CM35I2C_DEBUG_DMA
#endif

#if defined(DEBUG) || defined(DEBUG_ALL)
	#define CM35I2C_DEBUG
#endif


/*===============================================================
Linux Kernel Version specific defines.
 ===============================================================*/
#define IO_MEMORY_READ8     ioread8
#define IO_MEMORY_READ16    ioread16
#define IO_MEMORY_READ32    ioread32
#define IO_MEMORY_WRITE8    iowrite8
#define IO_MEMORY_WRITE16   iowrite16
#define IO_MEMORY_WRITE32   iowrite32

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
#define IRQF_SHARED     SA_SHIRQ
#endif


/*===============================================================
Driver Interrupt constants
 ===============================================================*/
#define TOO_MANY_MISSED_IRQ	10


/*=============================================================================
Global variables
 =============================================================================*/

/**
 * Character device major number; dynamically assigned
 */
static int cm35i2c_major;

/**
 * CM35I2C device descriptors
 */

static struct cm35i2c_device_descriptor *cm35i2c_devices[CM35I2C_MAX_NUM_DEVICES];

static struct class *dev_class = NULL;

/**
 * Table of devices supported by the driver.  This array is used by
 * cm35i2c_probe_devices() to walk the entire PCI device list looking for CM35I2C
 * devices.  The table is terminated by a "NULL" entry.
 *
 * The individual structures in this array are set up using ANSI C standard
 * format initialization (which is the preferred method in 2.6 kernels) instead
 * of tagged initialization (which is the preferred method in 2.4 kernels).
 */

static const struct pci_device_id cm35i2c_pci_device_table[] = {
	{
	 .vendor = CM35I2C_PCI_VENDOR_ID,
	 .device = CM35I2C_PCI_DEVICE_ID,
	 .subvendor = PCI_ANY_ID,
	 .subdevice = PCI_ANY_ID,
	 .class = 0,
	 .class_mask = 0,
	 .driver_data = 0},
	{
	 .vendor = 0,
	 .device = 0,
	 .subvendor = 0,
	 .subdevice = 0,
	 .class = 0,
	 .class_mask = 0,
	 .driver_data = 0}
};

MODULE_DEVICE_TABLE(pci, cm35i2c_pci_device_table);





/*=============================================================================
Driver functions
 =============================================================================*/

/******************************************************************************
*  CM35I2C device descriptor initialization
*  Perform any required initialization of data structures in the device
*  descriptor.
 ******************************************************************************/
static void
cm35i2c_reset_device_desc(struct cm35i2c_device_descriptor *cm35i2c_device)
{
	cm35i2c_device->remove_isr_flag = 0x00;
	cm35i2c_device->int_queue_missed = 0;
	cm35i2c_device->int_queue_count = 0;
	cm35i2c_device->int_queue_in_marker = 0;
	cm35i2c_device->int_queue_out_marker = 0;
}
static void
cm35i2c_init_device_list(struct cm35i2c_device_descriptor *cm35i2c_device)
{
	init_waitqueue_head(&(cm35i2c_device->int_wait_queue));
	INIT_LIST_HEAD(&(cm35i2c_device->dma_descr_list));
}



/******************************************************************************
Write to a standard PCI region
 ******************************************************************************/
static void
cm35i2c_region_write(const struct cm35i2c_device_descriptor *cm35i2c_device,
			struct cm35i2c_pci_access_request *pci_request,
			unsigned long address)
{

	/*###########################################################
	   Determine whether the region is memory or I/O mapped
	   ####################################################### */

	if (cm35i2c_device->pci[pci_request->region].virt_addr != NULL) {
		/*
		 * Region is memory mapped
		 */

		/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		   Determine how many bits are to be accessed
		   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */

		switch (pci_request->size) {
		case CM35I2C_PCI_REGION_ACCESS_8:
			IO_MEMORY_WRITE8(pci_request->data.data8,
					 (unsigned long *)address);
#ifdef CM35I2C_DEBUG
			printk(KERN_DEBUG "%s Write to address 0x%lx (0x%x) << 0x%x",
				cm35i2c_device->name,
				address,
				pci_request->offset,
				pci_request->data.data8);
#endif
			break;

		case CM35I2C_PCI_REGION_ACCESS_16:
			IO_MEMORY_WRITE16(pci_request->data.data16,
					  (unsigned long *)address);
#ifdef CM35I2C_DEBUG
			printk(KERN_DEBUG "%s Write to address 0x%lx (0x%x) << 0x%x",
				cm35i2c_device->name,
				address,
				pci_request->offset,
				pci_request->data.data16);
#endif
			break;

		case CM35I2C_PCI_REGION_ACCESS_32:
			IO_MEMORY_WRITE32(pci_request->data.data32,
					  (unsigned long *)address);
#ifdef CM35I2C_DEBUG
			printk(KERN_DEBUG "%s Write to address 0x%lx (0x%x) << 0x%x",
				cm35i2c_device->name,
				address,
				pci_request->offset,
				pci_request->data.data32);
#endif
			break;
		default:
			printk(KERN_ERR "Could not determine write access size (%d)",
				pci_request->size);
			break;
		}
	} else {

		/*
		 * Region is I/O mapped
		 */

		/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		   Determine how many bits are to be accessed
		   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */

		switch (pci_request->size) {
		case CM35I2C_PCI_REGION_ACCESS_8:
			outb(pci_request->data.data8, address);
			break;

		case CM35I2C_PCI_REGION_ACCESS_16:
			outw(pci_request->data.data16, address);
			break;

		case CM35I2C_PCI_REGION_ACCESS_32:
			outl(pci_request->data.data32, address);
			break;
		default:
			printk(KERN_ERR "Could not determine write access size (%d)",
				pci_request->size);
			break;
		}
	}
}


/******************************************************************************
Read from a standard PCI region
 ******************************************************************************/
static void
cm35i2c_region_read(const struct cm35i2c_device_descriptor *cm35i2c_device,
			struct cm35i2c_pci_access_request *pci_request,
			unsigned long address)
{
	/*############################################################
	   Determine whether the region is memory or I/O mapped
	######################################################### */

	if (cm35i2c_device->pci[pci_request->region].virt_addr != NULL) {

		/*
		 * Region is memory mapped
		 */
		/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		   Determine how many bits are to be accessed
		   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */

		switch (pci_request->size) {
		case CM35I2C_PCI_REGION_ACCESS_8:
			pci_request->data.data8 =
				IO_MEMORY_READ8((unsigned long *)address);

#ifdef CM35I2C_DEBUG
			printk(KERN_DEBUG "%s Read from address 0x%lx (0x%x) >> 0x%x",
				cm35i2c_device->name,
				address,
				pci_request->offset,
				pci_request->data.data8);
#endif
			break;

		case CM35I2C_PCI_REGION_ACCESS_16:
			pci_request->data.data16 =
				IO_MEMORY_READ16((unsigned long *)address);
#ifdef CM35I2C_DEBUG
			printk(KERN_DEBUG "%s Read from address 0x%lx (0x%x) >> 0x%x",
				cm35i2c_device->name,
				address,
				pci_request->offset,
				pci_request->data.data16);
#endif

			break;

		case CM35I2C_PCI_REGION_ACCESS_32:
			pci_request->data.data32 =
				IO_MEMORY_READ32((unsigned long *)address);
#ifdef CM35I2C_DEBUG
			printk(KERN_DEBUG "%s Read from address 0x%lx (0x%x) >> 0x%x",
				cm35i2c_device->name,
				address,
				pci_request->offset,
				pci_request->data.data32);
#endif

			break;
		default:
			printk(KERN_ERR "Could not determine read access size (%d)",
				pci_request->size);
			break;
		}
	} else {

		/*
		 * Region is I/O mapped
		 */

		/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		   Determine how many bits are to be accessed
		   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */

		switch (pci_request->size) {
		case CM35I2C_PCI_REGION_ACCESS_8:
			pci_request->data.data8 =
				(uint8_t) inb(address);
			break;

		case CM35I2C_PCI_REGION_ACCESS_16:
			pci_request->data.data16 =
				(uint16_t) inw(address);
			break;

		case CM35I2C_PCI_REGION_ACCESS_32:
			pci_request->data.data32 =
				(uint32_t) inl(address);
			break;
		default:
			printk(KERN_ERR "Could not determine read access size (%d)",
				pci_request->size);
			break;
		}
	}
}


/******************************************************************************
Access a standard PCI region
 ******************************************************************************/
static void
cm35i2c_access_pci_region(const struct cm35i2c_device_descriptor *cm35i2c_device,
				struct cm35i2c_pci_access_request *pci_request,
				enum cm35i2c_pci_region_access_dir direction)
{
	unsigned long address;

	/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	   Compute the address to be accessed
	   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

	address = pci_request->offset;

	if (cm35i2c_device->pci[pci_request->region].virt_addr != NULL) {
		address += (unsigned long)
			cm35i2c_device->pci[pci_request->region].virt_addr;
	} else {
		address += cm35i2c_device->pci[pci_request->region].io_addr;
	}

	/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	   Determine whether access is a read or write
	   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

	if (direction == CM35I2C_PCI_REGION_ACCESS_READ) {

		/*
		 * Region is to be read
		 */
		cm35i2c_region_read(cm35i2c_device,
					pci_request,
					address);


	} else {

		/*
		 * Region is to be written
		 */
		cm35i2c_region_write(cm35i2c_device,
					pci_request,
					address);

	}
}



/******************************************************************************
Validate an CM35I2C device descriptor
 ******************************************************************************/
static int
cm35i2c_validate_device(const struct cm35i2c_device_descriptor *cm35i2c_device)
{
	int device_idx = cm35i2c_device->device_index;

	// Check if descriptor is in place
	if (cm35i2c_devices[device_idx] == cm35i2c_device)
		return 0;

	printk(KERN_ERR "%s: Could not validate device descriptor.",
	   	cm35i2c_device->name);

	return -EBADFD;
}



/******************************************************************************
Validate user-space PCI region access
 ******************************************************************************/
static int
cm35i2c_validate_pci_access(const struct cm35i2c_device_descriptor * cm35i2c_device,
				const struct cm35i2c_pci_access_request *pci_request)
{
	uint16_t align_mask;
	uint8_t access_bytes;

	/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	   Validate the data size
	   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

	/*
	 * Verify the data size in bits.  Set the number of bytes being accessed;
	 * this is used to determine whether or not the region offset actually lies
	 * within the region.  Set the offset alignment bit mask; this is used to
	 * determine whether the region offset is suitably aligned for the access.
	 */

	switch (pci_request->size) {
	case CM35I2C_PCI_REGION_ACCESS_8:
		access_bytes = 1;
		align_mask = 0x0;
		break;

	case CM35I2C_PCI_REGION_ACCESS_16:
		access_bytes = 2;
		align_mask = 0x1;
		break;

	case CM35I2C_PCI_REGION_ACCESS_32:
		access_bytes = 4;
		align_mask = 0x3;
		break;

	default:
		printk(KERN_ERR "%s: Attempting to access memory with size %d.",
			cm35i2c_device->name, pci_request->size);
		return -EMSGSIZE;
		break;
	}

	/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	   Validate the PCI region
	   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

	switch (pci_request->region) {
	case CM35I2C_PCI_REGION_GBC:
	case CM35I2C_PCI_REGION_FB:
		break;

	default:
		return -EINVAL;
		break;
	}

	/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	   Validate the PCI region offset
	   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

	/*
	 * All bytes being accessed must lie entirely within the PCI region
	 */

	if (pci_request->offset >
		  (cm35i2c_device->pci[pci_request->region].length - access_bytes)) {
		printk(KERN_ERR "%s Request for PCI access outside of allowed range: "
				"Region: %d  Length: 0x%lx  Offset: 0x%x  Access bytes: %d.",
				cm35i2c_device->name, pci_request->region,
	      			cm35i2c_device->pci[pci_request->region].length,
				pci_request->offset,
				access_bytes);
		return -ERANGE;
	}

	/*
	 * Offset where access will occur must be suitably aligned for the size of
	 * access
	 */

	if (pci_request->offset & align_mask) {
		return -EOPNOTSUPP;
	}

	return 0;
}



/******************************************************************************
Validate DMA function
 ******************************************************************************/
static int
cm35i2c_validate_dma(const struct cm35i2c_device_descriptor *cm35i2c_device,
			const struct cm35i2c_ioctl_dma *dma_function)
{

	int result = 0;

	/**
	  * Validate DMA channel being request
	  */
	if (dma_function->channel < 0 ||
		dma_function->buffer < 0) {
		return -EINVAL;
	}

	switch (dma_function->function) {
	case CM35I2C_DMA_INITIALIZE:
		if ((dma_function->buffer_size <= 0) ||
		    (dma_function->buffer_size & 0x03) ||
		    (dma_function->buffer_size > CM35I2C_DMA_MAX_BUFFER_SIZE)) {
			    printk(KERN_ERR "%s: Invalid buffer size value (%d)",
			    	cm35i2c_device->name,
			    	dma_function->buffer_size);
			    return -EINVAL;
		}

		switch (dma_function->pci.region) {
		case CM35I2C_PCI_REGION_FB:
			break;

		default:
			printk(KERN_ERR "%s: Invalid PCI region (%d)",
				cm35i2c_device->name,
				dma_function->pci.region);
			return -EINVAL;
			break;
		}

		result = cm35i2c_validate_pci_access(cm35i2c_device,
							&(dma_function->pci));
		if (result != 0) {
			return result;
		}

		break;
	case CM35I2C_DMA_READ:
		/* break omitted */
	case CM35I2C_DMA_WRITE:
		if ((dma_function->buffer_size <= 0) ||
		    (dma_function->buffer_size & 0x03) ||
		    (dma_function->buffer_size > CM35I2C_DMA_MAX_BUFFER_SIZE)) {
			    printk(KERN_ERR "%s: Invalid buffer size value (%d)",
			    	cm35i2c_device->name,
			    	dma_function->buffer_size);
			    return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}


	return 0;
}




/******************************************************************************
Read from a PCI region
 ******************************************************************************/
static int
cm35i2c_pci_region_read(struct cm35i2c_device_descriptor * cm35i2c_device,
			   unsigned long ioctl_param)
{
	union cm35i2c_ioctl_argument ioctl_argument;
	int status;
	unsigned long irq_flags;

	/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	   Copy arguments in from user space and validate them
	   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

	if (copy_from_user(&ioctl_argument,
			   (union cm35i2c_ioctl_argument *) ioctl_param,
			   sizeof(union cm35i2c_ioctl_argument))) {
		return -EFAULT;
	}


	status = cm35i2c_validate_pci_access(cm35i2c_device,
					   &(ioctl_argument.readwrite.access));
	if (status != 0) {

		return status;
	}

	/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	   Do the actual read
	   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

	spin_lock_irqsave(&(cm35i2c_device->device_lock), irq_flags);
	cm35i2c_access_pci_region(cm35i2c_device,
				 &(ioctl_argument.readwrite.access),
				 CM35I2C_PCI_REGION_ACCESS_READ);

	spin_unlock_irqrestore(&(cm35i2c_device->device_lock), irq_flags);

	/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	   Copy results back to user space
	   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

	if (copy_to_user((union cm35i2c_ioctl_argument *) ioctl_param,
			 &ioctl_argument, sizeof(union cm35i2c_ioctl_argument))) {
		return -EFAULT;
	}

	return 0;
}


/******************************************************************************
Write to a PCI region
 ******************************************************************************/
static int
cm35i2c_pci_region_write(struct cm35i2c_device_descriptor * cm35i2c_device,
			unsigned long ioctl_param)
{
	union cm35i2c_ioctl_argument ioctl_argument;
	int status;
	unsigned long irq_flags;

	/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	   Copy arguments in from user space and validate them
	   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

	if (copy_from_user(&ioctl_argument,
			   (union cm35i2c_ioctl_argument *) ioctl_param,
			   sizeof(union cm35i2c_ioctl_argument))) {
		return -EFAULT;
	}

	status = cm35i2c_validate_pci_access(cm35i2c_device,
					   &(ioctl_argument.readwrite.access));
	if (status != 0) {
		return status;
	}

	/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	   Do the actual write
	   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
	spin_lock_irqsave(&(cm35i2c_device->device_lock), irq_flags);

	cm35i2c_access_pci_region(cm35i2c_device,
				 &(ioctl_argument.readwrite.access),
				 CM35I2C_PCI_REGION_ACCESS_WRITE);

	spin_unlock_irqrestore(&(cm35i2c_device->device_lock), irq_flags);

	return 0;
}




/******************************************************************************
Read from a PCI region, modify bits in the value, and write the new value back
to the region
 ******************************************************************************/
static int
cm35i2c_pci_region_modify(struct cm35i2c_device_descriptor *cm35i2c_device,
			 unsigned long ioctl_param)
{
	union cm35i2c_ioctl_argument ioctl_argument;
	struct cm35i2c_pci_access_request pci_request;
	int status;
	unsigned long irq_flags;

	/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	   Copy arguments in from user space and validate them
	   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

	if (copy_from_user(&ioctl_argument,
			   (union cm35i2c_ioctl_argument *) ioctl_param,
			   sizeof(union cm35i2c_ioctl_argument))) {
		return -EFAULT;
	}

	status = cm35i2c_validate_pci_access(cm35i2c_device,
					   &(ioctl_argument.modify.access));
	if (status != 0) {

		return status;
	}

	/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	   Do the actual read/modify/write
	   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

	/*
	 * Make a copy of user arguments to keep from overwriting them
	 */

	pci_request = ioctl_argument.modify.access;

	/*
	 * Read current value
	 */
	spin_lock_irqsave(&(cm35i2c_device->device_lock), irq_flags);
	cm35i2c_access_pci_region(cm35i2c_device, &pci_request,
				 CM35I2C_PCI_REGION_ACCESS_READ);

	/*
	 * Modify the value based upon mask
	 */

	switch (ioctl_argument.modify.access.size) {
	case CM35I2C_PCI_REGION_ACCESS_8:

		/*
		 * Preserve bits which are not to be changed and clear bits which
		 * will be changed
		 */

		pci_request.data.data8 &= ~ioctl_argument.modify.mask.mask8;

		/*
		 * Fold in the new value but don't allow bits to be set which are
		 * are not modifiable according to the mask
		 */

		pci_request.data.data8 |=
			(ioctl_argument.modify.access.data.data8 & ioctl_argument.
			 modify.mask.mask8);

		break;

	case CM35I2C_PCI_REGION_ACCESS_16:

		/*
		 * Preserve bits which are not to be changed and clear bits which
		 * will be changed
		 */

		pci_request.data.data16 &= ~ioctl_argument.modify.mask.mask16;

		/*
		 * Fold in the new value but don't allow bits to be set which are
		 * are not modifiable according to the mask
		 */

		pci_request.data.data16 |=
			(ioctl_argument.modify.access.data.data16 & ioctl_argument.
			 modify.mask.mask16);

		break;

	case CM35I2C_PCI_REGION_ACCESS_32:

		/*
		 * Preserve bits which are not to be changed and clear bits which
		 * will be changed
		 */

		pci_request.data.data32 &= ~ioctl_argument.modify.mask.mask32;

		/*
		 * Fold in the new value but don't allow bits to be set which are
		 * are not modifiable according to the mask
		 */

		pci_request.data.data32 |=
			(ioctl_argument.modify.access.data.data32 & ioctl_argument.
			 modify.mask.mask32);

		break;
	default:
		printk(KERN_ERR "Could not determine modify access size (%d)",
			ioctl_argument.modify.access.size);
		break;
	}

	/*
	 * Write new value
	 */

	cm35i2c_access_pci_region(cm35i2c_device, &pci_request,
				 CM35I2C_PCI_REGION_ACCESS_WRITE);

	spin_unlock_irqrestore(&(cm35i2c_device->device_lock), irq_flags);

	return 0;
}




/******************************************************************************
Pull the next interrupt off the queue (if there is one)
This function assumes the caller has a spinlock
*******************************************************************************/
static void
cm35i2c_dequeue_interrupt(struct cm35i2c_device_descriptor *cm35i2c_device,
			  int *int_fb,
			  int *int_available)
{

	*int_available = 0;

	/*
	 * If there is an interrupt in the queue then retrieve the data.
	 */
	if (cm35i2c_device->int_queue_count > 0) {

		/*
		 * Cache local copies of the interrupt status
		 */

		*int_fb = cm35i2c_device->interrupt_fb[cm35i2c_device->int_queue_out_marker];
		*int_available = 1;

		/*
		 * Make copy of the calculated number of interrupts in the queue and
		 * return this value -1 to signify how many more interrupts the
		 * reading device needs to receive
		 */

		cm35i2c_device->int_queue_count--;


		cm35i2c_device->int_queue_out_marker++;

		if (cm35i2c_device->int_queue_out_marker == CM35I2C_INT_QUEUE_SIZE) {

			/*
			 * wrap around if we have to
			 */

			cm35i2c_device->int_queue_out_marker = 0;

		}
#if defined(CM35I2C_DEBUG_INTERRUPTS)

		if (*int_fb < 0) {
			printk(KERN_DEBUG
				   "%s: Removing DMA interrupt: FB%d (Remaining: %d)\n",
				   cm35i2c_device->name,
				   ((*int_fb) & 0x7FFFFFFF),
				   cm35i2c_device->int_queue_count);
		} else {
			printk(KERN_DEBUG
				   "%s: Removing interrupt: FB%d (Remaining: %d)\n",
				   cm35i2c_device->name,
				   *int_fb,
				   cm35i2c_device->int_queue_count);
		}
#endif

	}

}



/******************************************************************************
Send interrupt status information back to user
 ******************************************************************************/
static int
cm35i2c_get_interrupt_info(struct cm35i2c_device_descriptor *cm35i2c_device,
			  unsigned long ioctl_param)
{
	int int_fb;
	unsigned long irq_flags;
	union cm35i2c_ioctl_argument ioctl_arg;
	int remaining = 0;
	int interrupt_available = 0;

	spin_lock_irqsave(&(cm35i2c_device->device_lock), irq_flags);

	cm35i2c_dequeue_interrupt(cm35i2c_device,
				  &int_fb,
				  &interrupt_available);

	remaining = cm35i2c_device->int_queue_count;

	spin_unlock_irqrestore(&(cm35i2c_device->device_lock), irq_flags);

	if (interrupt_available) {
		ioctl_arg.interrupt.valid_interrupt = 1;
		ioctl_arg.interrupt.interrupt_fb = int_fb;
		ioctl_arg.interrupt.error_occurred = 0;
		ioctl_arg.interrupt.interrupts_remaining = remaining;

	} else {
		printk(KERN_WARNING "%s: Attempted to get interrupt function "
		       "block, but none were in the queue.",
		       cm35i2c_device->name);

		ioctl_arg.interrupt.valid_interrupt = 0;
		ioctl_arg.interrupt.error_occurred = 1;

	}


	/*
	 * Copy the interrupt status back to user space
	 */
	if (copy_to_user((union cm35i2c_ioctl_argument *) ioctl_param,
			 &ioctl_arg,
			 sizeof(union cm35i2c_ioctl_argument))) {
		return -EFAULT;
	}

	return 0;
}



/******************************************************************************
Read from DMA (Copy DMA buffer to user space)
 ******************************************************************************/
static int
cm35i2c_dma_read(struct cm35i2c_device_descriptor *cm35i2c_device,
				struct cm35i2c_ioctl_dma *dma)
{


	struct list_head *cursor;
	int status = -ENXIO;

    	list_for_each(cursor, &(cm35i2c_device->dma_descr_list)) {
        	struct cm35i2c_dma_descriptor *list_item;

        	list_item = list_entry(cursor,
        				struct cm35i2c_dma_descriptor,
        				list);

        	if (list_item->fb_num == dma->fb_num &&
        		list_item->channel == dma->channel &&
        		list_item->buffer == dma->buffer) {

			status = copy_to_user(dma->buffer_ptr,
				list_item->virt_addr,
				dma->buffer_size);

			if (status != 0) {

			    printk(KERN_ERR "ERROR: DMA Read failed when copying to user space.");
			    return -EFAULT;
			}
#ifdef CM35I2C_DEBUG_DMA
			printk(KERN_DEBUG "%s: Reading DMA buffer for FB 0x%x, Channel %d, Buffer %d\n",
				cm35i2c_device->name,
				list_item->fb_num,
				list_item->channel,
				list_item->buffer);
#endif
		}

	}

	return status;

}


/******************************************************************************
Write to DMA (Copy DMA buffer from user space)
 ******************************************************************************/
static int
cm35i2c_dma_write(struct cm35i2c_device_descriptor *cm35i2c_device,
				struct cm35i2c_ioctl_dma *dma)
{

	struct list_head *cursor;
	int status = -ENXIO;


    	list_for_each(cursor, &(cm35i2c_device->dma_descr_list)) {
        	struct cm35i2c_dma_descriptor *list_item;

        	list_item = list_entry(cursor,
        				struct cm35i2c_dma_descriptor,
        				list);

        	if (list_item->fb_num == dma->fb_num &&
        		list_item->channel == dma->channel &&
        		list_item->buffer == dma->buffer) {

			status = copy_from_user(list_item->virt_addr,
						dma->buffer_ptr,
						dma->buffer_size);

			if (status != 0) {

			    printk(KERN_ERR "ERROR: DMA Write failed copying "
			    			"data from user space\n");
			    return -EFAULT;
			}
#ifdef CM35I2C_DEBUG_DMA
			printk(KERN_DEBUG "%s: Writing to DMA buffer for FB 0x%x, Channel %d, Buffer %d\n",
				cm35i2c_device->name,
				list_item->fb_num,
				list_item->channel,
				list_item->buffer);
#endif

		}

	}

	return status;

}


/******************************************************************************
Initialize DMA buffer areas
 ******************************************************************************/
static int
cm35i2c_dma_initialize(struct cm35i2c_device_descriptor *cm35i2c_device,
				struct cm35i2c_ioctl_dma *dma)
{

	dma_addr_t bus_address;
	void *virtual_address;
	struct list_head *cursor;
	struct cm35i2c_dma_descriptor *dma_descriptor;

	int already_allocated = 0;

	/**
	 * Check that we haven't already allocated a buffer for
	 * this function block and channel.
	 */
    	list_for_each(cursor, &(cm35i2c_device->dma_descr_list)) {
        	struct cm35i2c_dma_descriptor *list_item;

        	list_item = list_entry(cursor,
        				struct cm35i2c_dma_descriptor,
        				list);

        	if (list_item->fb_num == dma->fb_num &&
        		list_item->channel == dma->channel &&
        		list_item->buffer == dma->buffer) {
				already_allocated = 1;
		}

	}

	if (already_allocated) {
		printk(KERN_WARNING "%s: Tried to initialize an already allocated "
			"DMA buffer.  Func block: %u, Channel: %d.\n",
			cm35i2c_device->name,
			dma->fb_num,
			dma->channel);
		return -EBUSY;
	}
	
	virtual_address = dma_alloc_coherent(&(cm35i2c_device->pdev->dev), dma->buffer_size,
					&bus_address,
					GFP_KERNEL);

	if (virtual_address == NULL) {
		return -ENOMEM;
	}

	/* Write DMA bus address to the 64-bit register on the board */
	dma->pci.data.data32 = (bus_address & 0xFFFFFFFF);
	cm35i2c_access_pci_region(cm35i2c_device,
					&(dma->pci),
					CM35I2C_PCI_REGION_ACCESS_WRITE);

	/* Write upper 4 bytes of the bus address as needed */
	if (sizeof(bus_address) > 4) {
		//printk(KERN_INFO "size of bus address is %lu", sizeof(bus_address));
		dma->pci.data.data32 = bus_address >> 32;
	} else {
		dma->pci.data.data32 = 0;
	}
	dma->pci.offset += 4;

	cm35i2c_access_pci_region(cm35i2c_device,
					&(dma->pci),
					CM35I2C_PCI_REGION_ACCESS_WRITE);


	dma_descriptor = kmalloc(sizeof(struct cm35i2c_dma_descriptor), GFP_KERNEL);

	if (dma_descriptor == NULL) {
		printk(KERN_WARNING "%s: Could not allocate memory for DMA descriptor\n",
			cm35i2c_device->name);
		dma_free_coherent(&(cm35i2c_device->pdev->dev), dma->buffer_size, virtual_address, bus_address);
		
		return -ENOMEM;

	}

	dma_descriptor->fb_num = dma->fb_num;
	dma_descriptor->channel = dma->channel;
	dma_descriptor->virt_addr = virtual_address;
	dma_descriptor->bus_addr = bus_address;
	dma_descriptor->buffer_size = dma->buffer_size;
	dma_descriptor->buffer = dma->buffer;

	list_add_tail(&(dma_descriptor->list), &(cm35i2c_device->dma_descr_list));

#ifdef CM35I2C_DEBUG_DMA
	printk(KERN_DEBUG "%s: Allocated DMA buffer for FB 0x%x, Channel %d, Buffer %d, Bus Addr %llx, Length: %d\n",
		cm35i2c_device->name,
		dma->fb_num,
		dma->channel,
		dma->buffer,
		bus_address,
		dma->buffer_size);
#endif

	return 0;
}


/******************************************************************************
 * Release DMA buffers
 ******************************************************************************/
static void
cm35i2c_dma_release(struct cm35i2c_device_descriptor *cm35i2c_device)
{
	unsigned long irq_flags;
	struct list_head *cursor;
	struct list_head *next;

	list_for_each_safe(cursor, next, &(cm35i2c_device->dma_descr_list)) {
		struct cm35i2c_dma_descriptor *dma_descr;

		/*
		 * Get address of containing structure of this list_head; the structure
		 * is a DMA buffer list item
		 */
		spin_lock_irqsave(&(cm35i2c_device->device_lock), irq_flags);
		dma_descr = list_entry(cursor, struct cm35i2c_dma_descriptor, list);

		/*
		 * Delete element from list to prevent duplicate memory frees
		 */
		list_del(cursor);
		spin_unlock_irqrestore(&(cm35i2c_device->device_lock), irq_flags);

		dma_free_coherent(&(cm35i2c_device->pdev->dev), dma_descr->buffer_size,
                		                dma_descr->virt_addr,
                               			dma_descr->bus_addr);

#ifdef CM35I2C_DEBUG_DMA

		printk(KERN_DEBUG "%s: Releasing DMA resources for FB 0x%x, Channel %d, Buffer %d\n",
			cm35i2c_device->name,
			dma_descr->fb_num,
			dma_descr->channel,
			dma_descr->buffer);

#endif
		kfree(dma_descr);

	}

}


/******************************************************************************
Process the DMA-related function
 ******************************************************************************/
static int
cm35i2c_dma_function(struct cm35i2c_device_descriptor *cm35i2c_device,
			  unsigned long ioctl_param)
{

	int status = 0;
	union cm35i2c_ioctl_argument ioctl_argument;

	/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	   Copy arguments in from user space and validate them
	   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

	if (copy_from_user(&ioctl_argument,
			   (union cm35i2c_ioctl_argument *) ioctl_param,
			   sizeof(union cm35i2c_ioctl_argument))) {
		return -EFAULT;
	}

	status = cm35i2c_validate_dma(cm35i2c_device,
					   &(ioctl_argument.dma));
	if (status != 0) {

		return status;
	}

	switch(ioctl_argument.dma.function) {
	case CM35I2C_DMA_INITIALIZE:
		status = cm35i2c_dma_initialize(cm35i2c_device,
						&(ioctl_argument.dma));
		break;
	case CM35I2C_DMA_READ:
		status = cm35i2c_dma_read(cm35i2c_device,
						&(ioctl_argument.dma));
		break;
	case CM35I2C_DMA_WRITE:
		status = cm35i2c_dma_write(cm35i2c_device,
						&(ioctl_argument.dma));
		break;
	default:
		break;

	}

	return status;

}


/******************************************************************************
Handle ioctl(2) system calls
 ******************************************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
static int cm35i2c_ioctl(struct inode *inode,
			  struct file *file,
			  unsigned int request_code,
			  unsigned long ioctl_param)
#else
static long cm35i2c_ioctl(struct file *file,
			  unsigned int request_code,
			  unsigned long ioctl_param)
#endif
{


	int result = 0;
	struct cm35i2c_device_descriptor *cm35i2c_device;

	//printk(KERN_INFO "%s: ioctl called, request code: %u, parameter: 0x%lx", DRIVER_NAME, request_code, ioctl_param);

	result = cm35i2c_validate_device(file->private_data);

	if (result != 0) {
		return result;
	}


	cm35i2c_device = (struct cm35i2c_device_descriptor *) file->private_data;


	switch (request_code) {

	case CM35I2C_IOCTL_REGION_READ:
		result = cm35i2c_pci_region_read(cm35i2c_device, ioctl_param);
		break;

	case CM35I2C_IOCTL_REGION_WRITE:
		result = cm35i2c_pci_region_write(cm35i2c_device, ioctl_param);
		break;

	case CM35I2C_IOCTL_REGION_MODIFY:
		result = cm35i2c_pci_region_modify(cm35i2c_device, ioctl_param);
		break;

	case CM35I2C_IOCTL_INTERRUPT_GET:
		result = cm35i2c_get_interrupt_info(cm35i2c_device, ioctl_param);
		break;

	case CM35I2C_IOCTL_DMA_FUNCTION:
		result = cm35i2c_dma_function(cm35i2c_device, ioctl_param);
		break;

	case CM35I2C_IOCTL_WAKEUP:
	{
		unsigned long irq_flags;
		spin_lock_irqsave(&(cm35i2c_device->device_lock),
		      irq_flags);
		cm35i2c_device->remove_isr_flag = 0xFF;
		spin_unlock_irqrestore(&(cm35i2c_device->device_lock),
			   irq_flags);
		wake_up_interruptible(&(cm35i2c_device->int_wait_queue));
		result = 0;
		break;
	}
	case CM35I2C_IOCTL_GET_DEVICE_ID:
		result = copy_to_user((unsigned int *) ioctl_param,
			 &cm35i2c_device->device_id, sizeof(unsigned int));
		break;

	default:

		result = -ENOTTY;
		break;


	}


	return result;

}




/******************************************************************************
 Add an interrupt to the interrupt queue
 This function assumes the caller has a spinlock
 ******************************************************************************/
static void
cm35i2c_int_queue_add(struct cm35i2c_device_descriptor *cm35i2c_device,
			 int func_block_num)
{

	/*
	 * This is where the information is added to the queue if there is room
	 * otherwise we indicate queue overflow and log a missed interrupt
	 */

	if (cm35i2c_device->int_queue_count < CM35I2C_INT_QUEUE_SIZE) {
		/*
		 * Collect interrupt data and store in the device structure
		 */
		cm35i2c_device->interrupt_fb[cm35i2c_device->int_queue_in_marker] =
			func_block_num;

		cm35i2c_device->int_queue_in_marker++;

		if (cm35i2c_device->int_queue_in_marker == (CM35I2C_INT_QUEUE_SIZE)) {
			/*
			 * Wrap around to the front of the queue
			 */
			cm35i2c_device->int_queue_in_marker = 0;

		}

		cm35i2c_device->int_queue_count++;
#if defined(CM35I2C_DEBUG_INTERRUPTS)

		if (func_block_num < 0) {

			printk(KERN_DEBUG "%s: Adding DMA interrupt: FB%d (Count now: %d)\n",
				   cm35i2c_device->name, (func_block_num & 0x7FFFFFFF),
				   cm35i2c_device->int_queue_count);
		} else {
			   printk(KERN_DEBUG "%s: Adding interrupt: FB%d (Count now: %d)\n",
			   cm35i2c_device->name, func_block_num,
			   cm35i2c_device->int_queue_count);
		}
#endif

	} else {
		/*
		 * Indicate interrupt status queue overflow
		 */
		printk(KERN_WARNING
			   "%s: WARNING: Missed interrupt info because queue is full\n",
			   cm35i2c_device->name);

		cm35i2c_device->int_queue_missed++;

	}
}



/******************************************************************************
Put an interrupt in the queue for every bit in the interrupt status
This function assumes the caller has a spinlock
*******************************************************************************/
static int
cm35i2c_process_interrupt_status(struct cm35i2c_device_descriptor *cm35i2c_device)
{
	struct cm35i2c_pci_access_request pci_request;
	int fb_num;
	uint32_t fb_mask = 1;
	uint32_t irq_status_register = 0;
	uint32_t dma_irq_status_register = 0;
	uint32_t fb_clear_mask0 = 0;
	uint32_t fb_clear_mask1 = 0;
	int num_ints_processed = 0;

	/**
	 * Read the lower 32 bits of the IRQ and DMA IRQ registers
	 */
	pci_request.region = CM35I2C_PCI_REGION_GBC;
	pci_request.offset = CM35I2C_OFFSET_GBC_IRQ_STATUS;
	pci_request.size = CM35I2C_PCI_REGION_ACCESS_32;
	cm35i2c_access_pci_region(cm35i2c_device, &pci_request,
				 CM35I2C_PCI_REGION_ACCESS_READ);
	irq_status_register = pci_request.data.data32;

	pci_request.offset = CM35I2C_OFFSET_GBC_DMA_IRQ_STATUS;
	pci_request.size = CM35I2C_PCI_REGION_ACCESS_32;
	cm35i2c_access_pci_region(cm35i2c_device, &pci_request,
				 CM35I2C_PCI_REGION_ACCESS_READ);
	dma_irq_status_register = pci_request.data.data32;

#if defined(CM35I2C_DEBUG_INTERRUPTS)

		printk(KERN_DEBUG "%s: IRQ Status (lower 32): 0x%x  "
				  "DMA_IRQ Status (lower 32): 0x%x\n",
			   cm35i2c_device->name, irq_status_register,
			   dma_irq_status_register);
#endif

	if (irq_status_register || dma_irq_status_register) {

		for (fb_num = 0; fb_num < 32; fb_num ++) {

			if ((irq_status_register & fb_mask) ||
				(dma_irq_status_register & fb_mask)) {

				if (dma_irq_status_register & fb_mask) {

					cm35i2c_int_queue_add(cm35i2c_device,
								0x80000000 | fb_num);
				}
				if (irq_status_register & fb_mask) {

					cm35i2c_int_queue_add(cm35i2c_device,
								fb_num);
				}

				num_ints_processed++;
				/* Create our mask to clear the */
				/* status register when done */
				fb_clear_mask0 |= fb_mask;
			}

			fb_mask *= 2;

		}

	}


	/**
	 * Read the upper 32 bits of the IRQ and DMA IRQ registers
	 */
	pci_request.offset = CM35I2C_OFFSET_GBC_IRQ_STATUS + 4;
	cm35i2c_access_pci_region(cm35i2c_device, &pci_request,
				 CM35I2C_PCI_REGION_ACCESS_READ);
	irq_status_register = pci_request.data.data32;

	pci_request.offset = CM35I2C_OFFSET_GBC_DMA_IRQ_STATUS + 4;
	cm35i2c_access_pci_region(cm35i2c_device, &pci_request,
				 CM35I2C_PCI_REGION_ACCESS_READ);
	dma_irq_status_register = pci_request.data.data32;


#if defined(CM35I2C_DEBUG_INTERRUPTS)

		printk(KERN_DEBUG "%s: IRQ Status (upper 32): 0x%x  DMA_IRQ Status (upper 32): 0x%x\n",
			   cm35i2c_device->name, irq_status_register, dma_irq_status_register);
#endif

	/*
	 * Mask off the reserved bits before any processing
	 */
	irq_status_register &= 0x0FFFFFFF;
	dma_irq_status_register &= 0x0FFFFFFF;

	fb_mask = 1;
	if (irq_status_register || dma_irq_status_register) {

		for (fb_num = 0; fb_num < 28; fb_num ++) {

			if ((irq_status_register & fb_mask) ||
				(dma_irq_status_register & fb_mask)) {
				if (dma_irq_status_register & fb_mask) {
					cm35i2c_int_queue_add(cm35i2c_device,
								0x80000000 | (fb_num + 32));

				}

				if (irq_status_register & fb_mask) {

					cm35i2c_int_queue_add(cm35i2c_device,
								(fb_num + 32));
				}

				num_ints_processed++;

				/* Create our mask to clear the */
				/* status register when done */
				fb_clear_mask1 |= fb_mask;
			}

			fb_mask *= 2;

		}

	}

	/*
	 * Clear the status registers, if required
	 */

	if (fb_clear_mask0 != 0) {
		pci_request.offset = CM35I2C_OFFSET_GBC_IRQ_STATUS;
		pci_request.data.data32 = fb_clear_mask0;
		cm35i2c_access_pci_region(cm35i2c_device, &pci_request,
				 CM35I2C_PCI_REGION_ACCESS_WRITE);

		pci_request.offset = CM35I2C_OFFSET_GBC_DMA_IRQ_STATUS;
		cm35i2c_access_pci_region(cm35i2c_device, &pci_request,
				 CM35I2C_PCI_REGION_ACCESS_WRITE);

	}

	if (fb_clear_mask1 != 0) {
		pci_request.data.data32 = fb_clear_mask1;
		pci_request.offset = CM35I2C_OFFSET_GBC_IRQ_STATUS + 4;
		cm35i2c_access_pci_region(cm35i2c_device, &pci_request,
				 CM35I2C_PCI_REGION_ACCESS_WRITE);
		pci_request.offset = CM35I2C_OFFSET_GBC_DMA_IRQ_STATUS + 4;
		cm35i2c_access_pci_region(cm35i2c_device, &pci_request,
				 CM35I2C_PCI_REGION_ACCESS_WRITE);

	}

	return num_ints_processed;
}


/******************************************************************************
CM35I2C device hardware initialization
 ******************************************************************************/
static void
cm35i2c_initialize_hardware(const struct cm35i2c_device_descriptor *cm35i2c_device)
{

	struct cm35i2c_pci_access_request pci_request;

	/*
	 * Reset the board
	 */
	pci_request.region = CM35I2C_PCI_REGION_GBC;
	pci_request.offset = CM35I2C_OFFSET_GBC_BOARD_RESET;
	pci_request.size = CM35I2C_PCI_REGION_ACCESS_8;
	pci_request.data.data8 = CM35I2C_BOARD_RESET_VALUE;

	cm35i2c_access_pci_region(cm35i2c_device, &pci_request,
				 CM35I2C_PCI_REGION_ACCESS_WRITE);

}



/******************************************************************************
CM35I2C interrupt handler
 ******************************************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
static irqreturn_t cm35i2c_interrupt_handler(int irq_number, void *device_id,
						struct pt_regs *registers)

#else
static irqreturn_t cm35i2c_interrupt_handler(int irq_number, void *device_id)
#endif
{
	struct cm35i2c_device_descriptor *cm35i2c_device;

	int result = 0;
	int interrupts_processed = 0;

	cm35i2c_device = (struct cm35i2c_device_descriptor *) device_id;

	/**
	 * Verify that the device ID passed is one of ours
	 */
	result = cm35i2c_validate_device(cm35i2c_device);

	if (result != 0) {

		printk(KERN_ERR
			"%s: ERROR: Invalid device descriptor in interrupt\n",
			cm35i2c_device->name);
		return IRQ_NONE;
	}


	spin_lock(&(cm35i2c_device->device_lock));

	/*
	 * Verify this IRQ number is ours
	 */
	if (irq_number != cm35i2c_device->irq_number) {


		printk(KERN_ERR
			"%s: ERROR: IRQ passed (%d) to handler "
			"was not device IRQ (%d)\n",
			cm35i2c_device->name,
			irq_number,
			cm35i2c_device->irq_number);
		spin_unlock(&(cm35i2c_device->device_lock));
		return IRQ_NONE;
	}

	if (cm35i2c_device->int_queue_missed > TOO_MANY_MISSED_IRQ) {
		printk(KERN_EMERG "%s: Missed %d interrupts due to full queue.  Resetting board.",
		 	cm35i2c_device->name, TOO_MANY_MISSED_IRQ);
		 cm35i2c_initialize_hardware(cm35i2c_device);

		 spin_unlock(&(cm35i2c_device->device_lock));
		 return IRQ_HANDLED;
	 }

	interrupts_processed = cm35i2c_process_interrupt_status(cm35i2c_device);

	/* No interrupts found?  Must be someone else's IRQ */
	if (interrupts_processed == 0) {
		spin_unlock(&(cm35i2c_device->device_lock));

		return IRQ_NONE;
	}

	spin_unlock(&(cm35i2c_device->device_lock));

	wake_up_interruptible(&(cm35i2c_device->int_wait_queue));
#if (defined(CM35I2C_DEBUG) || defined(CM35I2C_DEBUG_INTERRUPTS))
	printk(KERN_INFO "%s Interrupt Handled\n", cm35i2c_device->name);
#endif
	return IRQ_HANDLED;

}


/******************************************************************************
Release region resources
 ******************************************************************************/
static void
cm35i2c_release_region_resources(struct cm35i2c_device_descriptor *cm35i2c_device)
{
	int region;

	for (region = 0; region < CM35I2C_PCI_NUM_REGIONS; region++) {

		/*
		 * Determine how region is mapped
		 */
		if (cm35i2c_device->pci[region].virt_addr != NULL) {

			/*
			 * Region is memory-mapped
			 */

			/*
			 * If memory range allocation succeeded, free the range
			 */

			if (cm35i2c_device->pci[region].allocated != 0x00) {
				release_mem_region(cm35i2c_device->pci[region].phys_addr,
						   cm35i2c_device->pci[region].length);

				printk(KERN_INFO
					   "%s: Released I/O memory range %#lx-%#lx\n",
					   &((cm35i2c_device->name)[0]), (unsigned long)
					   cm35i2c_device->pci[region].phys_addr,
					   ((unsigned long)
					cm35i2c_device->pci[region].phys_addr +
					cm35i2c_device->pci[region].length - 1));
			}

			/*
			 * Unmap region from kernel's address space
			 */

			iounmap(cm35i2c_device->pci[region].virt_addr);

			printk(KERN_INFO
				   "%s: Unmapped kernel mapping at %#lx\n",
				   &((cm35i2c_device->name)[0]),
				   (unsigned long)
				   cm35i2c_device->pci[region].virt_addr);
		} else if (cm35i2c_device->pci[region].io_addr != 0) {

			/*
			 * Region is I/O-mapped
			 */

			/*
			 * Free I/O port range
			 */

			release_region(cm35i2c_device->pci[region].
					   phys_addr,
					   cm35i2c_device->pci[region].
					   length);

			printk(KERN_INFO
				   "%s: Released I/O port range %#lx-%#lx\n",
				   &((cm35i2c_device->name)[0]),
				   (unsigned long)
				   cm35i2c_device->pci[region].phys_addr,
				   ((unsigned long)
				cm35i2c_device->pci[region].phys_addr +
				cm35i2c_device->pci[region].length - 1));
		}
	}


}


/******************************************************************************
Release resources allocated by driver
 ******************************************************************************/

static void cm35i2c_release_resources(struct cm35i2c_device_descriptor * cm35i2c_device)
{
	/*
	 * Free any allocated IRQ
	 */

	if (cm35i2c_device->irq_number != 0) {
		free_irq(cm35i2c_device->irq_number, cm35i2c_device);
		printk(KERN_INFO "%s: Freed IRQ %u\n",
				&((cm35i2c_device->name)[0]),
				cm35i2c_device->irq_number);
	}

	/*
  	 * Free any resources allocated for the PCI regions
	 */

	cm35i2c_release_region_resources(cm35i2c_device);
}


/******************************************************************************
Set up standard PCI regions
 ******************************************************************************/
static int
cm35i2c_process_pci_regions(struct cm35i2c_device_descriptor * cm35i2c_device,
			   struct pci_dev *pci_device)
{
	uint8_t region;

	/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	   Process each standard PCI region
	   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

	for (region = 0; region < CM35I2C_PCI_NUM_REGIONS; region++) {
		unsigned long address = 0;
		unsigned long length;
		unsigned long flags;

		/*#####################################################################
		   Get region's physical address and length in bytes.  If either is zero,
		   the region is unused and should be ignored.
		   ################################################################## */

		address = pci_resource_start(pci_device, region);
		if (address == 0) {
			continue;
		}

		length = pci_resource_len(pci_device, region);
		if (length == 0) {
			continue;
		}

		/*#####################################################################
		   Save information in PCI region descriptor
		   ################################################################## */

		cm35i2c_device->pci[region].phys_addr = address;
		cm35i2c_device->pci[region].length = length;

		/*#####################################################################
		   Determine how the region is mapped
		   ################################################################## */

		flags = pci_resource_flags(pci_device, region);

		if (flags & IORESOURCE_IO) {

			/*
			 * The region is I/O mapped
			 */

			/*
			 * Allocate the I/O port range
			 */

			if (request_region
				(address, length, &((cm35i2c_device->name)[0])) == NULL) {
				printk(KERN_ERR
					   "%s: ERROR: I/O port range %#lx-%#lx allocation FAILED\n",
					   &((cm35i2c_device->name)[0]),
					   address, (address + length - 1)
					);
				cm35i2c_release_resources(cm35i2c_device);
				return -EBUSY;
			}

			cm35i2c_device->pci[region].io_addr = address;

			printk(KERN_INFO
				   "%s: Allocated I/O port range %#lx-%#lx\n",
				   &((cm35i2c_device->name)[0]), address,
				   (address + length - 1)
				);
		} else if (flags & IORESOURCE_MEM) {

			/*
			 * The region is memory mapped
			 */

			/*
			 * Remap the region's physical address into the kernel's virtual
			 * address space and allocate the memory range
			 */

			cm35i2c_device->pci[region].virt_addr =
				ioremap(address, length);
				//pci_iomap(pci_device,region, length); 
			if (cm35i2c_device->pci[region].virt_addr == NULL) {
				printk(KERN_ERR
					   "%s: ERROR: BAR%u remapping FAILED\n",
					   &((cm35i2c_device->name)[0]),
					   region);
				cm35i2c_release_resources(cm35i2c_device);
				return -ENOMEM;
			}

			if (request_mem_region
				(address, length, &((cm35i2c_device->name)[0])
				)
				== NULL) {
				printk(KERN_ERR
					   "%s: ERROR: I/O memory range %#lx-%#lx allocation FAILED\n",
					   &((cm35i2c_device->name)[0]),
					   address, (address + length - 1)
					);
				cm35i2c_release_resources(cm35i2c_device);
				return -EBUSY;
			}

			cm35i2c_device->pci[region].allocated = 0xFF;

			printk(KERN_INFO
				   "%s: Allocated I/O memory range %#lx-%#lx\n",
				   &((cm35i2c_device->name)[0]), address,
				   (address + length - 1)
				);
		} else {

			/*
			 * The region has invalid resource flags
			 */

			printk(KERN_ERR "%s: ERROR: Invalid PCI region flags\n",
				   &((cm35i2c_device->name)[0])
				);
			cm35i2c_release_resources(cm35i2c_device);
			return -EIO;
		}

		/*#####################################################################
		   Print information about the region
		   ################################################################## */

		printk(KERN_INFO "%s: BAR%u Region:\n",
			   &((cm35i2c_device->name)[0]), region);

		if (cm35i2c_device->pci[region].io_addr != 0) {
			printk(KERN_INFO "	Address: %#lx (I/O mapped)\n",
				   cm35i2c_device->pci[region].io_addr);
		} else {
			printk(KERN_INFO "	Address: %#lx (memory mapped)\n",
				   (unsigned long)cm35i2c_device->pci[region].
				   virt_addr);
			printk(KERN_INFO "	Address: %#lx (physical)\n",
				   cm35i2c_device->pci[region].phys_addr);
		}

		printk(KERN_INFO "	Length:  %#lx\n",
			   cm35i2c_device->pci[region].length);
	}

	return 0;
}


/******************************************************************************
 IRQ line allocation
 ******************************************************************************/
static int
cm35i2c_allocate_irq(struct cm35i2c_device_descriptor *cm35i2c_device,
			const struct pci_dev *pci_device)
{
	int status;

	/*
	 * The fourth request_irq() argument MUST refer to memory which will remain
	 * valid until the driver is unloaded.  request_irq() simply stores this
	 * address in a structure rather than making a copy of the string it points
	 * to.
	 */
	cm35i2c_device->irq_number = pci_device->irq;
	status = request_irq(pci_device->irq,
				(irq_handler_t) cm35i2c_interrupt_handler,
				 IRQF_SHARED,
				 cm35i2c_device->name,
				 (void *) cm35i2c_device);
	if (status != 0) {
		printk(KERN_ERR
			   "%s: ERROR: Unable to allocate IRQ %u (error = %u)\n",
			   &((cm35i2c_device->name)[0]), pci_device->irq,
			   -status);
		cm35i2c_device->irq_number = 0;
		cm35i2c_release_resources(cm35i2c_device);
		return status;
	}

	printk(KERN_INFO "%s: Allocated IRQ %u\n",
		   &((cm35i2c_device->name)[0]), pci_device->irq);

	return 0;
}


/******************************************************************************
Determine whether or not a device is readable
 ******************************************************************************/
static unsigned int
cm35i2c_poll(struct file *file, struct poll_table_struct *poll_table)
{
	struct cm35i2c_device_descriptor *cm35i2c_device;
	unsigned int interrupts_in_queue;
	unsigned int status_mask = 0;
	unsigned long irq_flags;

	/*
	 * If we don't have a valid CM35I2C device descriptor, no status is available
	 */

	if (cm35i2c_validate_device(file->private_data) != 0) {

		/*
		 * This value causes select(2) to indicate that a file descriptor is
		 * present in its file descriptor sets but it will be in the exception
		 * set rather than in the input set.
		 */

		return POLLPRI;
	}

	cm35i2c_device = (struct cm35i2c_device_descriptor *) file->private_data;

	/*
	 * Register with the file system layer so that it can wait on and check for
	 * CM35I2C events
	 */

	poll_wait(file, &(cm35i2c_device->int_wait_queue), poll_table);

	/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	   Waiting is done interruptibly, which means that a signal could have been
	   delivered.  Thus we might have been woken up by a signal before an
	   interrupt occurred.  Therefore, the process needs to examine the device's
	   interrupt flag.
	   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

	/*
	 * Prevent a race condition with the interrupt handler and make a local copy
	 * of the interrupt count
	 */

	spin_lock_irqsave(&(cm35i2c_device->device_lock), irq_flags);

	interrupts_in_queue = cm35i2c_device->int_queue_count;

	if (cm35i2c_device->remove_isr_flag) {
		status_mask = (POLLIN | POLLRDNORM);
	}

	spin_unlock_irqrestore(&(cm35i2c_device->device_lock), irq_flags);

	/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	   Interpret interrupt flag
	   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

	/*
	 * The flag is cleared after reading.  See if it is clear or not.
	 */

	if (interrupts_in_queue > 0) {

		status_mask |= (POLLIN | POLLRDNORM);

	}

	return status_mask;
}


/****
Probe
*/
static int
cm35i2c_probe(struct pci_dev *pci_device, const struct pci_device_id *id)
{
	struct cm35i2c_device_descriptor *cm35i2c_device;
	struct cm35i2c_pci_access_request pci_request;
	int err, name_len;
	int dev_idx;
	int minor_number = -1;
	char dev_file_name[32];
	struct device *dev = NULL;
	dev_t devno;


	printk(KERN_INFO "%s: Probing 0x%.4x:0x%.4x\n",
		DRIVER_NAME,
	    pci_device->vendor, pci_device->device);


	printk(KERN_INFO "%s: CM35I2C found at bus %u, slot %02X, function %02X\n",
		DRIVER_NAME,
		pci_device->bus->number,
		PCI_SLOT(pci_device->devfn),
		PCI_FUNC(pci_device->devfn)
	);

	// Find a free device index
	for (dev_idx = 0; dev_idx < CM35I2C_MAX_NUM_DEVICES; ++dev_idx) {
		if (cm35i2c_devices[dev_idx] == NULL) {
			break;
		}
	}

	// No free devices found
	if (dev_idx >= CM35I2C_MAX_NUM_DEVICES) {
		printk(KERN_ERR "%s: Too many devices\n", DRIVER_NAME);
		return -ENOMEM;
	}

	// Calculate minor number
	minor_number = CM35I2C_NUM_MINORS_PER_DEVICE * dev_idx;

	printk(KERN_INFO "%s: Device is assigned the index: %d, starting minor: %d\n",
		DRIVER_NAME,
		dev_idx,
		minor_number
	);

	// Allocate memory for the devices descriptor
	cm35i2c_device = kmalloc(sizeof(struct cm35i2c_device_descriptor), GFP_KERNEL);
	if (cm35i2c_device == NULL) {
		printk(KERN_ERR
			   "%s: ERROR: Device descriptor memory allocation FAILED\n",
			   DRIVER_NAME);
		return -ENOMEM;
	}
	memset(cm35i2c_device, 0, sizeof(struct cm35i2c_device_descriptor));

	// Store the device descriptor pointer
	cm35i2c_devices[dev_idx] = cm35i2c_device;


	// Initialize the spin lock
	spin_lock_init(&(cm35i2c_device->device_lock));
	// Initilaize the device descriptor
	cm35i2c_reset_device_desc(cm35i2c_device);
	cm35i2c_init_device_list(cm35i2c_device);
	// Store device index in descriptor
	cm35i2c_device->device_index = dev_idx;
	// Assign PCI device to the descriptor
	cm35i2c_device->pdev = pci_device;
	// Assign PCI device id to the descriptor
	cm35i2c_device->device_id = id->device;

	/*
	 * Create the full device name
	 */
	name_len = snprintf(&((cm35i2c_device->name)[0]),
			  CM35I2C_NAME_LENGTH,
			  "%s-%u", DRIVER_NAME, minor_number);
	if (name_len >= CM35I2C_NAME_LENGTH) {
		printk(KERN_ERR
			   "%s-%u> ERROR: Device name creation FAILED\n",
			   DRIVER_NAME, minor_number);
		cm35i2c_release_resources(cm35i2c_device);
		return -ENAMETOOLONG;
	}

	/*
	 * Initialize character device
	 */
	cdev_init(&(cm35i2c_device->cdev), &cm35i2c_file_ops);
	cm35i2c_device->cdev.owner = THIS_MODULE;
	cm35i2c_device->cdev.ops = &cm35i2c_file_ops;

	/*
	 * Enable the PCI device
	 */
	err = pci_enable_device(pci_device);
	if (err) {
		printk(KERN_ERR "%s: Error attempting to enable PCI device.\n", cm35i2c_device->name);
		cm35i2c_release_resources(cm35i2c_device);
		return err;
	}

	/*
 	 * Determine 1) how many standard PCI regions are present, 2) how the
	 * regions are mapped, and 3) how many bytes are in each region.  Also,
	 * remap any memory-mapped region into the kernel's address space.
	 */
	err = cm35i2c_process_pci_regions(cm35i2c_device, pci_device);
	if (err) {
		printk(KERN_ERR "%s: Error processing PCI regions.\n", cm35i2c_device->name);
		cm35i2c_release_resources(cm35i2c_device);
		return err;
	}

	/*
	 * Associate device IRQ line with device in kernel
	 */
	err = cm35i2c_allocate_irq(cm35i2c_device, pci_device);
	if (err) {
		printk(KERN_ERR "%s: Error allocating IRQ.\n", cm35i2c_device->name);
		cm35i2c_release_resources(cm35i2c_device);
		pci_dev_put(pci_device);
		return err;
	}

	/* Add character device */
	devno = MKDEV(cm35i2c_major, cm35i2c_device->device_index * CM35I2C_NUM_MINORS_PER_DEVICE);
	err = cdev_add(&(cm35i2c_device->cdev), devno,
			  CM35I2C_NUM_MINORS_PER_DEVICE);
	/* Check if the character device was added successfully */
	if (err) {
		dev_err(&pci_device->dev, "Error adding cdev\n");
		err = -ENODEV;
		cm35i2c_release_resources(cm35i2c_device);
		return err;
	}

	/* Assemble device file name */
	sprintf(dev_file_name, "%s-%u", DRIVER_NAME, dev_idx);
	/* Trigger creation of the device file */
	dev = device_create(dev_class,
			NULL,
			devno,
			NULL,
			dev_file_name
	);

	if (dev == NULL) {
		printk(KERN_ERR "%s: Failed to create device file: %s",
			DRIVER_NAME, dev_file_name);
		cm35i2c_release_resources(cm35i2c_device);
		return -ENODEV;
	}


	/*
	 * Read and print FPGA version information
	 */
	pci_request.region = CM35I2C_PCI_REGION_GBC;
	pci_request.offset = CM35I2C_OFFSET_GBC_FPGA_BUILD;
	pci_request.size = CM35I2C_PCI_REGION_ACCESS_32;

	cm35i2c_access_pci_region(cm35i2c_device, &pci_request,
				 CM35I2C_PCI_REGION_ACCESS_READ);

	printk(KERN_INFO "%s: FPGA version: %u\n",
		   cm35i2c_device->name,
		   (uint32_t) pci_request.data.data32);

	cm35i2c_initialize_hardware(cm35i2c_device);

	/*
	 * Make sure the pci_device has bus mastering capabilities for DMA
	 */
	pci_set_master(pci_device);

	/* store the device pointer in the drvdata for remove call */
	pci_set_drvdata(pci_device, cm35i2c_device);

	return 0;
}

/****
Remove
*/
static void cm35i2c_remove(struct pci_dev *pci_device)
{
	struct cm35i2c_device_descriptor *cm35i2c_device;
	int dev_idx, minor;

	printk(KERN_INFO "%s: Removing 0x%.4x:0x%.4x\n",
		DRIVER_NAME,
	    pci_device->vendor, pci_device->device
	);

	// Retrieve device pointer
	cm35i2c_device = pci_get_drvdata(pci_device);
	// Check device pointer
	if (cm35i2c_device == NULL) {
		printk(KERN_ERR "%s: Remove called for invalid device", 
			DRIVER_NAME
		);
	}

	dev_idx = cm35i2c_device->device_index;
	printk(KERN_INFO "%s Remove for device index %d\n", DRIVER_NAME, dev_idx);

	// Remove the driver data from the PCI device
	pci_set_drvdata(pci_device, NULL);

	/*
	 * Free any allocated IRQ
	 */
	if (cm35i2c_device->irq_number != 0) {
		free_irq(cm35i2c_device->irq_number, cm35i2c_device);
		printk(KERN_INFO "%s: Freed IRQ %u\n",
				&((cm35i2c_device->name)[0]),
				cm35i2c_device->irq_number);
	}

	/*
	 * Free any resources allocated for the PCI regions
	 */
	cm35i2c_release_region_resources(cm35i2c_device);

	/*
	 * Remove Pci bus mastering Privledges
	 */ 
	pci_clear_master(pci_device);
	/*
	 * Delete the character device
	 */
	cdev_del(&(cm35i2c_device->cdev));

	/*
	 * Destroy the device
	 */
	minor = dev_idx * CM35I2C_NUM_MINORS_PER_DEVICE;
	device_destroy(dev_class, MKDEV(cm35i2c_major, minor));

	/*
	 * Free the allocated device descriptor and indicate free device by NULL
	 */
	kfree(cm35i2c_device);
	cm35i2c_devices[dev_idx] = NULL;
}


// PCI driver structure 
static struct pci_driver cm35i2c_pci_driver = {
	.name = "CM35I2C",
	.id_table = cm35i2c_pci_device_table,
	.probe = cm35i2c_probe,
	.remove = cm35i2c_remove,
};


/******************************************************************************
Perform a board reset
 ******************************************************************************/
#ifdef RESET_ON_CLOSE
static void cm35i2c_board_reset(struct cm35i2c_device_descriptor *cm35i2c_device) {

	struct cm35i2c_pci_access_request pci_request;
	/*
	 * Reset the board
	 */
	pci_request.region = CM35I2C_PCI_REGION_GBC;
	pci_request.offset = CM35I2C_OFFSET_GBC_BOARD_RESET;
	pci_request.size = CM35I2C_PCI_REGION_ACCESS_8;
	pci_request.data.data8 = CM35I2C_BOARD_RESET_VALUE;

	cm35i2c_access_pci_region(cm35i2c_device, &pci_request,
				 CM35I2C_PCI_REGION_ACCESS_WRITE);

}
#endif

/******************************************************************************
Open a CM35I2C device
 ******************************************************************************/
static int cm35i2c_open(struct inode *inode, struct file *file)
{

	struct cm35i2c_device_descriptor *cm35i2c_device;
	unsigned int minor_number;
	unsigned long irq_flags;
	int dev_idx;
	
	minor_number = iminor(inode);
	dev_idx = minor_number / CM35I2C_NUM_MINORS_PER_DEVICE;


	cm35i2c_device = cm35i2c_devices[dev_idx];
	
	spin_lock_irqsave(&(cm35i2c_device->device_lock), irq_flags);
	
	if (cm35i2c_device->reference_count) {
		spin_unlock_irqrestore(&(cm35i2c_device->device_lock), irq_flags);
		return -EBUSY;
	}
	cm35i2c_device->reference_count++;
	file->private_data = cm35i2c_device;
	spin_unlock_irqrestore(&(cm35i2c_device->device_lock), irq_flags);

	// Device descriptor needs to be reset to a known
	// State because it isn't cleaned on removal.	
	cm35i2c_reset_device_desc(cm35i2c_device);

	return 0;
}


/******************************************************************************
Close a CM35I2C device file
 ******************************************************************************/
static int cm35i2c_release(struct inode *inode, struct file *file)
{

	struct cm35i2c_device_descriptor *cm35i2c_device;
	unsigned long irq_flags;
	
	/*
	 * If we don't have a valid CM35I2C device descriptor, no status is available
	 */
	if (cm35i2c_validate_device(file->private_data) != 0) {
		return -EBADF;
	}

	cm35i2c_device = (struct cm35i2c_device_descriptor *) file->private_data;
#ifdef RESET_ON_CLOSE	
	cm35i2c_board_reset(cm35i2c_device);
#endif
	cm35i2c_dma_release(cm35i2c_device);
	spin_lock_irqsave(&(cm35i2c_device->device_lock), irq_flags);
	cm35i2c_device->reference_count--;
	file->private_data = NULL;
	spin_unlock_irqrestore(&(cm35i2c_device->device_lock), irq_flags);

	return 0;

}

/******************************************************************************
Initialization upon driver module load
 ******************************************************************************/
static int cm35i2c_init(void)
{
	dev_t device;
	int status;
	int i;

	printk(KERN_INFO "%s: Initializing module (version %s).\n",
		   DRIVER_NAME, DRIVER_VERSION);

	printk(KERN_INFO "%s: %s\n", DRIVER_NAME, DRIVER_DESCRIPTION);
	printk(KERN_INFO "%s: %s\n", DRIVER_NAME, DRIVER_COPYRIGHT);

	cm35i2c_major = 0;
	// Fill all device descriptor pointers by NULL
	for (i = 0; i < CM35I2C_MAX_NUM_DEVICES; ++i) {
		cm35i2c_devices[i] = NULL;
	}

	// Allocate character device region
	status = alloc_chrdev_region(&device,
					0,
					CM35I2C_MAX_NUM_DEVICES * CM35I2C_NUM_MINORS_PER_DEVICE,
					DRIVER_NAME);
	if (status < 0) {
		printk(KERN_ERR "%s ERROR: Failed to allocate %d minors",
			DRIVER_NAME,
			CM35I2C_MAX_NUM_DEVICES * CM35I2C_NUM_MINORS_PER_DEVICE
		);
		return status;
	}

	// Store the major number
	cm35i2c_major = MAJOR(device);
	printk(KERN_INFO "%s: Allocated %d devs starting from <%d, %d>\n",
		DRIVER_NAME,
		CM35I2C_MAX_NUM_DEVICES * CM35I2C_NUM_MINORS_PER_DEVICE,
	    MAJOR(device), MINOR(device));

	// Register the class
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	dev_class = class_create(DRIVER_NAME);
#else
	dev_class = class_create(THIS_MODULE, DRIVER_NAME);
#endif

	// Check failure of class creation
	if (dev_class == NULL) {
		printk(KERN_ERR "%s: Failed to create class %s\n",
		       DRIVER_NAME, DRIVER_NAME);
		// Unregister character device region
		unregister_chrdev_region(device, CM35I2C_MAX_NUM_DEVICES * CM35I2C_NUM_MINORS_PER_DEVICE);
		return -ENODEV;
	}
	
	// Register the driver
	status = pci_register_driver(&cm35i2c_pci_driver);

	/* Check if driver registration was successful */
	if (status) {
		printk(KERN_ERR "%s: Failed to register PCI driver\n",
			DRIVER_NAME
		);

		/* The class must be unregistered here */
		class_destroy(dev_class);

		/* The character devices must be unregistered here */
		unregister_chrdev_region(device, CM35I2C_MAX_NUM_DEVICES * CM35I2C_NUM_MINORS_PER_DEVICE);

		return status;
	}
	
	printk(KERN_INFO
		   "%s: Driver registered using character major number %d\n",
		   DRIVER_NAME, cm35i2c_major);


	return 0;
}


/******************************************************************************
Deinitialize CM35I2C driver and devices
 ******************************************************************************/
static void cm35i2c_unload(void)
{
	/* Unregister the driver */
	pci_unregister_driver(&cm35i2c_pci_driver);

	/* Unregister and destroy the class */
	class_destroy(dev_class);

	/* Unregister the character device region */
	unregister_chrdev_region(cm35i2c_major,
		CM35I2C_MAX_NUM_DEVICES * CM35I2C_NUM_MINORS_PER_DEVICE);

	printk(KERN_INFO "%s: Module unloaded.\n", DRIVER_NAME);
}



/******************************************************************************
Define the functions to execute when the driver module is loaded and unloaded
 ******************************************************************************/
module_init(cm35i2c_init);
module_exit(cm35i2c_unload);


/******************************************************************************
Set module properties
 ******************************************************************************/
MODULE_AUTHOR(DRIVER_COPYRIGHT);
MODULE_DESCRIPTION(DRIVER_DESCRIPTION);
MODULE_LICENSE("GPL");


/******************************************************************************
Set file operation functions
 ******************************************************************************/
static struct file_operations cm35i2c_file_ops = {
	.owner = THIS_MODULE,
	.poll = cm35i2c_poll,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
	.ioctl = cm35i2c_ioctl,
#else
	.unlocked_ioctl = cm35i2c_ioctl,
#endif
	.open = cm35i2c_open,
	.release = cm35i2c_release,
};


