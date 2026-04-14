#ifndef __DEV_CXL_CXLMEM_CXLMEMDEV_HH__
#define __DEV_CXL_CXLMEM_CXLMEMDEV_HH__

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
#include "params/CxlMemDev.hh"
#include "sim/byteswap.hh"
#include "dev/pci/device.hh"
#include "dev/cxl/device.hh"
#include "mem/port.hh"

namespace gem5
{

class CxlMemDev : public CxlDevice
{
  private:
    // 
    std::vector<AddrRange> internal_mem_ranges;
    CoherentXBar* internal_mem_bar;
  public:
    CxlMemDev(const CxlMemDevParams &p);

    /**
     * Methods inherited from PciDevice
     */
    // void intrPost();

    Tick writeConfig(PacketPtr pkt) override;
    Tick readConfig(PacketPtr pkt) override;

    Tick read(PacketPtr pkt) override;
    Tick write(PacketPtr pkt) override;

    AddrRangeList getAddrRanges() const override;

    /**
     * Checkpoint support
     */
    void serialize(CheckpointOut &cp) const override;
    void unserialize(CheckpointIn &cp) override;
};

}

#endif