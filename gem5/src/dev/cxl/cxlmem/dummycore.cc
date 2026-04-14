#include "dev/cxl/cxlmem/cxlmemdev.hh"
#include "debug/AddrRanges.hh"
#include "debug/CxlDevice.hh"
#include "dev/cxl/cxlmem/dummycore.hh"

namespace gem5{
bool
DummyCore::DummyProcPort::recvTimingResp(PacketPtr pkt)
{return true; }

void
DummyCore::DummyProcPort::recvReqRetry()
{}

DummyCore::DummyCore(const DummyCoreParams &p)
    : ClockedObject(p), dummyMemSidePort(name()+".dummyMemSidePort", this)
{}


Port&
DummyCore::getPort(const std::string &if_name,
            PortID idx)
{
    if (if_name == "dummyMemSidePort")
        return dummyMemSidePort;
    else
        return ClockedObject::getPort(if_name, idx); 
}
}