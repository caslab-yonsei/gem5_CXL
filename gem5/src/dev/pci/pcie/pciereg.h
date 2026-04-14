/*
 * Copyright (c) 2013 ARM Limited
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

#ifndef __PCIEREG_H__
#define __PCIEREG_H__

#include "../pcireg.h"

#define MASK_BYTEENABLE                     0xFFFFFFFFFFFFFFFFL
#define MASK_BASEADDR                       0xFFFFFFFFF0000000L     // 63:28
#define MASK_BUS_NUMBER                     0x000000000FF00000L     // 27:20
#define MASK_DEVICE_NUMBER                  0x00000000000F8000L     // 19:15
#define MASK_FUNCTION_NUMBER                0x0000000000007000L     // 14:12
#define MASK_REGISTER_NUMBER                0x0000000000000FFFL     // 11:2
#define MASK_EXTENDED_REGISTER_NUMBER       0x0000000000000FFFL     // 11:8
#define MASK_LEGACY_REGISTER_NUMBER         0x00000000000000FFL     // 7:2
#define MASK_BYTEENABLE                     0x0000000000000003L     // 1:0

#define LOW_BITNUM_BASEADDR                 28
#define LOW_BITNUM_BUS_NUMBER               20
#define LOW_BITNUM_DEVICE_NUMBER            15
#define LOW_BITNUM_FUNCTION_NUMBER          12
#define LOW_BITNUM_REGISTER_NUMBER          2
#define LOW_BITNUM_EXTENDED_REGISTER_NUMBER 8
#define LOW_BITNUM_LEGACY_REGISTER_NUMBER   2
#define LOW_BITNUM_BYTEENABLE               0


#define PCIE_EXTENDED_CONFIG_SIZE       0xFFF

// PCIe Extended Capability [0x100-0xFFF]
#define E_CAPAID_NULL                                               0x0000
#define E_CAPAID_AER                                                0x0001  // Advanced Error Reporting
#define E_CAPAID_VC_WO_MFVC                                         0x0002  // VC : virtual Channel
#define E_CAPAID_DSN                                                0x0003   // Device Serial Number
#define E_CAPAID_POWER_BUDGETING                                    0x0004
#define E_CAPAID_ROOTCOMPLEX_LINK_DECLARATION                       0x0005
#define E_CAPAID_ROOTCOMPLEX_INTERNAL_LINK_CTRL                     0x0006
#define E_CAPAID_ROOTCOMPLEX_EVENT_COLLECTOR_ENDPOINT_ASSOCIATION   0x0007
#define E_CAPAID_MFVC                                               0x0008  // Multi Function VC
#define E_CAPAID_VC_W_MFVC                                          0x0009
#define E_CAPAID_RCRB_HEADER                                        0x000A  // Root Complex Register Block (RCRB) Header
#define E_CAPAID_VSEC                                               0x000B  // Vendor-Specific Extended Capability (VSEC)
#define E_CAPAID_CAC                                                0x000C  
#define E_CAPAID_ACS                                                0x000D  // Access Control Services 
#define E_CAPAID_ARI                                                0x000E  // Alternative Routing-ID Interpretation
#define E_CAPAID_ATS                                                0x000F  // Address Translation Services 
#define E_CAPAID_SRIOV                                              0x0010  // Single Root I/O Virtualization
#define E_CAPAID_MRIOV                                              0x0011  // Deprecated; formerly Multi-Root I/O Virtualization (MR-IOV)
#define E_CAPAID_MULTICAST                                          0x0012
#define E_CAPAID_PRI                                                0x0013  // Page Request Interface
#define E_CAPAID_RESERVED_FOR_AMD                                   0x0014
#define E_CAPAID_RESIZABLE_BAR                                      0x0015
#define E_CAPAID_DPA                                                0x0016  // Dinamic Power Allocation
#define E_CAPAID_TPH_REQUESTER                                      0x0017
#define E_CAPAID_LTR                                                0x0018  // Latency Tolerance Reporting
#define E_CAPAID_SECONDARY_PCIE                                     0x0019  // Secondary PCI Express
#define E_CAPAID_PMMUX                                              0x001A  // Protocol Multiplexing
#define E_CAPAID_PASID                                              0x001B  // Process Address Space ID
#define E_CAPAID_LNR                                                0x001C  // LN Requester
#define E_CAPAID_DPC                                                0x001D  // Downstream Port Containment
#define E_CAPAID_L1_PM_SUBSTATES                                    0x001E  // L1 PM Substates
#define E_CAPAID_PTM                                                0x001F  // Precision Time Measurement
#define E_CAPAID_MPCIE                                              0x0020  // PCI Express over M-PHY (M-PCIe)
#define E_CAPAID_FRS_QUEUEING                                       0x0021  // FRS Queueing
#define E_CAPAID_READINESS_TIME_REPORTING                           0x0022
#define E_CAPAID_DVSEC                                              0x0023  // Designated Vendor-Specific Extended Capability
#define E_CAPAID_VF_RESIZABLE_BAR                                   0x0024  // VF Resizable BAR
#define E_CAPAID_DATALINK_FEATURE                                   0x0025  // Data Link Feature
#define E_CAPAID_PHY_LAYER_16GTPS                                   0x0026  // Physical Layer 16.0 GT/s
#define E_CAPAID_LANE_MERGINING_AT_THE_RECEIVER                     0x0027  // Lane Margining at the Receiver
#define E_CAPAID_HIERARCHY_ID                                       0x0028  // Hierarchy ID
#define E_CAPAID_NPEM                                               0x0029  // Native PCIe Enclosure Management (NPEM)
#define E_CAPAID_PHY_LAYER_32GTPS                                   0x002A  // Physical Layer 32.0 GT/s
#define E_CAPAID_ALTERNATE_PROTOCOL                                 0x002B  // Alternate Protocol 
#define E_CAPAID_SFI                                                0x002C  // System Firmware Intermediary (SFI)
#define E_CAPAID_SHADOW_FUNCTIONS                                   0x002D  // Shadow Functions 
#define E_CAPAID_DATA_OBJECT_EXCHANGE                               0x002E  // Data Object Exchange

struct Ecam
{
    uint64_t baseAddr;
    uint8_t busnum;
    uint8_t devnum;
    uint8_t fnnum;
    uint16_t regnum;
    uint8_t extReg;
    uint8_t legacyReg;
    uint8_t gitEnable;
};

union ENULL
{
    uint8_t data[8];
};

union ECAPAHeader
{
    uint8_t data[4];
    struct 
    {
        uint16_t e_capa_id;     // [0:15]
        uint16_t ver_next;      // capa ver [16:19], next offset [20:31]
    };
};

union DVSECHeader
{
    uint8_t data[6];
    struct 
    {
        uint16_t vendor_id;     // [0:15]           must qualify it before attempting to dvsecid
        uint16_t rev_leng;      // rev [16:19], length [20:31]
        uint16_t dvsec_id;      // [32:47]
    };
};

union DVSEC
{
    uint8_t data[0];
    struct
    {
        ECAPAHeader e_capa_header;
        DVSECHeader dvsec_header;
        uint8_t dvsec_registers[0];     // Specific
    };
};
#endif // __PCIEREG_H__
