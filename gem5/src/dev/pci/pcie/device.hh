/*
 * Copyright (c) 2022 SHIN. Jongmin Limited
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
 * Interface for devices using CXL configuration
 */

#ifndef __DEV_PCIE_DEVICE_HH__
#define __DEV_PCIE_DEVICE_HH__

#include <array>
#include <cstring>
#include <vector>

#include "dev/dma_device.hh"
#include "dev/pci/host.hh"
#include "dev/pci/pcireg.h"
#include "params/PciBar.hh"
#include "params/PciBarNone.hh"
#include "params/PcieDevice.hh"
#include "params/PciDevice.hh"
#include "params/PciIoBar.hh"
#include "params/PciLegacyIoBar.hh"
#include "params/PciMemBar.hh"
#include "params/PciMemUpperBar.hh"
#include "sim/byteswap.hh"
#include "dev/pci/device.hh"

namespace gem5
{


class PcieDevice : public PciDevice
{
  public:
    bool getBAR(Addr addr, int &num, Addr &offs);
    virtual Tick writeConfig(PacketPtr pkt);
    virtual Tick readConfig(PacketPtr pkt);

    void intrPost();
    void intrClear();

    uint8_t interruptLine() const;
    virtual AddrRangeList getAddrRanges() const override;

    PcieDevice(const PcieDeviceParams &params);

    void serialize(CheckpointOut &cp) const override;
    void unserialize(CheckpointIn &cp) override;
    void match_dvsec_struct_config();

    //const CxlBusAddr &busAddr() const;
};

}

#endif