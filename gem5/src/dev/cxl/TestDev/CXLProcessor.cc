#include <algorithm>
#include <cassert>
#include <cstring>
#include <utility>

#include "debug/CXLProcessor.hh"
#include "debug/Drain.hh"
#include "params/CXLProcessor.hh"
#include "dev/cxl/TestDev/CXLProcessor.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "sim/system.hh"
#include "sim/clocked_object.hh"
#include "mem/port.hh"
#include "base/chunk_generator.hh"
#include "sim/cur_tick.hh"
#include "sim/eventq.hh"
#include "mem/request.hh"

namespace gem5 {

CXLProcPort::CXLProcPort(CXLProcessor *proc, System *s)
: RequestPort(proc->name() + ".cxlprocport", proc), processor(proc), sys(s), cacheLineSize(s->cacheLineSize()),
sendEvent([this]{ sendPacket(); }, proc->name())
{}

CXLProcessor::CXLProcessor(const CXLProcessorParams &p)
: ClockedObject(p), mem_side_ports(this, p.system), sys(p.system),
sendCXLProcEvent([this]{ CXLProcRUN(); }, name())
{}

CXLProcessor::~CXLProcessor()
{}


void
CXLProcessor::init()
{
    ClockedObject::init();

    if (!mem_side_ports.isConnected())
        panic("cxlproc mem_side_ports of %s not connected to anything!", name());

    DPRINTF(CXLProcessor, "(CXL_Debug) Init CXLProcessor\n");
    // schedule(sendCXLProcEvent, 1400000000L+clockEdge(Cycles(1)));
    // schedule(sendCXLProcEvent, 1200000000L+clockEdge(Cycles(1)));
}

Port &
CXLProcessor::getPort(const std::string &if_name, PortID idx) {
    if (if_name == "mem_side_ports") {
      return mem_side_ports;
    }
    return ClockedObject::getPort(if_name, idx);
}

PacketPtr
CXLProcPort::CXLProcReqState::createPacket()
{
    RequestPtr req = std::make_shared<Request>(
            gen.addr(), gen.size(), flags, 0);
    req->taskId(context_switch_task_id::DMA);

    PacketPtr pkt = new Packet(req, cmd);

    pkt->req->from_cxl_proc = true;
    pkt->req->cxl_packet_type = CXLPacketType::CXLProcReq;

    if (data)
        pkt->dataStatic(data + gen.complete());

    pkt->senderState = this;

    return pkt;
}

void
CXLProcPort::execCXLProc(Addr addr, int size, Packet::Command cmd, uint8_t *data)
{
    DPRINTF(CXLProcessor, "(CXL_Debug) CXLProcessor create Req Packet to %llx\n", addr);
    
    PacketPtr pkt;

    transmitList.push_back(
            new CXLProcReqState(cmd, addr,
                sys->cacheLineSize(), size, data, 0, NULL, 100));
    
    sendPacket();
}

void
CXLProcPort::sendPacket()
{
    assert(transmitList.size());

    if (sys->isTimingMode()) {
        // If we are either waiting for a retry or are still waiting after
        // sending the last packet, then do not proceed.
        if (retryPending || sendEvent.scheduled()) {
            DPRINTF(CXLProcessor, "Can't send immediately, waiting to send\n");
            return;
        }
        trySendTimingReq();
    }
    else if (sys->isAtomicMode()) {
        const bool bypass = sys->bypassCaches();

        // Send everything there is to send in zero time.
        while (!transmitList.empty()) {
            CXLProcReqState *state = transmitList.front();
            transmitList.pop_front();

            bool done = state->gen.done();
            while (!done)
                done = sendReq(state);
        }
    } else {
        panic("Unknown memory mode.");
    }
}

bool
CXLProcPort::sendReq(CXLProcReqState *state)
{
    PacketPtr pkt= state->createPacket();
    // pkt->pkt_cacheCASE = choiceType(pkt->getAddr());
    DPRINTF(CXLProcessor, "CXLProcessor::sendReq pkt->req->from_cxl_proc = %s\n", pkt->req->from_cxl_proc ? "True" : "False");
    pendingCount++;
    if (pkt->cmd == MemCmd::WriteReq) {
        DPRINTF(CXLProcessor, "(CXL_Debug) CXLProcessor::sendReq from CXLProc pkt->cmd = MemCmd::WriteReq\n");
    }
    else if (pkt->cmd == MemCmd::ReadReq) {
        DPRINTF(CXLProcessor, "(CXL_Debug) CXLProcessor::sendReq from CXLProc pkt->cmd = MemCmd::ReadReq\n");
    }
    else {
        fatal("CXLProcessor::sendReq unknown cmd!!\n");
    }
    Tick lat = sendAtomic(pkt);
    bool done = !state->gen.next();
    handleRespPacket(pkt, lat);
    return done;
}

void
CXLProcPort::handleRespPacket(PacketPtr pkt, Tick delay)
{
    assert(pkt->isResponse());
    warn_if(pkt->isError(), "Response pkt error.");

    auto *state = dynamic_cast<CXLProcReqState*>(pkt->senderState);
    assert(state);

    handleResp(state, pkt->getAddr(), pkt->req->getSize(), delay);

    delete pkt;

}

void
CXLProcPort::handleResp(CXLProcReqState* state, Addr addr, Addr size, Tick delay)
{
    assert(pendingCount != 0);
    pendingCount--;

    state->numBytes += size;
    assert(state->totBytes >= state->numBytes);

    bool all_bytes = (state->totBytes == state->numBytes);
    if (state->aborted) {
        if (all_bytes || pendingCount == 0) {
            // If yes, signal its abort event (if any) and delete the state.
            if (state->abortEvent) {
                processor->schedule(state->abortEvent, curTick());
            }
            delete state;
    }
    } else if (all_bytes) {
        if (state->completionEvent) {
            delay += state->delay;
            processor->schedule(state->completionEvent, curTick() + delay);
        }
        delete state;
    }

    if (pendingCount == 0)
        signalDrainDone();

}

void
CXLProcPort::trySendTimingReq()
{
    CXLProcReqState *state = transmitList.front();

    PacketPtr pkt = inRetry ? inRetry : state->createPacket();
    inRetry = nullptr;
    DPRINTF(CXLProcessor, "CXLProcessor::trySendTimingReq pkt->req->from_cxl_proc = %s\n", pkt->req->from_cxl_proc ? "True" : "False");

    if (pkt->cmd == MemCmd::WriteReq) {
        DPRINTF(CXLProcessor, "(CXL_Debug) CXLProcessor::trySendTimingReq from CXLProc pkt->cmd = MemCmd::WriteReq\n");
    }
    else if (pkt->cmd == MemCmd::ReadReq) {
        DPRINTF(CXLProcessor, "(CXL_Debug) CXLProcessor::trySendTimingReq from CXLProc pkt->cmd = MemCmd::ReadReq\n");
    }
    else {
        fatal("CXLProcessor::trySendTimingReq unknown cmd!!\n");
    }

    DPRINTF(CXLProcessor, "CXLProcessor: Trying to send %s addr %#x\n", pkt->cmdString(),
            pkt->getAddr());

    bool last = state->gen.last();
    if (sendTimingReq(pkt)) {
        pendingCount++;
    } else {
        retryPending = true;
        inRetry = pkt;
    }
    if (!retryPending) {
        state->gen.next();
        // If that was the last packet from this request, pop it from the list.
        if (last)
            transmitList.pop_front();
        DPRINTF(CXLProcessor, "-- Done\n");
        // If there is more to do, then do so.
        if (!transmitList.empty()) {
            // This should ultimately wait for as many cycles as the device
            // needs to send the packet, but currently the port does not have
            // any known width so simply wait a single cycle.
            processor->schedule(sendEvent, processor->clockEdge(Cycles(1)));
        }
    } else {
        DPRINTF(CXLProcessor, "-- Failed, waiting for retry\n");
    }

    DPRINTF(CXLProcessor, "TransmitList: %d, retryPending: %d\n",
            transmitList.size(), retryPending ? 1 : 0);
}

void
CXLProcessor::CXLWrite(Addr addr, int size, uint8_t *data)
{
    DPRINTF(CXLProcessor, "(CXL_Debug) CXLProcessor write access to %llx\n", addr);
    mem_side_ports.execCXLProc(addr, size, MemCmd::WriteReq, data);

}

void
CXLProcessor::CXLRead(Addr addr, int size, uint8_t *data) {
    DPRINTF(CXLProcessor, "(CXL_Debug) CXLProcessor read access to %llx\n", addr);
    mem_side_ports.execCXLProc(addr, size, MemCmd::ReadReq, data);

}

DrainState
CXLProcPort::drain()
{
    if (pendingCount == 0) {
        return DrainState::Drained;
    } else {
        DPRINTF(Drain, "CXLProcPort not drained\n");
        return DrainState::Draining;
    }
}

void
CXLProcessor::CXLProcRUN()
{
    // TEST RUNSET
    uint8_t num = 13;
    uint8_t read_num;
    uint8_t *data = &num;
    uint8_t *read_data = &read_num;

    // CXLWrite(0x80000000L, 4, data); // 호스트 캐시 invalid필요
    CXLWrite(0xA0000000L, 4, data);
    // CXLRead(0xA0000000L, 4, read_data);
    // CXLRead(0x80000000L, 4, read_data);
}

}