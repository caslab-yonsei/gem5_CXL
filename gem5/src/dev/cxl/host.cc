/*
 * Copyright (c) 2022 SHIN.Jongmin
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

#include "dev/cxl/host.hh"
#include "dev/pci/host.hh"

#include <utility>

#include "debug/CxlHost.hh"
#include "dev/cxl/device.hh"
#include "dev/platform.hh"
#include "params/GenericCxlHost.hh"
#include "params/CxlHost.hh"
#include "dev/pci/device.hh"
#include "params/GenericCxlHost.hh"
#include "params/PciHost.hh"

namespace gem5
{

CxlHost::CxlHost(const CxlHostParams &p)
    : PciHost(p)
{}

CxlHost::~CxlHost()
{
    //PciHost::~PciHost();
}



GenericCxlHost::GenericCxlHost(const GenericCxlHostParams &p)
    : CxlHost(p),
      platform(*p.platform),
      confBase(p.conf_base), confSize(p.conf_size),
      confDeviceBits(p.conf_device_bits),
      cxlPioBase(p.cxl_pio_base), cxlMemBase(p.cxl_mem_base),
      cxlDmaBase(p.cxl_dma_base), 
      pciPioBase(p.pci_pio_base), pciMemBase(p.pci_mem_base),
      pciDmaBase(p.pci_dma_base)
{
}

GenericCxlHost::~GenericCxlHost()
{
}


Tick
GenericCxlHost::read(PacketPtr pkt)
{
    const auto dev_addr(decodeAddress(pkt->getAddr() - confBase));
    const Addr size(pkt->getSize());

    DPRINTF(CxlHost, "%02x:%02x.%i: read: offset=0x%x, size=0x%x\n",
            dev_addr.first.bus, dev_addr.first.dev, dev_addr.first.func,
            dev_addr.second,
            size);

    PciDevice *const pci_dev(getDevice(dev_addr.first));
    if (pci_dev) {
        // @todo Remove this after testing
        pkt->headerDelay = pkt->payloadDelay = 0;
        return pci_dev->readConfig(pkt);
    } else {
        uint8_t *pkt_data(pkt->getPtr<uint8_t>());
        std::fill(pkt_data, pkt_data + size, 0xFF);
        pkt->makeAtomicResponse();
        return 0;
    }
}

Tick
GenericCxlHost::write(PacketPtr pkt)
{
    const auto dev_addr(decodeAddress(pkt->getAddr() - confBase));

    DPRINTF(CxlHost, "%02x:%02x.%i: write: offset=0x%x, size=0x%x\n",
            dev_addr.first.bus, dev_addr.first.dev, dev_addr.first.func,
            dev_addr.second,
            pkt->getSize());

    PciDevice *const pci_dev(getDevice(dev_addr.first));
    panic_if(!pci_dev,
             "%02x:%02x.%i: Write to config space on non-existent PCI device\n",
             dev_addr.first.bus, dev_addr.first.dev, dev_addr.first.func);

    // @todo Remove this after testing
    pkt->headerDelay = pkt->payloadDelay = 0;

    return pci_dev->writeConfig(pkt);
}

AddrRangeList
GenericCxlHost::getAddrRanges() const
{
    return AddrRangeList({ RangeSize(confBase, confSize) });
}

std::pair<PciBusAddr, Addr>
GenericCxlHost::decodeAddress(Addr addr)
{
    const Addr offset(addr & mask(confDeviceBits));
    const Addr bus_addr(addr >> confDeviceBits);

    return std::make_pair(
        PciBusAddr(bits(bus_addr, 15, 8),
                   bits(bus_addr,  7, 3),
                   bits(bus_addr,  2, 0)),
        offset);
}


void
GenericCxlHost::postInt(const PciBusAddr &addr, PciIntPin pin)
{
    platform.postPciInt(mapPciInterrupt(addr, pin));
}

void
GenericCxlHost::clearInt(const PciBusAddr &addr, PciIntPin pin)
{
    platform.clearPciInt(mapPciInterrupt(addr, pin));
}


uint32_t
GenericCxlHost::mapPciInterrupt(const PciBusAddr &addr, PciIntPin pin) const
{
    const PciDevice *dev(getDevice(addr));
    assert(dev);

    return dev->interruptLine();
}

}