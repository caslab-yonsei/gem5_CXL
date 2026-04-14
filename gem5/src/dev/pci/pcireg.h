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

#ifndef __PCIREG_H__
#define __PCIREG_H__

#include <sys/types.h>

#include "base/bitfield.hh"
#include "base/bitunion.hh"

BitUnion16(PciCommandRegister)
    Bitfield<15, 10> reserved;
    Bitfield<9> fastBackToBackEn;
    Bitfield<8> serrEn;
    Bitfield<7> steppingControl;
    Bitfield<6> parityErrResp;
    Bitfield<5> vgaPaletteSnoopEn;
    Bitfield<4> memWriteInvEn;
    Bitfield<3> specialCycles;
    Bitfield<2> busMaster;
    Bitfield<1> memorySpace;
    Bitfield<0> ioSpace;
EndBitUnion(PciCommandRegister)

union PCIConfig
{
    uint8_t data[0x1000];

    // PCI Legacy Configure Registers
    struct
    {
        uint16_t vendor;
        uint16_t device;
        uint16_t command;
        uint16_t status;        
        uint8_t revision;
        uint8_t progIF;
        uint8_t subClassCode;
        uint8_t classCode;
        uint8_t cacheLineSize;
        uint8_t latencyTimer;
        uint8_t headerType;
        uint8_t bist;           // 0x10
        uint32_t baseAddr[6];   // 0x28 
        uint32_t cardbusCIS;
        uint16_t subsystemVendorID;
        uint16_t subsystemID;   // 0x30
        uint32_t expansionROM;
        uint8_t capabilityPtr;
        // Was 8 bytes in the legacy PCI spec, but to support PCIe
        // this field is now 7 bytes with PCIe's addition of the
        // capability list pointer.
        uint8_t reserved[7];
        uint8_t interruptLine;
        uint8_t interruptPin;
        uint8_t minimumGrant;
        uint8_t maximumLatency; // 0x40

        uint8_t capability_area[0x100-0x40];
        uint8_t ext_capability_area[0x1000-0x100];      // Fore PCIe
    };
};

// Common PCI offsets
#define PCI_VENDOR_ID           0x00    // Vendor ID                    ro
#define PCI_DEVICE_ID           0x02    // Device ID                    ro
#define PCI_COMMAND             0x04    // Command                      rw
#define PCI_STATUS              0x06    // Status                       rw
#define PCI_REVISION_ID         0x08    // Revision ID                  ro
#define PCI_CLASS_CODE          0x09    // Class Code                   ro
#define PCI_SUB_CLASS_CODE      0x0A    // Sub Class Code               ro
#define PCI_BASE_CLASS_CODE     0x0B    // Base Class Code              ro
#define PCI_CACHE_LINE_SIZE     0x0C    // Cache Line Size              ro+
#define PCI_LATENCY_TIMER       0x0D    // Latency Timer                ro+
#define PCI_HEADER_TYPE         0x0E    // Header Type                  ro
#define PCI_BIST                0x0F    // Built in self test           rw

// some pci command reg bitfields
#define PCI_CMD_BME     0x04 // Bus master function enable
#define PCI_CMD_MSE     0x02 // Memory Space Access enable
#define PCI_CMD_IOSE    0x01 // I/O space enable

// Type 0 PCI offsets
#define PCI0_BASE_ADDR0         0x10    // Base Address 0               rw
#define PCI0_BASE_ADDR1         0x14    // Base Address 1               rw
#define PCI0_BASE_ADDR2         0x18    // Base Address 2               rw
#define PCI0_BASE_ADDR3         0x1C    // Base Address 3               rw
#define PCI0_BASE_ADDR4         0x20    // Base Address 4               rw
#define PCI0_BASE_ADDR5         0x24    // Base Address 5               rw
#define PCI0_CIS                0x28    // CardBus CIS Pointer          ro
#define PCI0_SUB_VENDOR_ID      0x2C    // Sub-Vendor ID                ro
#define PCI0_SUB_SYSTEM_ID      0x2E    // Sub-System ID                ro
#define PCI0_ROM_BASE_ADDR      0x30    // Expansion ROM Base Address   rw
#define PCI0_CAP_PTR            0x34    // Capability list pointer      ro
#define PCI0_RESERVED           0x35
#define PCI0_INTERRUPT_LINE     0x3C    // Interrupt Line               rw
#define PCI0_INTERRUPT_PIN      0x3D    // Interrupt Pin                ro
#define PCI0_MINIMUM_GRANT      0x3E    // Maximum Grant                ro
#define PCI0_MAXIMUM_LATENCY    0x3F    // Maximum Latency              ro

// Type 1 PCI offsets
#define PCI1_BASE_ADDR0         0x10    // Base Address 0               rw
#define PCI1_BASE_ADDR1         0x14    // Base Address 1               rw
#define PCI1_PRI_BUS_NUM        0x18    // Primary Bus Number           rw
#define PCI1_SEC_BUS_NUM        0x19    // Secondary Bus Number         rw
#define PCI1_SUB_BUS_NUM        0x1A    // Subordinate Bus Number       rw
#define PCI1_SEC_LAT_TIMER      0x1B    // Secondary Latency Timer      ro+
#define PCI1_IO_BASE            0x1C    // I/O Base                     rw
#define PCI1_IO_LIMIT           0x1D    // I/O Limit                    rw
#define PCI1_SECONDARY_STATUS   0x1E    // Secondary Status             rw
#define PCI1_MEM_BASE           0x20    // Memory Base                  rw
#define PCI1_MEM_LIMIT          0x22    // Memory Limit                 rw
#define PCI1_PRF_MEM_BASE       0x24    // Prefetchable Memory Base     rw
#define PCI1_PRF_MEM_LIMIT      0x26    // Prefetchable Memory Limit    rw
#define PCI1_PRF_BASE_UPPER     0x28    // Prefetchable Base Upper 32   rw
#define PCI1_PRF_LIMIT_UPPER    0x2C    // Prefetchable Limit Upper 32  rw
#define PCI1_IO_BASE_UPPER      0x30    // I/O Base Upper 16 bits       rw
#define PCI1_IO_LIMIT_UPPER     0x32    // I/O Limit Upper 16 bits      rw
#define PCI1_RESERVED           0x34    // Reserved                     ro
#define PCI1_ROM_BASE_ADDR      0x38    // Expansion ROM Base Address   rw
#define PCI1_INTR_LINE          0x3C    // Interrupt Line               rw
#define PCI1_INTR_PIN           0x3D    // Interrupt Pin                ro
#define PCI1_BRIDGE_CTRL        0x3E    // Bridge Control               rw

// Device specific offsets
#define PCI_DEVICE_SPECIFIC             0x40    // 192 bytes
#define PCI_CONFIG_SIZE         0xFF

// Some Vendor IDs
#define PCI_VENDOR_DEC                  0x1011
#define PCI_VENDOR_NCR                  0x101A
#define PCI_VENDOR_QLOGIC               0x1077
#define PCI_VENDOR_SIMOS                0x1291

// Some Product IDs
#define PCI_PRODUCT_DEC_PZA             0x0008
#define PCI_PRODUCT_NCR_810             0x0001
#define PCI_PRODUCT_QLOGIC_ISP1020      0x1020
#define PCI_PRODUCT_SIMOS_SIMOS         0x1291
#define PCI_PRODUCT_SIMOS_ETHER         0x1292

/**
 * PCIe capability list offsets internal to the entry.
 * Actual offsets in the PCI config space are defined in
 * the python files setting up the system.
 */
#define PMCAP_ID 0x00
#define PMCAP_PC 0x02
#define PMCAP_PMCS 0x04
#define PMCAP_SIZE 0x06

#define MSICAP_ID 0x00
#define MSICAP_MC 0x02
#define MSICAP_MA 0x04
#define MSICAP_MUA 0x08
#define MSICAP_MD 0x0C
#define MSICAP_MMASK 0x10
#define MSICAP_MPEND 0x14
#define MSICAP_SIZE 0x18

#define MSIXCAP_ID 0x00
#define MSIXCAP_MXC 0x02
#define MSIXCAP_MTAB 0x04
#define MSIXCAP_MPBA 0x08
#define MSIXCAP_SIZE 0x0C

#define PXCAP_ID 0x00
#define PXCAP_PXCAP 0x02
#define PXCAP_PXDCAP 0x04
#define PXCAP_PXDC 0x08
#define PXCAP_PXDS 0x0A
#define PXCAP_PXLCAP 0x0C
#define PXCAP_PXLC 0x10
#define PXCAP_PXLS 0x12
#define PXCAP_PXDCAP2 0x24
#define PXCAP_PXDC2 0x28
#define PXCAP_SIZE 0x30

#define _PMCAP_BASE 0xC8
#define _MSICAP_BASE 0xD0
#define _MSIXCAP_BASE 0xA0
#define _PXCAP_BASE   0xE0
#define PMCAP_LIMIT  0xCD
#define MSICAP_LIMIT 0xDF
#define MSIXCAP_LIMIT 0xAB
#define PXCAP_LIMIT  0xF3


union NULLCAPA
{
    uint8_t data[6];
};

union CAP_HEADER
{
    uint8_t data[2];
    struct
    {
        uint8_t id;
        uint8_t next;
    };
};

/** @struct PMCAP
 *  Defines the Power Management capability register and all its associated
 *  bitfields for a PCIe device.
 */
union PMCAP
{
    uint8_t data[6];
    struct
    {
        uint16_t pid;  /* 0:7  cid
                        * 8:15 next
                        */
        uint16_t pc;   /* 0:2   vs
                        * 3     pmec
                        * 4     reserved
                        * 5     dsi
                        * 6:8   auxc
                        * 9     d1s
                        * 10    d2s
                        * 11:15 psup
                        */
        uint16_t pmcs; /* 0:1   ps
                        * 2     reserved
                        * 3     nsfrst
                        * 4:7   reserved
                        * 8     pmee
                        * 9:12  dse
                        * 13:14 dsc
                        * 15    pmes
                        */
    };
};

/** @struct MSICAP
 *  Defines the MSI Capability register and its associated bitfields for
 *  the a PCI/PCIe device.  Both the MSI capability and the MSIX capability
 *  can be filled in if a device model supports both, but only 1 of
 *  MSI/MSIX/INTx interrupt mode can be selected at a given time.
 */
union MSICAP
{
    uint8_t data[24];
    struct
    {
        uint16_t mid;  /* 0:7  cid
                        *  8:15 next
                        */
        uint16_t mc;   /* 0     msie;
                        * 1:3   mmc;
                        * 4:6   mme;
                        * 7     c64;
                        * 8     pvm;
                        * 9:15  reserved;
                        */
        uint32_t ma;   /* 0:1  reserved
                        * 2:31 addr
                        */
        uint32_t mua;
        uint16_t md;
        uint32_t mmask;
        uint32_t mpend;
   };
};

/** @struct MSIX
 *  Defines the MSI-X Capability register and its associated bitfields for
 *  a PCIe device.
 */
union MSIXCAP
{
    uint8_t data[12];
    struct
    {
        uint16_t mxid; /* 0:7  cid
                        *  8:15 next
                        */
        uint16_t mxc;  /* 0:10  ts;
                        * 11:13 reserved;
                        * 14    fm;
                        * 15    mxe;
                        */
        uint32_t mtab; /* 0:2   tbir;
                        * 3:31  to;
                        */
        uint32_t mpba; /* 0:2   pbir;
                        * 3:31>  pbao;
                        */
    };
};

union MSIXTable
{
    struct
    {
        uint32_t addr_lo;
        uint32_t addr_hi;
        uint32_t msg_data;
        uint32_t vec_ctrl;
    } fields;
    uint32_t data[4];
};

#define MSIXVECS_PER_PBA 64
struct MSIXPbaEntry
{
    uint64_t bits;
};

/** @struct PXCAP
 *  Defines the PCI Express capability register and its associated bitfields
 *  for a PCIe device.
 */
union PXCAP
{
    uint8_t data[48];
    struct
    {
        uint16_t pxid; /* 0:7  cid
                        *  8:15 next
                        */
        uint16_t pxcap; /* 0:3   ver;
                         * 4:7   dpt;
                         * 8     si;
                         * 9:13  imn;
                         * 14:15 reserved;
                         */
        uint32_t pxdcap; /* 0:2   mps;
                          * 3:4   pfs;
                          * 5     etfs;
                          * 6:8   l0sl;
                          * 9:11  l1l;
                          * 12:14 reserved;
                          * 15    rer;
                          * 16:17 reserved;
                          * 18:25 csplv;
                          * 26:27 cspls;
                          * 28    flrc;
                          * 29:31 reserved;
                          */
        uint16_t pxdc; /* 0     cere;
                        * 1     nfere;
                        * 2     fere;
                        * 3     urre;
                        * 4     ero;
                        * 5:7   mps;
                        * 8     ete;
                        * 9     pfe;
                        * 10    appme;
                        * 11    ens;
                        * 12:14 mrrs;
                        * 15    func_reset;
                        */
        uint16_t pxds; /* 0     ced;
                        * 1     nfed;
                        * 2     fed;
                        * 3     urd;
                        * 4     apd;
                        * 5     tp;
                        * 6:15  reserved;
                        */
        uint32_t pxlcap; /* 0:3   sls;
                          * 4:9   mlw;
                          * 10:11 aspms;
                          * 12:14 l0sel;
                          * 15:17 l1el;
                          * 18    cpm;
                          * 19    sderc;
                          * 20    dllla;
                          * 21    lbnc;
                          * 22:23 reserved;
                          * 24:31 pn;
                          */
        uint16_t pxlc; /* 0:1   aspmc;
                        * 2     reserved;
                        * 3     rcb;
                        * 4:5   reserved;
                        * 6     ccc;
                        * 7     es;
                        * 8     ecpm;
                        * 9     hawd;
                        * 10:15 reserved;
                        */
        uint16_t pxls; /* 0:3   cls;
                        * 4:9   nlw;
                        * 10:11 reserved;
                        * 12    slot_clk_config;
                        * 13:15 reserved;
                        */
        uint8_t reserved[20];
        uint32_t pxdcap2; /* 0:3   ctrs;
                           * 4     ctds;
                           * 5     arifs;
                           * 6     aors;
                           * 7     aocs32;
                           * 8     aocs64;
                           * 9     ccs128;
                           * 10    nprpr;
                           * 11    ltrs;
                           * 12:13 tphcs;
                           * 14:17 reserved;
                           * 18:19 obffs;
                           * 20    effs;
                           * 21    eetps;
                           * 22:23 meetp;
                           * 24:31 reserved;
                           */
        uint32_t pxdc2; /* 0:3   ctv;
                         * 4     ctd;
                         * 5:9   reserved;
                         * 10    ltrme;
                         * 11:12 reserved;
                         * 13:14 obffe;
                         * 15:31 reserved;
                         */
    };
};

// CAPA_IDs [0x40-0xFF]
#define NUM_CAPA_IDS                        0x16
#define CAPAID_NULL                         0x00
#define CAPAID_PM                           0x01
#define CAPAID_AGP                          0x02
#define CAPAID_VPD                          0x03
#define CAPAID_SLOT_IDENTIFIER              0x04
#define CAPAID_MSI                          0x05
#define CAPAID_CPCI_HOTSWAP                 0x06
#define CAPAID_HYPER_TRANSPORT              0x08
#define CAPAID_VENDOR_SPECIFIC              0x09
#define CAPAID_DEBUG_PORT                   0x0A
#define CAPAID_CPIC_CENTRAL_RES_CTRL        0x0B
#define CAPAID_PCI_HOT_PLUG                 0x0C
#define CAPAID_PCI_BRIDGE_SUBSYS_VENDORID   0x0D
#define CAPAID_AGP8X                        0x0E
#define CAPAID_SECURE_DEVICE                0x0F
#define CAPAID_PCIE                         0x10
#define CAPAID_MSIX                         0x11
#define CAPAID_SATA_DATA_INDEX_CONFIG       0x12
#define CAPAID_ADVANCED_FEATURES            0x13
#define CAPAID_ENHANCED_ALLOCATION          0x14
#define CAPAID_FLATTENING_PORTAL_BRIDGE     0x15

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



#endif // __PCIREG_H__
