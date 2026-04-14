# gem5_CXL

gem5_CXL is full-system simulator based on gem5 that provides cxl device specifications, packets, and protocols.

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

## Building gem5_CXL

1. build gem5 simulator

```bash
   git clone https://github.com/-------/gem5_CXL
   cd gem5_CXL/gem5
   scon build/ARM/gem5.opt -j`nproc`
   cd system/arm/dt
   make
   ```

2. build gem5-bare_metal

```bash
   cd gem5_CXL/gem5.bare-metal/Simple
   make
   ```

## Using gem5_CXL


1. Write the file path(vmlinux, images, etc.) in runscripts

2. Test case setting

3. Run
```bash
   cd gem5_CXL/runscripts
   ./run_bm.sh
   ```
4. Result on gem5_CXL/output-basic