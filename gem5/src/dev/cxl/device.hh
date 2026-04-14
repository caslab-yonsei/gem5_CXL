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

#ifndef __DEV_CXL_DEVICE_HH__
#define __DEV_CXL_DEVICE_HH__

#include <array>
#include <cstring>
#include <vector>

#include "dev/dma_device.hh"
#include "dev/pci/host.hh"
#include "dev/pci/pcie/device.hh"
#include "dev/pci/pcireg.h"
#include "params/PciBar.hh"
#include "params/PciBarNone.hh"
#include "params/PciDevice.hh"
#include "params/PciIoBar.hh"
#include "params/PciLegacyIoBar.hh"
#include "params/PciMemBar.hh"
#include "params/PciMemUpperBar.hh"
#include "dev/cxl/host.hh"
#include "dev/cxl/cxlreg.h"
#include "params/CxlBar.hh"
#include "params/CxlBarNone.hh"
#include "params/CxlDevice.hh"
#include "params/CxlIoBar.hh"
#include "params/CxlLegacyIoBar.hh"
#include "params/CxlMemBar.hh"
#include "params/CxlMemUpperBar.hh"
#include "sim/byteswap.hh"
#include "dev/pci/device.hh"

namespace gem5
{

class CxlBar : public PciBar
{
  public:
    CxlBar(const CxlBarParams &p);

    bool isCxlDevice() { return true; }

    virtual bool isMem() const { return false; };
    virtual bool isIo() const { return false; }

    virtual uint32_t write(const CxlHost::DeviceInterface &host, uint32_t val) = 0;

    AddrRange range() const;
    Addr addr() const;
    Addr size() const;
    void size(Addr value) { _size = value; }
};

class CxlBarNone : public CxlBar
{
  public:
    CxlBarNone(const CxlBarNoneParams &p);

    uint32_t write(const CxlHost::DeviceInterface &host, uint32_t val) override;
};

class CxlIoBar : public CxlBar
{
  protected:
    BitUnion32(Bar)
        Bitfield<31, 2> addr;
        Bitfield<1> reserved;
        Bitfield<0> io;
    EndBitUnion(Bar)
  public:
    CxlIoBar(const CxlIoBarParams &p, bool legacy);
    //bool isIO() const override { return true; }

    uint32_t write(const CxlHost::DeviceInterface &host, uint32_t val) override;
};

// PciLegacyIoBar : 레거시 중 레거시라서 CXL용으로 만들 이유가 없음.
class CxlLegacyIoBar : public CxlIoBar
{
  protected:
    Addr fixedAddr;

  public:
    CxlLegacyIoBar(const CxlLegacyIoBarParams &p) : CxlIoBar(p, true)
    {
        // Save the address until we get a host to translate it.
        fixedAddr = p.addr;
    }

    uint32_t
    write(const CxlHost::DeviceInterface &host, uint32_t val) override
    {
        // Update the address now that we have a host to translate it.
        _addr = host.pioAddr(fixedAddr);
        // Ignore writes.
        return 0;
    }
};

// PciMemBar
class CxlMemBar : public CxlBar
{
  BitUnion32(Bar)
        Bitfield<31, 3> addr;
        SubBitUnion(type, 2, 1)
            Bitfield<2> wide;
            Bitfield<1> reserved;
        EndSubBitUnion(type)
        Bitfield<0> io;
    EndBitUnion(Bar)
    bool _wide = false;
    uint64_t _lower = 0;
    uint64_t _upper = 0;
  public:
    CxlMemBar(const CxlMemBarParams &p);
    bool isMem() const override { return true; }
    uint32_t write(const CxlHost::DeviceInterface &host, uint32_t val) override;
    bool wide() const { return _wide; }
    void wide(bool val) { _wide = val; }

    uint64_t upper() const { return _upper; }
    void
    upper(const PciHost::DeviceInterface &host, uint32_t val)
    {
        _upper = (uint64_t)val << 32;

        // Update our address.
        _addr = host.memAddr(upper() + lower());
    }

    uint64_t lower() const { return _lower; }
};


// PciMemUpperBar
class CxlMemUpperBar : public CxlBar
{
  private:
    PciMemBar *_lower = nullptr;
  public:
    CxlMemUpperBar(const CxlMemUpperBarParams &p);
    void
    lower(PciMemBar *val)
    {
        _lower = val;
        // Let our lower half know we're up here.
        _lower->wide(true);
    }
    uint32_t write(const CxlHost::DeviceInterface &host, uint32_t val) override;
};


class CxlDevice : public PcieDevice
{
  public:
    const static int revlist[MAXID+1];
    const static int lenlist[MAXID+1];
    const static int idlist[MAXID+1];

    PCIE_DVSEC_FOR_CXL_DEV*     dvsec_pci_dvsec_for_cxl_dev;
    NON_CXL_FUNCTION_MAP*       dvsec_non_cxl_func_map;
    CXL_EXTENSIONS_FOR_PORTS*   dvsec_cxl_ext_for_ports;
    GPF_FOR_CXL_PORTS*          dvsec_gpf_for_cxl_ports;
    GPF_FOR_CXL_DEV*            dvsec_gpf_for_cxl_dev;
    FLEX_BUS_PORTS*             dvsec_flex_bus_ports;
    REGISTER_LOCATOR*           dvsec_reg_locator;
    MLD*                        dvsec_mld;

  public:
    bool getBAR(Addr addr, int &num, Addr &offs);
    virtual Tick writeConfig(PacketPtr pkt);
    virtual Tick readConfig(PacketPtr pkt);
    Addr cxlToDma(Addr cxl_addr) const;

    void intrPost();
    void intrClear();

    uint8_t interruptLine() const;
    AddrRangeList getAddrRanges() const override;

    CxlDevice(const CxlDeviceParams &params);
    void initCxlDvsecs(const CxlDeviceParams &params);

    void serialize(CheckpointOut &cp) const override;
    void unserialize(CheckpointIn &cp) override;
    void match_dvsec_struct_config();

    const CxlBusAddr &busAddr() const;
};

}

#endif