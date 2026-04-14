/*
 * Copyright (c) 2011-2013, 2015 ARM Limited
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
 * Copyright (c) 2006 The Regents of The University of Michigan
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
 *
 * Authors: Ali Saidi
 *          Steve Reinhardt
 *          Andreas Hansson
 */

/**
 * @file
 * Implementation of a memory-mapped bridge that connects a master
 * and a slave through a request and response queue.
 */
#include "base/inifile.hh"
#include "base/intmath.hh"
//#include "base/misc.hh"
#include "base/str.hh"
#include "base/trace.hh"
#include "mem/PciBridge.hh"

#include "base/trace.hh"
#include "debug/PciBridge.hh"
#include "params/PciBridge.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "sim/byteswap.hh"
#include "sim/core.hh"

#include "debug/PciBridgeX.hh"

#include "debug/CXLProtocol.hh"
#include "mem/cxl/cxl_packet.hh"
#include "mem/cxl/cxl_cache_packet.hh"
#include "mem/request.hh"

using namespace std ;

namespace gem5{

PciBridge::PciBridgeSlavePort::PciBridgeSlavePort(const std::string& _name,
                                         PciBridge& _bridge,
                                         bool upstream , 
                                         uint8_t _slaveID , 
                                         uint8_t _rcid , 
                                         Cycles _delay, int _resp_limit
                                          )
    : SlavePort(_name, &_bridge), bridge(_bridge),
      delay(_delay),
      outstandingResponses(0), retryReq(false), respQueueLimit(_resp_limit),
      sendEvent([this]{ trySendTiming(); }, _name)
{
  is_upstream = upstream ; // this is an upstream slave port used to accept requests from CPU.
  slaveID = _slaveID ;
  rcid = _rcid ; 
  // slave ID : 0 for upstream root port , 1 for downstream root port 1 , 2 for downstream root port 2 , 3 for downstream root port 3 . 
}

PciBridge::PciBridgeMasterPort::PciBridgeMasterPort(const std::string& _name,
                                           PciBridge& _bridge, uint8_t _rcid , 
                                           Cycles _delay, int _req_limit)
    : MasterPort(_name, &_bridge), bridge(_bridge), totalCount(0) , 
      delay(_delay), reqQueueLimit(_req_limit),
      sendEvent([this]{ trySendTiming(); }, _name) , 
      countEvent([this]{ incCount() ; } , _name )
{
rcid = _rcid ; 
bridge.schedule(countEvent, curTick() + (uint64_t)1000000000000) ; 

}

PciBridge::~PciBridge()
{
  delete storage_ptr1 ;
  delete storage_ptr2 ; 
  delete storage_ptr3 ; 
  delete storage_ptr4 ;
}



void PciBridge::initialize_ports ( config_class * storage_ptr , const PciBridgeParams& p)
{
  storage_ptr->valid_io_limit = false ; 
  storage_ptr->valid_io_base = false ; 
  storage_ptr->valid_memory_base = false ; 
  storage_ptr->valid_memory_limit = false ; 
  storage_ptr->valid_prefetchable_memory_base = false ; 
  storage_ptr->valid_prefetchable_memory_limit = false ; 
  storage_ptr->is_switch = p.is_switch ; 
  storage_ptr->storage1.VendorId = htole(p.VendorId) ;
  storage_ptr->storage1.Command = htole(p.Command) ;
  storage_ptr->storage1.Status = htole(p.Status) ;
  storage_ptr->storage1.ClassCode = htole(p.ClassCode) ; 
  storage_ptr->storage1.SubClassCode = htole(p.SubClassCode) ;
  storage_ptr->storage1.ProgIF = htole(p.ProgIF) ;
  storage_ptr->storage1.Revision = htole(p.Revision) ;
  storage_ptr->storage1.BIST = htole(p.BIST) ;
  storage_ptr->storage1.HeaderType = htole(p.HeaderType) ;
  storage_ptr->storage1.LatencyTimer = htole(p.LatencyTimer) ;
  storage_ptr->storage1.CacheLineSize = htole(p.CacheLineSize) ;
  storage_ptr->storage1.Bar0 = htole(p.Bar0) ;
  storage_ptr->storage1.Bar1 = htole(p.Bar1) ; 
  storage_ptr->storage1.SecondaryLatencyTimer = htole(p.SecondaryLatencyTimer) ; 
  storage_ptr->storage1.MemoryLimit = htole(p.MemoryLimit) ;
  storage_ptr->storage1.MemoryBase = htole(p.MemoryBase) ;
  storage_ptr->storage1.PrefetchableMemoryLimit = htole(p.PrefetchableMemoryLimit) ;
  storage_ptr->storage1.PrefetchableMemoryBase = htole(p.PrefetchableMemoryBase) ;
  storage_ptr->storage1.PrefetchableMemoryLimitUpper = htole(p.PrefetchableMemoryLimitUpper) ;
  storage_ptr->storage1.PrefetchableMemoryBaseUpper = htole(p.PrefetchableMemoryBaseUpper) ; 
  storage_ptr->storage1.IOLimit = htole(p.IOLimit) ;
  storage_ptr->storage1.IOBase = htole(p.IOBase) ;
  storage_ptr->storage1.IOLimitUpper = htole(p.IOLimitUpper) ;
  storage_ptr->storage1.IOBaseUpper = htole(p.IOBaseUpper) ;
  storage_ptr->storage1.CapabilityPointer = htole(p.CapabilityPointer) ;
  storage_ptr->storage1.ExpansionROMBaseAddress = htole(p.ExpansionROMBaseAddress) ;
  storage_ptr->storage1.InterruptPin = htole(p.InterruptPin); 
  storage_ptr->storage1.InterruptLine = htole(p.InterruptLine) ;
  storage_ptr->storage1.BridgeControl = htole(p.BridgeControl) ;
  storage_ptr->storage1.SecondaryStatus = htole(p.SecondaryStatus);
  storage_ptr->storage1.PrimaryBusNumber = htole(p.PrimaryBusNumber) ; 
  storage_ptr->storage1.SecondaryBusNumber = htole(p.SecondaryBusNumber) ;
  storage_ptr->storage1.SubordinateBusNumber = htole(p.SubordinateBusNumber) ;
  for (int i = 0; i<3; i++) storage_ptr->storage1.reserved[i] = 0 ;  // set the reserved values according to PCI bridge header specifications to 0 
  
  // Now need to configure the PCIe capability field , which is implemented for every PCIe device, including a root port
  storage_ptr->storage2.PXCAPNextCapability = htole(p.PXCAPNextCapability) ;
  storage_ptr->storage2.PXCAPCapId = htole(p.PXCAPCapId) ;
 // storage_ptr->storage2.PXCAPCapabilities = htole(p.PXCAPCapabilities) ;
  storage_ptr->storage2.PXCAPDevCapabilities = htole(p.PXCAPDevCapabilities) ; 
  storage_ptr->storage2.PXCAPDevCtrl = htole(p.PXCAPDevCtrl) ;
  storage_ptr->storage2.PXCAPDevStatus = htole(p.PXCAPDevStatus) ; 
  storage_ptr->storage2.PXCAPLinkCap = htole(p.PXCAPLinkCap) ; 
  storage_ptr->storage2.PXCAPLinkCtrl = htole(p.PXCAPLinkCtrl) ;
  storage_ptr->storage2.PXCAPLinkStatus = htole(p.PXCAPLinkStatus) ;
  storage_ptr->storage2.PXCAPRootStatus = htole(p.PXCAPRootStatus) ;
  storage_ptr->storage2.PXCAPRootControl = htole(p.PXCAPRootControl) ;
  
  storage_ptr->storage2.PXCAPSlotCapabilities = 0 ;
  storage_ptr->storage2.PXCAPSlotControl = 0 ;
  storage_ptr->storage2.PXCAPSlotStatus = 0 ;
  
  storage_ptr->configDelay = p.config_latency ; // Latency for configuration space accesses. Copied over from PCI device configuration access time. 
  storage_ptr->barsize[0] = p.BAR0Size ; 
  storage_ptr->barsize[1] = p.BAR1Size ; // indicates whether BAR for the bridges are implemented or not.
  storage_ptr->barflags[0] = 0 ;
  storage_ptr->barflags[1] = 0 ; 
  storage_ptr->PXCAPBaseOffset = p.PXCAPBaseOffset ; 
  storage_ptr->is_valid = 0 ; 
}  

PciBridge::PciBridge(const PciBridgeParams& p)
    : ClockedObject(p),
      slavePort(p.name + ".slave", *this, true, 0, p.rc_id , 
                ticksToCycles(p.delay), p.resp_size),
      slavePort_DMA1( p.name + ".slave_dma1" , *this , false, 1 , p.rc_id , ticksToCycles(p.delay) , p.resp_size) , 
      slavePort_DMA2(p.name + ".slave_dma2" , *this , false ,2 , p.rc_id ,  ticksToCycles(p.delay) , p.resp_size) ,
      slavePort_DMA3(p.name + ".slave_dma3" , *this , false, 3, p.rc_id , ticksToCycles(p.delay) , p.resp_size) ,  
      masterPort_DMA(p.name + ".master_dma" , *this , p.rc_id ,  ticksToCycles(p.delay) , p.req_size) ,  
      masterPort1 (p.name + ".master1", *this,p.rc_id , 
                 ticksToCycles(p.delay), p.req_size) , 
      masterPort2 (p.name + ".master2" , *this , p.rc_id ,  ticksToCycles(p.delay) , p.req_size) , 
      masterPort3(p.name + ".master3" , *this , p.rc_id , ticksToCycles(p.delay) , p.req_size),
      bridge_id(p.BridgeID), CXLAgent(this)
{
  cout<<"PCI Bridge name"<<p.name<<"\n"  ; 
  
  is_transmit = p.is_transmit ; 
  int pci_bus = (p.is_switch) ? p.pci_bus + 1 : p.pci_bus ;
  storage_ptr1 = new config_class (pci_bus , p.pci_dev1, p.pci_func1, 0) ;                        // allocate a new config class to provide PCI functionality to bridge corresponding to first root port
  storage_ptr2 = new config_class (pci_bus , p.pci_dev2 , p.pci_func2,0) ;  // config structure for second root port
  storage_ptr3 = new config_class (pci_bus , p.pci_dev3 , p.pci_func3, 0) ;  // config structure for third root port
  storage_ptr4 = new config_class (p.pci_bus , p.pci_upstream_dev , p.pci_upstream_func, 0) ; 
  storage_ptr1->storage1.DeviceId = htole(p.DeviceId1) ;   // Assign values depending on the PCI-PCI Bridge header configured in python
  storage_ptr2->storage1.DeviceId = htole(p.DeviceId2) ; 
  storage_ptr3->storage1.DeviceId = htole(p.DeviceId3) ; 
  storage_ptr4->storage1.DeviceId = htole(p.DeviceId_upstream) ; 
  
  initialize_ports(storage_ptr1 , p) ; 
  initialize_ports(storage_ptr2, p) ; 
  initialize_ports(storage_ptr3, p) ; 
  initialize_ports(storage_ptr4, p) ; 

  uint16_t capabilities = (is_switch) ? p.PXCAPCapabilities_downstream : p.PXCAPCapabilities ; 
  storage_ptr4->storage2.PXCAPCapabilities = htole(p.PXCAPCapabilities_upstream) ;
  storage_ptr1->storage2.PXCAPCapabilities = htole(capabilities) ;
  storage_ptr2->storage2.PXCAPCapabilities = htole(capabilities) ;
  storage_ptr3->storage2.PXCAPCapabilities = htole(capabilities) ;

 
  

  storage_ptr1->pci_bus = pci_bus ; // should be the same as the Primary Bus Number of the Bridge (Upstream bus).
  storage_ptr1->pci_dev = p.pci_dev1 ;
  storage_ptr1->pci_func = p.pci_func1 ; // should be 0 , because it is configured to be a single function device
  storage_ptr2->pci_bus = pci_bus ; 
  storage_ptr2->pci_dev = p.pci_dev2 ; 
  storage_ptr2->pci_func = p.pci_func2 ;
  storage_ptr3->pci_bus = pci_bus ; 
  storage_ptr3->pci_dev = p.pci_dev3 ; 
  storage_ptr3->pci_func = p.pci_func3 ; 
  storage_ptr4->pci_bus = p.pci_bus ; 
  storage_ptr4->pci_dev = p.pci_upstream_dev ; 
  storage_ptr4->pci_func = p.pci_upstream_func ;  

  storage_ptr1->id = 1 ; 
  storage_ptr2->id = 2 ; 
  storage_ptr3->id = 3 ;
  storage_ptr4->id = 0 ;  
  storage_ptr1->bridge = this ; // Assign the Port being used as the port that is being configured.
  storage_ptr2->bridge = this ; 
  storage_ptr3->bridge = this ;
  storage_ptr4->bridge = this ;   
  p.host->registerBridge(storage_ptr1 , storage_ptr1->BridgeAddr) ; //register the pci-pci bridge with the pci host for config acceses
  p.host->registerBridge(storage_ptr2 , storage_ptr2->BridgeAddr) ; //register the pci-pci bridge with the pci host for config acceses
  p.host->registerBridge(storage_ptr3 , storage_ptr3->BridgeAddr) ; //register the pci-pci bridge with the pci host for config acceses
  if(p.is_switch) p.host->registerBridge(storage_ptr4 , storage_ptr4->BridgeAddr) ;
  rc_id = p.rc_id ; 
  is_switch = p.is_switch ; 

}

// Accessor functions to get address register or base and limit registers values . 
Addr config_class::getBar0()
{
  uint32_t bar0 = letoh(storage1.Bar0) ;   // Assume it is only a 32-bit BAR and never a 64 bit one
  bar0 = bar0 & ~(barsize[0] - 1) ; 
  return (uint64_t)bar0 ; 
}

Addr config_class::getBar1()
{
  uint32_t bar1 = letoh(storage1.Bar1) ;
  bar1 = bar1 & ~(barsize[1] - 1 ) ; 
  return (uint64_t)bar1 ; 
}

Addr config_class::getIOBase()
{
  uint8_t stored_base = letoh(storage1.IOBase) ;
  uint16_t stored_base_upper = letoh(storage1.IOBaseUpper) ; 
 // if(id == 2 ) printf("Stored Base : %02x , Stored Base upper : %04x\n" , stored_base , stored_base_upper) ; 
  uint32_t base_val = 0 ;
  base_val = (uint32_t)stored_base_upper ;
  base_val = base_val << 16 ;  
  stored_base = stored_base >> 4 ; 
  base_val += stored_base * IO_BASE_SHIFT ; // value of 32 bit IO Base
  return (Addr)base_val ; 
}  
  
Addr config_class::getIOLimit()
{
  uint8_t stored_limit = letoh(storage1.IOLimit) ;
  uint16_t stored_limit_upper = letoh(storage1.IOLimitUpper) ; 
  uint32_t limit_val = 0 ;
  limit_val = (uint32_t)stored_limit_upper ;
  limit_val = limit_val << 16 ;  
  stored_limit = stored_limit >> 4 ; 
  limit_val += stored_limit * IO_BASE_SHIFT ; // value of 32 bit IO Limit . Still need to make the lower bits all 1s to satisfy alignment. 
  limit_val |= 0x00000FFF ; 
  return (Addr)limit_val ; 
}
Addr config_class::getMemoryBase()
{
  uint16_t stored_base = letoh(storage1.MemoryBase) ; 
  stored_base = stored_base >> 4 ; 
  uint32_t base_val = (uint32_t)stored_base ; 
  base_val = base_val << 20 ; 
  return (Addr)base_val ; 
}

Addr config_class::getMemoryLimit()
{
  uint16_t stored_limit = letoh(storage1.MemoryLimit) ;
  stored_limit = stored_limit >> 4 ;
  uint32_t limit_val = (uint32_t)stored_limit ; 
  limit_val = limit_val <<20 ;
  limit_val |= 0x000FFFFF ;   // make the botton 20 bits of memory limit all 1's 
  return (Addr)limit_val ; 
}

Addr config_class::getPrefetchableMemoryBase()
{
  uint16_t stored_base = letoh(storage1.PrefetchableMemoryBase) ;
  uint32_t stored_base_upper = letoh(storage1.PrefetchableMemoryBaseUpper) ;
  uint64_t base_val = 0 ; 
  base_val = (uint64_t)stored_base_upper ; 
  base_val = base_val << 32 ; 
  stored_base = stored_base >> 4 ; 
  base_val += stored_base * PREFETCH_BASE_SHIFT ; 
  return base_val ; 
}

Addr config_class::getPrefetchableMemoryLimit()
{

 uint16_t stored_limit = letoh(storage1.PrefetchableMemoryLimit) ;
 uint16_t stored_limit_upper = letoh(storage1.PrefetchableMemoryLimitUpper) ; 
 uint64_t limit_val = 0 ;
 limit_val = (uint64_t)stored_limit_upper ;
 limit_val = limit_val<<32 ; 
 stored_limit = stored_limit >> 4 ;
 limit_val += stored_limit * PREFETCH_BASE_SHIFT ;
 limit_val |=((uint64_t)0x000FFFFF) ; 
 return limit_val ; 
}  

  

// PciBridge::PciBridgeMasterPort*
// PciBridge::getMasterPort (Addr address)
// {
//   DPRINTF(PciBridge, "getMasterPort(by Addr) Port1 MEMBASE %llx PREFETCHBASE %llx IOBASE %llx\n", storage_ptr1->getMemoryBase(), storage_ptr1->getPrefetchableMemoryBase(), storage_ptr1->getIOBase());
//   DPRINTF(PciBridge, "getMasterPort(by Addr) Port2 MEMBASE %llx PREFETCHBASE %llx IOBASE %llx\n", storage_ptr2->getMemoryBase(), storage_ptr2->getPrefetchableMemoryBase(), storage_ptr2->getIOBase());
//   DPRINTF(PciBridge, "getMasterPort(by Addr) Port3 MEMBASE %llx PREFETCHBASE %llx IOBASE %llx\n", storage_ptr3->getMemoryBase(), storage_ptr3->getPrefetchableMemoryBase(), storage_ptr3->getIOBase());
//   DPRINTF(PciBridge, "getMasterPort(by Addr) Try to get Port1 (1) Addr %llx, base %llx\n", address, storage_ptr1->getMemoryBase());
//   if((address >= storage_ptr1->getMemoryBase()) && (address <= storage_ptr1->getMemoryLimit()) && (storage_ptr1->getMemoryBase()!=0) &&(storage_ptr1->getMemoryLimit()!=0))
//      return (PciBridge::PciBridgeMasterPort*) &masterPort1 ;
//   DPRINTF(PciBridge, "getMasterPort(by Addr) Try to get Port1 (2) Addr %llx\n", address);
//   if((address >= storage_ptr1->getPrefetchableMemoryBase()) && (address <= storage_ptr1->getPrefetchableMemoryLimit()) && (storage_ptr1->getPrefetchableMemoryBase()!=0) &&(storage_ptr1->getPrefetchableMemoryLimit()!=0))
//      return (PciBridge::PciBridgeMasterPort*) &masterPort1 ;
//   DPRINTF(PciBridge, "getMasterPort(by Addr) Try to get Port1 (3) Addr %llx\n", address);
//   if((address >= storage_ptr1->getIOBase()) && (address <= storage_ptr1->getIOLimit()) && (storage_ptr1->getIOBase()!=0) &&(storage_ptr1->getIOLimit()!=0))
//      return (PciBridge::PciBridgeMasterPort*) &masterPort1;
  
//   DPRINTF(PciBridge, "getMasterPort(by Addr) Try to get Port2 (1) Addr %llx, base %llx\n", address, storage_ptr2->getMemoryBase());
//   if((address >= storage_ptr2->getMemoryBase()) && (address <= storage_ptr2->getMemoryLimit()) && (storage_ptr2->getMemoryBase()!=0) &&(storage_ptr2->getMemoryLimit()!=0))
//      return (PciBridge::PciBridgeMasterPort*) &masterPort2 ;
//   DPRINTF(PciBridge, "getMasterPort(by Addr) Try to get Port2 (2) Addr %llx\n", address);
//   if((address >= storage_ptr2->getPrefetchableMemoryBase()) && (address <= storage_ptr2->getPrefetchableMemoryLimit()) && (storage_ptr2->getPrefetchableMemoryBase()!=0) &&(storage_ptr2->getPrefetchableMemoryLimit()!=0))
//      return (PciBridge::PciBridgeMasterPort*) &masterPort2 ;
//   DPRINTF(PciBridge, "getMasterPort(by Addr) Try to get Port2 (3) Addr %llx\n", address);
//   if((address >= storage_ptr2->getIOBase()) && (address <= storage_ptr2->getIOLimit()) && (storage_ptr2->getIOBase()!=0) &&(storage_ptr2->getIOLimit()!=0))
//      return (PciBridge::PciBridgeMasterPort*) &masterPort2 ; 

//   DPRINTF(PciBridge, "getMasterPort(by Addr) Try to get Port3 (1) Addr %llx, base %llx\n", address, storage_ptr3->getMemoryBase());
//   if((address >= storage_ptr3->getMemoryBase()) && (address <= storage_ptr3->getMemoryLimit()) && (storage_ptr3->getMemoryBase()!=0) &&(storage_ptr3->getMemoryLimit()!=0))
//      return (PciBridge::PciBridgeMasterPort*) &masterPort3 ;
//   DPRINTF(PciBridge, "getMasterPort(by Addr) Try to get Port3 (2) Addr %llx\n", address);
//   if((address >= storage_ptr3->getPrefetchableMemoryBase()) && (address <= storage_ptr3->getPrefetchableMemoryLimit()) && (storage_ptr3->getPrefetchableMemoryBase()!=0) &&(storage_ptr3->getPrefetchableMemoryLimit()!=0))
//      return (PciBridge::PciBridgeMasterPort*) &masterPort3 ;
//   DPRINTF(PciBridge, "getMasterPort(by Addr) Try to get Port3 (3) Addr %llx\n", address);
//   if((address >= storage_ptr3->getIOBase()) && (address <= storage_ptr3->getIOLimit()) && (storage_ptr3->getIOBase()!=0) &&(storage_ptr3->getIOLimit()!=0))
//      return (PciBridge::PciBridgeMasterPort*) &masterPort3 ; 
 
//   //printf("Forwarding packet to master port dma \n") ; 
//   DPRINTF(PciBridge, "getMasterPort(by Addr) Try to get DMAport Addr %#x\n", address);
//   return (PciBridge::PciBridgeMasterPort*) &masterPort_DMA ; // If no downstream ports are willing to handle the request , just give it to the DMA Port 
// }

PciBridge::PciBridgeMasterPort*
PciBridge::getMasterPort (Addr address)
{
  DPRINTF(PciBridge, "getMasterPort(by Addr) Try to get Port1 (1) Addr %llx\n", address);
    if((address >= storage_ptr1->getMemoryBase()) && (address <= storage_ptr1->getMemoryLimit()) && (storage_ptr1->getMemoryBase()!=0) &&(storage_ptr1->getMemoryLimit()!=0))
     return (PciBridge::PciBridgeMasterPort*) &masterPort1 ;
    DPRINTF(PciBridge, "getMasterPort(by Addr) Try to get Port1 (2) Addr %llx\n", address);
    if((address >= storage_ptr1->getPrefetchableMemoryBase()) && (address <= storage_ptr1->getPrefetchableMemoryLimit()) && (storage_ptr1->getPrefetchableMemoryBase()!=0) &&(storage_ptr1->getPrefetchableMemoryLimit()!=0))
      return (PciBridge::PciBridgeMasterPort*) &masterPort1 ;
    DPRINTF(PciBridge, "getMasterPort(by Addr) Try to get Port1 (3) Addr %llx\n", address);
    if((address >= storage_ptr1->getIOBase()) && (address <= storage_ptr1->getIOLimit()) && (storage_ptr1->getIOBase()!=0) &&(storage_ptr1->getIOLimit()!=0))
      return (PciBridge::PciBridgeMasterPort*) &masterPort1;
      DPRINTF(PciBridge, "getMasterPort(by Addr) Try to get Port2 (1) Addr %llx, base %llx\n", address, storage_ptr2->getMemoryBase());
  if((address >= storage_ptr2->getMemoryBase()) && (address <= storage_ptr2->getMemoryLimit()) && (storage_ptr2->getMemoryBase()!=0) &&(storage_ptr2->getMemoryLimit()!=0))
     return (PciBridge::PciBridgeMasterPort*) &masterPort2 ;
  DPRINTF(PciBridge, "getMasterPort(by Addr) Try to get Port2 (2) Addr %llx\n", address);
  if((address >= storage_ptr2->getPrefetchableMemoryBase()) && (address <= storage_ptr2->getPrefetchableMemoryLimit()) && (storage_ptr2->getPrefetchableMemoryBase()!=0) &&(storage_ptr2->getPrefetchableMemoryLimit()!=0))
     return (PciBridge::PciBridgeMasterPort*) &masterPort2 ;
  DPRINTF(PciBridge, "getMasterPort(by Addr) Try to get Port2 (3) Addr %llx\n", address);
  if((address >= storage_ptr2->getIOBase()) && (address <= storage_ptr2->getIOLimit()) && (storage_ptr2->getIOBase()!=0) &&(storage_ptr2->getIOLimit()!=0))
     return (PciBridge::PciBridgeMasterPort*) &masterPort2 ; 

  DPRINTF(PciBridge, "getMasterPort(by Addr) Try to get Port3 (1) Addr %llx, base %llx\n", address, storage_ptr3->getMemoryBase());
  if((address >= storage_ptr3->getMemoryBase()) && (address <= storage_ptr3->getMemoryLimit()) && (storage_ptr3->getMemoryBase()!=0) &&(storage_ptr3->getMemoryLimit()!=0))
     return (PciBridge::PciBridgeMasterPort*) &masterPort3 ;
  DPRINTF(PciBridge, "getMasterPort(by Addr) Try to get Port3 (2) Addr %llx\n", address);
  if((address >= storage_ptr3->getPrefetchableMemoryBase()) && (address <= storage_ptr3->getPrefetchableMemoryLimit()) && (storage_ptr3->getPrefetchableMemoryBase()!=0) &&(storage_ptr3->getPrefetchableMemoryLimit()!=0))
     return (PciBridge::PciBridgeMasterPort*) &masterPort3 ;
  DPRINTF(PciBridge, "getMasterPort(by Addr) Try to get Port3 (3) Addr %llx\n", address);
  if((address >= storage_ptr3->getIOBase()) && (address <= storage_ptr3->getIOLimit()) && (storage_ptr3->getIOBase()!=0) &&(storage_ptr3->getIOLimit()!=0))
     return (PciBridge::PciBridgeMasterPort*) &masterPort3 ; 

  DPRINTF(PciBridge, "getMasterPort(by Addr) Port1\n");
  AddrRangeList range_port1 = masterPort1.getAddrRanges();
  for (auto it = range_port1.begin() ; it != range_port1.end() ; it++){
      bool is_contain = (*it).contains(address);
      DPRINTF(PciBridge, "getMasterPort(by Addr) Port1 %s\n", (*it).to_string());
      if(is_contain) 
      {
        DPRINTF(PciBridge, "getMasterPort(by Addr) Port1 %s. succ\n", (*it).to_string());
        return (PciBridge::PciBridgeMasterPort*) &masterPort1;
      }
  }

  DPRINTF(PciBridge, "getMasterPort(by Addr) Port2\n");
  AddrRangeList range_port2 = masterPort2.getAddrRanges();
  for (auto it = range_port2.begin() ; it != range_port2.end() ; it++){
      bool is_contain = (*it).contains(address);
      DPRINTF(PciBridge, "getMasterPort(by Addr) Port2 %s\n", (*it).to_string());
      if(is_contain) 
      {
        DPRINTF(PciBridge, "getMasterPort(by Addr) Port2 %s. succ\n", (*it).to_string());
        return (PciBridge::PciBridgeMasterPort*) &masterPort2;
      }
  }

  DPRINTF(PciBridge, "getMasterPort(by Addr) Port3\n");
  AddrRangeList range_port3 = masterPort3.getAddrRanges();
  for (auto it = range_port3.begin() ; it != range_port3.end() ; it++){
      bool is_contain = (*it).contains(address);
      DPRINTF(PciBridge, "getMasterPort(by Addr) Port3 %s\n", (*it).to_string());
      if(is_contain) 
      {
        DPRINTF(PciBridge, "getMasterPort(by Addr) Port3 %s. succ\n", (*it).to_string());
        return (PciBridge::PciBridgeMasterPort*) &masterPort3;
      }
  }

    

  DPRINTF(PciBridge, "getMasterPort(by Addr) Port DMA %#x\n", address);
  return (PciBridge::PciBridgeMasterPort*) &masterPort_DMA;
}
 
PciBridge::PciBridgeSlavePort*
PciBridge::getSlavePort(int bus_num)
{
  if ( (bus_num >= storage_ptr1->storage1.SecondaryBusNumber) && (bus_num <=storage_ptr1->storage1.SubordinateBusNumber) && (storage_ptr1->is_valid == 1))
    return (PciBridgeSlavePort*)&slavePort_DMA1 ; 
  else if ( (bus_num >= storage_ptr2->storage1.SecondaryBusNumber) && (bus_num <= storage_ptr2->storage1.SubordinateBusNumber) && (storage_ptr2->is_valid ==1))
   {
    //printf("Slaveport 2 has primary bus number %02x , Subordinate Bus numberr %02x, given : %d\n", (uint8_t)storage_ptr2->storage1.PrimaryBusNumber, (uint8_t)storage_ptr2->storage1.SubordinateBusNumber, bus_num) ;   
    return (PciBridgeSlavePort*) &slavePort_DMA2 ;
   } 
  else if ( (bus_num >= storage_ptr3->storage1.SecondaryBusNumber) && (bus_num <= storage_ptr3->storage1.SubordinateBusNumber) && (storage_ptr3->is_valid == 1))
    return (PciBridgeSlavePort*) &slavePort_DMA3 ;

   //printf("Returning upstream slavePort for bus num %d\n" , bus_num) ; 
   return (PciBridgeSlavePort*)&slavePort ; 
}

  
Port&
PciBridge::getMasterPort(const std::string &if_name, PortID idx)
{
    if (if_name == "master1")
        return masterPort1;
    else if(if_name == "master2")
        return masterPort2 ; 
    else if (if_name == "master3")
        return masterPort3 ; 
    else if (if_name == "master_dma")
        return masterPort_DMA ; 
    else
        // pass it along to our super class
        return ClockedObject::getPort(if_name, idx);
}

Port&
PciBridge::getSlavePort(const std::string &if_name, PortID idx)
{
    if (if_name == "slave")
        return slavePort;
    else if (if_name == "slave_dma1")
        return slavePort_DMA1 ;
    else if (if_name == "slave_dma2")
        return slavePort_DMA2 ; 
    else if (if_name == "slave_dma3")
        return slavePort_DMA3 ; 

    else
        // pass it along to our super class
        return ClockedObject::getPort(if_name, idx);
}

Port&
PciBridge::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "master1")
        return masterPort1;
    else if(if_name == "master2")
        return masterPort2 ; 
    else if (if_name == "master3")
        return masterPort3 ; 
    else if (if_name == "master_dma")
        return masterPort_DMA ; 
    else if (if_name == "slave")
        return slavePort;
    else if (if_name == "slave_dma1")
        return slavePort_DMA1 ;
    else if (if_name == "slave_dma2")
        return slavePort_DMA2 ; 
    else if (if_name == "slave_dma3")
        return slavePort_DMA3 ; 

    else
        // pass it along to our super class
        return ClockedObject::getPort(if_name, idx);
}

  

// Accepts an offset and decides if this location can be written to. Use the PCI bridge specification to decide what registers are RO or R/W.

bool config_class::isWritable(int offset)
{
  if (offset == PCI_VENDOR_ID || offset==PCI_DEVICE_ID || offset == PCI_REVISION_ID || offset ==PCI_CLASS_CODE || offset == PCI_SUB_CLASS_CODE || offset == PCI_BASE_CLASS_CODE || offset == PCI_CACHE_LINE_SIZE || offset == PCI_LATENCY_TIMER || offset == PCI_HEADER_TYPE || offset == PCI1_SEC_LAT_TIMER || offset == PCI1_RESERVED || offset == PCI1_INTR_PIN) return false ; 

 return true ;
}

    


Tick config_class::readConfig(PacketPtr pkt) 
{
  int offset = pkt->getAddr() & PCIE_CONFIG_SIZE ; 
  DPRINTF(PciBridge, "RootComplex readConfig! pkt %s\n", pkt->print());
  if (offset < PCIE_HEADER_SIZE)
  {
     // we now know that it is an access to standard PCI-PCI bridge header
     switch(pkt->getSize())
     {
       case sizeof(uint8_t): pkt->setLE<uint8_t>(*((uint8_t*)&storage1.data[offset])) ; /*printf("Bridge offset %d data %02x\n" , offset , *((uint8_t*)&storage1.data[offset])) ;*/ break ; 
       case sizeof(uint16_t): pkt->setLE<uint16_t>(*((uint16_t*)&storage1.data[offset])) ; /*printf("Bridge offset %d data %04x\n" , offset ,*((uint16_t*)&storage1.data[offset])) ;*/ break ; 
       case sizeof(uint32_t): pkt->setLE<uint32_t>(*((uint32_t*)&storage1.data[offset])) ; /*printf("Bridge offset %d data %08x\n" , offset , *((uint32_t*)&storage1.data[offset]));*/ break ; 
       default: panic("invalid access size\n") ;
     
     }
   }
  else if (offset >= PXCAPBaseOffset && offset < PXCAPBaseOffset + PXCAPSIZE)
  {
     switch(pkt->getSize())
     {
        case sizeof(uint8_t): pkt->setLE<uint8_t>(*((uint8_t*)&storage2.data[offset - PXCAPBaseOffset])) ; break ; 
        case sizeof(uint16_t): pkt->setLE<uint16_t>(*((uint16_t*)&storage2.data[offset - PXCAPBaseOffset])) ; break ; 
        case sizeof(uint32_t): pkt->setLE<uint32_t>(*((uint32_t*)&storage2.data[offset - PXCAPBaseOffset])) ; break ; 
        default:  panic("invalid access size\n") ;
     
     }
  }
  else
  {
    switch(pkt->getSize())
    {
       case sizeof(uint8_t): pkt->setLE<uint8_t>(0) ; break ; 
       case sizeof(uint16_t): pkt->setLE<uint16_t>(0); break ; 
       case sizeof(uint32_t):pkt->setLE<uint32_t>(0); break ; 
       default:  panic("invalid access size\n")  ;
    }
   }
  pkt->makeAtomicResponse() ;
  
  return configDelay ;
}  

Tick config_class::writeConfig(PacketPtr pkt)
{
  int offset = pkt->getAddr() & PCIE_CONFIG_SIZE ;

  DPRINTF(PciBridge, "RootComplex WriteConfig! pkt %s\n", pkt->print());

  if(pkt->getSize() == sizeof(uint32_t)){
    DPRINTF(PciBridge, "RootComplex WriteConfig 32B! pkt %s, data %#x\n", pkt->print(), (uint32_t)pkt->getLE<uint32_t>());
  }
  
 // if((offset == PCI1_SUB_BUS_NUM) &&(id==3)) printf("Subordinate bus num written with size %d\n" , (int)pkt->getSize()) ;
 // if((offset == PCI1_SEC_BUS_NUM) &&(id==3)) printf("Secondary bus num written with size %d\n" , (int)pkt->getSize()) ; 
 // if((offset == PCI1_PRI_BUS_NUM) &&(id==3)) printf("Primary bus num written with size %d\n" , (int)pkt->getSize()) ;  

  
 

  int flag = 0 ;  // set flag if the IO Base/Limit or Mem Base/Limit or BARs are modified.  
  if( offset < PCIE_HEADER_SIZE )
  {
      if(!isWritable(offset))  // if we can't write to this location 
      {
         pkt->makeAtomicResponse() ;
         return configDelay ;
      }
      else
      {
          switch(pkt->getSize())
          {
             case sizeof(uint8_t):  if(offset == PCI1_PRI_BUS_NUM)  // Must be writing to one of the 1 B RW registers : Primary Bus Num,Secondary Bus Num,Subordinate Bus Num, IO Limit, IO Base,Int line,BIST  
                                        storage1.PrimaryBusNumber = pkt->getLE<uint8_t>() ; 
                                    else if (offset == PCI1_SEC_BUS_NUM){
                                           storage1.SecondaryBusNumber = pkt->getLE<uint8_t>() ; is_valid = 1 ; }
                                     else if (offset == PCI1_SUB_BUS_NUM)
                                         storage1.SubordinateBusNumber = pkt->getLE<uint8_t>() ;
                                    else if (offset ==PCI1_IO_BASE)                                   // For the IO Base register, only the top 4 bits are writable by software. The hex value represents the most significant
                                                                                                      // Hex Digit in 16 Bit IO address. Aligned in 4 KB granularity since bottom 12 address bits are always 0. 
                                    {
                                        uint8_t temp = letoh(storage1.IOBase) ; 
                                        uint8_t val  = letoh(pkt->getLE<uint8_t>()) ;        
                                        temp = (val & ~IO_BASE_MASK) | (temp & IO_BASE_MASK) ; 
                                       // printf("Val is %02x , Temp is %02x\n" , val , temp) ;  
                                        storage1.IOBase = htole(temp) ;   
                                        flag = 1 ; // Ranges change
                                        valid_io_base = true ; 
                                    }
                   
                                    else if (offset == PCI1_IO_LIMIT)
                                     {
                                        uint8_t temp = letoh(storage1.IOLimit) ;                         // Only top 4 bits of IO Limit register are writable by S.W. This hex value represents the most significanr 4 bits
                                                                                                         // of 16 bit IO address. Bottom 12 bits are all assumed to be 1 . E.G. IO_Limit register [ 7:4] = 0x4 indicates
                                                                                                         // that the IO Limit is 0x4FFF . This makes sense since IO Bases are aligned on  4 KB boundaries.  
                                        uint8_t val  = letoh(pkt->getLE<uint8_t>()) ;
                                        temp = (val & ~IO_LIMIT_MASK) | (temp & IO_LIMIT_MASK) ; 
                                        storage1.IOLimit = htole(temp) ;
                                        flag = 1 ;   // Ranges change
                                        valid_io_limit = true ;
                                     }
                                    else if (offset == PCI1_INTR_LINE)
                                         storage1.InterruptLine = pkt->getLE<uint8_t>() ;
                                    else if (offset == PCI_BIST)
                                         storage1.BIST = pkt->getLE<uint8_t>() ; 
                                    break ; 
              case sizeof(uint16_t):                                // Writing to 2B RW config regs. Command , Status , Secondary Status, Mem base, Mem limit, Prefetch mem base, prefetch mem limit, IO limit 
                                                                    // upper , IO Base Upper , Bridge Control   
                                    if(offset == PCI_COMMAND)
                                       storage1.Command = htole(letoh(pkt->getLE<uint8_t>()) | 0x0400)  ;  // disable intx interrupts at all times 
                                    else if (offset == PCI_STATUS)
                                       storage1.Status = pkt->getLE<uint8_t>() ; 
                                    else if (offset == PCI1_SECONDARY_STATUS)
                                       storage1.SecondaryStatus = pkt->getLE<uint8_t>() ; 
                                    else if (offset == PCI1_MEM_BASE)
                                       {
                                          uint16_t temp = letoh(storage1.MemoryBase) ;                // Upper 12 bits of this register are the upper 12 bits of 32 bit memory base address. The lower 20 bits are assumed to be
                                                                                                      //0 . Thus memory base addresses are aligned at 1 MB boundary. 
                                          uint16_t val = letoh(pkt->getLE<uint16_t>()) ; 
                                          temp = (val & ~MMIO_BASE_MASK) | (temp & MMIO_BASE_MASK) ; 
                                          storage1.MemoryBase = htole(temp) ; 
                                          flag =1 ; 
                                          valid_memory_base = true ;
                                       }
                                    else if (offset == PCI1_MEM_LIMIT)
                                       {
                                          uint16_t temp = letoh ( storage1.MemoryLimit) ;
                                          uint16_t val = letoh(pkt->getLE<uint16_t>()) ; 
                                          temp = (val & ~MMIO_LIMIT_MASK) | (temp & MMIO_LIMIT_MASK) ;  //  Upper 12 bits of this register are writable and are upper 12 bits of 32 bit memory limit. The lower 20 bits are 1. 
                                          storage1.MemoryLimit = htole(temp) ; 
                                          flag = 1 ;
                                          valid_memory_limit = true ; 
                                        }
                  
       
                                    else if (offset == PCI1_PRF_MEM_BASE)
                                       {
                                          uint16_t temp = letoh(storage1.PrefetchableMemoryBase) ;                   // Upper 12 bits of this register are writable and are the upper 12 bits of 32 bit Prefetch Mem Base address.
                                                                                                                     // The lower 20 bits are assumed to be 0 , so the pretchable mem base is aligned at 1 MB. 
                                          uint16_t val = letoh(pkt->getLE<uint16_t>()) ;
                                          temp = (val & ~PREFETCH_MEM_BASE_MASK) | (temp & PREFETCH_MEM_BASE_MASK) ;
                                          storage1.PrefetchableMemoryBase = htole(temp) ;
                                          flag = 1 ;
                                          valid_prefetchable_memory_base = true ; 
                                       }
                                    else if (offset == PCI1_PRF_MEM_LIMIT)
                                       {
                                          uint16_t temp = letoh(storage1.PrefetchableMemoryLimit) ;                       // Upper 12 bits are writable and are the upper 12 bits of 32 bit Prefetch Mem Limit. the Lower 20 bits
                                                                                                                          // are all 1 , so each limit ends with FFFFF. 
                                          uint16_t val = letoh( pkt->getLE<uint16_t>()) ;
                                          temp = (val & ~PREFETCH_MEM_LIMIT_MASK) | (temp & PREFETCH_MEM_BASE_MASK) ;
                                          storage1.PrefetchableMemoryLimit = htole(temp) ;
                                          flag = 1 ;
                                          valid_prefetchable_memory_limit = true ; 
                                       }
                             
                                          
                                    else if (offset == PCI1_IO_BASE_UPPER)                 // Upper 16 bits of 32 bit IO Base. Valid if bit 0 of IO Base register is set.
                                      {
                                //         printf("trying to store %04x in IO Base upper\n" , pkt->getLE<uint16_t>()) ;                                         
                                      // storage1.IOBaseUpper = pkt->getLE<uint16_t>() ; //comment this out
                                       flag = 1 ;
                                       valid_io_base = true ; 
                                      }
                                    else if (offset == PCI1_IO_LIMIT_UPPER)                // Upper 16 bits of 32 bit IO Limit. Valid if bit 0 of IO Limit register is set. 
                                       {
                                        //  storage1.IOLimitUpper = pkt->getLE<uint16_t>() ; //comment this out
                                  //      printf("trying to store %04x in IO limit upper\n" , pkt->getLE<uint16_t>()) ; 
                                        flag = 1 ;
                                        valid_io_limit = true ; 
                                       }
                                    else if (offset == PCI1_BRIDGE_CTRL)
                                       storage1.BridgeControl = pkt->getLE<uint16_t>() ;
                                    else if (offset == PCI1_IO_BASE)
                                    {
                                     //   printf("Assigning io base and limit\n") ; 
                                        uint16_t temp = letoh(pkt->getLE<uint16_t>()) ;
                                        
                                        flag = 1 ; 
                                        valid_io_base = true ; valid_io_limit = true ; 
                                        uint8_t t = (uint8_t)(temp & 0x00FF);
                                        uint8_t stored_val = letoh(storage1.IOBase) ; 
                                        stored_val = (t & ~IO_BASE_MASK) | (stored_val & IO_BASE_MASK) ; 
                                        storage1.IOBase = htole(stored_val) ; 
                                        temp = temp >> 8 ; 
                                        stored_val = letoh(storage1.IOLimit) ; 
                                        t = (uint8_t)temp ; 
                                        stored_val = (t & ~IO_LIMIT_MASK) | (stored_val & IO_LIMIT_MASK) ; 
                                        storage1.IOLimit = htole(stored_val) ; 
                                    }
                                          
                                    break ; 

                      
              case sizeof(uint32_t):                                                             // Can be expansion ROM Base Address , BAR0 , BAR1 or PrefetchableBase/Limit Upper
                                   
                                   if(offset == PCI1_ROM_BASE_ADDR)
                                        //storage1.ExpansionROMBaseAddress = pkt->getLE<uint32_t>() ;
                                         storage1.ExpansionROMBaseAddress = htole((uint32_t)0) ; 
                                   else if (offset == PCI1_PRF_BASE_UPPER)                            // Upper 32 bits of 64 bit Prefetchable Memory base
                                      {
                                        valid_prefetchable_memory_base = true ; 
                                        storage1.PrefetchableMemoryBaseUpper = pkt->getLE<uint32_t>() ; 
                                        flag = 1 ;
                                      }
                                   else if (offset == PCI1_PRF_LIMIT_UPPER)                             // Upper 32 bits of 64 bit Prefetchable Memory Limit
                                      {
                                         valid_prefetchable_memory_limit = true ; 
                                         storage1.PrefetchableMemoryLimitUpper = pkt->getLE<uint32_t>() ; 
                                         flag = 1 ;
                                      }
                                   else if (offset == PCI1_BASE_ADDR0)
                                   {
                                       int val = letoh(pkt->getLE<uint32_t>()) ;
                                       uint32_t Bar0 = letoh(storage1.Bar0) ; // BAR's are stored in Little Endian order ( may be different from host order depending on ISA) 
                                       if(val == 0xFFFFFFFF) //writing into BAR initially to determine memory size requested.
                                       {
                                          if(Bar0 & 1) // LSB is set , indicating IO BAR
                                          {
                                              Bar0 = (~(barsize[0] ) - 1 )  | (Bar0 & IO_MASK)  ;  // LSB always has to remain set , otherwise it becomes a memory BAR. Minimum size is 4 B. 
                                          }
                                          else
                                          { 
                                              Bar0 = ~( barsize[0] - 1 )  | (Bar0 & MEM_MASK); // minimum Bar Size is 128 B for a memory BAr , according to PCI Express spec. Can't set it lower than that. Want to preserve size
                                                                                               //and prefetchable bit values in the memory BAR
                                          } 
                                       }
                                       else 
                                       {
                                          barflags[0] = 1 ;
                                          flag = 1 ; 
                                          // Base address field for MEM BAR is [31:7] .
                                          if(Bar0 & 1) // IO BAr
                                          {
                                              Bar0 = (val & ~(barsize[0] - 1)) | (Bar0  &IO_MASK) ; 
                                          }
                                          else
                                          {    
                                          
                                          Bar0 = (val & ~(barsize[0] - 1)) | (Bar0 & MEM_MASK) ; // Again, the BAR Size requested has to be >= 128 B for a memory BAR , and the base address has to be aligned to the Bar Size.
                                          }
                                       }
                                       storage1.Bar0 = htole(Bar0) ; // convert from host to little endian

                                       DPRINTF(PciBridge, "RootComplex WriteConfig 32B! BASE_ADDR0 pkt %s, data %#x, new Bar0 %#x\n", pkt->print(), (uint32_t)pkt->getLE<uint32_t>(), storage1.Bar0);
                                   }
                                  else if (offset == PCI1_BASE_ADDR1)
                                  {
                                      
                                      int val = letoh(pkt->getLE<uint32_t>()) ;
                                      uint32_t Bar1 = letoh(storage1.Bar1) ;
                                           
                                      if(val == 0xFFFFFFFF) //writing into BAR initially to determine memory size requested. 
                                       {
                                          if(Bar1 & 1) // LSB is set , indicating IO BAR
                                          {
                                              Bar1 = ~(barsize[1] - 1 )  | (Bar1 & IO_MASK)  ;  // LSB always has to remain set , otherwise it becomes a memory BAR. Minimum size is 4 B. 
                                          }
                                          else
                                          { 
                                              Bar1 = ~( barsize[1] - 1 )   | (Bar1 & MEM_MASK) ; // minimum Bar Size is 128 B for a memory BAr , according to PCI Express spec. Can't set it lower than that.
                                          } 
                                       }
                                       else 
                                       {
                                          flag = 1 ;
                                          barflags[1] = 1 ;  
                                          // Base address field is [31:7] .
                                          if(Bar1 & 1) // IO BAr
                                          {
                                              Bar1 = (val & ~(barsize[1] - 1)) | (Bar1 & IO_MASK) ; 
                                          }
                                          else
                                          {    
                                          
                                             Bar1 = (val & ~(barsize[1] - 1)) | (Bar1 & MEM_MASK) ; // Again, the BAR Size requested has to be >= 128 B for a memory BAR , and the base address has to be aligned to the Bar Size.
                                          }
                                       }
                                       storage1.Bar1 = htole(Bar1) ;  
                                       DPRINTF(PciBridge, "RootComplex WriteConfig 32B! BASE_ADDR0 pkt %s, data %#x, new Bar1 %#x\n", pkt->print(), (uint32_t)pkt->getLE<uint32_t>(), storage1.Bar1);
                                 }
                                  
                                  else if (offset == PCI1_MEM_BASE)
                                  {
                                     valid_memory_base = true ; valid_memory_limit = true ;
                                     uint32_t val = pkt->getLE<uint32_t>() ;
                                     uint16_t base_val = (uint16_t)(val & 0x0000FFFF) ;
                                     uint16_t cur_base_val = letoh(storage1.MemoryBase) ; 
                                     uint16_t limit_val = (uint16_t)(val >>16) ; 
                                     uint16_t cur_limit_val = letoh(storage1.MemoryLimit) ;
                                     cur_base_val = (base_val & ~MMIO_BASE_MASK) | (cur_base_val & MMIO_LIMIT_MASK) ; 
                                     cur_limit_val = (limit_val & ~MMIO_LIMIT_MASK) | (cur_limit_val & MMIO_LIMIT_MASK) ;
                                     storage1.MemoryBase = htole(cur_base_val) ; 
                                     storage1.MemoryLimit = htole(cur_limit_val) ;
                                     flag = 1 ;  
                                  } 
                                 else if (offset == PCI1_IO_BASE_UPPER)
                                  {
                                     valid_io_base = true ; valid_io_limit = true ; 
                                     printf("trying to store %08x in IO Base upper, IO Limit upper \n" , pkt->getLE<uint32_t>()) ; 
                               /*      uint32_t val = pkt->getLE<uint32_t>() ;               // Comment out from here
                                     printf("Val for IOBase upper is %08x\n" , val) ; 
                                     storage1.IOBaseUpper = (uint16_t)(val & 0x0000FFFF) ;
                                     storage1.IOLimitUpper = (uint16_t)(val >> 16) ; */    // To here
                                     flag = 1 ;
                                  }
                                else if (offset == PCI1_PRF_MEM_BASE)
                                  {
                                     valid_prefetchable_memory_base = true ; valid_prefetchable_memory_limit = true ; 
                                     uint32_t val = pkt->getLE<uint32_t>() ; 
                                     uint16_t base_val = (uint16_t)(val & 0x0000FFFF) ; 
                                     uint16_t cur_base_val = letoh(storage1.PrefetchableMemoryBase) ; 
                                     uint16_t limit_val = (uint16_t)(val >> 16) ;
                                     uint16_t cur_limit_val = letoh(storage1.PrefetchableMemoryLimit) ; 
                                     cur_base_val = (base_val & ~PREFETCH_MEM_BASE_MASK) | (cur_base_val & PREFETCH_MEM_BASE_MASK) ; 
                                     cur_limit_val = (limit_val & ~PREFETCH_MEM_LIMIT_MASK) | (cur_limit_val & PREFETCH_MEM_BASE_MASK)  ; 
                                     storage1.PrefetchableMemoryBase = htole(cur_base_val) ; 
                                     storage1.PrefetchableMemoryLimit = htole(cur_limit_val) ; 
                                     flag = 1 ;
                                  }
                                else if (offset == PCI_COMMAND)
                                  {
                                    uint32_t val = pkt->getLE<uint32_t>() ;
                                    storage1.Command = (uint16_t)(val & 0x0000FFFF) ;
                                    storage1.Status = (uint16_t)(val >>16) ; 
                                  }
                                else if (offset == PCI1_PRI_BUS_NUM)
                                  {
                                    *(uint32_t*)&storage1.data[PCI1_PRI_BUS_NUM] = pkt->getLE<uint32_t>() ;
                                    is_valid = 1 ; 
                                   //  if(id == 3)printf ("Pri bus num , Sec bus num , sub bus num = %d,%d,%d\n" , (int)storage1.PrimaryBusNumber , (int)storage1.SecondaryBusNumber, (int)storage1.SubordinateBusNumber) ; 
                                  }
                                 
                                 break ; 
                    
              default: panic("invalid access size\n") ; 
           }  // end of switch            
                                                                

      } // end of else
  
  }  // end of if offset < PCIE_HEADER_SIZE
  
  else if (offset >= PXCAPBaseOffset && offset < PXCAPBaseOffset + PXCAPSIZE) // writing to PCIe capability structure. Ignore writes to slot registers, not implemented. 
  {
       offset -= PXCAPBaseOffset ; // get the offset from the base of the PCIe cap register set
       if ((offset < SLOT_REG_BASE) && (offset > SLOT_REG_LIMIT)) // ignoring writes to slot registers
       {
           switch(pkt->getSize())
           {
              case sizeof(uint8_t): *(uint8_t*)&storage2.data[offset] = pkt->getLE<uint8_t>() ; break ; 
              case sizeof(uint16_t): *(uint16_t*)&storage2.data[offset] = pkt->getLE<uint16_t>() ; break ; 
              case sizeof(uint32_t): *(uint32_t*)&storage2.data[offset] = pkt->getLE<uint32_t>() ; break ; 
              default: panic("invalid access size\n") ;
           }
       }
  }
  else panic("out of range access to pcie config space\n") ;
  pkt->makeAtomicResponse() ;
  if(flag) { 
     if(id == 0 || is_switch == 0 )
     bridge->slavePort.public_sendRangeChange() ; // send a range change when any of the bridge limit/BAR registers are changed
     if(id == 1)
     bridge->slavePort_DMA1.public_sendRangeChange() ;
     if(id == 2)
     bridge->slavePort_DMA2.public_sendRangeChange() ;
     if(id ==3 )
     bridge->slavePort_DMA3.public_sendRangeChange() ; 

  }
  return configDelay ; 
}   
     



void
PciBridge::init()
{
    // make sure both sides are connected and have the same block size
    if (!slavePort.isConnected() || !slavePort_DMA1.isConnected() || !slavePort_DMA2.isConnected() || !slavePort_DMA3.isConnected() || !masterPort_DMA.isConnected() || !masterPort1.isConnected() || !masterPort2.isConnected() || !masterPort3.isConnected())
        fatal("Both ports of a bridge must be connected.\n");

    // notify the master side  of our address ranges
    slavePort.sendRangeChange();
}
 
bool
PciBridge::PciBridgeSlavePort::respQueueFull() const
{
    return outstandingResponses == respQueueLimit;
} 

bool
PciBridge::PciBridgeMasterPort::reqQueueFull() const
{
    return transmitList.size() == reqQueueLimit;
}

bool
PciBridge::PciBridgeMasterPort::recvTimingResp(PacketPtr pkt)
{
    // all checks are done when the request is accepted on the slave
    // side, so we are guaranteed to have space for the response
    DPRINTF(PciBridge, "call PciBridge::PciBridgeMasterPort::recvTimingResp\n");

    if(pkt->getAddr() == 0xfac00000)
    {
        DPRINTF(PciBridgeX, "%s: src %s packet %s\n", __func__,
            name(), pkt->print());
    }

    DPRINTF(PciBridge, "recvTimingResp: %s addr 0x%x\n",
            pkt->cmdString(), pkt->getAddr());

    DPRINTF(PciBridge, "Request queue size: %d\n", transmitList.size());
   // if((pkt->req_bus[rcid] == 0) && (bridge.is_switch == 1)) printf("Master Port received response to switch port\n"); 
    PciBridgeSlavePort * slavePort = bridge.getSlavePort(pkt->req_bus[rcid]) ;

    // if(pkt->req->pciBridgeSlaveID == 0) slavePort = &(bridge.slavePort);
    // if(pkt->req->pciBridgeSlaveID == 1) slavePort = &(bridge.slavePort_DMA1);
    // if(pkt->req->pciBridgeSlaveID == 2) slavePort = &(bridge.slavePort_DMA2);
    // if(pkt->req->pciBridgeSlaveID == 3) slavePort = &(bridge.slavePort_DMA3);

    auto slaveIter = bridge.routeTo.find(pkt->req);

    if ((pkt->req->cxl_packet_type != CXLPacketType::HostBiasCheck) && (pkt->req->cxl_packet_type != CXLPacketType::CXLcacheH2DResp)) {
        int slaveID = slaveIter->second;
        if(slaveID == 0)
        {
            slavePort = &bridge.slavePort;
        }
        else if(slaveID == 1)
        {
            slavePort = &bridge.slavePort_DMA1;
        }
        else if(slaveID == 2)
        {
            slavePort = &bridge.slavePort_DMA2;
        }
        else if(slaveID == 3)
        {
            slavePort = &bridge.slavePort_DMA3;
        }

    // if(bridge.is_switch == 1) printf("Found slave %d\n" , slavePort->slaveID) ; 
        if((slavePort->slaveID == 1) && (bridge.is_switch==0) && (pkt->isResponse()) && (pkt->hasData()) && (bridge.is_transmit == 1))totalCount += pkt->getSize() ;  
        // 기능 물어보기???
    }


    // CXL_Debug
    if (pkt->req->cxl_packet_type == CXLPacketType::HostBiasCheck) {
        DPRINTF(PciBridge, "(CXL_Debug) PciBridge::PciBridgeSlavePort::recvTimingReq: Coherence check Snoop HIT work!\n");
        // const auto route_lookup = bridge.routeTo.find(pkt->req);
        // bridge.routeTo.erase(route_lookup);
        slavePort = &bridge.slavePort_DMA2;
        bridge.checkCoherenceTiming(pkt);
    }
    if (pkt->req->cxl_packet_type == CXLPacketType::CXLcacheH2DResp) {
        DPRINTF(PciBridge, "(CXL_Debug) PciBridge::PciBridgeSlavePort::recvTimingReq: Snoop Resp HIT work! \n%s\n", pkt->req->cxl_cache->print());
        // const auto route_lookup = bridge.routeTo.find(pkt->req);
        // bridge.routeTo.erase(route_lookup);
        slavePort = &bridge.slavePort;
    }
    else if (pkt->req->cxl_packet_type != CXLPacketType::DefaultPacket) {
        DPRINTF(PciBridge, "(CXL_Debug) PciBridge::PciBridgeMasterPort::recvTimingResp recieve CXL Resp\n");
        bridge.reciveCXL(pkt);
    }

    
  
   

    // technically the packet only reaches us after the header delay,
    // and typically we also need to deserialise any payload (unless
    // the two sides of the bridge are synchronous)
    Tick receive_delay = pkt->headerDelay + pkt->payloadDelay;
    pkt->headerDelay = pkt->payloadDelay = 0;
    //if(bridge.is_switch==0)printf("sched resp : cur:%lu, when :%lu, r_d %lu\n",curTick(), bridge.clockEdge(delay) + receive_delay, receive_delay) ;  

    slavePort->schedTimingResp(pkt, bridge.clockEdge(delay) +
                              receive_delay);
    // if((slavePort->slaveID == 0) && (bridge.is_switch ==1)) printf("Scheduling response to upstream slave port\n") ; 
    if ((pkt->req->cxl_packet_type != CXLPacketType::HostBiasCheck) && (pkt->req->cxl_packet_type != CXLPacketType::CXLcacheH2DResp)) {
        if (pkt->getAddr() >= 0x80000000)
                    DPRINTF(PciBridge, "PciBridge::PciBridgeMasterPort::recvTimingResp Erase routeTO pkt=%s is_cxl=%d\n", pkt->print(), pkt->req->cxl_packet_type);
        bridge.routeTo.erase(slaveIter);
    }
    
    return true;
}

bool
PciBridge::PciBridgeSlavePort::recvTimingReq(PacketPtr pkt)
{
   Addr pkt_Addr = pkt->getAddr() ;

  DPRINTF(PciBridge, "PciBridge::PciBridgeSlavePort::recvTimingReq. pkt %s\n", pkt->print());

  pkt->req->pciBridgeSlaveIDs[bridge.bridge_id] = slaveID;

  if(pkt->getAddr() == 0xfac00000)
    {
        DPRINTF(PciBridgeX, "%s: src %s packet %s\n", __func__,
            name(), pkt->print());
    }

   //if((slaveID==0) &&(bridge.is_switch==0))printf("Slave received timing req to Addr: %08x\n" , (unsigned int)pkt_Addr) ; 
   PciBridgeMasterPort * masterPort  = bridge.getMasterPort(pkt_Addr) ; 
   if(pkt->req_bus[rcid] == -1) // slave port assigns the requester id
   {
     if(slaveID ==0)
     {
     
        pkt->req_bus[rcid] = bridge.storage_ptr4->pci_bus ; 
        pkt->req_dev[rcid] = 0 ;
        pkt->req_func[rcid] = 0 ;
       
     }
    else if (slaveID == 1)
     {
       // printf("Root Port 1 received DMA access\n") ; 
        pkt->req_bus[rcid] = bridge.storage_ptr1->storage1.SecondaryBusNumber ; 
        pkt->req_dev[rcid] = 0 ;
        pkt->req_func[rcid] = 0 ;
      //  printf("Root Port 1 received DMA access , appending %d to address %16lx \n" , (int)pkt->req_bus[rcid] , pkt_Addr) ;  
     }
    else if (slaveID == 2)
    {
         
        pkt->req_bus[rcid] = bridge.storage_ptr2->storage1.SecondaryBusNumber ; 
        pkt->req_dev[rcid] = 0 ;
        pkt->req_func[rcid] = 0 ; 
     //   printf("Root Port 2 received DMA access , appending %d\n" , (int)pkt->req_bus[rcid]) ; 
    }
  
    else if (slaveID == 3)
    {
       // printf("Root port 3 received DMA\n") ; 
        
        pkt->req_bus[rcid] = bridge.storage_ptr3->storage1.SecondaryBusNumber ; 
        pkt->req_dev[rcid] = 0 ;
        pkt->req_func[rcid] = 0 ;
      //  printf("Root Port 3 received DMA access , appending %d\n" , (int)pkt->req_bus[rcid]) ; 
    }
   }
    
    DPRINTF(PciBridge, "recvTimingReq: %s addr 0x%x\n",
            pkt->cmdString(), pkt->getAddr());
    DPRINTF(PciBridge, "pkt: Addr=%llx, is_cxl=%d, SlaveID=%d\n",
            pkt->getAddr(), pkt->req->cxl_packet_type, slaveID);

    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");

    // we should not get a new request after committing to retry the
    // current one, but unfortunately the CPU violates this rule, so
    // simply ignore it for now
    if (retryReq)
        return false;

    DPRINTF(PciBridge, "Response queue size: %d pkt->resp: %d\n",
            transmitList.size(), outstandingResponses);


    // CXL_Debug
    bool Need_CXLResp = false;
    MemCmd snoop_response_cmd;


    if ((pkt->getAddr() >= 0x15000000L) && (pkt->getAddr() < 0x15040000L)) {
        if (slaveID ==0) {
            // CXL 해당 주소 내역으로 수정 필요
            DPRINTF(PciBridge, "(CXL_Debug) PciBridge::PciBridgeSlavePort::recvTimingReq: Access to CXL Dev from Host CPU\n");
            bool checkCXLPkt = bridge.makeCXLPacket(pkt);
            if (!checkCXLPkt) {
                panic("failed make CXL Packet\n");
            }
        }
    }
    // else if ((pkt->getAddr() >= 0x16000000L) && (pkt->getAddr() < 0x17000000L)) {
    else if ((pkt->getAddr() >= 0x80000000L) && (pkt->getAddr() < 0xA0000000L)) {
        if ((slaveID == 0) && (pkt->req->cxl_packet_type != CXLPacketType::HostBiasCheck)) {
        // CXL 해당 주소 내역으로 수정 필요
        DPRINTF(PciBridge, "(CXL_Debug) PciBridge::PciBridgeSlavePort::recvTimingReq: Access to CXL Mem from Host CPU\n");
        bool checkCXLPkt = bridge.makeCXLPacket(pkt);
        if (!checkCXLPkt) 
            panic("failed make CXL Packet\n");
        }
    }
    else if (pkt->getAddr() >= 0xa0000000L) {
        if (slaveID == 0) {
            DPRINTF(PciBridge, "(CXL_Debug) PciBridge::PciBridgeSlavePort::recvTimingReq: Access to Host Mem from Host CPU\n");
            if (pkt->req->cxl_packet_type == CXLPacketType::CXLcacheH2DReq) {
                DPRINTF(PciBridge, "(CXL_Debug) Need Snoop to CXL Dev/Cache, pkt=%s\n", pkt->print());
                bridge.makeCXLSnpReq(pkt);
                // PciBridgeMasterPort * ToCXLPort = (PciBridgeMasterPort * ) (bridge.getMasterPort(0x15000000L));
                masterPort = bridge.getMasterPort(0x15000000);
                // Tick snoop_latency = ToCXLPort->sendAtomic(pkt);
                // coherent_latency += snoop_latency;
                // DPRINTF(PciBridge, "(CXL_Debug) Snoop Response Packet \n%s\n", pkt->req->cxl_cache->print());
                // delete pkt->req->cxl_cache;
                // pkt->req->cxl_cache = nullptr;
                // sink_packet = bridge.CXLAgent.sinkPacket(pkt);
                // Need_CXLResp = true;
            }
            else {
                panic("(CXL_Debug) TimingCPU: For host-to-host access where snoop is not required, packets should not reach PCIBrige.\n");
            }
        }
        else if (slaveID != 0) {
        // CXL 해당 주소 내역으로 수정 필요
            if (pkt->req->cxl_packet_type == CXLPacketType::CXLcacheH2DResp) {
                DPRINTF(PciBridge, "(CXL_Debug) PciBridge::PciBridgeSlavePort::recvTimingReq: Snoop return from CXL Proc \n%s\n", pkt->req->cxl_cache->print());

            }
            else if (pkt->req->cxl_packet_type != CXLPacketType::CXLcacheH2DReq) {
                DPRINTF(PciBridge, "(CXL_Debug) PciBridge::PciBridgeSlavePort::recvTimingReq: Access to Host Mem from CXL Proc\n");
                // DPRINTF(PciBridge, "(CXL_Debug) Need Snoop to host Cache, pkt=%s\n", pkt->print());
                // snoop_response_cmd = MemCmd::InvalidCmd;
                // Tick snoop_response_latency = 0;
                // std::pair<MemCmd, Tick> snoop_result;
                
                // snoop_result = bridge.forwardAtomic(pkt, 0);
                // snoop_response_cmd = snoop_result.first;
                // coherent_latency += snoop_result.second;
                // // pkt->clearCacheResponding();
                // // pkt->clearResponderHadWritable();
                // sink_packet = bridge.CXLAgent.sinkPacket(pkt);
                // Need_CXLResp = true;
                // if (snoop_response_cmd != MemCmd::InvalidCmd)
                // pkt->cmd = snoop_response_cmd;
            }
            
        }
    }

    if ((pkt->req->cxl_packet_type == CXLPacketType::HostBiasCheck) && (slaveID != 0)) {
        DPRINTF(PciBridge, "(CXL_Debug) PciBridge::PciBridgeSlavePort::recvTimingReq: Coherence check CXL Mem Data from CXL Proc\n");
        // if ((pkt->req->cxl_cache->getAddr() >= 0x16000000L) && (pkt->req->cxl_cache->getAddr() < 0x17000000L)) {
        if ((pkt->req->cxl_cache->getAddr() >= 0x80000000L) && (pkt->req->cxl_cache->getAddr() < 0xA0000000L)) {
            DPRINTF(PciBridge, "before snoop pkt: %s\n", pkt->print());
            masterPort = bridge.getMasterPort(0xa0000000);
            // bridge.forwardTiming(pkt);

            // if (sinkPacket(pkt)) {

            // }

            // bridge.checkCoherenceTiming(pkt);
            // DPRINTF(PciBridge, "finish snoop pkt: %s\n", pkt->print());
            // panic("break3!!\n");
        }
        else {
            panic("incorrect CXL Req\n");
        }
    }
    else if (pkt->req->cxl_packet_type == CXLPacketType::HostBiasCheck) {
        DPRINTF(PciBridge, "(CXL_Debug) PciBridge::PciBridgeSlavePort::recvTimingReq: Coherence check Snoop no work!\n");
        // const auto route_lookup = bridge.routeTo.find(pkt->req);
        // bridge.routeTo.erase(route_lookup);
        bridge.checkCoherenceTiming(pkt);
    }

    // if the request queue is full then there is no hope
    if (masterPort->reqQueueFull()) {
        DPRINTF(PciBridge, "Request queue full\n");
        retryReq = true;
    } else {
        // look at the response queue if we expect to see a response
        bool expects_response = pkt->needsResponse();
        if (expects_response) {
            if (respQueueFull()) {
                DPRINTF(PciBridge, "Response queue full\n");
                retryReq = true;
            } else {
                // ok to send the request with space for the response
                DPRINTF(PciBridge, "Reserving space for response\n");
                assert(outstandingResponses != respQueueLimit);
                ++outstandingResponses;
               //if((slaveID==2) && bridge.is_switch==0) printf("Outstanding responses inc : %u\n" , outstandingResponses) ; 

                // no need to set retryReq to false as this is already the
                // case
            }
        }

        if (!retryReq) {
            // technically the packet only reaches us after the header
            // delay, and typically we also need to deserialise any
            // payload (unless the two sides of the bridge are
            // synchronous)
            Tick receive_delay = pkt->headerDelay + pkt->payloadDelay;
            pkt->headerDelay = pkt->payloadDelay = 0;

            //if(bridge.is_switch==0)printf("sched req : cur:%lu, when :%lu, r_d %lu\n",curTick(), bridge.clockEdge(delay) + receive_delay, receive_delay) ;  
            masterPort->schedTimingReq(pkt, bridge.clockEdge(delay) +
                                      receive_delay);
            // panic("break!!\n");
        }
    }
    // panic("break2!!\n");

    // remember that we are now stalling a packet and that we have to
    // tell the sending master to retry once space becomes available,
    // we make no distinction whether the stalling is due to the
    // request queue or response queue being full
    return !retryReq;
}

void
PciBridge::PciBridgeSlavePort::retryStalledReq()
{
    if (retryReq) {
        DPRINTF(PciBridge, "Request waiting for retry, now retrying\n");
        retryReq = false;
        sendRetryReq();
    }
}

void
PciBridge::PciBridgeMasterPort::schedTimingReq(PacketPtr pkt, Tick when)
{
    // If we're about to put this packet at the head of the queue, we
    // need to schedule an event to do the transmit.  Otherwise there
    // should already be an event scheduled for sending the head
    // packet.
    if (transmitList.empty()) {
        bridge.schedule(sendEvent, when);
    }

    assert(transmitList.size() != reqQueueLimit);

    transmitList.emplace_back(pkt, when);
}


void
PciBridge::PciBridgeSlavePort::schedTimingResp(PacketPtr pkt, Tick when)
{
    // If we're about to put this packet at the head of the queue, we
    // need to schedule an event to do the transmit.  Otherwise there
    // should already be an event scheduled for sending the head
    // packet.
    if (transmitList.empty()) {
        bridge.schedule(sendEvent, when);
    }

    transmitList.emplace_back(pkt, when);
}

void
PciBridge::PciBridgeMasterPort::trySendTiming()
{
    assert(!transmitList.empty());

    DeferredPacket req = transmitList.front();

    assert(req.tick <= curTick());

    PacketPtr pkt = req.pkt;
   
    DPRINTF(PciBridge, "trySend request addr 0x%x, queue size %d\n",
            pkt->getAddr(), transmitList.size());

    if (sendTimingReq(pkt)) {
        // send successful
        DPRINTF(PciBridge, "routeTo size=%d\n", bridge.routeTo.size());
        assert(bridge.routeTo.find(pkt->req) == bridge.routeTo.end());

        if ((pkt->req->cxl_packet_type != CXLPacketType::HostBiasCheck) && (pkt->req->cxl_packet_type != CXLPacketType::CXLcacheH2DReq) && (pkt->req->cxl_packet_type != CXLPacketType::CXLcacheH2DResp)) {     
            if (pkt->getAddr() >= 0x80000000)
                DPRINTF(PciBridge, "PciBridge::PciBridgeMasterPort::trySendTiming Add routeTO pkt=%s is_cxl=%d\n", pkt->print(), pkt->req->cxl_packet_type);  
            bridge.routeTo[pkt->req] = pkt->req->pciBridgeSlaveIDs[bridge.bridge_id];
        }
        transmitList.pop_front();
        DPRINTF(PciBridge, "trySend request successful\n");

        // If there are more packets to send, schedule event to try again.
        if (!transmitList.empty()) {
            DeferredPacket next_req = transmitList.front();
            DPRINTF(PciBridge, "Scheduling next send\n");
            bridge.schedule(sendEvent, std::max(next_req.tick,
                                                bridge.clockEdge()));
        }

        // if we have stalled a request due to a full request queue,
        // then send a retry at this point, also note that if the
        // request we stalled was waiting for the response queue
        // rather than the request queue we might stall it again
        bridge.slavePort.retryStalledReq();
        bridge.slavePort_DMA1.retryStalledReq() ;
        bridge.slavePort_DMA2.retryStalledReq() ;
        bridge.slavePort_DMA3.retryStalledReq() ;  // call all slave ports retry stalled request just to be safe and avoid a request being infinitely stalled . 
 
    }

    // if the send failed, then we try again once we receive a retry,
    // and therefore there is no need to take any action
}

void PciBridge::PciBridgeMasterPort:: incCount()
{

   if(totalCount !=0) { printf("R.P Count : %lu\n" , totalCount) ; } 
   totalCount = 0 ; 
   bridge.schedule(countEvent , curTick() + 1000000000000) ; 
}

void
PciBridge::updateRanges()
{
    DPRINTF(PciBridge, "bridge updateRanges was called\n");
    slavePort.public_sendRangeChange();
}

void PciBridge::PciBridgeMasterPort::recvRangeChange()
{
    DPRINTF(PciBridge, "bridge recvRangeChange was called\n");
    bridge.updateRanges();
}

void
PciBridge::PciBridgeSlavePort::trySendTiming()
{
    
    assert(!transmitList.empty());

    DeferredPacket resp = transmitList.front();

    assert(resp.tick <= curTick());

    PacketPtr pkt = resp.pkt;
  //  if((slaveID==0) && (bridge.is_switch == 0)) printf("Upstream switch port received response\n") ; 

    DPRINTF(PciBridge, "trySend response addr 0x%x, outstanding %d\n",
            pkt->getAddr(), outstandingResponses);

    if (sendTimingResp(pkt)) {
        // send successful
        transmitList.pop_front();
        DPRINTF(PciBridge, "trySend response successful\n");

        assert(outstandingResponses != 0);
        --outstandingResponses;
       // if(slaveID==2 && bridge.is_switch ==0) printf("Outstanding responses dec %u\n", outstandingResponses) ; 

        // If there are more packets to send, schedule event to try again.
        if (!transmitList.empty()) {
            DeferredPacket next_resp = transmitList.front();
            DPRINTF(PciBridge, "Scheduling next send\n");
            bridge.schedule(sendEvent, std::max(next_resp.tick,
                                                bridge.clockEdge()));
        }

        // if there is space in the request queue and we were stalling
        // a request, it will definitely be possible to accept it now
        // since there is guaranteed space in the response queue
        if (retryReq) {
            DPRINTF(PciBridge, "Request waiting for retry, now retrying\n");   
            retryReq = false;
            sendRetryReq();
        }
    }

    // if the send failed, then we try again once we receive a retry,
    // and therefore there is no need to take any action
}

void
PciBridge::PciBridgeMasterPort::recvReqRetry()
{
    trySendTiming();
}

void
PciBridge::PciBridgeSlavePort::recvRespRetry()
{
    trySendTiming();
}

Tick
PciBridge::PciBridgeSlavePort::recvAtomic(PacketPtr pkt)
{
    DPRINTF(PciBridge, "RootComplex recvAtomic! pkt %s, slaveID %d\n", pkt->print(), slaveID);
    Tick coherent_latency = 0;
    bool sink_packet = false;
    if(pkt->getSize()==sizeof(uint32_t))
    {
      DPRINTF(PciBridge, "RootComplex recvAtomic32! pkt %s, slaveID %d, data %#x\n", pkt->print(), slaveID, (uint32_t)pkt->getLE<uint32_t>());
    }
   //printf("REcv atomic called\n") ; 
    if(pkt->req_bus[rcid] == -1) // slave port assigns the requester id
   {
     if(slaveID ==0)
     {
     
        pkt->req_bus[rcid] = bridge.storage_ptr4->pci_bus ; 
        pkt->req_dev[rcid] = 0 ;
        pkt->req_func[rcid] = 0 ;
       
     }
    else if (slaveID == 1)
     {
      //  printf("Root Port 1 received atomic DMA access\n") ; 
        pkt->req_bus[rcid] = bridge.storage_ptr1->storage1.SecondaryBusNumber ; 
        pkt->req_dev[rcid] = 0 ;
        pkt->req_func[rcid] = 0 ;
      //  printf("Root Port 1 received atomic DMA access , appending %d\n" , (int)pkt->req_bus[rcid]) ;  
     }
    else if (slaveID == 2)
    {
         
        pkt->req_bus[rcid] = bridge.storage_ptr2->storage1.SecondaryBusNumber ; 
        pkt->req_dev[rcid] = 0 ;
        pkt->req_func[rcid] = 0 ; 
     //   printf("Root Port 2 received atomic DMA access , appending %d\n" , (int)pkt->req_bus[rcid]) ; 
    }
  
    else if (slaveID == 3)
    {
      //  printf("Root port 3 received DMA\n") ; 
        
        pkt->req_bus[rcid] = bridge.storage_ptr3->storage1.SecondaryBusNumber ; 
        pkt->req_dev[rcid] = 0 ;
        pkt->req_func[rcid] = 0 ;
      //  printf("Root Port 3 received atomic DMA access , appending %d\n" , (int)pkt->req_bus[rcid]) ; 
    }
   }
 
   PciBridgeMasterPort * masterPort = bridge.getMasterPort(pkt->getAddr()) ; 
    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");


   // CXL_Debug
   bool Need_CXLResp = false;
   MemCmd snoop_response_cmd;

   if ((pkt->getAddr() >= 0x15000000L) && (pkt->getAddr() < 0x15040000L)) {
    if (slaveID ==0) {
      // CXL 해당 주소 내역으로 수정 필요
      DPRINTF(PciBridge, "(CXL_Debug) PciBridge::PciBridgeSlavePort::recvAtomic: Access to CXL Dev from Host CPU\n");
      bool checkCXLPkt = bridge.makeCXLPacket(pkt);
      if (!checkCXLPkt) {
        panic("failed make CXL Packet\n");
      }
    }
   }
   // else if ((pkt->getAddr() >= 0x16000000L) && (pkt->getAddr() < 0x17000000L)) {
   else if ((pkt->getAddr() >= 0x80000000L) && (pkt->getAddr() < 0xA0000000L)) {
    if (slaveID ==0) {
      // CXL 해당 주소 내역으로 수정 필요
      DPRINTF(PciBridge, "(CXL_Debug) PciBridge::PciBridgeSlavePort::recvAtomic: Access to CXL Mem from Host CPU\n");
      bool checkCXLPkt = bridge.makeCXLPacket(pkt);
      if (!checkCXLPkt) 
        panic("failed make CXL Packet\n");
    }
   }
   else if (pkt->getAddr() >= 0xa0000000L) {
    if (slaveID == 0) {
      DPRINTF(PciBridge, "(CXL_Debug) PciBridge::PciBridgeSlavePort::recvAtomic: Access to Host Mem from Host CPU\n");
    //   if (bridge.getFakeSF(pkt)) {
    //     DPRINTF(PciBridge, "(CXL_Debug) Need Snoop to CXL Dev/Cache, pkt=%s\n", pkt->print());
    //     // PacketPtr snoop_pkt = new Packet(pkt, true, true);
    //     // bridge.makeCXLSnpReq(snoop_pkt);
    //     // PciBridgeMasterPort * ToCXLPort = (PciBridgeMasterPort * ) (bridge.getMasterPort(0x15000000L));
    //     // Tick snoop_latency = ToCXLPort->sendAtomic(snoop_pkt);
    //     // coherent_latency += snoop_latency;
    //     // delete snoop_pkt;
    //     // Need Snoop Fiter
    //     bridge.makeCXLSnpReq(pkt);
    //     PciBridgeMasterPort * ToCXLPort = (PciBridgeMasterPort * ) (bridge.getMasterPort(0x15000000L));
    //     Tick snoop_latency = ToCXLPort->sendAtomic(pkt);
    //     coherent_latency += snoop_latency;
    //     DPRINTF(PciBridge, "(CXL_Debug) Snoop Response Packet \n%s\n", pkt->req->cxl_cache->print());
    //     pkt->req->cxl_packet_type = CXLPacketType::DefaultPacket;
    //     delete pkt->req->cxl_cache;
    //     pkt->req->cxl_cache = nullptr;
    //     // return coherent_latency;
    //     sink_packet = bridge.CXLAgent.sinkPacket(pkt);
    //     Need_CXLResp = true;
    //   }
    //   else {
    //     DPRINTF(PciBridge, "(CXL_Debug) No Need Snoop!!, pkt=%s\n", pkt->print());
    //     // return coherent_latency;
    //   }
      if (pkt->req->cxl_packet_type == CXLPacketType::CXLcacheH2DReq) {
        DPRINTF(PciBridge, "(CXL_Debug) Need Snoop to CXL Dev/Cache, pkt=%s\n", pkt->print());
        bridge.makeCXLSnpReq(pkt);
        PciBridgeMasterPort * ToCXLPort = (PciBridgeMasterPort * ) (bridge.getMasterPort(0x15000000L));
        Tick snoop_latency = ToCXLPort->sendAtomic(pkt);
        coherent_latency += snoop_latency;
        if (pkt->req->cxl_cache != nullptr)
            DPRINTF(PciBridge, "(CXL_Debug) Snoop Response Packet \n%s\n", pkt->req->cxl_cache->print());
        delete pkt->req->cxl_cache;
        pkt->req->cxl_cache = nullptr;
        sink_packet = bridge.CXLAgent.sinkPacket(pkt);
        Need_CXLResp = true;
      }
      else {
        DPRINTF(PciBridge, "(CXL_Debug) No Need Snoop!!, pkt=%s\n", pkt->print());
      }
    }
    else if (slaveID != 0) {
      // CXL 해당 주소 내역으로 수정 필요
      if (pkt->req->cxl_packet_type != CXLPacketType::CXLcacheH2DReq) {
        DPRINTF(PciBridge, "(CXL_Debug) PciBridge::PciBridgeSlavePort::recvAtomic: Access to Host Mem from CXL Proc\n");
        DPRINTF(PciBridge, "(CXL_Debug) Need Snoop to host Cache, pkt=%s\n", pkt->print());
        snoop_response_cmd = MemCmd::InvalidCmd;
        Tick snoop_response_latency = 0;
        std::pair<MemCmd, Tick> snoop_result;
        
        snoop_result = bridge.forwardAtomic(pkt, 0);
        snoop_response_cmd = snoop_result.first;
        coherent_latency += snoop_result.second;
        // pkt->clearCacheResponding();
        // pkt->clearResponderHadWritable();
        sink_packet = bridge.CXLAgent.sinkPacket(pkt);
        Need_CXLResp = true;
        if (snoop_response_cmd != MemCmd::InvalidCmd)
          pkt->cmd = snoop_response_cmd;
      }
    }
   }

   if (pkt->req->cxl_packet_type == CXLPacketType::HostBiasCheck) {
    DPRINTF(PciBridge, "(CXL_Debug) PciBridge::PciBridgeSlavePort::recvAtomic: Coherence check CXL Mem Data from CXL Proc\n");
    // if ((pkt->req->cxl_cache->getAddr() >= 0x16000000L) && (pkt->req->cxl_cache->getAddr() < 0x17000000L)) {
    if ((pkt->req->cxl_cache->getAddr() >= 0x80000000L) && (pkt->req->cxl_cache->getAddr() < 0xA0000000L)) {
      DPRINTF(PciBridge, "(CXL_Debug) find from=CXLDevice Mem\n");
      coherent_latency = bridge.checkCoherence(pkt);
      return 0*delay * bridge.clockPeriod() + coherent_latency;
    }
   }

   Tick access_latency = 0;
   if ((Need_CXLResp) && (sink_packet)) {
      DPRINTF(PciBridge, "%s: Not forwarding %s\n", __func__,
                pkt->print());
      
      // DPRINTF(CXLProtocol, "(CXL_Debug) Snoop Hit!!\n");
   }

   else {
    access_latency = masterPort->sendAtomic(pkt);
    // DPRINTF(CXLProtocol, "(CXL_Debug) Snoop Miss!!\n");
    // ((cxl_packet::m2s::M2S_Req*)(pkt->req->cxl_req))->meta_value = cxl_packet::MetaValue::Invalied;
   }

   if (Need_CXLResp) {
    bridge.reciveCXL(pkt);
   }

   return 0*delay * bridge.clockPeriod() + coherent_latency + access_latency;

   // return 0*delay * bridge.clockPeriod() + masterPort->sendAtomic(pkt);
      
}

void
PciBridge::PciBridgeSlavePort::recvFunctional(PacketPtr pkt)
{
   // printf("REceive functional called\n") ; 
   DPRINTF(PciBridge, "PciBridge::PciBridgeSlavePort::recvFunctional. pkt %s\n", pkt->print());
    PciBridgeMasterPort * masterPort = bridge.getMasterPort(pkt->getAddr()) ;
    pkt->pushLabel(name());

    // check the response queue
    for (auto i = transmitList.begin();  i != transmitList.end(); ++i) {
        if (pkt->trySatisfyFunctional((*i).pkt)) {
            pkt->makeResponse();
            return;
        }
    }

    // also check the master port's request queue
    if (masterPort->trySatisfyFunctional(pkt)) {
        return;
    }

    pkt->popLabel();

    // fall through if pkt still not satisfied
    masterPort->sendFunctional(pkt);
}

bool
PciBridge::PciBridgeMasterPort::trySatisfyFunctional(PacketPtr pkt)
{
    bool found = false;
    auto i = transmitList.begin();

    while (i != transmitList.end() && !found) {
        if (pkt->trySatisfyFunctional((*i).pkt)) {
            pkt->makeResponse();
            found = true;
        }
        ++i;
    }

    return found;
}


void 
PciBridge::PciBridgeSlavePort::fill_ranges(AddrRangeList & ranges , config_class * storage_ptr) const 
{
   uint32_t barsize[2] ; 
   barsize[0] = storage_ptr->barsize[0] ;
   barsize[1] = storage_ptr->barsize[1] ;
   Addr Bar0 =  storage_ptr->getBar0() ;
   Addr Bar1 =  storage_ptr->getBar1() ;
   Addr MemoryBase  = storage_ptr->getMemoryBase() ;
   Addr MemoryLimit = storage_ptr->getMemoryLimit() ;
   Addr PrefetchableMemoryBase =  storage_ptr->getPrefetchableMemoryBase() ;
   Addr PrefetchableMemoryLimit = storage_ptr->getPrefetchableMemoryLimit() ;
   Addr IOLimit = storage_ptr->getIOLimit() ; 
   Addr IOBase =  storage_ptr->getIOBase() ;
   //uint8_t flag1, flag2, flag3 ;
   //flag1 = 0 ; 
   //flag2 = 0 ; 
   //flag3 = 0 ; 

   DPRINTF(PciBridge, "PciBridge::PciBridgeSlavePort::fill_ranges. membase %#x, IOBase %#x, Bar0 %#x, Bar1 %#x\n", MemoryBase, IOBase, Bar0, Bar1);

   if(barsize[0]!=0 && Bar0!=0)
   {
      printf("Bar0 assigned\n") ; 
      DPRINTF(PciBridge, "PciBridge::PciBridgeSlavePort::fill_ranges. BAR0\n");
      ranges.push_back(RangeSize(Bar0 , barsize[0])) ;
   }
 
   if(barsize[1]!=0 && Bar1!=0)
   { 
      ranges.push_back(RangeSize(Bar1, barsize[1])) ;
      DPRINTF(PciBridge, "PciBridge::PciBridgeSlavePort::fill_ranges. BAR1\n");
      printf("Bar 1 assigned \n") ;
   }
   if(storage_ptr->valid_memory_base && storage_ptr->valid_memory_limit && (MemoryBase < MemoryLimit) && (MemoryBase != 0))
   {
     //flag1 = 1 ;
   //  printf("Memory base is %08x , memory limit is %08x\n" , (unsigned int)MemoryBase, (unsigned int)MemoryLimit) ;  
      ranges.push_back(RangeIn(MemoryBase, MemoryLimit)) ;
      DPRINTF(PciBridge, "PciBridge::PciBridgeSlavePort::fill_ranges. case1 %llx to %llx\n", MemoryBase, MemoryLimit);
   }
   if(storage_ptr->valid_prefetchable_memory_base && storage_ptr->valid_prefetchable_memory_limit && (PrefetchableMemoryBase < PrefetchableMemoryLimit) && (PrefetchableMemoryBase != 0))
   {
     //flag2 = 1 ; 
   //  printf("Prefetch Memory Base is %08x , Prefetch MEmory Limit is %08x\n" , (unsigned int) PrefetchableMemoryBase , (unsigned int) PrefetchableMemoryLimit) ;  
      ranges.push_back(RangeIn(PrefetchableMemoryBase, PrefetchableMemoryLimit)) ;
      DPRINTF(PciBridge, "PciBridge::PciBridgeSlavePort::fill_ranges. case2 %llx to %llx\n", MemoryBase, MemoryLimit);
   } 
   if(storage_ptr->valid_io_base && storage_ptr->valid_io_limit && (IOBase < IOLimit) && (IOBase != 0)) 
   {
     //flag3 = 1 ;
   //  printf("pushing back addr io ranges %08x - %08x\n" , (unsigned int)IOBase ,(unsigned int) IOLimit) ;     
     ranges.push_back(RangeIn(IOBase , IOLimit)) ;
     DPRINTF(PciBridge, "PciBridge::PciBridgeSlavePort::fill_ranges. case3 %llx to %llx\n", MemoryBase, MemoryLimit);
  
    // printf("IO Base is %08x , IO limit is %08x\n" , (unsigned int) IOBase , (unsigned int)IOLimit) ;
   }
}



AddrRangeList
PciBridge::PciBridgeSlavePort::getAddrRanges() const
{
    
   AddrRangeList  ranges ;
   DPRINTF(PciBridge, "PciBridge::PciBridgeSlavePort::getAddrRanges. is upstream %d. is switch %d. slaveID %d\n", is_upstream, bridge.is_switch, slaveID);
   if((slaveID == 0) && (bridge.is_switch ==0))
   {
      DPRINTF(PciBridge, "fill_ranges is upstream %d. is switch %d. slaveID %d. RP1\n", is_upstream, bridge.is_switch, slaveID);
      fill_ranges(ranges , bridge.storage_ptr1) ;
      DPRINTF(PciBridge, "fill_ranges is upstream %d. is switch %d. slaveID %d. RP2\n", is_upstream, bridge.is_switch, slaveID);
      fill_ranges(ranges , bridge.storage_ptr2) ; 
      DPRINTF(PciBridge, "fill_ranges is upstream %d. is switch %d. slaveID %d. RP3\n", is_upstream, bridge.is_switch, slaveID);
      fill_ranges(ranges , bridge.storage_ptr3) ; 

      DPRINTF(PciBridge, "fill_ranges is upstream %d. is switch %d. slaveID %d\n", is_upstream, bridge.is_switch, slaveID);

      // AddrRangeList ranges_p1 = bridge.masterPort1.getAddrRanges(); //getAddrRanges();
      // AddrRangeList ranges_p2 = bridge.masterPort2.getAddrRanges(); //getAddrRanges();
      // AddrRangeList ranges_p3 = bridge.masterPort3.getAddrRanges(); //getAddrRanges();

      // // ranges_p1.merge(ranges_p2);
      // // ranges_p1.merge(ranges_p3);

      // //DPRINTF(PciBridge, "Start merge1\n");

      // for (auto it = ranges_p1.begin() ; it != ranges_p1.end() ; it++){
      //     bool can_add = true;
      //     for (auto it2 = ranges.begin() ; it2 != ranges.end() ; it2++){
      //         //DPRINTF(PciBridge, "intersects test\n");
      //         if((*it).intersects(((*it2)))){
      //             //DPRINTF(PciBridge, "intersects addr %s, %s\n", (*it).to_string(), (*it2).to_string());
      //             can_add = false;
      //             break;
      //         }
      //     }
      //     if(can_add){
      //       //DPRINTF(PciBridge, "add range %s\n", (*it).to_string());
      //       //ranges.push_back((*it));
      //     }
      // }

      // //DPRINTF(PciBridge, "Start merge2\n");

      // for (auto it = ranges_p2.begin() ; it != ranges_p2.end() ; it++){
      //     bool can_add = true;
      //     for (auto it2 = ranges.begin() ; it2 != ranges.end() ; it2++){
      //         //DPRINTF(PciBridge, "intersects test\n");
      //         if((*it).intersects(((*it2)))){
      //             //DPRINTF(PciBridge, "intersects addr %s, %s\n", (*it).to_string(), (*it2).to_string());
      //             can_add = false;
      //             break;
      //         }
      //     }
      //     if(can_add){
      //       //DPRINTF(PciBridge, "add range %s\n", (*it).to_string());
      //       //ranges.push_back((*it));
      //     }
      // }

      // //DPRINTF(PciBridge, "Start merge3\n");

      // for (auto it = ranges_p3.begin() ; it != ranges_p3.end() ; it++){
      //     bool can_add = true;
      //     for (auto it2 = ranges.begin() ; it2 != ranges.end() ; it2++){
      //         //DPRINTF(PciBridge, "intersects test\n");
      //         if((*it).intersects(((*it2)))){
      //             //DPRINTF(PciBridge, "intersects addr %s, %s\n", (*it).to_string(), (*it2).to_string());
      //             can_add = false;
      //             break;
      //         }
      //     }
      //     if(can_add){
      //       //DPRINTF(PciBridge, "add range %s\n", (*it).to_string());
      //       //ranges.push_back((*it));
      //     }
      // }

      // ranges.merge(ranges_p1);
      // ranges.merge(ranges_p2);
      // ranges.merge(ranges_p3);

      // ranges.sort();
      // ranges.unique();

      
    // printf("Root port ranges\n") ; 
    // for (auto it = ranges.begin() ; it != ranges.end() ; it++)
     //  printf("Range is %16lx to %16lx\n" , (*it).start() , (*it).end()) ;  
   }
   else if ((slaveID == 0) && (bridge.is_switch == 1))
   {
     //printf("Upstream switch port !!!\n") ; 
     fill_ranges(ranges , bridge.storage_ptr4) ;
    // for (auto it = ranges.begin() ;  it!=ranges.end(); it++)
     //  printf("Upstream switch port Range start %16lx , Upstream switch port Range end %16lx\n" , (*it).start() , (*it).end()) ; 
   }
   else if (slaveID == 1)
   {
      DPRINTF(PciBridge, "fill_ranges is upstream %d. is switch %d. slaveID %d. RP1\n", is_upstream, bridge.is_switch, slaveID);
      fill_ranges(ranges , bridge.storage_ptr1) ; 
   }

   else if (slaveID == 2)
   {
      DPRINTF(PciBridge, "fill_ranges is upstream %d. is switch %d. slaveID %d. RP1\n", is_upstream, bridge.is_switch, slaveID);
      fill_ranges(ranges , bridge.storage_ptr2) ; 
   }
  else if (slaveID == 3)
   {
      DPRINTF(PciBridge, "fill_ranges is upstream %d. is switch %d. slaveID %d. RP1\n", is_upstream, bridge.is_switch, slaveID);
      fill_ranges(ranges , bridge.storage_ptr3) ;
   }
   
   ranges.sort() ; // sort this list
     // This is a downstream slave port used to accept dma requests from the device. Need to configure the range as the ~ of downstream ranges

   if(is_upstream)
      return ranges ;
   else
   {
     AddrRangeList downstream_ranges ;
      
     
     Addr last_val = 0 ;
     for (auto it = ranges.begin() ; it !=ranges.end() ; it++)
     {
          if(last_val < (*it).start()) downstream_ranges.push_back(RangeIn(last_val , ((*it).start()) - 1)) ;
          last_val = (*it).end() + 1 ;
     }
    
      if(last_val == 0) return downstream_ranges ; 
      downstream_ranges.push_back(RangeIn(last_val , ADDR_MAX)) ;
      DPRINTF(PciBridge, "downstream_ranges %llx to %llx\n", last_val, ADDR_MAX);
      
     return downstream_ranges ;
   }      
 return ranges ;      
      
}
    
   
void
PciBridge::serialize(CheckpointOut &cp) const
{
  SERIALIZE_ARRAY(storage_ptr4->storage1.data, sizeof(storage_ptr4->storage1.data)/sizeof(storage_ptr4->storage1.data[0])) ; 
  SERIALIZE_ARRAY(storage_ptr4->storage2.data, sizeof(storage_ptr4->storage2.data)/sizeof(storage_ptr4->storage2.data[0])) ;
  SERIALIZE_ARRAY(storage_ptr1->storage1.data, sizeof(storage_ptr1->storage1.data)/sizeof(storage_ptr1->storage1.data[0])) ;
  SERIALIZE_ARRAY(storage_ptr1->storage2.data, sizeof(storage_ptr1->storage2.data)/sizeof(storage_ptr1->storage2.data[0])) ;
  SERIALIZE_ARRAY(storage_ptr2->storage1.data, sizeof(storage_ptr2->storage1.data)/sizeof(storage_ptr2->storage1.data[0])) ;
  SERIALIZE_ARRAY(storage_ptr2->storage2.data, sizeof(storage_ptr2->storage2.data)/sizeof(storage_ptr2->storage2.data[0])) ;
  SERIALIZE_ARRAY(storage_ptr3->storage1.data, sizeof(storage_ptr3->storage1.data)/sizeof(storage_ptr3->storage1.data[0])) ;
  SERIALIZE_ARRAY(storage_ptr3->storage2.data, sizeof(storage_ptr3->storage2.data)/sizeof(storage_ptr3->storage2.data[0])) ;
  SERIALIZE_SCALAR(storage_ptr4->is_valid);
  SERIALIZE_SCALAR(storage_ptr4->valid_io_base) ;
  SERIALIZE_SCALAR(storage_ptr4->valid_io_limit) ; 
  SERIALIZE_SCALAR(storage_ptr4->valid_memory_base) ; 
  SERIALIZE_SCALAR(storage_ptr4->valid_memory_limit) ; 
  SERIALIZE_SCALAR(storage_ptr4->valid_prefetchable_memory_base) ; 
  SERIALIZE_SCALAR(storage_ptr4->valid_prefetchable_memory_limit) ; 
  SERIALIZE_SCALAR(storage_ptr3->is_valid);
  SERIALIZE_SCALAR(storage_ptr3->valid_io_base) ;
  SERIALIZE_SCALAR(storage_ptr3->valid_io_limit) ; 
  SERIALIZE_SCALAR(storage_ptr3->valid_memory_base) ; 
  SERIALIZE_SCALAR(storage_ptr3->valid_memory_limit) ; 
  SERIALIZE_SCALAR(storage_ptr3->valid_prefetchable_memory_base) ; 
  SERIALIZE_SCALAR(storage_ptr3->valid_prefetchable_memory_limit) ; 
  SERIALIZE_SCALAR(storage_ptr2->is_valid);
  SERIALIZE_SCALAR(storage_ptr2->valid_io_base) ;
  SERIALIZE_SCALAR(storage_ptr2->valid_io_limit) ; 
  SERIALIZE_SCALAR(storage_ptr2->valid_memory_base) ; 
  SERIALIZE_SCALAR(storage_ptr2->valid_memory_limit) ; 
  SERIALIZE_SCALAR(storage_ptr2->valid_prefetchable_memory_base) ; 
  SERIALIZE_SCALAR(storage_ptr2->valid_prefetchable_memory_limit) ; 
  SERIALIZE_SCALAR(storage_ptr1->is_valid);
  SERIALIZE_SCALAR(storage_ptr1->valid_io_base) ;
  SERIALIZE_SCALAR(storage_ptr1->valid_io_limit) ; 
  SERIALIZE_SCALAR(storage_ptr1->valid_memory_base) ; 
  SERIALIZE_SCALAR(storage_ptr1->valid_memory_limit) ; 
  SERIALIZE_SCALAR(storage_ptr1->valid_prefetchable_memory_base) ; 
  SERIALIZE_SCALAR(storage_ptr1->valid_prefetchable_memory_limit) ; 
 
 
}

void
PciBridge::unserialize(CheckpointIn &cp)
{
  printf("unserializing R.C\n") ; 

  UNSERIALIZE_ARRAY(storage_ptr4->storage1.data, sizeof(storage_ptr4->storage1.data)/sizeof(storage_ptr4->storage1.data[0])) ; 
  UNSERIALIZE_ARRAY(storage_ptr4->storage2.data, sizeof(storage_ptr4->storage2.data)/sizeof(storage_ptr4->storage2.data[0])) ;
  UNSERIALIZE_ARRAY(storage_ptr1->storage1.data, sizeof(storage_ptr1->storage1.data)/sizeof(storage_ptr1->storage1.data[0])) ;
  UNSERIALIZE_ARRAY(storage_ptr1->storage2.data, sizeof(storage_ptr1->storage2.data)/sizeof(storage_ptr1->storage2.data[0])) ;
  UNSERIALIZE_ARRAY(storage_ptr2->storage1.data, sizeof(storage_ptr2->storage1.data)/sizeof(storage_ptr2->storage1.data[0])) ;
  UNSERIALIZE_ARRAY(storage_ptr2->storage2.data, sizeof(storage_ptr2->storage2.data)/sizeof(storage_ptr2->storage2.data[0])) ;
  UNSERIALIZE_ARRAY(storage_ptr3->storage1.data, sizeof(storage_ptr3->storage1.data)/sizeof(storage_ptr3->storage1.data[0])) ;
  UNSERIALIZE_ARRAY(storage_ptr3->storage2.data, sizeof(storage_ptr3->storage2.data)/sizeof(storage_ptr3->storage2.data[0])) ;

  UNSERIALIZE_SCALAR(storage_ptr4->is_valid);
  UNSERIALIZE_SCALAR(storage_ptr4->valid_io_base) ;
  UNSERIALIZE_SCALAR(storage_ptr4->valid_io_limit) ; 
  UNSERIALIZE_SCALAR(storage_ptr4->valid_memory_base) ; 
  UNSERIALIZE_SCALAR(storage_ptr4->valid_memory_limit) ; 
  UNSERIALIZE_SCALAR(storage_ptr4->valid_prefetchable_memory_base) ; 
  UNSERIALIZE_SCALAR(storage_ptr4->valid_prefetchable_memory_limit) ; 
  UNSERIALIZE_SCALAR(storage_ptr3->is_valid);
  UNSERIALIZE_SCALAR(storage_ptr3->valid_io_base) ;
  UNSERIALIZE_SCALAR(storage_ptr3->valid_io_limit) ; 
  UNSERIALIZE_SCALAR(storage_ptr3->valid_memory_base) ; 
  UNSERIALIZE_SCALAR(storage_ptr3->valid_memory_limit) ; 
  UNSERIALIZE_SCALAR(storage_ptr3->valid_prefetchable_memory_base) ; 
  UNSERIALIZE_SCALAR(storage_ptr3->valid_prefetchable_memory_limit) ; 
  UNSERIALIZE_SCALAR(storage_ptr2->is_valid);
  UNSERIALIZE_SCALAR(storage_ptr2->valid_io_base) ;
  UNSERIALIZE_SCALAR(storage_ptr2->valid_io_limit) ; 
  UNSERIALIZE_SCALAR(storage_ptr2->valid_memory_base) ; 
  UNSERIALIZE_SCALAR(storage_ptr2->valid_memory_limit) ; 
  UNSERIALIZE_SCALAR(storage_ptr2->valid_prefetchable_memory_base) ; 
  UNSERIALIZE_SCALAR(storage_ptr2->valid_prefetchable_memory_limit) ; 
  UNSERIALIZE_SCALAR(storage_ptr1->is_valid);
  UNSERIALIZE_SCALAR(storage_ptr1->valid_io_base) ;
  UNSERIALIZE_SCALAR(storage_ptr1->valid_io_limit) ; 
  UNSERIALIZE_SCALAR(storage_ptr1->valid_memory_base) ; 
  UNSERIALIZE_SCALAR(storage_ptr1->valid_memory_limit) ; 
  UNSERIALIZE_SCALAR(storage_ptr1->valid_prefetchable_memory_base) ; 
  UNSERIALIZE_SCALAR(storage_ptr1->valid_prefetchable_memory_limit) ; 
 
   
   
   slavePort.public_sendRangeChange() ;
   slavePort_DMA1.public_sendRangeChange() ;
   slavePort_DMA2.public_sendRangeChange() ;
   slavePort_DMA3.public_sendRangeChange() ; 


}


// PciBridge *
// PciBridgeParams::create()
// {
//     return new PciBridge(this);
// }




// CXL_Debug
PciBridge::HomeAgent::HomeAgent(PciBridge* parent)
: parent(parent), _name(parent->name()+".HomeAgent")
{}


// CXL_Debug
std::string
PciBridge::HomeAgent::name() const
{
    return _name;
}

// CXL_Debug
bool
PciBridge::HomeAgent::makeCXLPacket(PacketPtr pkt)
{
    DPRINTF(CXLProtocol, "(CXL_Debug) Call PciBridge::HomeAgent::makeCXLPacket\n");
    cxl_packet::m2s::M2S_SnpTypes snp_type;
    cxl_packet::MetaValue meta_value;
    cxl_packet::MetaFiled meta_filed;
     // -------- test case (Invalid) --------
    bool wantOwnNoData = 0; // 
    bool wantDevShared = 0;
    bool wantDevInvalid = 0;
    // -------------------------------------
    // -------- test case (Read) --------
    bool ExclusiveRead = 1; // 장치 메모리의 독점 데이터 요구
    bool SharedRead = 1; // 장치메모리의 공유 데이터 요구
    bool NonCacheAbleReadFlushed = 0; // 호스트에 캐시하지 않는 현재 데이터 요구 (장치의 캐시는 Flushe됨)
    bool NonCacheAbleRead = 0; // 호스트에 캐시하지 않는 현재 데이터 요구 (장치의 데이터는 그대로)
    bool PassCacheReadFlushed = 0; // 호스트의 캐시 상태를 바꾸지 않고 현재 데이터 요구 (장치의 캐시는 Flush됨)
    bool PassCacheRead = 0; // 호스트의 캐시 상태를 바꾸지 않고 현재 데이터 요구 (장치의 데이터는 그대로)
    bool PassCacheReadNoSnoop = 0; // 호스트의 캐시 상태를 바꾸지 않고 현재 데이터 요구 (장치 캐시를 스누핑 하지않음)
                                   // 예: 호스트가 데이터는 없는데 E 또는 S 상태일때 데이터만 요청하는 case
    // bool MemRdData_E = 0; 
    // bool MemRdData_S = 0;
    // ----------------------------------
    // -------- test case (Write) --------
    bool ExclusiveWrite = 1; // 메모리를 업데이트하고 라인의 독점 사본을 보관
    bool SharedWrite = 0; // 메모리를 업데이트하고 라인의 공유 사본을 보관
    bool NoOwner_wantWB = 0; // 메모리에 writeback, 메모리에 다시 쓰기를 하기 전에 장치가 캐시를 무효화
    bool wantUpdataMem = 0; // 호스트는 메모리를 업데이트, Dirty(M-상태) 캐시 제거일떄 발동
    // -----------------------------------
    // Input_case
    snp_type = cxl_packet::m2s::M2S_SnpTypes::No_Op;
    meta_value = cxl_packet::MetaValue::Any;
    meta_filed = cxl_packet::MetaFiled::No_Op;
    if (pkt->cmd == MemCmd::InvalidateReq) {
    }
    else if (pkt->cmd == MemCmd::ReadExReq) {
        cxl_packet::m2s::M2S_Req_OpCodes req_opcode;
        cxl_packet::m2s::M2S_Req* m2s_req;
        
        if (ExclusiveRead) {
            meta_filed = cxl_packet::MetaFiled::Meta0_State;
            meta_value = cxl_packet::MetaValue::Any;
            snp_type = cxl_packet::m2s::M2S_SnpTypes::SnpInv;
        }
        req_opcode = cxl_packet::m2s::M2S_Req_OpCodes::MemRd;
        m2s_req = new cxl_packet::m2s::M2S_Req(
            req_opcode, snp_type,
            meta_filed, meta_value, 0,
            pkt->getAddr(), 0, 0, 0, 0,
            cxl_packet::m2s::M2S_TC::Reserved);
        
        pkt->req->cxl_packet_type = CXLPacketType::CXLmemPacket;
        pkt->req->cxl_req = m2s_req;
        pkt->req->cxl_req->cxl_req_type = cxl_packet::CXL_Req_Types::_M2S_Req;
        return true;
    }
    else if (pkt->cmd == MemCmd::ReadSharedReq) {
        cxl_packet::m2s::M2S_Req_OpCodes req_opcode;
        cxl_packet::m2s::M2S_Req* m2s_req;
        if (SharedRead) {
            meta_filed = cxl_packet::MetaFiled::Meta0_State;
            meta_value = cxl_packet::MetaValue::Shared;
            snp_type = cxl_packet::m2s::M2S_SnpTypes::SnpData;
        }
        req_opcode = cxl_packet::m2s::M2S_Req_OpCodes::MemRd;
        m2s_req = new cxl_packet::m2s::M2S_Req(
            req_opcode, snp_type,
            meta_filed, meta_value, 0,
            pkt->getAddr(), 0, 0, 0, 0,
            cxl_packet::m2s::M2S_TC::Reserved);
        
        pkt->req->cxl_packet_type = CXLPacketType::CXLmemPacket;
        pkt->req->cxl_req = m2s_req;
        pkt->req->cxl_req->cxl_req_type = cxl_packet::CXL_Req_Types::_M2S_Req;
        return true;
    }
    else if (pkt->cmd == MemCmd::ReadReq) {
        cxl_packet::m2s::M2S_Req_OpCodes req_opcode;
        cxl_packet::m2s::M2S_Req* m2s_req;
        if (NonCacheAbleReadFlushed) {
            meta_filed = cxl_packet::MetaFiled::Meta0_State;
            meta_value = cxl_packet::MetaValue::Invalied;
            snp_type = cxl_packet::m2s::M2S_SnpTypes::SnpInv;
        }
        else if (NonCacheAbleRead) {
            meta_filed = cxl_packet::MetaFiled::Meta0_State;
            meta_value = cxl_packet::MetaValue::Invalied;
            snp_type = cxl_packet::m2s::M2S_SnpTypes::SnpCur;
        }
        else if (PassCacheReadFlushed) {
            meta_filed = cxl_packet::MetaFiled::No_Op;
            snp_type = cxl_packet::m2s::M2S_SnpTypes::SnpInv;
        }
        else if (PassCacheRead) {
            meta_filed = cxl_packet::MetaFiled::No_Op;
            snp_type = cxl_packet::m2s::M2S_SnpTypes::SnpCur;
        }
        else if (PassCacheReadNoSnoop) {
            meta_filed = cxl_packet::MetaFiled::No_Op;
            snp_type = cxl_packet::m2s::M2S_SnpTypes::No_Op;
        }
        req_opcode = cxl_packet::m2s::M2S_Req_OpCodes::MemRd;
        m2s_req = new cxl_packet::m2s::M2S_Req(
            req_opcode, snp_type,
            meta_filed, meta_value, 0,
            pkt->getAddr(), 0, 0, 0, 0,
            cxl_packet::m2s::M2S_TC::Reserved);
        
        pkt->req->cxl_packet_type = CXLPacketType::CXLmemPacket;
        pkt->req->cxl_req = m2s_req;
        pkt->req->cxl_req->cxl_req_type = cxl_packet::CXL_Req_Types::_M2S_Req;
        return true;
    }
    else if (pkt->cmd == MemCmd::WriteReq) {
        cxl_packet::m2s::M2S_RwD_OpCodes rwd_opcode;
        cxl_packet::m2s::M2S_RwD* m2s_rwd;
        meta_filed = cxl_packet::MetaFiled::Meta0_State;
        if (ExclusiveWrite) {
            meta_value = cxl_packet::MetaValue::Any;
            snp_type = cxl_packet::m2s::M2S_SnpTypes::No_Op;
        }
        else if (SharedWrite) {
            meta_value = cxl_packet::MetaValue::Shared;
            snp_type = cxl_packet::m2s::M2S_SnpTypes::No_Op;
        }
        else if (NoOwner_wantWB) {
            meta_value = cxl_packet::MetaValue::Invalied;
            snp_type = cxl_packet::m2s::M2S_SnpTypes::SnpInv;
        }
        else if (wantUpdataMem) {
            meta_value = cxl_packet::MetaValue::Invalied;
            snp_type = cxl_packet::m2s::M2S_SnpTypes::No_Op;
        }
        rwd_opcode = cxl_packet::m2s::M2S_RwD_OpCodes::MemWr;
        m2s_rwd = new cxl_packet::m2s::M2S_RwD(
            rwd_opcode, snp_type,
            meta_filed, meta_value, 0,
            pkt->getAddr(), 0, 0, 0, 0, 0, 0,
            cxl_packet::m2s::M2S_TC::Reserved);
        
        pkt->req->cxl_packet_type = CXLPacketType::CXLmemPacket;
        pkt->req->cxl_req = m2s_rwd;
        pkt->req->cxl_req->cxl_req_type = cxl_packet::CXL_Req_Types::_M2S_RwD;
        return true;
    }
    return false;
}

// CXL_Debug
bool
PciBridge::HomeAgent::makeCXLResp(PacketPtr pkt)
{
    cxl_cache_packet::CXL_Cache_Types cxl_cache_type = pkt->req->cxl_cache->cxl_cache_type;
    cxl_cache_packet::d2h::D2H_Req_OpCodes op;
    cxl_cache_packet::h2d::H2D_Data* h2d_data;
    cxl_cache_packet::h2d::H2D_Resp* h2d_resp;
    cxl_cache_packet::h2d::RSP_Data_Encoding rsp_data;
    DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Make CXL.cache Response\n");
    if (pkt->hasSharers()) {
        rsp_data = cxl_cache_packet::h2d::RSP_Data_Encoding::Shared;
    }
    else {
        rsp_data = cxl_cache_packet::h2d::RSP_Data_Encoding::Exclusive;
    }
    switch (cxl_cache_type)
    {
    case cxl_cache_packet::CXL_Cache_Types::_D2H_Req:
        op = ((cxl_cache_packet::d2h::D2H_Req *)(pkt->req->cxl_cache))-> Opcode;
        if(op == cxl_cache_packet::d2h::D2H_Req_OpCodes::RdCurr) {
            DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Recieve RdCurr Req from Device\n");
            // 호스트를 포함한 어느 캐시도 기존 상태 변화없이 최신 데이터를 요청
            // Data 응답,GO 응답하지 않음
            // 장치는 invalid 상태로 라인 수신, 라인을 한번만 사용하고 캐시할 수 없음
            h2d_data = new cxl_cache_packet::h2d::H2D_Data(
                0, false, false, false, 0, 0, 0);
            h2d_data->last_resp = 1;
            pkt->req->cxl_packet_type = CXLPacketType::CXLcacheDataPacket;
            pkt->req->cxl_data_atomic2 = h2d_data;
            if (pkt->req->cxl_data_atomic2 != nullptr)  
                DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Data Packet\n%s\n", pkt->req->cxl_data_atomic2->print());
            pkt->req->cxl_cache = nullptr;
        }
        else if (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::RdOwn) {
            DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Recieve RdOwn Req from Device\n");
            // 쓰기 가능한 상태에서 캐시할 라인에 대한 장치의 읽기 요청
            // Exclusive(GO-E) 또는 Modidied(GO-M) 으로 응답 (Modified인 경우 호스트에 writeback 필요)
            // 에러 조건: invalid(GO-I) 또는 Error(Go-Err) -> 1로 채워진 데이터로 반환 -> 장치에서 오류 처리
            h2d_resp = new cxl_cache_packet::h2d::H2D_Resp(
                cxl_cache_packet::h2d::H2D_Resp_Opcodes::GO,
                cxl_cache_packet::h2d::RSP_Data_Encoding::Exclusive,
                cxl_cache_packet::h2d::RSP_PRE_Encoding::Host_CacheHit,
                0, 0, 0, 0);
            h2d_data = new cxl_cache_packet::h2d::H2D_Data(
                0, false, false, false, 0, 0, 0);
            h2d_resp->last_resp = 1;
            h2d_data->last_resp = 1;
            pkt->req->cxl_packet_type = CXLPacketType::CXLcachePacket;
            pkt->req->cxl_cache = h2d_resp;
            pkt->req->cxl_packet_type = CXLPacketType::CXLcacheDataPacket;
            pkt->req->cxl_data_atomic2 = h2d_data;
            if (pkt->req->cxl_data_atomic2 != nullptr)  
                DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Data Packet\n%s\n", pkt->req->cxl_data_atomic2->print());
            if (pkt->req->cxl_cache != nullptr)
                DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Response Packet\n%s\n", pkt->req->cxl_cache->print());
        }
        else if (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::RdShared) {
            DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Recieve RdShared Req from Device\n");
            // Shared 상태에서 캐시할 라인에 대한 장치의 읽기 요청
            // Shared(GO-S)로 응답
            // 에러 조건: invalid(GO-I) 또는 Error(Go-Err) -> 1로 채워진 데이터로 반환 -> 장치에서 오류 처리
            h2d_resp = new cxl_cache_packet::h2d::H2D_Resp(
                cxl_cache_packet::h2d::H2D_Resp_Opcodes::GO,
                rsp_data,
                cxl_cache_packet::h2d::RSP_PRE_Encoding::Host_CacheHit,
                0, 0, 0, 0);
            h2d_data = new cxl_cache_packet::h2d::H2D_Data(
                0, false, false, false, 0, 0, 0);
            h2d_resp->last_resp = 1;
            h2d_data->last_resp = 1;
            pkt->req->cxl_packet_type = CXLPacketType::CXLcachePacket;
            pkt->req->cxl_cache = h2d_resp;
            pkt->req->cxl_packet_type = CXLPacketType::CXLcacheDataPacket;
            pkt->req->cxl_data_atomic2 = h2d_data;
            if (pkt->req->cxl_data_atomic2 != nullptr)  
                DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Data Packet\n%s\n", pkt->req->cxl_data_atomic2->print());
            if (pkt->req->cxl_cache != nullptr)
                DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Response Packet\n%s\n", pkt->req->cxl_cache->print());
        }
        else if (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::RdAny) {
            DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Recieve RdAny Req from Device\n");
            // 모든 상태에서 캐시할 라인에 대한 장치의 읽기 요청
            // Shared(GO-S), Exclusive(GO-E) 또는 Modidied(GO-M) 으로 응답 (Modified인 경우 호스트에 writeback 필요)
            // 에러 조건: invalid(GO-I) 또는 Error(Go-Err) -> 1로 채워진 데이터로 반환 -> 장치에서 오류 처리
            h2d_resp = new cxl_cache_packet::h2d::H2D_Resp(
                cxl_cache_packet::h2d::H2D_Resp_Opcodes::GO,
                rsp_data,
                cxl_cache_packet::h2d::RSP_PRE_Encoding::Host_CacheHit,
                0, 0, 0, 0);
            h2d_data = new cxl_cache_packet::h2d::H2D_Data(
                0, false, false, false, 0, 0, 0);
            h2d_resp->last_resp = 1;
            h2d_data->last_resp = 1;
            pkt->req->cxl_packet_type = CXLPacketType::CXLcachePacket;
            pkt->req->cxl_cache = h2d_resp;
            pkt->req->cxl_packet_type = CXLPacketType::CXLcacheDataPacket;
            pkt->req->cxl_data_atomic2 = h2d_data;
            if (pkt->req->cxl_data_atomic2 != nullptr)  
                DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Data Packet\n%s\n", pkt->req->cxl_data_atomic2->print());
            if (pkt->req->cxl_cache != nullptr)      
                DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Response Packet\n%s\n", pkt->req->cxl_cache->print());      
        }
        else if (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::RdOwnNoData) {
            DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Recieve RdOwnNoData Req from Device\n");
            // 캐시라인의 exclusive 소유권 얻기 위한 요청
            //  Exclusive(GO-E) 으로 응답
            // 에러 조건: Error(Go-Err) -> 1로 채워진 데이터로 반환 -> 장치에서 오류 처리
            h2d_resp = new cxl_cache_packet::h2d::H2D_Resp(
                cxl_cache_packet::h2d::H2D_Resp_Opcodes::GO,
                cxl_cache_packet::h2d::RSP_Data_Encoding::Exclusive,
                cxl_cache_packet::h2d::RSP_PRE_Encoding::Host_CacheHit,
                0, 0, 0, 0);
            h2d_resp->last_resp = 1;
            pkt->req->cxl_packet_type = CXLPacketType::CXLcachePacket;
            pkt->req->cxl_cache = h2d_resp;
            if (pkt->req->cxl_cache != nullptr)
                DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Response Packet\n%s\n", pkt->req->cxl_cache->print());
        }
        else if (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::ItoMWr) {
            DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Recieve ItoMWr Req from Device\n");
            // exlusive 소유권을 요청하고 호스트에 atomically하게 WriteBack
            // 장치는 전체 라인이 Modified될것이라고 보장. 데이터를 장치로 전송할 필요없음
            // 소유권 부여 이후 GO_WritePull 응답
            // 장치에는 라인에 사본이 없음
            // 오류: GO-Err-WritePull (장치는 데이터를 호스트로 보내고 호스트는 삭제)  -> 장치에서 오류 처리
            h2d_resp = new cxl_cache_packet::h2d::H2D_Resp(
                cxl_cache_packet::h2d::H2D_Resp_Opcodes::GO_WritePull,
                cxl_cache_packet::h2d::RSP_Data_Encoding::Modified,
                cxl_cache_packet::h2d::RSP_PRE_Encoding::Host_CacheHit,
                0, 0, 0, 0);
            h2d_resp->last_resp = 1;
            pkt->req->cxl_packet_type = CXLPacketType::CXLcachePacket;
            pkt->req->cxl_cache = h2d_resp;
            if (pkt->req->cxl_cache != nullptr)
                DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Response Packet\n%s\n", pkt->req->cxl_cache->print());
        }
        else if (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::WrCur) {
            DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Recieve WrCur Req from Device\n");
            // ItoMWr와 유사하게 캐시라인 소유권을 atomically으로 요청한뒤 WriteBack
            // 데이터 기록되는 위치 다름. 명령이 cache hit일때만 기록
            // miss이면 메모리에 직접 기록
            // 소유권 부여 이후 GO_WritePull 응답
            // 장치에는 라인에 사본이 없음
            // 오류: GO-Err-WritePull (장치는 데이터를 호스트로 보내고 호스트는 삭제)  -> 장치에서 오류 처리
            h2d_resp = new cxl_cache_packet::h2d::H2D_Resp(
                cxl_cache_packet::h2d::H2D_Resp_Opcodes::GO_WritePull,
                cxl_cache_packet::h2d::RSP_Data_Encoding::Modified,
                cxl_cache_packet::h2d::RSP_PRE_Encoding::Host_CacheHit,
                0, 0, 0, 0);
            h2d_resp->last_resp = 1;
            pkt->req->cxl_packet_type = CXLPacketType::CXLcachePacket;
            pkt->req->cxl_cache = h2d_resp;
            if (pkt->req->cxl_cache != nullptr)
                DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Response Packet\n%s\n", pkt->req->cxl_cache->print());
        }
        else if (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::CLFlush) {
            DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Recieve CLFlush Req from Device\n");
            // 주소 필드의 지정된 캐시라인 invalidate
            // 메모리에서 완료된 후 GO-I 응답
            h2d_resp = new cxl_cache_packet::h2d::H2D_Resp(
                cxl_cache_packet::h2d::H2D_Resp_Opcodes::GO,
                cxl_cache_packet::h2d::RSP_Data_Encoding::Invalid,
                cxl_cache_packet::h2d::RSP_PRE_Encoding::Host_CacheHit,
                0, 0, 0, 0);
            h2d_resp->last_resp = 1;
            pkt->req->cxl_packet_type = CXLPacketType::CXLcachePacket;
            pkt->req->cxl_cache = h2d_resp;
            if (pkt->req->cxl_cache != nullptr)
                DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Response Packet\n%s\n", pkt->req->cxl_cache->print());
        }
        else if (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::CleanEvict) {
            DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Recieve CleanEvict Req from Device\n");
            // 장치에서 호스트로 64바이트 캐시라인 evict 요청
            // GO-WritePull 또는 GO-WritePullDrop 응답
            // 장치는 해당 라인의 snoop 소유권 포기
            // GO-WritePull: 장치는 정상적으로 데이터를 호스트로 전송
            // GO-WritePullDrop: 장치는 데이터를 단순히 삭제
            // 장치에 더이상 캐시된 사본이 없음을 보장
            h2d_resp = new cxl_cache_packet::h2d::H2D_Resp(
                cxl_cache_packet::h2d::H2D_Resp_Opcodes::GO_WritePull,
                cxl_cache_packet::h2d::RSP_Data_Encoding::Invalid,
                cxl_cache_packet::h2d::RSP_PRE_Encoding::Host_CacheHit,
                0, 0, 0, 0);
            h2d_resp->last_resp = 1;
            pkt->req->cxl_packet_type = CXLPacketType::CXLcachePacket;
            pkt->req->cxl_cache = h2d_resp;
            if (pkt->req->cxl_cache != nullptr)
                DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Response Packet\n%s\n", pkt->req->cxl_cache->print());
        }
        else if (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::DirtyEvict) {
            DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Recieve DirtyEvict Req from Device\n");
            // 장치에서 호스트로 수정(Modified)된 64바이트 캐시라인 evict 요청
            // GO-WritePull 또는 GO-WritePullDrop 응답
            // 장치는 해당 라인의 snoop 소유권 포기
            // GO-WritePull: 장치는 정상적으로 데이터를 호스트로 전송
            // 장치에 더이상 캐시된 사본이 없음을 보장
            h2d_resp = new cxl_cache_packet::h2d::H2D_Resp(
                cxl_cache_packet::h2d::H2D_Resp_Opcodes::GO_WritePull,
                cxl_cache_packet::h2d::RSP_Data_Encoding::Invalid,
                cxl_cache_packet::h2d::RSP_PRE_Encoding::Host_CacheHit,
                0, 0, 0, 0);
            h2d_resp->last_resp = 1;
            pkt->req->cxl_packet_type = CXLPacketType::CXLcachePacket;
            pkt->req->cxl_cache = h2d_resp;
            if (pkt->req->cxl_cache != nullptr)
                DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Response Packet\n%s\n", pkt->req->cxl_cache->print());
        }
        else if (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::CleanEvictNoData) {
            DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Recieve CleanEvictNoData Req from Device\n");
            // 호스트에 clean 라인이 삭제되었음을 업데이트 하라는 장치의 요청
            // 호스트의 모든 SnoopFliter 업데이트 (데이터 교환 X)
            DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Response Packet (CleanEvictResp)\n");
        }
        else if (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::WOWrInv) {
            DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Recieve WOWrInv Req from Device\n");
            // weakly ordered write invalidate 라인 요청
            // FastGO-WritePull 응답이후 ExtCmp 응답
            // FastGO-WritePull 응답이후 장치는 호스트로 데이터 전송
            // 이후, 호스트의 메모리 쓰기 완료시 ExtCmp 응답
            h2d_resp = new cxl_cache_packet::h2d::H2D_Resp(
                cxl_cache_packet::h2d::H2D_Resp_Opcodes::Fast_GO_writePull,
                cxl_cache_packet::h2d::RSP_Data_Encoding::Invalid,
                cxl_cache_packet::h2d::RSP_PRE_Encoding::Host_CacheHit,
                0, 0, 0, 0);
            h2d_resp->last_resp = 1;
            pkt->req->cxl_packet_type = CXLPacketType::CXLcachePacket;
            pkt->req->cxl_cache = h2d_resp;
            if (pkt->req->cxl_cache != nullptr)
                DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Response Packet\n%s\n", pkt->req->cxl_cache->print());
        }
        else if (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::WOWrInvF) {
            DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Recieve WOWrInvF Req from Device\n");
            // 64 바이트 쓰기 (WOWrInv와 동일)
            h2d_resp = new cxl_cache_packet::h2d::H2D_Resp(
                cxl_cache_packet::h2d::H2D_Resp_Opcodes::Fast_GO_writePull,
                cxl_cache_packet::h2d::RSP_Data_Encoding::Invalid,
                cxl_cache_packet::h2d::RSP_PRE_Encoding::Host_CacheHit,
                0, 0, 0, 0);
            h2d_resp->last_resp = 1;
            pkt->req->cxl_packet_type = CXLPacketType::CXLcachePacket;
            pkt->req->cxl_cache = h2d_resp;
            if (pkt->req->cxl_cache != nullptr)
                DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Response Packet\n%s\n", pkt->req->cxl_cache->print());
        }
        else if (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::WrInv) {
            DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Recieve WrInv Req from Device\n");
            // write invalidate 라인 요청
            // WritePull 응답이후 GO 응답
            // WritePull 응답이후 장치는 호스트로 데이터 전송
            // 이후, 호스트의 메모리 쓰기 완료시 GO 응답
            h2d_resp = new cxl_cache_packet::h2d::H2D_Resp(
                cxl_cache_packet::h2d::H2D_Resp_Opcodes::WritePull,
                cxl_cache_packet::h2d::RSP_Data_Encoding::Invalid,
                cxl_cache_packet::h2d::RSP_PRE_Encoding::Host_CacheHit,
                0, 0, 0, 0);
            h2d_resp->last_resp = 1;
            pkt->req->cxl_packet_type = CXLPacketType::CXLcachePacket;
            pkt->req->cxl_cache = h2d_resp;
            if (pkt->req->cxl_cache != nullptr)
                DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Response Packet\n%s\n", pkt->req->cxl_cache->print());
        }
        else if (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::CacheFlushed) {
            DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Recieve CacheFlushed Req from Device\n");
            // 캐시가 플러시 되었으며, 더이상 Shared, Exclusive or Modified 상태의 캐시라인이 없다는 것을 호스트로 알리기 위함
            // 호스트는 SnoopFliter를 지우고 장치에 대한 snoop을 차단하고 GO 응답
            // 이후, 장치가 다음 캐시가능한 D2H_Req를 보내기 전까지 호스트에서 Snoop를 수신하지않음이 보장
            h2d_resp = new cxl_cache_packet::h2d::H2D_Resp(
                cxl_cache_packet::h2d::H2D_Resp_Opcodes::GO,
                cxl_cache_packet::h2d::RSP_Data_Encoding::Invalid,
                cxl_cache_packet::h2d::RSP_PRE_Encoding::Host_CacheHit,
                0, 0, 0, 0);
            h2d_resp->last_resp = 1;
            pkt->req->cxl_packet_type = CXLPacketType::CXLcachePacket;
            pkt->req->cxl_cache = h2d_resp;
            if (pkt->req->cxl_cache != nullptr)
                DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Response Packet\n%s\n", pkt->req->cxl_cache->print());
        }
        return true;
    default:
        panic("incorrect CXL.cache Req in PciBridge::makeCXLResp");
    }
    return false;
}

// CXL_Debug
Tick
PciBridge::HomeAgent::checkCoherence(PacketPtr pkt)
{
    Tick Coherence_latency = 0;
    if (pkt->req->cxl_cache != nullptr)
        DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::checkCoherence Repuest Packet\n%s\n", pkt->req->cxl_cache->print());
    cxl_cache_packet::CXL_Cache_Types cxl_cache_type = pkt->req->cxl_cache->cxl_cache_type;
    cxl_cache_packet::d2h::D2H_Req_OpCodes op;
    cxl_packet::m2s::M2S_Req* m2s_req;
    switch (cxl_cache_type)
    {
    case cxl_cache_packet::CXL_Cache_Types::_D2H_Req:
    {
        DPRINTF(CXLProtocol, "(CXL_Debug) For Coherence check snooping to slavePort(L2Cache) pkt=%s\n", pkt->print());
        // // Packet *snoop_pkt = new Packet(pkt, true, false);
        // // snoop_pkt->makeResponse();
        MemCmd snoop_response_cmd = MemCmd::InvalidCmd;
        Tick snoop_response_latency = 0;
        std::pair<MemCmd, Tick> snoop_result;
        
        snoop_result = parent->forwardAtomic(pkt, 0);
        snoop_response_cmd = snoop_result.first;
        snoop_response_latency += snoop_result.second;
        // pkt->clearCacheResponding();
        // pkt->clearResponderHadWritable();
        if (snoop_response_cmd != MemCmd::InvalidCmd)
            pkt->cmd = snoop_response_cmd;
        Coherence_latency += snoop_response_latency;
        
        op = ((cxl_cache_packet::d2h::D2H_Req *)(pkt->req->cxl_cache))-> Opcode;
        if (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::RdCurr) {
            m2s_req = new cxl_packet::m2s::M2S_Req(
                cxl_packet::m2s::M2S_Req_OpCodes::MemRdFwd,
                cxl_packet::m2s::M2S_SnpTypes::No_Op,
                cxl_packet::MetaFiled::Meta0_State,
                cxl_packet::MetaValue::Any,
                0, pkt->getAddr(), 0, 0, 0, 0, cxl_packet::m2s::M2S_TC::Reserved);
        }
        else if (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::RdShared) {
            m2s_req = new cxl_packet::m2s::M2S_Req(
                cxl_packet::m2s::M2S_Req_OpCodes::MemRdFwd,
                cxl_packet::m2s::M2S_SnpTypes::No_Op,
                cxl_packet::MetaFiled::Meta0_State,
                cxl_packet::MetaValue::Shared,
                0, pkt->getAddr(), 0, 0, 0, 0, cxl_packet::m2s::M2S_TC::Reserved);
        }
        else if (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::RdAny) {
            m2s_req = new cxl_packet::m2s::M2S_Req(
                cxl_packet::m2s::M2S_Req_OpCodes::MemRdFwd,
                cxl_packet::m2s::M2S_SnpTypes::No_Op,
                cxl_packet::MetaFiled::Meta0_State,
                cxl_packet::MetaValue::Shared,
                0, pkt->getAddr(), 0, 0, 0, 0, cxl_packet::m2s::M2S_TC::Reserved);
        }
        else if ((op == cxl_cache_packet::d2h::D2H_Req_OpCodes::RdOwn) || (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::RdOwnNoData)) {
            m2s_req = new cxl_packet::m2s::M2S_Req(
                cxl_packet::m2s::M2S_Req_OpCodes::MemRdFwd,
                cxl_packet::m2s::M2S_SnpTypes::No_Op,
                cxl_packet::MetaFiled::Meta0_State,
                cxl_packet::MetaValue::Invalied,
                0, pkt->getAddr(), 0, 0, 0, 0, cxl_packet::m2s::M2S_TC::Reserved);
        }
        else if ((op == cxl_cache_packet::d2h::D2H_Req_OpCodes::WOWrInv) || (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::WOWrInvF)) {
            m2s_req = new cxl_packet::m2s::M2S_Req(
                cxl_packet::m2s::M2S_Req_OpCodes::MemWrFwd,
                cxl_packet::m2s::M2S_SnpTypes::No_Op,
                cxl_packet::MetaFiled::Meta0_State,
                cxl_packet::MetaValue::Invalied,
                0, pkt->getAddr(), 0, 0, 0, 0, cxl_packet::m2s::M2S_TC::Reserved);
        }
        else {
            panic("incorrect CXL.cache Req in PciBridge::checkCoherence");
        }
        if (pkt->hasSharers()) {
            DPRINTF(CXLProtocol, "(CXL_Debug) Snoop Miss!!\n");
            m2s_req->meta_value = cxl_packet::MetaValue::Shared;
        }
        else {
            DPRINTF(CXLProtocol, "(CXL_Debug) Snoop Hit!!\n");
            m2s_req->meta_value = cxl_packet::MetaValue::Invalied;
        }
        delete pkt->req->cxl_cache;
        pkt->req->cxl_cache = nullptr;
        // pkt->req->cxl_packet_type = CXLPacketType::CXLmemPacket;
        pkt->req->cxl_packet_type = CXLPacketType::HostBiasCheck;
        pkt->req->cxl_req = m2s_req;
        return Coherence_latency;
    }
    default:
        panic("incorrect CXL.cache Req in PciBridge::checkCoherence");
    }
    panic("incorrect CXL.cache Req in PciBridge::checkCoherence");
}

void
PciBridge::HomeAgent::checkCoherenceTiming(PacketPtr pkt)
{   
    if (pkt->req->cxl_cache != nullptr)
        DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::checkCoherenceTiming Repuest Packet\n%s\n", pkt->req->cxl_cache->print());
    cxl_cache_packet::CXL_Cache_Types cxl_cache_type = pkt->req->cxl_cache->cxl_cache_type;
    cxl_cache_packet::d2h::D2H_Req_OpCodes op;
    cxl_packet::m2s::M2S_Req* m2s_req;
    switch (cxl_cache_type)
    {
    case cxl_cache_packet::CXL_Cache_Types::_D2H_Req:
    {
              
        op = ((cxl_cache_packet::d2h::D2H_Req *)(pkt->req->cxl_cache))-> Opcode;
        if (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::RdCurr) {
            m2s_req = new cxl_packet::m2s::M2S_Req(
                cxl_packet::m2s::M2S_Req_OpCodes::MemRdFwd,
                cxl_packet::m2s::M2S_SnpTypes::No_Op,
                cxl_packet::MetaFiled::Meta0_State,
                cxl_packet::MetaValue::Any,
                0, pkt->getAddr(), 0, 0, 0, 0, cxl_packet::m2s::M2S_TC::Reserved);
        }
        else if (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::RdShared) {
            m2s_req = new cxl_packet::m2s::M2S_Req(
                cxl_packet::m2s::M2S_Req_OpCodes::MemRdFwd,
                cxl_packet::m2s::M2S_SnpTypes::No_Op,
                cxl_packet::MetaFiled::Meta0_State,
                cxl_packet::MetaValue::Shared,
                0, pkt->getAddr(), 0, 0, 0, 0, cxl_packet::m2s::M2S_TC::Reserved);
        }
        else if (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::RdAny) {
            m2s_req = new cxl_packet::m2s::M2S_Req(
                cxl_packet::m2s::M2S_Req_OpCodes::MemRdFwd,
                cxl_packet::m2s::M2S_SnpTypes::No_Op,
                cxl_packet::MetaFiled::Meta0_State,
                cxl_packet::MetaValue::Shared,
                0, pkt->getAddr(), 0, 0, 0, 0, cxl_packet::m2s::M2S_TC::Reserved);
        }
        else if ((op == cxl_cache_packet::d2h::D2H_Req_OpCodes::RdOwn) || (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::RdOwnNoData)) {
            m2s_req = new cxl_packet::m2s::M2S_Req(
                cxl_packet::m2s::M2S_Req_OpCodes::MemRdFwd,
                cxl_packet::m2s::M2S_SnpTypes::No_Op,
                cxl_packet::MetaFiled::Meta0_State,
                cxl_packet::MetaValue::Invalied,
                0, pkt->getAddr(), 0, 0, 0, 0, cxl_packet::m2s::M2S_TC::Reserved);
        }
        else if ((op == cxl_cache_packet::d2h::D2H_Req_OpCodes::WOWrInv) || (op == cxl_cache_packet::d2h::D2H_Req_OpCodes::WOWrInvF)) {
            m2s_req = new cxl_packet::m2s::M2S_Req(
                cxl_packet::m2s::M2S_Req_OpCodes::MemWrFwd,
                cxl_packet::m2s::M2S_SnpTypes::No_Op,
                cxl_packet::MetaFiled::Meta0_State,
                cxl_packet::MetaValue::Invalied,
                0, pkt->getAddr(), 0, 0, 0, 0, cxl_packet::m2s::M2S_TC::Reserved);
        }
        else {
            panic("incorrect CXL.cache Req in PciBridge::checkCoherence");
        }
        if (pkt->hasSharers()) {
            DPRINTF(CXLProtocol, "(CXL_Debug) Snoop Miss!!\n");
            m2s_req->meta_value = cxl_packet::MetaValue::Shared;
        }
        else {
            DPRINTF(CXLProtocol, "(CXL_Debug) Snoop Hit!!\n");
            m2s_req->meta_value = cxl_packet::MetaValue::Invalied;
        }
        delete pkt->req->cxl_cache;
        pkt->req->cxl_cache = nullptr;
        // pkt->req->cxl_packet_type = CXLPacketType::CXLmemPacket;
        pkt->req->cxl_packet_type = CXLPacketType::HostBiasCheck;
        pkt->req->cxl_req = m2s_req;
        return;
    }
    default:
        panic("incorrect CXL.cache Req in PciBridge::checkCoherence");
    }
    panic("incorrect CXL.cache Req in PciBridge::checkCoherence");
}

// CXL_Debug
bool
PciBridge::HomeAgent::makeCXLData(PacketPtr pkt)
{
    DPRINTF(CXLProtocol, "(CXL_Debug) Call PciBridge::HomeAgent::makeCXLData\n");
    if (pkt->req->cxl_data_atomic2->last_data == 0) {
        pkt->req->cxl_data_atomic2 = nullptr;
        pkt->req->cxl_cache = nullptr;
        return true;
    }
    cxl_cache_packet::h2d::H2D_Resp* h2d_resp;
    DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLResp Recieve Data from Device\n");
    h2d_resp = new cxl_cache_packet::h2d::H2D_Resp(
        cxl_cache_packet::h2d::H2D_Resp_Opcodes::ExtCmp,
        cxl_cache_packet::h2d::RSP_Data_Encoding::Invalid,
        cxl_cache_packet::h2d::RSP_PRE_Encoding::Host_CacheHit,
        0, 0, 0, 0);
    h2d_resp->last_resp = 1;
    pkt->req->cxl_packet_type = CXLPacketType::CXLcachePacket;
    pkt->req->cxl_cache = h2d_resp;
    if (pkt->req->cxl_cache != nullptr)
        DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::makeCXLData Data Packet\n%s\n", pkt->req->cxl_cache->print());
    pkt->req->cxl_data_atomic2 = nullptr;
    return true;
}

// CXL_Debug
bool
PciBridge::HomeAgent::makeCXLSnpReq(PacketPtr snoop_pkt)
{
    DPRINTF(CXLProtocol, "(CXL_Debug) Call PciBridge::HomeAgent::makeCXLSnpReq\n");
    cxl_cache_packet::h2d::H2D_Req_Opcodes opcode;
    cxl_cache_packet::h2d::H2D_Req* h2d_req;
    MemCmd cmd = snoop_pkt->cmd;
    if (cmd == MemCmd::ReadSharedReq) {
        // snoop_pkt->cmd == MemCmd::CleanSharedReq;
        opcode = cxl_cache_packet::h2d::H2D_Req_Opcodes::SnpData;
    }
    else if (cmd == MemCmd::ReadExReq) {
        // snoop_pkt->cmd == MemCmd::CleanInvalidReq;
        opcode = cxl_cache_packet::h2d::H2D_Req_Opcodes::SnpInv;
    }
    h2d_req = new cxl_cache_packet::h2d::H2D_Req(
        opcode, snoop_pkt->getAddr(), 0, 0, 0, 0, 0);
    snoop_pkt->req->cxl_packet_type = CXLPacketType::CXLcacheH2DReq;
    snoop_pkt->req->cxl_cache = h2d_req;
    // eraseFakeSF(snoop_pkt);
    return true;
}

// CXL_Debug
void
PciBridge::HomeAgent::reciveCXL(PacketPtr pkt)
{
    DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::HomeAgent::reciveCXL call\n");
        if (pkt->req->cxl_cache != nullptr) {
            DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::DimmSidePort::sendAtomic calles makeCXLResp(pkt)\n");
            // if (pkt->req->cxl_cache->getAddr() >= 0x80000000L) {
            if (pkt->req->cxl_cache->getAddr() >= 0xA0000000L) {
                bool checkCXLPkt = makeCXLResp(pkt);
                if (!checkCXLPkt) 
                    panic("failed make CXL.cache Response\n");
            }
            // else if ((pkt->req->cxl_cache->getAddr() >= 0x80000000L) && (pkt->req->cxl_cache->getAddr() < 0xA0000000L)) {
            //     bool checkCXLPkt = rootComplex.checkCoherence(pkt);
            //     if (!checkCXLPkt) 
            //         panic("failed make CXL.cache Coherence check\n");
            // }
        }
        else if (pkt->req->cxl_data_atomic2 != nullptr) {
            DPRINTF(CXLProtocol, "(CXL_Debug) PciBridge::DimmSidePort::sendAtomic calles makeCXLData(pkt)\n");
            bool checkCXLPkt = makeCXLData(pkt);
            if (!checkCXLPkt) 
                panic("failed make CXL.cache Response\n");    
        }   
}


bool
PciBridge::HomeAgent::sinkPacket(const PacketPtr pkt) const
{
    // we can sink the packet if:
    // 1) the crossbar is the point of coherency, and a cache is
    //    responding after being snooped
    // 2) the crossbar is the point of coherency, and the packet is a
    //    coherency packet (not a read or a write) that does not
    //    require a response
    // 3) this is a clean evict or clean writeback, but the packet is
    //    found in a cache above this crossbar
    // 4) a cache is responding after being snooped, and the packet
    //    either does not need the block to be writable, or the cache
    //    that has promised to respond (setting the cache responding
    //    flag) is providing writable and thus had a Modified block,
    //    and no further action is needed
    return (pkt->cacheResponding()) ||
        (!(pkt->isRead() || pkt->isWrite()) &&
         !pkt->needsResponse()) ||
        (pkt->isCleanEviction() && pkt->isBlockCached()) ||
        (pkt->cacheResponding() &&
         (!pkt->needsWritable() || pkt->responderHadWritable()));
}

std::pair<MemCmd, Tick>
PciBridge::forwardAtomic(PacketPtr pkt, PortID exclude_cpu_side_port_id,
                           PortID source_mem_side_port_id)
{

    MemCmd orig_cmd = pkt->cmd;
    MemCmd snoop_response_cmd = MemCmd::InvalidCmd;
    Tick snoop_response_latency = 0;
    
    Addr dest_addr = pkt->getAddr();

    // if (dest_addr >= 0xa0000000)
    //     CXLAgent.setFakeSF(pkt);



    unsigned fanout = 0;


        Tick latency = slavePort.sendAtomicSnoop(pkt);
        fanout++;

        if (pkt->isResponse()) {
            assert(pkt->cmd != orig_cmd);
            assert(pkt->cacheResponding());
            // should only happen once
            assert(snoop_response_cmd == MemCmd::InvalidCmd);
            // save response state
            snoop_response_cmd = pkt->cmd;
            snoop_response_latency = latency;

            pkt->cmd = orig_cmd;
        }
    return std::make_pair(snoop_response_cmd, snoop_response_latency);
}

void
PciBridge::forwardTiming(PacketPtr pkt)
{
    DPRINTF(PciBridge, "%s for %s\n", __func__, pkt->print());

    DPRINTF(PciBridge, "PciBridge::forwardTiming pkt cacheResponding=%d, ExpressSnoop=%d\n", pkt->cacheResponding(), pkt->isExpressSnoop());


    // pkt->setExpressSnoop();
    // if (pkt->cacheResponding()) {
    //     pkt->setExpressSnoop();
    // }

    // snoops should only happen if the system isn't bypassing caches

    slavePort.sendTimingSnoopReq(pkt);
}






}