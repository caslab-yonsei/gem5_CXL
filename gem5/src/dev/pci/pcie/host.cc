/*
 * Copyright (c) 2022 SHIN
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
 * Copyright (c) 2004-2005 The Regents of The University of Michigan
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
 * Interface for devices using PCIe configuration
 */

#include "host.hh"
#include "device.hh"
#include "debug/PcieHost.hh"
#include "dev/pci/host.hh"
#include "dev/pci/pcie/host.hh"

namespace gem5
{

PcieHost::PcieHost(const PcieHostParams &p)
    : PciDevice(p)//, legacy_host(p.legacy)
{
    legacy_host = nullptr;
}

bool 
PcieHost::isPcieEcam(Addr addr)
{

    return false;
}

Ecam 
PcieHost::hostAddrToDevId(Addr ecam_addr)
{
    Ecam ecam_info;
    ecam_info.baseAddr  = (ecam_addr & MASK_BASEADDR);
    ecam_info.busnum = (ecam_addr & MASK_BUS_NUMBER) >> LOW_BITNUM_BUS_NUMBER;
    ecam_info.devnum = (ecam_addr & MASK_DEVICE_NUMBER) >> LOW_BITNUM_DEVICE_NUMBER;
    ecam_info.fnnum =  (ecam_addr & MASK_FUNCTION_NUMBER) >> LOW_BITNUM_FUNCTION_NUMBER;
    ecam_info.regnum = (ecam_addr & MASK_REGISTER_NUMBER);// >> LOW_BITNUM_REGISTER_NUMBER;
    ecam_info.extReg = (ecam_addr & MASK_EXTENDED_REGISTER_NUMBER) >> LOW_BITNUM_EXTENDED_REGISTER_NUMBER;
    ecam_info.legacyReg =  (ecam_addr & MASK_LEGACY_REGISTER_NUMBER) >> LOW_BITNUM_LEGACY_REGISTER_NUMBER;
    ecam_info.gitEnable =  (ecam_addr & MASK_BYTEENABLE) >> LOW_BITNUM_BYTEENABLE;

    return ecam_info;
}

GenericPcieHost::GenericPcieHost(const GenericPcieHostParams &p)
    : PcieHost(p),
    platform(*p.platform),
    confBase(p.conf_base), confSize(p.conf_size), confDeviceBits(p.conf_device_bits),
    pciPioBase(p.conf_base), pciMemBase(0), pciDmaBase(0)
{
    legacy_host = p.host;
    ExtendedBaseConfigAddr = p.pci_pio_base;
}

// GenericPcieHost::~GenericPcieHost()
// {}

AddrRangeList 
GenericPcieHost::getAddrRanges() const
{
    return AddrRangeList({ RangeSize(confBase, confSize) });
}

Addr
GenericPcieHost::pioAddr(const PciBusAddr &bus_addr, Addr pci_addr) const 
{
    return pciPioBase + pci_addr;
}

Addr 
GenericPcieHost::memAddr(const PciBusAddr &bus_addr, Addr pci_addr) const 
{
    return pciMemBase + pci_addr;
}

Addr 
GenericPcieHost::dmaAddr(const PciBusAddr &bus_addr, Addr pci_addr) const 
{
    return pciDmaBase + pci_addr;
}

Tick
GenericPcieHost::read(PacketPtr pkt)
{
    Ecam info = hostAddrToDevId(pkt->getAddr());

    DPRINTF(PcieHost, "NEPU PCIe host controller. read was called. Addr %llx, baseAddr %llx\n", pkt->getAddr(), info.baseAddr);
    
    if(info.baseAddr == ExtendedBaseConfigAddr){
        // It is a PCIe Extended config register
        const Addr size(pkt->getSize());
        const auto dev_addr(decodeAddress(info));

        DPRINTF(PcieHost, "%02x:%02x.%i: read: offset=0x%x, size=0x%x\n",
            dev_addr.first.bus, dev_addr.first.dev, dev_addr.first.func,
            dev_addr.second,
            size);

        PciDevice *const pci_dev(legacy_host->getDevice(dev_addr.first));
        if (pci_dev) {
            DPRINTF(PcieHost, "Found device %02x:%02x.%i\n", dev_addr.first.bus, dev_addr.first.dev, dev_addr.first.func);

            // @todo Remove this after testing
            pkt->headerDelay = pkt->payloadDelay = 0;
            return pci_dev->readConfig(pkt);
        } 
        else {
            DPRINTF(PcieHost, "Failed to find device %02x:%02x.%i\n", dev_addr.first.bus, dev_addr.first.dev, dev_addr.first.func);
            uint8_t *pkt_data(pkt->getPtr<uint8_t>());
            std::fill(pkt_data, pkt_data + size, 0xFF);
            pkt->makeAtomicResponse();
            return 0;
        }
    }
    else {
        DPRINTF(PcieHost, "NEPU PCIe host controller. Legacy Addr %llx, baseAddr %llx\n", pkt->getAddr(), info.baseAddr);
        return legacy_host->read(pkt);
    }
}

Tick
GenericPcieHost::write(PacketPtr pkt)
{
    Ecam info = hostAddrToDevId(pkt->getAddr());
    if(info.baseAddr == ExtendedBaseConfigAddr){
        // It is a PCIe Extended config register
        const Addr size(pkt->getSize());
        const auto dev_addr(decodeAddress(info));

        DPRINTF(PcieHost, "%02x:%02x.%i: write: offset=0x%x, size=0x%x\n",
            dev_addr.first.bus, dev_addr.first.dev, dev_addr.first.func,
            dev_addr.second,
            size);

        PciDevice *const pci_dev(legacy_host->getDevice(dev_addr.first));
        
        pkt->headerDelay = pkt->payloadDelay = 0;
        return pci_dev->writeConfig(pkt);
    }
    else {
        return legacy_host->write(pkt);
    }

}

std::pair<PciBusAddr, Addr>
GenericPcieHost::decodeAddress(Addr addr)
{
    const Addr offset(addr & mask(confDeviceBits));
    const Addr bus_addr(addr >> confDeviceBits);

    return std::make_pair(
        PciBusAddr(bits(bus_addr, 15, 8),
                   bits(bus_addr,  7, 3),
                   bits(bus_addr,  2, 0)),
        offset);
}

std::pair<PciBusAddr, Addr>
GenericPcieHost::decodeAddress(Ecam ecam)
{
    const Addr offset(ecam.regnum);
    return std::make_pair(
        PciBusAddr(ecam.busnum ,
                   ecam.devnum,
                   ecam.fnnum),
        offset);
}

void 
GenericPcieHost::postInt(const PciBusAddr &addr, PciIntPin pin)
{

}

void 
GenericPcieHost::clearInt(const PciBusAddr &addr, PciIntPin pin) 
{

}

uint32_t 
GenericPcieHost::mapPciInterrupt(const PciBusAddr &bus_addr,
                                     PciIntPin pin) const
{
    return 0;
}

}