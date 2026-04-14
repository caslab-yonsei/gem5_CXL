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
from m5.objects.PciDevice import PciDevice, PciBar, PciIoBar, PciMemBar,PciMemUpperBar,PciLegacyIoBar
from m5.objects.PcieDevice import PcieDevice
from m5.objects.CxlDevice import *
from m5.objects.MemCtrl import MemCtrl
from m5.objects.ClockedObject import ClockedObject

class CxlMemDev(CxlDevice):
    type = 'CxlMemDev'
    cxx_class = 'gem5::CxlMemDev'
    cxx_header = "dev/cxl/cxlmem/cxlmemdev.hh"
    #abstract = True

    #host = Param.CxlHost(Parent.any, "CXL host")
    # cxl_bus = Param.Int("CXL bus")
    # cxl_dev = Param.Int("CXL device number")
    # cxl_func = Param.Int("CXL function code")

    VendorID = 0x2010
    SubsystemVendorID = VendorID
    DeviceID = 0x0819

    ClassCode = 0x03 #0xff # Misc device



    # The size is overridden by the device model.
    BAR0 = PciIoBar(size='1MiB')
    #BAR1 = CxlMemUpperBar()
    #BAR2 = CxlMemBar(size='2MiB')
    #BAR3 = CxlMemUpperBar()
    #BAR4 = CxlLegacyIoBar(addr=0xf000, size='256B')
    #BAR5 = CxlMemBar(size='512KiB')

    InterruptPin = 0x01 # Use #INTA

    CapabilityPtr = 0x80

    Status = 0xff # Status = 16

    PXCAPBaseOffset = 0x80
    PXCAPNextCapability = 0x80
    PXCAPCapId = 0x10
    PXCAPCapabilities = Param.UInt16(0x0000, "PCIe Capabilities")
    PXCAPDevCapabilities = 0x20
    PXCAPDevCtrl = Param.UInt16(0x0000, "PCIe Device Control")
    PXCAPDevStatus = Param.UInt16(0x0000, "PCIe Device Status")
    PXCAPLinkCap = Param.UInt32(0x00000000, "PCIe Link Capabilities")
    PXCAPLinkCtrl = Param.UInt16(0x0000, "PCIe Link Control")
    PXCAPLinkStatus = Param.UInt16(0x0000, "PCIe Link Status")
    PXCAPDevCap2 = Param.UInt32(0x00000000, "PCIe Device Capabilities 2")
    PXCAPDevCtrl2 = Param.UInt32(0x00000000, "PCIe Device Control 2")

    PCIeDVSEC_EXT_BaseOffset = 0x100

    InternalMemBus = Param.CoherentXBar(NULL, "Membar")
    # InternalMemMidBus = None #CoherentXBar()

    #InternalIOBus = Param.NoncoherentXBar("IObar")

    #InternalMemCtrls = VectorParam.MemCtrl("internal mem")

    InternalMemRanges = VectorParam.AddrRange("ghjv")
    # Processors = []
    
    def create_cxlmem_intf(self, intf, r, i, intlv_bits, intlv_size,
                    xor_low_bit):
        return 0 

    def config_cxlmem(self, options):
        return 0
    
class DummyCore(ClockedObject):
    type = 'DummyCore'
    cxx_class = 'gem5::DummyCore'
    cxx_header = "dev/cxl/cxlmem/dummycore.hh"
    dummyMemSidePort = RequestPort("Dummy")