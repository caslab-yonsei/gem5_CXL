# Copyright (c) 2022 SHIN. Jongmin
#  All rights reserved
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Copyright (c) 2005-2007 The Regents of The University of Michigan
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from m5.SimObject import SimObject
from m5.params import *
from m5.proxy import *
from m5.objects.Device import DmaDevice
from m5.objects.CxlHost import PciHost
from m5.objects.PciDevice import PciDevice, PciBar, PciIoBar, PciMemBar
from m5.objects.PcieDevice import PcieDevice

class CxlBar(PciBar):
    type = 'CxlBar'
    cxx_class = 'gem5::CxlBar'
    cxx_header = "dev/cxl/device.hh"
    abstract = True

class CxlBarNone(CxlBar):
    type = 'CxlBarNone'
    cxx_class = 'gem5::CxlBarNone'
    cxx_header = "dev/cxl/device.hh"

class CxlIoBar(CxlBar):
    type = 'CxlIoBar'
    cxx_class = 'gem5::CxlIoBar'
    cxx_header = "dev/cxl/device.hh"

    size = Param.MemorySize32("IO region size")


    abstract = True

class CxlLegacyIoBar(CxlIoBar):
    type = 'CxlLegacyIoBar'
    cxx_class = 'gem5::CxlLegacyIoBar'
    cxx_header = "dev/cxl/device.hh"

    addr = Param.UInt32("Legacy IO address")

# To set up a 64 bit memory BAR, put a PciMemUpperBar immediately after
# a PciMemBar. The pair will take up the right number of BARs, and will be
# recognized by the device and turned into a 64 bit BAR when the config is
# consumed.
class CxlMemBar(CxlBar):
    type = 'CxlMemBar'
    cxx_class = 'gem5::CxlMemBar'
    cxx_header = "dev/cxl/device.hh"

    size = Param.MemorySize("Memory region size")

class CxlMemUpperBar(CxlBar):
    type = 'CxlMemUpperBar'
    cxx_class = 'gem5::CxlMemUpperBar'
    cxx_header = "dev/cxl/device.hh"

class CxlDevice(PcieDevice):
    type = 'CxlDevice'
    cxx_class = 'gem5::CxlDevice'
    cxx_header = "dev/cxl/device.hh"
    abstract = True

    #host = Param.CxlHost(Parent.any, "CXL host")
    cxl_bus = Param.Int("CXL bus")
    cxl_dev = Param.Int("CXL device number")
    cxl_func = Param.Int("CXL function code")

    # CXL DVSEC
    # PCIe DVSEC for CXL
    #PCIeDVSEC_EXT_CAP_ID = 0x0023
    PCIeDVSEC_EXT_BaseOffset = Param.UInt16(0x00, "Base offset of PCIeDVSEC in PCIe Ext Config space")
    PCIeDVSEC_EXT_CAP_VER = Param.UInt8(0,"Capability version")       
    #PCIeDVSEC_EXT_CAP_NextCapability = Param.UInt16(0x00, "Pointer to next capability block") # 자동 설정할 예정임
    #PCIeDVSEC_DVSEC_VendorId = 0x1E98
    PCIeDVSEC_DVSEC_Rev = Param.UInt8(0x2, "Rev")
    #PCIeDVSEC_DVSEC_Length = 0x3C
    #PCIeDVSEC_DVSEC_Id = 0x0
    PCIeDVSEC_CxlCap = Param.UInt16(0x00, "Cxl Cap (to do...)")
    PCIeDVSEC_CxlCtrl = Param.UInt16(0x00, "Cxl Ctrl (to do...)")
    PCIeDVSEC_CxlStatus = Param.UInt16(0x00, "Cxl Status(to do...)")
    PCIeDVSEC_CxlCtrl2 = Param.UInt16(0x00, "Cxl Ctrl2 (to do...)")
    PCIeDVSEC_CxlStatus2 = Param.UInt16(0x00, "Cxl Status2 (to do...)")
    PCIeDVSEC_CxlLock = Param.UInt16(0x00, "Cxl Lock (to do...)")
    PCIeDVSEC_CxlCap2 = Param.UInt16(0x00, "Cxl Cap2 (to do...)")
    PCIeDVSEC_Range1SizeHigh = Param.UInt32(0x00, "Range 1 size high")
    PCIeDVSEC_Range1SizeLow = Param.UInt32(0x00, "Range 1 size low")
    PCIeDVSEC_Range1BaseHigh = Param.UInt32(0x00, "Range 1 base high")
    PCIeDVSEC_Range1BaseLow = Param.UInt32(0x00, "Range 1 base low")
    PCIeDVSEC_Range2SizeHigh = Param.UInt32(0x00, "Range 2 size high")
    PCIeDVSEC_Range2SizeLow = Param.UInt32(0x00, "Range 2 size low")
    PCIeDVSEC_Range2BaseHigh = Param.UInt32(0x00, "Range 2 base high")
    PCIeDVSEC_Range2BaseLow = Param.UInt32(0x00, "Range 2 base low")
    PCIeDVSEC_CxlCap3 = Param.UInt16(0x00, "Cxl Cap3")
    PCIeDVSEC_Reserved = Param.UInt16(0x00, "reserved")

    # NON Cxl Function Map
    #NonCxlFunctionMap_EXT_CAP_ID = 0x0023
    NonCxlFunctionMap_EXT_BaseOffset = Param.UInt16(0x00, "Base offset of NonCxlFunctionMap in PCIe Ext Config space")
    NonCxlFunctionMap_EXT_CAP_VER = Param.UInt8(0, "Capability version")    
    #NonCxlFunctionMap_EXT_NextCapability = Param.UInt16(0x00, "Pointer to next capability block") # 자동 설정할 예정임
    #NonCxlFunctionMap_DVSEC_VendorId = 0x1E98
    NonCxlFunctionMap_DVSEC_Rev = Param.UInt8(0x0, "Rev")
    #NonCxlFunctionMap_DVSEC_Length = 0x2C
    #NonCxlFunctionMap_DVSEC_Id = 0x2
    NonCxlFunctionMap_Reserved = Param.UInt16(0, "reserved")
    NonCxlFunctionMap_Reg0 = Param.UInt32(0x0, "Reg0")
    NonCxlFunctionMap_Reg1 = Param.UInt32(0x0, "Reg1")
    NonCxlFunctionMap_Reg2 = Param.UInt32(0x0, "Reg2")
    NonCxlFunctionMap_Reg3 = Param.UInt32(0x0, "Reg3")
    NonCxlFunctionMap_Reg4 = Param.UInt32(0x0, "Reg4")
    NonCxlFunctionMap_Reg5 = Param.UInt32(0x0, "Reg5")
    NonCxlFunctionMap_Reg6 = Param.UInt32(0x0, "Reg6")
    NonCxlFunctionMap_Reg7 = Param.UInt32(0x0, "Reg7")

    # Cxl Extensions for ports
    #CxlExtForPorts_EXT_CAP_ID = 0x0023
    CxlExtForPorts_EXT_BaseOffset = Param.UInt16(0x00, "Base Offset for Cxl extension for ports")
    CxlExtForPorts_EXT_CAP_VER = Param.UInt8(0, "")
    #CxlExtForPorts_EXT_NextCapability = Param.UInt16(0x00, "Pointer to next capability block") # 자동 설정할 예정임
    #CxlExtForPorts_DVSEC_VendorId = 0x1E98
    CxlExtForPorts_DVSEC_Rev = Param.UInt8(0x0, "Rev")
    #CxlExtForPorts_DVSEC_Length = 0x28
    #CxlExtForPorts_DVSEC_Id = 0x3
    CxlExtForPorts_CxlPortExtensionStatus = Param.UInt16(0x0, "")
    CxlExtForPorts_PortCtrlExtensions = Param.UInt16(0x0, "")
    CxlExtForPorts_AltBusBase = Param.UInt8(0x0, "")
    CxlExtForPorts_AltBusLimit = Param.UInt8(0x0, "")
    CxlExtForPorts_AltMemBase = Param.UInt16(0x0, "")
    CxlExtForPorts_AltMemLimit = Param.UInt16(0x0, "")
    CxlExtForPorts_AltPrefetchMemBase = Param.UInt16(0x0, "")
    CxlExtForPorts_AltPrefetchMemLimit = Param.UInt16(0x0, "")
    CxlExtForPorts_AltPrefetchableMemBaseHigh = Param.UInt32(0x0, "")
    CxlExtForPorts_AltPrefetchableMemLimitHigh = Param.UInt32(0x0, "")
    CxlExtForPorts_CxlRcrbBase = Param.UInt32(0x0, "")
    CxlExtForPorts_CxlRcrbBaseHigh = Param.UInt32(0x0, "")

    # GPF for CXL Ports
    #GpfForCxlPorts_EXT_CAP_ID = 0x0023
    GpfForCxlPorts_EXT_BaseOffset = Param.UInt16(0x00, "Base Offset for GPF for CXL Ports")
    GpfForCxlPorts_EXT_CAP_VER = Param.UInt8(0, "")
    #GpfForCxlPorts_EXT_NextCapability = Param.UInt16(0x00, "Pointer to next capability block") # 자동 설정할 예정임
    #GpfForCxlPorts_DVSEC_VendorId = 0x1E98
    GpfForCxlPorts_DVSEC_Rev = Param.UInt8(0x0, "Rev")
    #GpfForCxlPorts_DVSEC_Length = 0x10
    #GpfForCxlPorts_DVSEC_Id = 0x4
    GpfForCxlPorts_Reserved = Param.UInt16(0x0, "reserved")
    GpfForCxlPorts_GpfPhase1Ctrl = Param.UInt16(0x0, "GPF Phase1 Ctrl")
    GpfForCxlPorts_GpfPhase2Ctrl = Param.UInt16(0x0, "GPF Phase2 Ctrl")


    # GPF for CXL Ports
    #GpfForDev_EXT_CAP_ID = 0x0023
    GpfForDev_EXT_BaseOffset = Param.UInt16(0x00, "Base Offset for GPF for CXL Device")
    GpfForDev_EXT_CAP_VER = Param.UInt8(0, "")
    #GpfForDev_EXT_NextCapability = Param.UInt16(0x00, "Pointer to next capability block") # 자동 설정할 예정임
    #GpfForDev_DVSEC_VendorId = 0x1E98
    GpfForDev_DVSEC_Rev = Param.UInt8(0x0, "Rev")
    #GpfForDev_DVSEC_Length = 0x10
    #GpfForDev_DVSEC_Id = 0x5
    GpfForDev_GpfPhase2Duration = Param.UInt16(0x0, "GPF Phase 2 Duration")
    GpfForDev_GpfPhase2Power = Param.UInt16(0x0, "GPF Phase 2 Power")


    # GPF for CXL Ports
    #FlexBusPorts_EXT_CAP_ID = 0x0023
    FlexBusPorts_EXT_BaseOffset = Param.UInt16(0x00, "Base Offset for FlexBusPorts")
    FlexBusPorts_EXT_CAP_VER = Param.UInt8(0, "")
    #FlexBusPorts_EXT_NextCapability = Param.UInt16(0x00, "Pointer to next capability block") # 자동 설정할 예정임
    #FlexBusPorts_DVSEC_VendorId = 0x1E98
    FlexBusPorts_DVSEC_Rev = Param.UInt8(0x2, "Rev")
    #FlexBusPorts_DVSEC_Length = 0x20
    #FlexBusPorts_DVSEC_Id = 0x7
    FlexBusPorts_Cap = Param.UInt16(0x0, "Flex Bus Port CAP")
    FlexBusPorts_Ctrl = Param.UInt16(0x0, "Flex Bus Port Ctrl")
    FlexBusPorts_Status = Param.UInt16(0x0, "Flex Bus Port Status")
    FlexBusPorts_ReceivedModifiedTsDataPhase1 = Param.UInt32(0x0, "Flex Bus Receive modified TS data phase1")
    FlexBusPorts_Cap2 = Param.UInt32(0x0, "Flex Bus Cap2")
    FlexBusPorts_Ctrl2 = Param.UInt32(0x0, "Flex Bus Ctrl2")
    FlexBusPorts_Status2 = Param.UInt32(0x0, "Flex Bus Status2")


    # Register Locator
    #RegLocator_EXT_CAP_ID = 0x0023
    RegLocator_EXT_BaseOffset = Param.UInt16(0x00, "Base Offset for Register Locator")
    RegLocator_EXT_CAP_VER = Param.UInt8(0, "")
    #RegLocator_EXT_NextCapability = Param.UInt16(0x00, "Pointer to next capability block") # 자동 설정할 예정임
    #RegLocator_DVSEC_VendorId = 0x1E98
    RegLocator_DVSEC_Rev = Param.UInt8(0x0, "Rev")
    RegLocator_DVSEC_Length = Param.UInt16(0x00, "length")
    #RegLocator_DVSEC_Id = 0x8
    RegLocator_Reserved = Param.UInt16(0, "reserved")
    RegLocator_RegBlockLow = VectorParam.UInt16(0x0, "register Blcok low")
    RegLocator_RegBlockHigh = VectorParam.UInt16(0x0, "register Blcok high")


    # MLD
    #MLD_EXT_CAP_ID = 0x0023
    MLD_EXT_BaseOffset = Param.UInt16(0x00, "Base Offset for MLD")
    MLD_EXT_CAP_VER = Param.UInt8(0, "")
    #MLD_EXT_NextCapability = Param.UInt16(0x00, "Pointer to next capability block") # 자동 설정할 예정임
    #MLD_DVSEC_VendorId = 0x1E98
    MLD_DVSEC_Rev = Param.UInt8(0x0, "Rev")
    #MLD_DVSEC_Length = 0x10
    #MLD_DVSEC_Id = 0x9
    MLD_NumOfLDsSupported = Param.UInt16(0, "number of LDs supported")
    MLD_LdIdHotResetVector = Param.UInt16(0, "LD ID hot reset vector")
    MLD_Reserved = Param.UInt16(0, "reserved")