#include "cxl_mem_dev.hh"
#include "mem/port.hh"

namespace gem5{
namespace memory{

Port &
CxlMemOnlyDev::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "to_internal_port") {
        return *to_internal_xbar_port;
    }
    return ClockedObject::getPort(if_name, idx);
}

CxlMemOnlyDev::CxlMemOnlyDev(const CxlMemOnlyDevParams &p)
    :ClockedObject(p), internal_xbar(p.InternalMemBus), external_xbar(p.ExternalBus),
    to_internal_xbar_port(NULL)
{
    to_internal_xbar_port = new MemSidePort("to_internal_port", InvalidPortID);
}

CxlMemOnlyDev::~CxlMemOnlyDev()
{
    delete to_internal_xbar_port;
}

}
}