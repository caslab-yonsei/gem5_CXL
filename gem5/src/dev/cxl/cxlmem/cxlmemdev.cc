#include "dev/cxl/cxlmem/cxlmemdev.hh"
#include "debug/AddrRanges.hh"
#include "debug/CxlDevice.hh"
#include "dev/cxl/cxlmem/dummycore.hh"

namespace gem5
{

CxlMemDev::CxlMemDev(const CxlMemDevParams &p)
    : CxlDevice(p),internal_mem_ranges(p.InternalMemRanges), internal_mem_bar(p.InternalMemBus)
{
    DPRINTF(AddrRanges, "sex\n");
}

/**
 * Methods inherited from PciDevice
 */
//void
// CxlMemDev::intrPost()
// {}

Tick
CxlMemDev::writeConfig(PacketPtr pkt)
{
    return CxlDevice::writeConfig(pkt);
}

Tick
CxlMemDev::readConfig(PacketPtr pkt)
{
    return CxlDevice::readConfig(pkt);
}

Tick
CxlMemDev::read(PacketPtr pkt)
{
    DPRINTF(CxlDevice, "Gay Read!!\n");
    return 0;//CxlDevice::read(pkt);
}

Tick
CxlMemDev::write(PacketPtr pkt)
{
    DPRINTF(CxlDevice, "Gay Write!!\n");
    return 0;//CxlDevice::write(pkt);
}

AddrRangeList
CxlMemDev::getAddrRanges() const
{
    // AddrRangeList list;
    // return list;
//     assert(pioSize != 0);
    // AddrRangeList ranges;
    // DPRINTF(AddrRanges, "registering range: %#x-%#x\n", pioAddr, pioSize);
    // ranges.push_back(RangeSize(pioAddr, pioSize));
    // return ranges;
    
    AddrRangeList ranges(internal_mem_ranges.begin(), internal_mem_ranges.end());
    DPRINTF(AddrRanges, "Gay getAddrRanges was called. %s\n", ranges.front().to_string());
    return ranges;
    // DPRINTF(CxlDevice, "Gay getAddrRanges!! %s\n", CxlDevice::getAddrRanges().front().to_string());
    // return CxlDevice::getAddrRanges();
}

/**
 * Checkpoint support
 */
void 
CxlMemDev::serialize(CheckpointOut &cp) const
{}

void 
CxlMemDev::unserialize(CheckpointIn &cp)
{}



} // namespace gem5
