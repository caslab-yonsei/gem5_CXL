/*
 * Copyright (c) 2022 SHIN. Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2001-2005 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* @file
 * Device register definitions for a device's PCI config space
 */

#ifndef __CXLREG_H__
#define __CXLREG_H__

#include <sys/types.h>

#include "base/bitfield.hh"
#include "base/bitunion.hh"

#include "dev/pci/pcie/pciereg.h"

#define DVSEC_VENDOR_ID_CXL         0x1E98


// CXL Capabilities
#define MAXID                               0x000A
#define CXL_CAP_ID_PCIE_DVSEC_FOR_CXL_DEV   0x0000
#define CXL_CAP_ID_NON_CXL_FUNCTION_MAP     0x0002
#define CXL_CAP_ID_CXL_EXTENSIONS_FOR_PORTS 0x0003
#define CXL_CAP_ID_GPF_FOR_CXL_PORTS        0x0004
#define CXL_CAP_ID_GPF_FOR_CXL_DEV          0x0005
#define CXL_CAP_ID_FOR_FLEX_BUS_PORTS       0x0007
#define CXL_CAP_ID_REGISTER_LOCATOR         0x0008
#define CXL_CAP_ID_MLD                      0x0009
#define CXL_CAP_ID_TEST                     0x000A




union PCIE_DVSEC_FOR_CXL_DEV
{
    uint8_t data[0x3C];
    struct
    {
        DVSEC dvsec_header;
        uint16_t cxl_cap;
        uint16_t cxl_ctrl;
        uint16_t cxl_status;
        uint16_t cxl_ctrl2;
        uint16_t cxl_status2;
        uint16_t cxl_lock;
        uint16_t cxl_cap2;
        uint32_t range1_size_high;
        uint32_t range1_size_low;
        uint32_t range1_base_high;
        uint32_t range1_base_low;
        uint32_t range2_size_high;
        uint32_t range2_size_low;
        uint32_t range2_base_high;
        uint32_t range2_base_low;
        uint16_t cxl_cap3;
        uint16_t reserved;
    };
};

union NON_CXL_FUNCTION_MAP
{
    uint8_t data[0x2C];
    struct
    {
        DVSEC dvsec_header;
        uint16_t reserved;
        uint32_t registers[8];
    };
};

union CXL_EXTENSIONS_FOR_PORTS
{
    uint8_t data[0x28];
    struct
    {
        DVSEC dvsec_header;
        uint16_t cxl_port_extension_status;
        uint16_t port_ctrl_extensions;
        uint8_t alt_bus_base;
        uint8_t alt_bus_limit;
        uint16_t alt_mem_base;
        uint16_t alt_mem_limit;
        uint16_t alt_prefetch_mem_base;
        uint16_t alt_prefetch_mem_limit;
        uint32_t alt_prefetchable_mem_base_high;
        uint32_t alt_prefetchable_mem_limit_high;
        uint32_t cxl_rcrb_base;
        uint32_t cxl_rcrb_base_high;
    };
};

union GPF_FOR_CXL_PORTS
{
    uint8_t data[0x10];
    struct
    {
        DVSEC dvsec_header;
        uint16_t reserved;
        uint16_t gpf_phase1_ctrl;
        uint16_t gpf_phase2_ctrl;
    };
};

union GPF_FOR_CXL_DEV
{
    uint8_t data[0x10];
    struct
    {
        DVSEC dvsec_header;
        uint16_t gpf_phase2_duration;
        uint32_t gpf_phase2_power;
    };
};

union FLEX_BUS_PORTS
{
    uint8_t data[0x20];
    struct
    {
        DVSEC dvsec_header;
        uint16_t flex_bus_port_cap;
        uint16_t flex_bus_port_ctrl;
        uint16_t flex_bus_port_status;
        uint32_t flex_bus_received_modified_ts_data_phase1;
        uint32_t flex_bus_port_cap2;
        uint32_t flex_bus_port_ctrl2;
        uint32_t flex_bus_port_status2;
    };
};

union REGISTER_LOCATOR
{
    uint8_t data[0];
    struct
    {
        DVSEC dvsec_header;
        uint16_t reserved;
        uint16_t register_block[][2];   // low high
    };
};

union MLD
{
    uint8_t data[0x10];
    struct
    {
        DVSEC dvsec_header;
        uint16_t num_of_LDs_supported;
        uint16_t ld_id_hot_reset_vector;
        uint16_t reserved;
    };
};
 

#endif // __CXLREG_H__
