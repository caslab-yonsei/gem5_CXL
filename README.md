# gem5_CXL

## Overview
gem5_CXL is full-system simulator based on gem5 that provides cxl device specifications, packets, and protocols.

## Features

gem5_CXL gincludes the following task elements.

Implementation of simple models for CXL devices

Implement CXL.mem and CXL.cache Request/Response Fields in gem5 packets that comply with the CXL spec:
- CXL.cache (D2H/H2D Request, Response, Data)
- CXL.mem  (M2S/S2M Request, Response, Data)

Supports CXL protocols between host and cxl device.

Simple protocol message test environment using gem5_bare_metal

## Requirements

Dependencies of gem5 version 23.10

Recommend Ubuntu 20.04, 22.04 or 24.04.
- gcc 9.4.0+
- SCons 3.0+
- Python 3.6+

The main website can be found at [gem5.org](http://www.gem5.org/).

## Setup

```bash
sudo apt install build-essential git m4 scons zlib1g zlib1g-dev \
    libprotobuf-dev protobuf-compiler libprotoc-dev libgoogle-perftools-dev \
    python3-dev python-is-python3 libboost-all-dev pkg-config libpng-dev
```

### Disk/Kernel Image / Bootloader
To prevent unexpected issues, it is recommended to use an older version of the Ubuntu image/FS Binaries(Kernel Image/Bootloader). You can create the Ubuntu 18.04 image yourself or download it from [gem5_guest_binaries](https://www.gem5.org/documentation/general_docs/fullsystem/guest_binaries).
Latest image: [gem5-resources](https://resources.gem5.org/)

### Kernel Build
Recommend using an older version of the linux kernel to prevent unexpected issues. The simulator test was performed on [Kernel 5.4.49](https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tag/?h=v5.4.49).


## Building gem5_CXL

1. build gem5 simulator

```bash
   # Clone the repository
   git clone https://github.com/caslab-yonsei/gem5_CXL.git
   cd gem5_CXL/gem5

   # Install dependencies
   # Please visit https://www.gem5.org/documentation/general_docs/building

   # Build gem5 (ARM only)
   scon build/ARM/gem5.opt -j`nproc`

   # Build DTB
   sudo apt install dtc
   cd system/arm/dt
   make
   ```

2. build gem5-bare_metal

```bash
   cd gem5_CXL/gem5.bare-metal/Simple
   make
   ```

## Using gem5_CXL


1. Write the file path(vmlinux, images, etc.) in `runscripts/env.sh`, `run_bm.sh` .

2. Run
```bash
   cd gem5_CXL/runscripts
   ./run_bm.sh
   ```

For host CPU instructions for CXL.mem testing, you can perform them by entering memory address access code in `gem5_CXL/gem5.bare-metal/Simple/main.cpp`, and for CXL processor instructions for CXL.cache testing, you can use the schedule function generation in `gem5/dev/cxl/TestDev/CXLProcessor.cc`.