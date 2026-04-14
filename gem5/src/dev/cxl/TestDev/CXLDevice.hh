#ifndef __CXLDEVICE_HH__
#define __CXLDEVICE_HH__
#include <array>
#include <cstring>
#include <vector>
#include "dev/io_device.hh"
#include "mem/tport.hh"
#include "sim/clocked_object.hh"
#include "mem/qport.hh"
#include "dev/dma_device.hh"
#include "mem/packet.hh"
#include "mem/cxl/cxl_packet.hh"
#include "mem/cxl/cxl_cache_packet.hh"
#include "dev/pci/device.hh"
#include "dev/pci/pcie/device.hh"
// #include "debug/TestCXLMem.hh"
#include "params/PciBar.hh"
#include "params/PciBarNone.hh"
#include "params/PcieDevice.hh"
#include "params/PciDevice.hh"
#include "params/PciIoBar.hh"
#include "params/PciLegacyIoBar.hh"
#include "params/PciMemBar.hh"
#include "params/PciMemUpperBar.hh"
#include "dev/dma_device.hh"
#include "base/addr_range.hh"
#include "base/types.hh"
#include "debug/CXLDevice.hh"
#include "params/CXLDevice.hh"
#include "dev/cxl/TestDev/CXLProcessor.hh"
namespace gem5{
    
class CXLDevice;
// template <class CXLDevice>
// class CXLPioPort : public PioPort<CXLDevice>
// {
//     public:
//         CXLPioPort(CXLDevice *dev) :
//             PioPort<CXLDevice>(dev)
//         {}
//         CXLDevice *device;

//         bool recvTimingReq(PacketPtr pkt);
// };


// class CoherentPort : public RequestPort
// {
//     public: 
//         CXLDevice *device;
//         CoherentPort(CXLDevice *dev);
//         // AddrRangeList
//         // getAddrRanges() const;
//         Tick recvAtomicSnoop(PacketPtr) override { return 0; }
//         bool recvTimingResp(PacketPtr) override { } 
//         void recvTimingSnoopReq(PacketPtr) override { }
//         void recvReqRetry() override { }
//         void recvRetrySnoopResp() override { }
//         void recvFunctionalSnoop(PacketPtr) override {  }
// };


class InternalPort : public RequestPort
{
    public: 
        CXLDevice *device;

        InternalPort(CXLDevice *dev);
        Tick memAction(PacketPtr pkt);
        bool memActionTiming(PacketPtr pkt, Tick latency);
        // AddrRangeList
        // getAddrRanges() const;
        Tick recvAtomicSnoop(PacketPtr) override { return 0; }
        bool recvTimingResp(PacketPtr pkt) override;
        void recvTimingSnoopReq(PacketPtr) override { }
        void recvReqRetry() override { }
        void recvRetrySnoopResp() override { }
        void recvFunctionalSnoop(PacketPtr) override {  }
};
class ExternalPort_cxl: public QueuedResponsePort
{
    public:
    // 내부 유닛으로 응답을 보냄 & 내부 유닛으로부터 요청을 받음
        CXLDevice *device;

        // CXLDevice &dev;
        RespPacketQueue queue;

        Tick recvAtomic(PacketPtr pkt);
        Tick recvBiasAtomic(Addr addr, Addr originAddr, int* bias_mode, bool get_set=0);
        ExternalPort_cxl(CXLDevice *dev, CXLDevice &d);
        void recvFunctional(PacketPtr pkt) {};
        bool recvTimingReq(PacketPtr pkt);
        void recvRespRetry() {};
        AddrRangeList
        getAddrRanges() const; 
};
class CXLDevice : public PcieDevice
{
  
    friend DmaPort;
    friend DmaDevice;
    // friend class CXLPioPort<CXLDevice>;
    friend class InternalPort;
    friend class ExternalPort_cxl;
    public:
        System *sys;
        uint8_t Test_value;
        bool is_cxl = 0;
        Addr     pioAddr;
        Addr     pioSize;
        Tick     pioDelay;
        uint8_t retData8;
        uint16_t retData16;
        uint32_t retData32;
        uint64_t retData64;
        EventFunctionWrapper sendTestEvent;
        EventFunctionWrapper FinishReqEvent;
        EventFunctionWrapper DataSendEvent;
        EventFunctionWrapper FinishDataEvent;
        EventFunctionWrapper waitEvent;
        uint8_t* Test_value2;
    
        // CXLPioPort<CXLDevice> pioPort;

        CXLDevice(const CXLDeviceParams &p);
        void wait_cxl();
        void finish_data();
        Tick read(PacketPtr pkt);
        Tick write(PacketPtr pkt);
        Tick callDCOH(PacketPtr pkt);
        Tick ReadData(PacketPtr pkt);
        Tick WriteData(PacketPtr pkt);
        bool ReadDataTiming(PacketPtr pkt, Tick latency);
        bool WriteDataTiming(PacketPtr pkt, Tick latency);
        Tick LoadData(PacketPtr pkt);
        Tick SnpProcess(PacketPtr pkt);
        // CoherentPort coh_port;
        InternalPort in_port;
        ExternalPort_cxl ex_port;
        // CXL.cache
        void hostSend(PacketPtr pkt);
        bool hostSendTiming(PacketPtr pkt);
        void hostWrite(Addr addr, int size, Event *event, uint8_t *data, Tick delay=0);
        void hostRead(Addr addr, int size, Event *event, uint8_t *data, Tick delay=0);
        void hostData(Addr addr, int size, Event *event, uint8_t *data, Tick delay=0);
        void send_host();
        void host_data();
        void finish_req();
        // void processResponse(PacketPtr pkt);
        void init() override; 
        void makeRequest(PacketPtr pkt);
        void makeDataPkt(PacketPtr pkt, bool lastpkt);
        void ReservedData(PacketPtr pkt);
        AddrRangeList getAddrRanges() const;
        Port & getPort(const std::string &if_name, PortID idx) override;


        bool recvTimingReq(PacketPtr pkt);
        
        bool recvTimingResp(PacketPtr pkt);

    
    class DCOH {
        public:
        CXLDevice* parent;
        Tick recvReq(PacketPtr pkt);
        // Tick sendReq(PacketPtr pkt);
        void makeCXLrequest(PacketPtr pkt);
        void makeCXLData(PacketPtr pkt, bool lastpkt);
        void makeCXLresponse(PacketPtr pkt);
        // void makeMemData(PacketPtr pkt, PacketPtr ori_pkt);
        void makeMemDataAtomic(PacketPtr pkt);
        void makeSnoopResp(PacketPtr pkt);
        DCOH(CXLDevice* parent_ptr);
        std::string _name;
        std::string name() const;
        bool need_data;
        Addr data_dest_addr;
        Tick recvSnpReq(PacketPtr pkt);
        std::unordered_map<Addr, bool>  Bias_Table;  // Original Implement space is Cache // //true: host mode
        void set_bias_mode(Addr addr, bool bias_bit); // Original Implement space is Cache
        bool is_host_bias_mode(Addr addr); // Original Implement space is Cache
        bool is_device_mem(Addr addr) {
            // if ((addr >= 0x16000000L) && (addr <0x17000000L)) {
            if ((addr >= 0x80000000L) && (addr <0xA0000000L)) {
                return true;
            }
            else {
                return false;
            }
        }

        bool sinkPacket(const PacketPtr pkt) const
        {
            return (pkt->cacheResponding()) ||
                (!(pkt->isRead() || pkt->isWrite()) &&
                !pkt->needsResponse()) ||
                (pkt->isCleanEviction() && pkt->isBlockCached()) ||
                (pkt->cacheResponding() &&
                (!pkt->needsWritable() || pkt->responderHadWritable()));
        }
        bool recvTimingReq(PacketPtr pkt);
        bool recvCoherentCheckReq(PacketPtr pkt);
    };
    DCOH dcoh;

    bool recvCoherentCheckReq(PacketPtr pkt) { return dcoh.recvCoherentCheckReq(pkt); }
};
}
#endif