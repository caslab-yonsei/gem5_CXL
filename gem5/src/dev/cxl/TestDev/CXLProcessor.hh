#ifndef __CXLPROCESSOR_HH__
#define __CXLPROCESSOR_HH__

#include <deque>

#include "sim/clocked_object.hh"
#include "base/str.hh"
#include "mem/port.hh"
#include "mem/request.hh"
#include "debug/CXLProcessor.hh"
#include "params/CXLProcessor.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "sim/drain.hh"
#include "sim/system.hh"
#include "base/chunk_generator.hh"

namespace gem5{



class CXLProcessor;

class CXLProcPort : public RequestPort, public Drainable
{

    friend class CXLProcessor;

    public: 
        CXLProcessor *processor;

        System *const sys;

        uint32_t pendingCount = 0;

        PacketPtr inRetry = nullptr;

        CXLProcPort(CXLProcessor *proc, System *s);

        // AddrRangeList
        // getAddrRanges() const;

        EventFunctionWrapper sendEvent;

        bool retryPending = false;

        const Addr cacheLineSize;


        Tick recvAtomicSnoop(PacketPtr) override { return 0; }
        bool recvTimingResp(PacketPtr) override { return 0;}
        void recvTimingSnoopReq(PacketPtr) override { }
        void recvReqRetry() override { }
        void recvRetrySnoopResp() override { }
        void recvFunctionalSnoop(PacketPtr) override {  }


        void execCXLProc(Addr addr, int size, Packet::Command cmd, uint8_t *data);
        void sendPacket();


        struct CXLProcReqState : public Packet::SenderState
        {
            Event *completionEvent;
            Event *abortEvent;
            bool aborted = false;
            const Addr totBytes;
            Addr numBytes = 0;
            const Tick delay;
            ChunkGenerator gen;
            uint8_t *const data = nullptr;
            const Request::Flags flags;
            const Packet::Command cmd;

            CXLProcReqState(Packet::Command _cmd, Addr addr, Addr chunk_sz, Addr tb,
                    uint8_t *_data, Request::Flags _flags, Event *ce, Tick _delay,
                    Event *ae=nullptr)
                : completionEvent(ce), abortEvent(ae), totBytes(tb), delay(_delay),
                gen(addr, tb, chunk_sz), data(_data), flags(_flags), cmd(_cmd)
            {}

            PacketPtr createPacket();
        };

        std::deque<CXLProcReqState *> transmitList;
        bool sendReq(CXLProcReqState *state);
        void handleRespPacket(PacketPtr pkt, Tick delay);
        void handleResp(CXLProcReqState* state, Addr addr, Addr size, Tick delay);

        void trySendTimingReq();

        DrainState drain() override;

};



class CXLProcessor : public ClockedObject
{

    friend class CXLProcPort;

    public:

        System *sys;
        EventFunctionWrapper sendCXLProcEvent;
        CXLProcPort mem_side_ports;
        // uint32_t pendingCount = 0;
        CXLProcessor(const CXLProcessorParams &p);

        virtual ~CXLProcessor();

        // void execCXLProc(Addr addr, int size, Packet::Command cmd, uint8_t *data);
        // void sendPacket();
        void CXLWrite(Addr addr, int size, uint8_t *data);
        void CXLRead(Addr addr, int size, uint8_t *data);
        void CXLProcRUN();

        void init() override;
        Port & getPort(const std::string &if_name, PortID idx) override;

        

        // struct CXLProcReqState : public Packet::SenderState
        // {
        //     Event *completionEvent;
        //     Event *abortEvent;
        //     bool aborted = false;
        //     const Addr totBytes;
        //     Addr numBytes = 0;
        //     const Tick delay;
        //     ChunkGenerator gen;
        //     uint8_t *const data = nullptr;
        //     const Request::Flags flags;
        //     const Packet::Command cmd;

        //     CXLProcReqState(Packet::Command _cmd, Addr addr, Addr chunk_sz, Addr tb,
        //             uint8_t *_data, Request::Flags _flags, Event *ce, Tick _delay,
        //             Event *ae=nullptr)
        //         : completionEvent(ce), abortEvent(ae), totBytes(tb), delay(_delay),
        //         gen(addr, tb, chunk_sz), data(_data), flags(_flags), cmd(_cmd)
        //     {}

        //     PacketPtr createPacket();
        // };

        // std::deque<CXLProcReqState *> transmitList;
        // bool sendReq(CXLProcReqState *state);
        // void handleRespPacket(PacketPtr pkt, Tick delay);
        // void handleResp(CXLProcReqState* state, Addr addr, Addr size, Tick delay);

        

        // enum Packet::cacheCASE choiceType(Addr addr) {
        //     if (addr == 0x90000000L) {
        //         return Packet::cacheCASE::hostAddr_hostcache;
        //     }
        //     else if (addr == 0x91000000L) {
        //         return Packet::cacheCASE::hostAddr_devcache;
        //     }
        //     else if (addr == 0x92000000L) {
        //         return Packet::cacheCASE::hostAddr_nocache;
        //     }
        //     else if (addr == 0x93000000L) {
        //         return Packet::cacheCASE::hostAddr_uncacheable;
        //     }
        //     else if (addr == 0x16000000L) {
        //         return Packet::cacheCASE::devAddr_hostcache;
        //     }
        //     else if (addr == 0x16100000L) {
        //         return Packet::cacheCASE::devAddr_devcache;
        //     }
        //     else if (addr == 0x16200000L) {
        //         return Packet::cacheCASE::devAddr_nocache;
        //     }
        //     else if (addr == 0x16300000L) {
        //         return Packet::cacheCASE::devAddr_uncacheable;
        //     }

        // };
 


};










}




#endif