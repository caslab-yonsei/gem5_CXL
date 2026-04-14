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

#include "dev/cxl/device.hh"

#include <list>
#include <string>
#include <vector>

#include "base/inifile.hh"
#include "base/intmath.hh"
#include "base/logging.hh"
#include "base/str.hh"
#include "base/trace.hh"
#include "debug/CxlDevice.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "sim/byteswap.hh"
#include "debug/PciDeviceECapability.hh"

namespace gem5
{

const int 
CxlDevice::revlist[MAXID+1] = {0x2,    -1,    0,      0,      0,      0,      -1, 0x2,
                                0,     0,      -2};
const int 
CxlDevice::lenlist[MAXID+1] = {0x3C,   -1,    0x2C,   0x28,   0x10,   0x10,   -1, 0x20,
                                0,     0x10,   -2};
const int 
CxlDevice::idlist[MAXID+1] = {0x0,    -1,    0x2,    0x3,    0x4,    0x5,    -1, 0x7,
                                0x8,   0x9,    -2};

inline bool testRange(uint8_t* map, int st, int size)
{
    DPRINTF(CxlDevice, "testRange Start!\n");
    if(st <= 0 || size < 0) {
        DPRINTF(CxlDevice, "testRange st %d, size %d. return false\n", st, size);
        return false;
    }
    DPRINTF(CxlDevice, "testRange Start Iter!\n");
    for (int i = st; i < st+size; i ++)
    {
        DPRINTF(CxlDevice, "testRange map[%d]\n", i);
        if(map[i] != 0) return false;
        if(st <= 0) return false;           // For CXL DVSEC, st must be bigger than 0xFF (maybe)
        map[i] = 1;
    }
    return true;
}

inline bool initCxlDVSECHeader(DVSEC* dvsec, int dvsecid, int len, int rev)
{
    DPRINTF(CxlDevice, "initCxlDVSECHeader called. dvsecid %#x\n", dvsecid);
    dvsec->e_capa_header.e_capa_id = htole((uint16_t)0x0023);
    dvsec->dvsec_header.vendor_id  = htole((uint16_t)0x1E98);
    dvsec->dvsec_header.dvsec_id = htole((uint16_t)dvsecid);
    dvsec->dvsec_header.rev_leng = htole( (uint16_t)((len << 4) | rev) );

    return true;
}


/*
 Map pointers to config register (extended extended area)
*/
void
CxlDevice::initCxlDvsecs(const CxlDeviceParams &params)
{
    uint8_t addrmap[0x1000] = {0,};
    std::map<int, DVSEC*> offsetlist;

    DPRINTF(CxlDevice, "CxlDevice::initCxlDvsecs Start!\n");

    for (int i = 0; i < MAXID+1; i ++)
    {
        int len = lenlist[i];
        int rev = revlist[i];
        int id = idlist[i];
        int offset = -1;
        DVSEC* curr = NULL;

        DPRINTF(CxlDevice, "CxlDevice::initCxlDvsecs id %#x (%d/%d)\n", id, i, MAXID+1);

        switch (id)
        {
        case CXL_CAP_ID_PCIE_DVSEC_FOR_CXL_DEV:
            offset = params.PCIeDVSEC_EXT_BaseOffset;
            rev = params.PCIeDVSEC_DVSEC_Rev;
            curr = (DVSEC*)(config.data + offset);
            dvsec_pci_dvsec_for_cxl_dev = (PCIE_DVSEC_FOR_CXL_DEV*)(curr);

            break;
        case CXL_CAP_ID_NON_CXL_FUNCTION_MAP:
            offset = params.NonCxlFunctionMap_EXT_BaseOffset;
            rev = params.NonCxlFunctionMap_EXT_BaseOffset;
            curr = (DVSEC*)(config.data + offset);
            dvsec_non_cxl_func_map = (NON_CXL_FUNCTION_MAP*)(curr);
            
            break;
        case CXL_CAP_ID_CXL_EXTENSIONS_FOR_PORTS:
            offset = params.CxlExtForPorts_EXT_BaseOffset;
            rev = params.CxlExtForPorts_EXT_BaseOffset;
            curr = (DVSEC*)(config.data + offset);
            dvsec_cxl_ext_for_ports = (CXL_EXTENSIONS_FOR_PORTS*)(curr);

            break;
        case CXL_CAP_ID_GPF_FOR_CXL_PORTS:
            offset = params.GpfForCxlPorts_EXT_BaseOffset;
            rev = params.GpfForCxlPorts_EXT_BaseOffset;
            curr = (DVSEC*)(config.data + offset);
            dvsec_gpf_for_cxl_ports = (GPF_FOR_CXL_PORTS*)(curr);

            break;
        case CXL_CAP_ID_GPF_FOR_CXL_DEV:
            offset = params.GpfForDev_EXT_BaseOffset;
            rev = params.GpfForDev_EXT_BaseOffset;
            curr = (DVSEC*)(config.data + offset);
            dvsec_gpf_for_cxl_dev = (GPF_FOR_CXL_DEV*)(curr);

            break;
        case CXL_CAP_ID_FOR_FLEX_BUS_PORTS:
            offset = params.FlexBusPorts_EXT_BaseOffset;
            rev = params.FlexBusPorts_EXT_BaseOffset;
            curr = (DVSEC*)(config.data + offset);
            dvsec_flex_bus_ports = (FLEX_BUS_PORTS*)(curr);

            break;
        case CXL_CAP_ID_REGISTER_LOCATOR:
            offset = params.RegLocator_EXT_BaseOffset;
            rev = params.RegLocator_EXT_BaseOffset;
            curr = (DVSEC*)(config.data + offset);
            dvsec_reg_locator = (REGISTER_LOCATOR*)(curr);

            break;
        case CXL_CAP_ID_MLD:
            offset = params.MLD_EXT_BaseOffset;
            rev = params.MLD_EXT_BaseOffset;
            curr = (DVSEC*)(config.data + offset);
            dvsec_mld = (MLD*)(curr);

            break;
        case CXL_CAP_ID_TEST:
            /* code */
            break;
        default:
            break;
        }

        DPRINTF(CxlDevice, "fin find id %d/%d. look addrmap offset %d, len %d\n", i, MAXID+1, offset, len);

        if( testRange(addrmap, offset, len) == true ){
            DPRINTF(CxlDevice, "testRange succ %d/%d\n", i, MAXID+1);
        }
        else {
            continue;
            //panic("CXL Addr overlaped");
        }

        if( !(offset < 0x100) ){
            DPRINTF(CxlDevice, "Inset to offsetlist. offset %#x\n", offset);
            offsetlist.insert({offset, curr});
        }

        else{
            panic("Offset Error");
        }

        initCxlDVSECHeader(curr, id, len, rev);
    }
    std::map<int, DVSEC*>::iterator iter;
    for (iter = offsetlist.begin(); iter != offsetlist.end(); iter++)
    {
        config_tool.setNextECapaPtr(&(iter->second->e_capa_header), iter->first);
    }
}


CxlBar::CxlBar(const CxlBarParams &p)
     : PciBar(p) 
{

}

AddrRange 
CxlBar::range() const
{
    return PciBar::range();
}

Addr
CxlBar::addr() const
{
    return PciBar::addr();
}

Addr
CxlBar::size() const
{
    return PciBar::size();
}

CxlBarNone::CxlBarNone(const CxlBarNoneParams &p) 
    : CxlBar(p) 
{

}

uint32_t
CxlBarNone::write(const CxlHost::DeviceInterface &host, uint32_t val)
{
    return 0;
}

CxlIoBar::CxlIoBar(const CxlIoBarParams &p, bool legacy)
        : CxlBar(p)
{
    _size = p.size;
    if (!legacy) {
        Bar bar = _size;
        fatal_if(!_size || !isPowerOf2(_size) || bar.io || bar.reserved,
                "Illegal size %d for bar %s.", _size, name());
    }
}


uint32_t
CxlIoBar::write(const CxlHost::DeviceInterface &host, uint32_t val)
{
    // Mask away the bits fixed by hardware.
    Bar bar = val & ~(_size - 1);
    // Set the fixed bits to their correct values.
    bar.reserved = 0;
    bar.io = 1;

    // Update our address.
    _addr = host.pioAddr(bar.addr << 2);

    // Return what should go into config space.
    return bar;
}


CxlMemBar::CxlMemBar(const CxlMemBarParams &p)
        : CxlBar(p)
{
    _size = p.size;
    Bar bar = _size;
    fatal_if(!_size || !isPowerOf2(_size) || bar.io || bar.type,
            "Illegal size %d for bar %s.", _size, name());
}

uint32_t 
CxlMemBar::write(const CxlHost::DeviceInterface &host, uint32_t val)
{
    // Mask away the bits fixed by hardware.
    Bar bar = val & ~(_size - 1);
    // Set the fixed bits to their correct values.
    bar.type.wide = wide() ? 1 : 0;
    bar.type.reserved = 0;
    bar.io = 0;

    // Keep track of our lower 32 bits.
    _lower = bar.addr << 3;

    // Update our address.
    _addr = host.memAddr(upper() + lower());

    // Return what should go into config space.
    return bar;
}

CxlMemUpperBar::CxlMemUpperBar(const CxlMemUpperBarParams &p)
        : CxlBar(p)
{
    
}

uint32_t
CxlMemUpperBar::write(const CxlHost::DeviceInterface &host, uint32_t val) 
{
    assert(_lower);

    // Mask away bits fixed by hardware, if any.
    Addr upper = val & ~((_lower->size() - 1) >> 32);

    // Let our lower half know about the update.
    _lower->upper(host, upper);

    return upper;
}

CxlDevice::CxlDevice(const CxlDeviceParams &p)
    : PcieDevice(p)
{
    initCxlDvsecs(p);
}

bool
CxlDevice::getBAR(Addr addr, int &num, Addr &offs)
{
    return PciDevice::getBAR(addr, num, offs);
}

Tick
CxlDevice::readConfig(PacketPtr pkt)
{
    return PciDevice::readConfig(pkt);
}

Tick
CxlDevice::writeConfig(PacketPtr pkt)
{
    return PciDevice::writeConfig(pkt);
}

Addr
CxlDevice::cxlToDma(Addr cxl_addr) const
{
    return PciDevice::pciToDma(cxl_addr);
}

void 
CxlDevice::intrPost()
{
    PciDevice::intrPost();
}

void 
CxlDevice::intrClear()
{
    PciDevice::intrClear();
}

uint8_t 
CxlDevice::interruptLine() const 
{ 
    return PciDevice::interruptLine();
}

AddrRangeList 
CxlDevice::getAddrRanges() const
{
    return PciDevice::getAddrRanges();
}

void 
CxlDevice::serialize(CheckpointOut &cp) const
{
    PciDevice::serialize(cp);
}

void 
CxlDevice::unserialize(CheckpointIn &cp)
{
    PciDevice::unserialize(cp);
}

const CxlBusAddr &
CxlDevice::busAddr() const
{
    return (CxlBusAddr&)PciDevice::busAddr();
}


}