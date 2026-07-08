# Linux Software (CM35I2C)

> SWP-700010185 rev C
> 
> Version v01.01.00.155797

Copyright (C) RTD Embedded Technologies, Inc.  All Rights Reserved.

This software package is dual-licensed.  Source code that is compiled for
kernel mode execution is licensed under the GNU General Public License
version 2.  For a copy of this license, refer to the file
LICENSE_GPLv2.TXT (which should be included with this software) or contact
the Free Software Foundation.  Source code that is compiled for user mode
execution is licensed under the RTD End-User Software License Agreement.
For a copy of this license, refer to LICENSE.TXT or contact RTD Embedded
Technologies, Inc.  Using this software indicates agreement with the
license terms listed above.

## Table of Contents

- [Supported Hardware](#supported-hardware)
- [Supported Kernel Versions](#supported-kernel-versions)
- [Supported CPU Architecture](#supported-cpu-architecture)
- [Supported Compilers](#supported-compilers)
- [CM35I2C Driver](#cm35i2c-driver)
- [Library Interface](#library-interface)
- [Header Files](#header-files)
- [Example Programs](#example-programs)
- [CM35I2C Example Programs](#cm35i2c-example-programs)
- [DM35MRM Example Programs](#dm35mrm-example-programs)
- [Known Limitations](#known-limitations)
- [Getting Technical Support](#getting-technical-support)

## Supported Hardware

This software supports the RTD [CM35I2C](https://www.rtd.com/PC104/UM/network/CM35I2C03.htm) and other RTD I2C boards such as
the [DM35MRM](https://www.rtd.com/PC104/DM/digital%20IO/DM35MRM.htm). Both boards aren't required to use the software, but the DM35MRM
library is based on the CM35I2C board level calls.


## Supported Kernel Versions

This software has been tested with the following Linux distributions and kernel
versions:

* Ubuntu 26.04 LTS (7.0.0 Kernel unmodified)
* Debian 13 (6.12.57 Kernel unmodified)

 Due to API differences between kernel versions, RTD cannot guarantee compatibility
 with kernels and distributions not listed above.  If a user wishes to use an 
 unsupported kernel/distribution, it may be necessary to modify the driver module 
 code and/or Makefiles for the specific Linux environment.
 
## Supported CPU Architecture
  
 This software has been validated on the following CPU architectures.
 
* x86_64 (64-bit) multi-core

## Supported Compilers

The driver software and example programs were compiled using the GNU gcc 
compiler, but porting to other compilers is possible.

## CM35I2C Driver

The directory `CM35I2C/driver/` contains source code related to the driver.


In order to use a driver, one must first compile it, load it into the kernel,
and create device files for the board(s).  To do this, issue the following
commands while sitting in the `driver/` directory:

* `make`
* `sudo make load`


The driver module must be loaded before running any program which accesses a
CM35I2C device.



## Library Interface

The directories `CM35I2C/lib/` and `DM35MRM/lib` contains source code related 
to the user library.


The CM35I2C library is created with a file name of `librtd-cm35i2c.a` and is
statically linked. The DM35MRM Library is likewise a statically linked library.
Both the CM35I2C library and the DM35MRM Libraries are required for compiling
DM35MRM example programs.

Please refer to the software manual for details on using the user level library
functions.  These functions are prototyped in the file 
`CM35I2C/include/cm35i2c_library.h`; this header file must be included in 
any code which wishes to call library functions.

The library must be built before compiling the example programs or your
application.

To build the library, issue the command `make` within `lib/`.

## Header Files

The directory `include/` contains all header files needed by the driver, example
programs, library, and user applications.


## Example Programs

The directory `examples/` contains source code related to the example programs,
which demonstrate how to use features of the CM35I2C boards, test the driver, or
test the library.  In addition to source files, `examples/` holds other files as
well; the purpose of these files will be explained below.


To build the example programs, issue the command `make` within `examples/`.


## CM35I2C Example Programs

The following files are provided in `CM35I2C/examples/`:

### [cm35i2c_list_fb.c](CM3I2C/examples/cm35i2c_list_fb.c)         

This example demonstrates basic board operation. The example
opens board 0 (even if there are multiple boards installed). It
then lists all the function blocks found on the board and displays
information about them.

Setup: None

Usage: Display the command syntax by executing:

```sh
./cm35i2c_list_fb --help
```

### [cm35i2c_set_clock_rate.c](CM35I2C/examples/cm35i2c_set_clock_rate.c)

This program is used to set the baud rate of an individual connector.
this value is reset when a --reset flag is called on any example program.
The value is stored in volatile memory, and will be reset to 100MHz on 
powerup. 

Setup: None

Usage: Display the command syntax by executing:

```sh
./cm35i2c_set_clock_rate --help
```

### [cm35i2c_send.c](CM35I2C/examples/cm35i2c_send.c)

This is used to send arbitrary I2C packets. Used for testing packet construction
and to verify ouput on each connector.

Setup: None

Usage: Display the command syntax by executing:

```sh
./cm35i2c_send --help
```

## DM35MRM Example Programs

The following files are provided in `DM35MRM/examples/`:

### [dm35mrm_csv_export.c](DM35MRM/examples/dm35mrm_csv_export.c)
        
This takes the current closings of relays and exports it as a CSV
formated file. Each entry is a (board, row, col) that specifies
the complete list of closed relays.


Setup: Requires A DM35MRM attached.
        
Usage: Display the command syntax by executing:

```sh
./dm35mrm_csv_export --help
```

### [dm35mrm_csv_import.c](DM35MRM/examples/dm35mrm/examples/dm35mrm_csv_import.c)

This program uses a file generated by `dm35mrm_csv_export` and sets
the boards to that given closings. This program uses a delayed write
operation to set all of the relays simultaneously after writing.

Setup: Requires A DM35MRM attached.
        
Usage: Display the command syntax by executing:

```sh
./dm35mrm_csv_import --help
```

### [dm35mrm_delay_write_test.c](DM35MRM/examples/dm35mrm_delay_write_test.c)
        
This is a basic test that requires no inputs from the user. This is
used to show how to do a delayed write operation by changing a single
relay (Board 0, Row 0, Col 0).

Setup: Requires A DM35MRM attached.
        
Usage: Display the command syntax by executing:

```sh
./dm35mrm_delay_write_test --help
```
   
### [dm35mrm_get_version.c](DM35MRM/examples/dm35mrm_get_version.c)

This is a simple utility that gets the version number for every microcontroller
on each DM35MRM board.

Setup: Requires a DM35MRM attached.

Usage: display the command syntax by executing:

```sh
./dm35mrm_get_version --help
```

### [dm35mrm_pin_connection.c](DM35MRM/examples/dm35mrm_pin_connection.c)

This program takes two pins provided by the user and attempts to either connect
or disconnect them. To connect pins, it will attempt to find two available relays
that correspond to the provided pins and close them. To disconnect pins, it will
search for the connection, and reopen those relays.

Setup: Requires a DM35MRM attached.

Usage: display the command syntax by executing:

```sh
./dm35mrm_pin_connection --help
```

### [dm35mrm_write_relay_immediate.c](DM35MRM/examples/dm35mrm_write_relay_immediate.c)
        
This program takes a board row column address and attempts to either
perform the --open or --close operation on that relay. This is used
to immediately open and close a single relay on a DM35MRM.

Setup: Requires A DM35MRM attached.
        
Usage: Display the command syntax by executing:

```sh
./dm35mrm_write_relay_write_immediate --help
```

### Known Limitations

1. This software was tested only on little-endian processors.  If you are using
    a big-endian CPU, you will need to examine the driver, example, and library
    source code for endianness issues and resolve them.

 2. Many conditions affect board throughput and interrupt performance.  For a
    discussion of these issues, please see the Application Note SWM-640000021
    (Linux Interrupt Performance) available on our web site.

 3. If you are using the interrupt wait mechanism, be aware that signals
    delivered to the application can cause the sleep to awaken prematurely.
    Interrupts may be missed if signals are delivered rapidly enough or at
    inopportune times.
    
 4. Boards with Rev A of the FPGA may have issues with DMA on 64-bit systems
    with more than 2 GB of RAM.



### Getting Technical Support

If you require additional support with this product, or any other products from
RTD Embedded Technologies, contact us using the information below:

RTD Embedded Technologies, Inc. \
103 Innovation Boulevard \
State College, PA 16803 USA \
Telephone: (814) 234-8087 \
Fax: (814) 234-5218

Sales Information and Quotes: sales@rtd.com \
Technical Assistance: techsupport@rtd.com \
Web Site: [http://www.rtd.com](http://www.rtd.com)
