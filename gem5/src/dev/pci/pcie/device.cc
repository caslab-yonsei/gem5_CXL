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

#include "dev/pci/pcie/device.hh"

#include <list>
#include <string>
#include <vector>

#include "base/inifile.hh"
#include "base/intmath.hh"
#include "base/logging.hh"
#include "base/str.hh"
#include "base/trace.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "sim/byteswap.hh"

namespace gem5
{


PcieDevice::PcieDevice(const PcieDeviceParams &p)
    : PciDevice(p)
{

}

bool
PcieDevice::getBAR(Addr addr, int &num, Addr &offs)
{
    return PciDevice::getBAR(addr, num, offs);
}

Tick
PcieDevice::readConfig(PacketPtr pkt)
{
    return PciDevice::readConfig(pkt);
}

Tick
PcieDevice::writeConfig(PacketPtr pkt)
{
    return PciDevice::writeConfig(pkt);
}

// Addr
// PcieDevice::pciToDma(Addr cxl_addr) const
// {
//     return PciDevice::pciToDma(cxl_addr);
// }

void 
PcieDevice::intrPost()
{
    PciDevice::intrPost();
}

void 
PcieDevice::intrClear()
{
    PciDevice::intrClear();
}

uint8_t 
PcieDevice::interruptLine() const 
{ 
    return PciDevice::interruptLine();
}

AddrRangeList 
PcieDevice::getAddrRanges() const
{
    return PciDevice::getAddrRanges();
}

void 
PcieDevice::serialize(CheckpointOut &cp) const
{
    PciDevice::serialize(cp);
}

void 
PcieDevice::unserialize(CheckpointIn &cp)
{
    PciDevice::unserialize(cp);
}

// const PciBusAddr &
// PcieDevice::busAddr() const
// {
//     return (PciBusAddr&)PciDevice::busAddr();
// }


}