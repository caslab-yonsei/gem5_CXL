#ifndef __DEV_CXL_CXLMEM_DUMMYCORE_HH__
#define __DEV_CXL_CXLMEM_DUMMYCORE_HH__

#include "params/DummyCore.hh"
#include "mem/port.hh"
#include "dev/dma_device.hh"

#include "params/DummyCore.hh"
namespace gem5
{

class DummyCore : public ClockedObject
{
  public:
    class DummyProcPort : public RequestPort
    {
      public:
        DummyProcPort(const std::string& name_, SimObject *cpu_)
            : RequestPort(name_, cpu_)
        { }
        ~DummyProcPort(){}
        virtual bool recvTimingResp(PacketPtr pkt) override;
        virtual Tick recvAtomic(PacketPtr pkt) { return 0; }
        virtual void recvFunctional(PacketPtr pkt) {}
        virtual void recvRangeChange() { }
        virtual void recvReqRetry() override;

        virtual void
        recvRespRetry()
        {
            fatal("recvRespRetry() not implemented in DummyPort");
        }
        
    };
    DummyProcPort dummyMemSidePort;
    // virtual AddrRangeList getAddrRanges() const = 0;

  public:
    DummyCore(const DummyCoreParams &p);
    ~DummyCore(){};

    void init() {}

    Port &getPort(const std::string &if_name,
            PortID idx=InvalidPortID) override;
};

}

#endif