
#include <memory>
#include <string>

#include "dev/cxl/TestDev/CXLXBar.hh"
#include "dev/cxl/TestDev/CXLDevice.hh"
#include "mem/coherent_xbar.hh"
#include "params/CXLXBar.hh"
#include "debug/CXLXBar.hh"
#include "base/compiler.hh"
#include "base/logging.hh"
#include "base/trace.hh"
#include "sim/system.hh"
#include "debug/AddrRanges.hh"
#include "mem/packet.hh"
#include "mem/request.hh"

namespace gem5
{

bool
CXLXBar::CXLXBarResponsePort::recvTimingReq(PacketPtr pkt)
{
    DPRINTF(CXLXBar, "call CXLXBar::CXLXBarResponsePort::recvTimingReq\n");
    DPRINTF(CXLXBar, "PciBridge::forwardTiming pkt cacheResponding=%d, ExpressSnoop=%d\n", pkt->cacheResponding(), pkt->isExpressSnoop());

    if (id != 1)  {// from CXLDev & Host
        if (pkt->req->cxl_packet_type == CXLPacketType::HostBiasCheck) {
            DPRINTF(CXLXBar, "return coherence check pkt from host!\n");
            // PacketPtr pending_pkt;
            // for (int i = 0; i < parent.PendingPacketList.size(); i++) {
            //     if (pkt->req == (parent.PendingPacketList[i])->req) {
            //         pending_pkt = parent.PendingPacketList[i];
            //         parent.PendingPacketList.erase(parent.PendingPacketList.begin() + i);
            //         break;
            //     }
            // }
            // pkt->req = nullptr;
            // delete pkt;
            // return parent.recvTimingReq(pending_pkt, 1);
            if (!pkt->cacheResponding()) {
                pkt->clearExpressSnoop();
            }
            return parent.recvTimingReq(pkt, 1);
        }
        else {
            DPRINTF(CXLXBar, "default access from host!\n");
            return parent.recvTimingReq(pkt, id);
        }
    }
    else {
        DPRINTF(CXLXBar, "access from CXLProc!\n");
        Tick bias_check_latency = 0;
        DPRINTF(CXLXBar, "CXLXBar::recvTimingReq pkt->req->from_cxl_proc = %s, PortID=%d\n", pkt->req->from_cxl_proc ? "True" : "False", id);
        Addr destAddr = pkt->getAddr();
        DPRINTF(CXLXBar, "Packet from CXL Proc! addr=%llx\n", destAddr);
        // if ((destAddr >= 0x16000000L) && (destAddr < 0x17000000L)) {
        if ((destAddr >= 0x80000000L) && (destAddr < 0xA0000000L)) {
            DPRINTF(CXLXBar, "Pkt dest is Local Mem, Check Bias mode, pkt=%s\n", pkt->print());
            int check_bias = 0; // 0: init, 1: HostBias, 2:DeviceBias
            int *bias_ptr= &check_bias;
            Addr originAddr = pkt->getAddr();
            pkt->setAddr(0x15000000L);
            bias_check_latency = parent.CheckBias(pkt, originAddr, bias_ptr);
            pkt->setAddr(originAddr);


            if (check_bias == 1) {
                DPRINTF(CXLXBar, "Check Bias mode is Host, Need Coherence check to host\n");
                pkt->req->coherent_check_dest = pkt->getAddr();
                pkt->req->cxl_packet_type = CXLPacketType::HostBiasCheck;
                // pkt->setAddr(0xA0000000L);
                DPRINTF(CXLXBar, "(CXL_Debug) Host Bias Device Mem Access Coherrence Check\n");
                // memSidePorts[0]->sendAtomic(pkt);

                // PacketPtr Coherent_check_pkt = new Packet(pkt, false, true);

                // parent.PendingPacketList.push_back(pkt);

                AddrRange cxldev = RangeSize((Addr)0x15000000L, (Addr)0x40000);

                // bool success = parent.memSidePorts[parent.BaseXBar::findPort(cxldev)]->sendTimingReq(Coherent_check_pkt);

                assert(parent.routeTo.find(pkt->req) == parent.routeTo.end());
                if (pkt->getAddr() >= 0x80000000)
                    DPRINTF(CXLXBar, "CXLXBar::CXLXBarResponsePort::recvTimingReq Add routeTO pkt=%s is_cxl=%d\n", pkt->print(), pkt->req->cxl_packet_type);
                parent.routeTo[pkt->req] = id;

                bool success = parent.memSidePorts[parent.BaseXBar::findPort(cxldev)]->sendTimingReq(pkt);
                
                return success;
            }
            else if (check_bias == 2) {
                DPRINTF(CXLXBar, "Check Bias mode is Device!\n");
            }
            else {
                panic("failed Bias mode check!\n");
            }
        }
        else {
            DPRINTF(CXLXBar, "Pkt dest is other(ex. host) Mem\n");
        }
        return parent.recvTimingReq(pkt, id);
    }
}

Tick 
CXLXBar::CXLXBarRequestPort::sendBiasAtomic(Addr addr, Addr originAddr, int* bias_mode, bool get_set)
{
    DPRINTF(CXLXBar, "call CXLXBar::CXLXBarRequestPort::sendBiasAtomic\n");
    Tick tick = RequestPort::sendBiasAtomic(addr, originAddr, bias_mode, get_set);
    return tick;
}




CXLXBar::CXLXBar(const CXLXBarParams &p)
: CoherentXBar(p)
{
    for (auto port : memSidePorts) {
        delete port;
    }
    for (auto port : cpuSidePorts) {
        delete port;
    }

    for (auto layer : reqLayers) {
        delete layer;
    }

    for (auto layer : snoopLayers) {
        delete layer;
    }

    for (auto layer : respLayers) {
        delete layer;
    }

    for (auto layer : snoopRespPorts) {
        delete layer;
    }
    
    memSidePorts.clear();
    cpuSidePorts.clear();
    reqLayers.clear();
    snoopLayers.clear();
    respLayers.clear();
    snoopRespPorts.clear();

    for (int i = 0; i < p.port_mem_side_ports_connection_count; ++i) {
        DPRINTF(AddrRanges, "(CXL_Debug) Init CXLXBar mem_side_port[%d]\n", i);
        std::string portName = csprintf("%s.cxlmem_side_port[%d]", name(), i);
        RequestPort* bp = new CXLXBarRequestPort(portName, *this, i);
        memSidePorts.push_back(bp);
        reqLayers.push_back(new ReqLayer(*bp, *this,
                                         csprintf("cxlreqLayer%d", i)));
        snoopLayers.push_back(
                new SnoopRespLayer(*bp, *this, csprintf("cxlsnoopLayer%d", i)));
    }

    if (p.port_default_connection_count) {
        defaultPortID = memSidePorts.size();
        std::string portName = name() + ".cxldefault";
        RequestPort* bp = new CXLXBarRequestPort(portName, *this,
                                                    defaultPortID);
        memSidePorts.push_back(bp);
        reqLayers.push_back(new ReqLayer(*bp, *this, csprintf("cxlreqLayer%d",
                                         defaultPortID)));
        snoopLayers.push_back(new SnoopRespLayer(*bp, *this,
                                                 csprintf("cxlsnoopLayer%d",
                                                          defaultPortID)));
    }
    for (int i = 0; i < p.port_cpu_side_ports_connection_count; ++i) {
        DPRINTF(AddrRanges, "(CXL_Debug) Init CXLXBar cpu_side_port[%d]\n", i);
        std::string portName = csprintf("%s.cxlcpu_side_port[%d]", name(), i);
        QueuedResponsePort* bp = new CXLXBarResponsePort(portName, *this, i);
        cpuSidePorts.push_back(bp);
        respLayers.push_back(new RespLayer(*bp, *this,
                                           csprintf("cxlrespLayer%d", i)));
        snoopRespPorts.push_back(new SnoopRespPort(*bp, *this));
    }
}

CXLXBar::~CXLXBar()
{
    CoherentXBar::~CoherentXBar();
}

void
CXLXBar::init()
{
    CoherentXBar::init();
}

Tick
CXLXBar::CheckBias(PacketPtr pkt, Addr originAddr, int* bias_mode)
{
    DPRINTF(CXLXBar, "call CXLXBar::CheckBias\n");
    Tick bias_check_latency = 0;
    PortID mem_side_port_id = findPort(pkt);
    Addr destAddr = pkt->getAddr();
    CXLXBarRequestPort* mem_side_port = (CXLXBarRequestPort*)(memSidePorts[mem_side_port_id]);
    DPRINTF(CXLXBar, "bias_mode = %d, &bias_mode = %llx\n", *bias_mode, bias_mode);
    bias_check_latency += mem_side_port->sendBiasAtomic(destAddr, originAddr, bias_mode, 0);
    DPRINTF(CXLXBar, "bias_mode = %d, &bias_mode = %llx\n", *bias_mode, bias_mode);

    return bias_check_latency;
}

Tick
CXLXBar::recvAtomicBackdoor(PacketPtr pkt, PortID cpu_side_port_id,
                                 MemBackdoorPtr *backdoor)
{
    DPRINTF(CXLXBar, "%s: src %s packet %s\n", __func__,
            cpuSidePorts[cpu_side_port_id]->name(), pkt->print());
    

    // -------------------------------------------- check Bias mode ------------------------------------------
    Tick bias_check_latency = 0;
    DPRINTF(CXLXBar, "CXLXBar::recvAtomicBackdoor pkt->req->from_cxl_proc = %s, PortID=%d\n", pkt->req->from_cxl_proc ? "True" : "False", cpu_side_port_id);
    if (cpu_side_port_id == 1) { // form CXL Proc
        Addr destAddr = pkt->getAddr();
        DPRINTF(CXLXBar, "Packet from CXL Proc! addr=%llx\n", destAddr);
        // if ((destAddr >= 0x16000000L) && (destAddr < 0x17000000L)) {
        if ((destAddr >= 0x80000000L) && (destAddr < 0xA0000000L)) {
            DPRINTF(CXLXBar, "Pkt dest is Local Mem, Check Bias mode, pkt=%s\n", pkt->print());
            int check_bias = 0; // 0: init, 1: HostBias, 2:DeviceBias
            int *bias_ptr= &check_bias;
            Addr originAddr = pkt->getAddr();
            pkt->setAddr(0x15000000L);
            bias_check_latency = CheckBias(pkt, originAddr, bias_ptr);
            pkt->setAddr(originAddr);


            if (check_bias == 1) {
                DPRINTF(CXLXBar, "Check Bias mode is Host, Need Coherence check to host\n");
                pkt->req->coherent_check_dest = pkt->getAddr();
                pkt->req->cxl_packet_type = CXLPacketType::HostBiasCheck;
                // pkt->setAddr(0xA0000000L);
                DPRINTF(CXLXBar, "(CXL_Debug) Host Bias Device Mem Access Coherrence Check\n");
                memSidePorts[0]->sendAtomic(pkt);
                if (pkt->req->cxl_req != nullptr)
                    DPRINTF(CXLXBar, "(CXL_Debug) After Coherrence Check \n%s\n", pkt->req->cxl_req->print());
                delete pkt->req->cxl_req;
                
            }
            else if (check_bias == 2) {
                DPRINTF(CXLXBar, "Check Bias mode is Device!\n");
            }
            else {
                panic("failed Bias mode check!\n");
            }
        }
        else {
            DPRINTF(CXLXBar, "Pkt dest is other(ex. host) Mem\n");
        }
    }

    // --------------------------------------------------------------------------------------------------------

    unsigned int pkt_size = pkt->hasData() ? pkt->getSize() : 0;
    unsigned int pkt_cmd = pkt->cmdToIndex();

    MemCmd snoop_response_cmd = MemCmd::InvalidCmd;
    Tick snoop_response_latency = 0;

    // is this the destination point for this packet? (e.g. true if
    // this xbar is the PoC for a cache maintenance operation to the
    // PoC) otherwise the destination is any cache that can satisfy
    // the request
    const bool is_destination = isDestination(pkt);

    const bool snoop_caches = !system->bypassCaches() &&
        pkt->cmd != MemCmd::WriteClean;
    if (snoop_caches) {
        // forward to all snoopers but the source
        std::pair<MemCmd, Tick> snoop_result;
        if ((snoopFilter) && (pkt->req->cxl_packet_type != CXLPacketType::CXLcacheH2DReq)) {
            // check with the snoop filter where to forward this packet
            auto sf_res =
                snoopFilter->lookupRequest(pkt,
                *cpuSidePorts [cpu_side_port_id]);
            snoop_response_latency += sf_res.second * clockPeriod();
            DPRINTF(CXLXBar, "%s: src %s packet %s SF size: %i lat: %i\n",
                    __func__, cpuSidePorts[cpu_side_port_id]->name(),
                    pkt->print(), sf_res.first.size(), sf_res.second);

            // let the snoop filter know about the success of the send
            // operation, and do it even before sending it onwards to
            // avoid situations where atomic upward snoops sneak in
            // between and change the filter state
            snoopFilter->finishRequest(false, pkt->getAddr(), pkt->isSecure());

            if (pkt->isEviction()) {
                // for block-evicting packets, i.e. writebacks and
                // clean evictions, there is no need to snoop up, as
                // all we do is determine if the block is cached or
                // not, instead just set it here based on the snoop
                // filter result
                if (!sf_res.first.empty())
                    pkt->setBlockCached();
            } else {
                snoop_result = forwardAtomic(pkt, cpu_side_port_id,
                                            InvalidPortID, sf_res.first);
            }
        } else {
            snoop_result = forwardAtomic(pkt, cpu_side_port_id);
        }
        snoop_response_cmd = snoop_result.first;
        snoop_response_latency += snoop_result.second;

        if (pkt->req->cxl_packet_type == CXLPacketType::CXLcacheH2DReq) {
            DPRINTF(CXLXBar, "finish opration in CXL XBar\n");
            return snoop_response_latency;

        }
    }



    

    // set up a sensible default value
    Tick response_latency = 0;

    // CXL_Debug
    response_latency += bias_check_latency;

    const bool sink_packet = sinkPacket(pkt);


    // if ((pkt->req->cxl_packet_type == CXLPacketType::CXLcacheH2DReq) && !sink_packet) {
    //     DPRINTF(CXLXBar, "complete host->cxl snoop operation in CXLXBar\n");
    //     return snoop_response_latency;
    // }
    

    // even if we had a snoop response, we must continue and also
    // perform the actual request at the destination
    PortID mem_side_port_id = findPort(pkt);

    if (sink_packet) {
        DPRINTF(CXLXBar, "%s: Not forwarding %s\n", __func__,
                pkt->print());
    } else {
        if (forwardPacket(pkt)) {
            // make sure that the write request (e.g., WriteClean)
            // will stop at the memory below if this crossbar is its
            // destination
            if (pkt->isWrite() && is_destination) {
                pkt->clearWriteThrough();
            }

            // forward the request to the appropriate destination
            auto mem_side_port = memSidePorts[mem_side_port_id];
            response_latency = backdoor ?
                mem_side_port->sendAtomicBackdoor(pkt, *backdoor) :
                mem_side_port->sendAtomic(pkt);
        } else {
            // if it does not need a response we sink the packet above
            assert(pkt->needsResponse());

            pkt->makeResponse();
        }
    }

    // stats updates for the request
    pktCount[cpu_side_port_id][mem_side_port_id]++;
    pktSize[cpu_side_port_id][mem_side_port_id] += pkt_size;
    transDist[pkt_cmd]++;


    // if lower levels have replied, tell the snoop filter
    if (!system->bypassCaches() && snoopFilter && pkt->isResponse()) {
        snoopFilter->updateResponse(pkt, *cpuSidePorts[cpu_side_port_id]);
    }

    // if we got a response from a snooper, restore it here
    if (snoop_response_cmd != MemCmd::InvalidCmd) {
        // no one else should have responded
        assert(!pkt->isResponse());
        pkt->cmd = snoop_response_cmd;
        response_latency = snoop_response_latency;
    }

    // If this is the destination of the cache clean operation the
    // crossbar is responsible for responding. This crossbar will
    // respond when the cache clean is complete. An atomic cache clean
    // is complete when the crossbars receives the cache clean
    // request (CleanSharedReq, CleanInvalidReq), as either:
    // * no cache above had a dirty copy of the block as indicated by
    //   the satisfied flag of the packet, or
    // * the crossbar has already seen the corresponding write
    //   (WriteClean) which updates the block in the memory below.
    if (pkt->isClean() && isDestination(pkt) && pkt->satisfied()) {
        auto it = outstandingCMO.find(pkt->id);
        assert(it != outstandingCMO.end());
        // we are responding right away
        outstandingCMO.erase(it);
    } else if (pkt->cmd == MemCmd::WriteClean && isDestination(pkt)) {
        // if this is the destination of the operation, the xbar
        // sends the responce to the cache clean operation only
        // after having encountered the cache clean request
        [[maybe_unused]] auto ret = outstandingCMO.emplace(pkt->id, nullptr);
        // in atomic mode we know that the WriteClean packet should
        // precede the clean request
        assert(ret.second);
    }

    // add the response data
    if (pkt->isResponse()) {
        pkt_size = pkt->hasData() ? pkt->getSize() : 0;
        pkt_cmd = pkt->cmdToIndex();

        // stats updates
        pktCount[cpu_side_port_id][mem_side_port_id]++;
        pktSize[cpu_side_port_id][mem_side_port_id] += pkt_size;
        transDist[pkt_cmd]++;
    }

    // @todo: Not setting header time
    pkt->payloadDelay = response_latency;
    return response_latency;
}


bool
CXLXBar::recvTimingReq(PacketPtr pkt, PortID cpu_side_port_id)
{
    // DPRINTF(CXLXBar, "CXLXBar::recvTimingReq: before snoop %s\n", pkt->print());
    // determine the source port based on the id
    ResponsePort *src_port = cpuSidePorts[cpu_side_port_id];

    // remember if the packet is an express snoop
    bool is_express_snoop = pkt->isExpressSnoop();
    bool cache_responding = pkt->cacheResponding();
    // for normal requests, going downstream, the express snoop flag
    // and the cache responding flag should always be the same
    assert(is_express_snoop == cache_responding);

    // determine the destination based on the destination address range
    PortID mem_side_port_id = findPort(pkt);

    // test if the crossbar should be considered occupied for the current
    // port, and exclude express snoops from the check
    if (!is_express_snoop &&
        !reqLayers[mem_side_port_id]->tryTiming(src_port)) {
        DPRINTF(CXLXBar, "%s: src %s packet %s BUSY\n", __func__,
                src_port->name(), pkt->print());
        return false;
    }

    DPRINTF(CXLXBar, "%s: src %s packet %s\n", __func__,
            src_port->name(), pkt->print());

    // store size and command as they might be modified when
    // forwarding the packet
    unsigned int pkt_size = pkt->hasData() ? pkt->getSize() : 0;
    unsigned int pkt_cmd = pkt->cmdToIndex();

    // store the old header delay so we can restore it if needed
    Tick old_header_delay = pkt->headerDelay;

    // a request sees the frontend and forward latency
    Tick xbar_delay = (frontendLatency + forwardLatency) * clockPeriod();

    // set the packet header and payload delay
    calcPacketTiming(pkt, xbar_delay);

    // determine how long to be crossbar layer is busy
    Tick packetFinishTime = clockEdge(headerLatency) + pkt->payloadDelay;

    // is this the destination point for this packet? (e.g. true if
    // this xbar is the PoC for a cache maintenance operation to the
    // PoC) otherwise the destination is any cache that can satisfy
    // the request
    const bool is_destination = isDestination(pkt);

    const bool snoop_caches = !system->bypassCaches() &&
        pkt->cmd != MemCmd::WriteClean;
    if (snoop_caches) {
        assert(pkt->snoopDelay == 0);

        if (pkt->isClean() && !is_destination) {
            // before snooping we need to make sure that the memory
            // below is not busy and the cache clean request can be
            // forwarded to it
            if (!memSidePorts[mem_side_port_id]->tryTiming(pkt)) {
                DPRINTF(CXLXBar, "%s: src %s packet %s RETRY\n", __func__,
                        src_port->name(), pkt->print());

                // update the layer state and schedule an idle event
                reqLayers[mem_side_port_id]->failedTiming(src_port,
                                                        clockEdge(Cycles(1)));
                return false;
            }
        }


        // the packet is a memory-mapped request and should be
        // broadcasted to our snoopers but the source
        if (snoopFilter) {
            // check with the snoop filter where to forward this packet
            auto sf_res = snoopFilter->lookupRequest(pkt, *src_port);
            // the time required by a packet to be delivered through
            // the xbar has to be charged also with to lookup latency
            // of the snoop filter
            pkt->headerDelay += sf_res.second * clockPeriod();
            DPRINTF(CXLXBar, "%s: src %s packet %s SF size: %i lat: %i\n",
                    __func__, src_port->name(), pkt->print(),
                    sf_res.first.size(), sf_res.second);

            if (pkt->isEviction()) {
                // for block-evicting packets, i.e. writebacks and
                // clean evictions, there is no need to snoop up, as
                // all we do is determine if the block is cached or
                // not, instead just set it here based on the snoop
                // filter result
                if (!sf_res.first.empty())
                    pkt->setBlockCached();
            } else {
                forwardTiming(pkt, cpu_side_port_id, sf_res.first);
            }
        } else {
            forwardTiming(pkt, cpu_side_port_id);
        }

        DPRINTF(CXLXBar, "CXLXBar::recvTimingReq: after snoop %s\n", pkt->print());

        // add the snoop delay to our header delay, and then reset it
        pkt->headerDelay += pkt->snoopDelay;
        pkt->snoopDelay = 0;
    }

    // set up a sensible starting point
    bool success = true;

    // remember if the packet will generate a snoop response by
    // checking if a cache set the cacheResponding flag during the
    // snooping above
    const bool expect_snoop_resp = !cache_responding && pkt->cacheResponding();
    bool expect_response = pkt->needsResponse() && !pkt->cacheResponding();

    const bool sink_packet = sinkPacket(pkt);

    // in certain cases the crossbar is responsible for responding
    bool respond_directly = false;
    // store the original address as an address mapper could possibly
    // modify the address upon a sendTimingRequest
    const Addr addr(pkt->getAddr());
    if (sink_packet) {
        DPRINTF(CXLXBar, "%s: Not forwarding %s\n", __func__,
                pkt->print());
    } else {
        // determine if we are forwarding the packet, or responding to
        // it
        if (forwardPacket(pkt)) {
            // if we are passing on, rather than sinking, a packet to
            // which an upstream cache has committed to responding,
            // the line was needs writable, and the responding only
            // had an Owned copy, so we need to immidiately let the
            // downstream caches know, bypass any flow control
            if (pkt->cacheResponding()) {
                pkt->setExpressSnoop();
            }

            // make sure that the write request (e.g., WriteClean)
            // will stop at the memory below if this crossbar is its
            // destination
            if (pkt->isWrite() && is_destination) {
                pkt->clearWriteThrough();
            }

            // since it is a normal request, attempt to send the packet
            success = memSidePorts[mem_side_port_id]->sendTimingReq(pkt);
        } else {
            // no need to forward, turn this packet around and respond
            // directly
            assert(pkt->needsResponse());

            respond_directly = true;
            assert(!expect_snoop_resp);
            expect_response = false;
        }
    }

    if (snoopFilter && snoop_caches) {
        // Let the snoop filter know about the success of the send operation
        snoopFilter->finishRequest(!success, addr, pkt->isSecure());
    }

    // check if we were successful in sending the packet onwards
    if (!success)  {
        // express snoops should never be forced to retry
        assert(!is_express_snoop);

        // restore the header delay
        pkt->headerDelay = old_header_delay;

        DPRINTF(CXLXBar, "%s: src %s packet %s RETRY\n", __func__,
                src_port->name(), pkt->print());

        // update the layer state and schedule an idle event
        reqLayers[mem_side_port_id]->failedTiming(src_port,
                                                clockEdge(Cycles(1)));
    } else {
        // express snoops currently bypass the crossbar state entirely
        if (!is_express_snoop) {
            // if this particular request will generate a snoop
            // response
            if (expect_snoop_resp) {
                // we should never have an exsiting request outstanding
                assert(outstandingSnoop.find(pkt->req) ==
                       outstandingSnoop.end());
                outstandingSnoop.insert(pkt->req);

                // basic sanity check on the outstanding snoops
                panic_if(outstandingSnoop.size() > maxOutstandingSnoopCheck,
                         "%s: Outstanding snoop requests exceeded %d\n",
                         name(), maxOutstandingSnoopCheck);
            }

            // remember where to route the normal response to
            if (expect_response || expect_snoop_resp) {
                if ((pkt->req->cxl_packet_type != CXLPacketType::HostBiasCheck) && (pkt->req->cxl_packet_type != CXLPacketType::CXLcacheH2DResp)) {
                    assert(routeTo.find(pkt->req) == routeTo.end());
                    if (pkt->getAddr() >= 0x80000000)
                        DPRINTF(CXLXBar, "CXLXBar::recvTimingReq Add routeTO pkt=%s is_cxl=%d\n", pkt->print(), pkt->req->cxl_packet_type);
                    routeTo[pkt->req] = cpu_side_port_id;

                    panic_if(routeTo.size() > maxRoutingTableSizeCheck,
                            "%s: Routing table exceeds %d packets\n",
                            name(), maxRoutingTableSizeCheck);
                }
                else if ((sink_packet) && (pkt->req->cxl_packet_type == CXLPacketType::CXLcacheH2DResp)) {
                    assert(routeTo.find(pkt->req) == routeTo.end());
                    if (pkt->getAddr() >= 0x80000000)
                        DPRINTF(CXLXBar, "CXLXBar::recvTimingReq Add routeTO pkt=%s is_cxl=%d\n", pkt->print(), pkt->req->cxl_packet_type);
                    routeTo[pkt->req] = cpu_side_port_id;

                    panic_if(routeTo.size() > maxRoutingTableSizeCheck,
                            "%s: Routing table exceeds %d packets\n",
                            name(), maxRoutingTableSizeCheck);
                }
            }

            // update the layer state and schedule an idle event
            reqLayers[mem_side_port_id]->succeededTiming(packetFinishTime);
        }

        // stats updates only consider packets that were successfully sent
        pktCount[cpu_side_port_id][mem_side_port_id]++;
        pktSize[cpu_side_port_id][mem_side_port_id] += pkt_size;
        transDist[pkt_cmd]++;

        if (is_express_snoop) {
            snoops++;
            snoopTraffic += pkt_size;
        }
    }

    if (sink_packet)
        // queue the packet for deletion
        pendingDelete.reset(pkt);

    // normally we respond to the packet we just received if we need to
    PacketPtr rsp_pkt = pkt;
    PortID rsp_port_id = cpu_side_port_id;

    // If this is the destination of the cache clean operation the
    // crossbar is responsible for responding. This crossbar will
    // respond when the cache clean is complete. A cache clean
    // is complete either:
    // * direcly, if no cache above had a dirty copy of the block
    //   as indicated by the satisfied flag of the packet, or
    // * when the crossbar has seen both the cache clean request
    //   (CleanSharedReq, CleanInvalidReq) and the corresponding
    //   write (WriteClean) which updates the block in the memory
    //   below.
    if (success &&
        ((pkt->isClean() && pkt->satisfied()) ||
         pkt->cmd == MemCmd::WriteClean) &&
        is_destination) {
        PacketPtr deferred_rsp = pkt->isWrite() ? nullptr : pkt;
        auto cmo_lookup = outstandingCMO.find(pkt->id);
        if (cmo_lookup != outstandingCMO.end()) {
            // the cache clean request has already reached this xbar
            respond_directly = true;
            if (pkt->isWrite()) {
                rsp_pkt = cmo_lookup->second;
                assert(rsp_pkt);

                // determine the destination
                const auto route_lookup = routeTo.find(rsp_pkt->req);
                assert(route_lookup != routeTo.end());
                rsp_port_id = route_lookup->second;
                assert(rsp_port_id != InvalidPortID);
                assert(rsp_port_id < respLayers.size());
                // remove the request from the routing table
                if (pkt->getAddr() >= 0x80000000)
                    DPRINTF(CXLXBar, "CXLXBar::recvTimingReq Erase routeTO pkt=%s is_cxl=%d\n", pkt->print(), pkt->req->cxl_packet_type);
                routeTo.erase(route_lookup);
            }
            outstandingCMO.erase(cmo_lookup);
        } else {
            respond_directly = false;
            outstandingCMO.emplace(pkt->id, deferred_rsp);
            if (!pkt->isWrite()) {
                assert(routeTo.find(pkt->req) == routeTo.end());
                if ((pkt->req->cxl_packet_type != CXLPacketType::HostBiasCheck) && (pkt->req->cxl_packet_type != CXLPacketType::CXLcacheH2DResp)) {
                    if (pkt->getAddr() >= 0x80000000)
                        DPRINTF(CXLXBar, "CXLXBar::recvTimingReq Add routeTO pkt=%s is_cxl=%d\n", pkt->print(), pkt->req->cxl_packet_type);
                    routeTo[pkt->req] = cpu_side_port_id;
                }
                else if ((sink_packet) && (pkt->req->cxl_packet_type == CXLPacketType::CXLcacheH2DResp)) {
                    if (pkt->getAddr() >= 0x80000000)
                        DPRINTF(CXLXBar, "CXLXBar::recvTimingReq Add routeTO pkt=%s is_cxl=%d\n", pkt->print(), pkt->req->cxl_packet_type);
                    routeTo[pkt->req] = cpu_side_port_id;
                }

                panic_if(routeTo.size() > maxRoutingTableSizeCheck,
                         "%s: Routing table exceeds %d packets\n",
                         name(), maxRoutingTableSizeCheck);
            }
        }
    }


    if (respond_directly) {
        assert(rsp_pkt->needsResponse());
        assert(success);

        rsp_pkt->makeResponse();

        if (snoopFilter && !system->bypassCaches()) {
            // let the snoop filter inspect the response and update its state
            snoopFilter->updateResponse(rsp_pkt, *cpuSidePorts[rsp_port_id]);
        }

        // we send the response after the current packet, even if the
        // response is not for this packet (e.g. cache clean operation
        // where both the request and the write packet have to cross
        // the destination xbar before the response is sent.)
        Tick response_time = clockEdge() + pkt->headerDelay;
        rsp_pkt->headerDelay = 0;

        cpuSidePorts[rsp_port_id]->schedTimingResp(rsp_pkt, response_time);
    }

    return success;
}



std::pair<MemCmd, Tick>
CXLXBar::forwardAtomic(PacketPtr pkt, PortID exclude_cpu_side_port_id,
                           PortID source_mem_side_port_id,
                           const std::vector<QueuedResponsePort*>& dests)
{
    // the packet may be changed on snoops, record the original
    // command to enable us to restore it between snoops so that
    // additional snoops can take place properly
    MemCmd orig_cmd = pkt->cmd;
    MemCmd snoop_response_cmd = MemCmd::InvalidCmd;
    Tick snoop_response_latency = 0;


    unsigned fanout = 0;

    if (pkt->req->cxl_packet_type == CXLPacketType::CXLcacheH2DReq) {
        for (const auto& p: dests) {
            Tick latency = p->sendAtomicSnoop(pkt);
            fanout++;

            if (pkt->isResponse()) {
                assert(pkt->cmd != orig_cmd);
                assert(pkt->cacheResponding());
                // should only happen once
                assert(snoop_response_cmd == MemCmd::InvalidCmd);
                // save response state
                snoop_response_cmd = pkt->cmd;
                snoop_response_latency = latency;

                // pkt->cmd = orig_cmd;
            }
            else {
                continue;
            }
        }
        return std::make_pair(snoop_response_cmd, snoop_response_latency);
    }

    // snoops should only happen if the system isn't bypassing caches
    assert(!system->bypassCaches());

    for (const auto& p: dests) {
        // we could have gotten this request from a snooping memory-side port
        // (corresponding to our own CPU-side port that is also in
        // snoopPorts) and should not send it back to where it came
        // from
        if (exclude_cpu_side_port_id != InvalidPortID &&
            p->getId() == exclude_cpu_side_port_id)
            continue;

        Tick latency = p->sendAtomicSnoop(pkt);
        fanout++;

        // in contrast to a functional access, we have to keep on
        // going as all snoopers must be updated even if we get a
        // response
        if (!pkt->isResponse())
            continue;

        // response from snoop agent
        assert(pkt->cmd != orig_cmd);
        assert(pkt->cacheResponding());
        // should only happen once
        assert(snoop_response_cmd == MemCmd::InvalidCmd);
        // save response state
        snoop_response_cmd = pkt->cmd;
        snoop_response_latency = latency;

        if (snoopFilter) {
            // Handle responses by the snoopers and differentiate between
            // responses to requests from above and snoops from below
            if (source_mem_side_port_id != InvalidPortID) {
                // Getting a response for a snoop from below
                assert(exclude_cpu_side_port_id == InvalidPortID);
                snoopFilter->updateSnoopForward(pkt, *p,
                             *memSidePorts[source_mem_side_port_id]);
            } else {
                // Getting a response for a request from above
                assert(source_mem_side_port_id == InvalidPortID);
                snoopFilter->updateSnoopResponse(pkt, *p,
                             *cpuSidePorts[exclude_cpu_side_port_id]);
            }
        }
        // restore original packet state for remaining snoopers
        pkt->cmd = orig_cmd;
    }

    // Stats for fanout
    snoopFanout.sample(fanout);

    // the packet is restored as part of the loop and any potential
    // snoop response is part of the returned pair
    return std::make_pair(snoop_response_cmd, snoop_response_latency);
}

bool
CXLXBar::recvTimingResp(PacketPtr pkt, PortID mem_side_port_id)
{
    // determine the source port based on the id
    RequestPort *src_port = memSidePorts[mem_side_port_id];

    // determine the destination
    const auto route_lookup = routeTo.find(pkt->req);

    if(pkt->getAddr() == 0xfac00000)
    {
        DPRINTF(CXLXBar, "%s: src %s packet %s\n", __func__,
            src_port->name(), pkt->print());
    }


    assert(route_lookup != routeTo.end());
    const PortID cpu_side_port_id = route_lookup->second;
    assert(cpu_side_port_id != InvalidPortID);
    assert(cpu_side_port_id < respLayers.size());


    // test if the crossbar should be considered occupied for the
    // current port
    if (!respLayers[cpu_side_port_id]->tryTiming(src_port)) {
        DPRINTF(CXLXBar, "%s: src %s packet %s BUSY\n", __func__,
                src_port->name(), pkt->print());
        return false;
    }


    DPRINTF(CXLXBar, "%s: src %s packet %s\n", __func__,
            src_port->name(), pkt->print());

    // store size and command as they might be modified when
    // forwarding the packet
    unsigned int pkt_size = pkt->hasData() ? pkt->getSize() : 0;
    unsigned int pkt_cmd = pkt->cmdToIndex();

    // a response sees the response latency
    Tick xbar_delay = responseLatency * clockPeriod();

    // set the packet header and payload delay
    calcPacketTiming(pkt, xbar_delay);

    // determine how long to be crossbar layer is busy
    Tick packetFinishTime = clockEdge(headerLatency) + pkt->payloadDelay;

    if (snoopFilter && !system->bypassCaches()) {
        // let the snoop filter inspect the response and update its state
        snoopFilter->updateResponse(pkt, *cpuSidePorts[cpu_side_port_id]);
    }


    // send the packet through the destination CPU-side port and pay for
    // any outstanding header delay
    Tick latency = pkt->headerDelay;
    pkt->headerDelay = 0;
    cpuSidePorts[cpu_side_port_id]->schedTimingResp(pkt, curTick()
                                        + latency);

    // remove the request from the routing table
    if (pkt->getAddr() >= 0x80000000)
        DPRINTF(CXLXBar, "CXLXBar::recvTimingResp Erase routeTO pkt=%s is_cxl=%d\n", pkt->print(), pkt->req->cxl_packet_type);
    routeTo.erase(route_lookup);

    respLayers[cpu_side_port_id]->succeededTiming(packetFinishTime);

    // stats updates
    pktCount[cpu_side_port_id][mem_side_port_id]++;
    pktSize[cpu_side_port_id][mem_side_port_id] += pkt_size;
    transDist[pkt_cmd]++;

    return true;
}


bool
CXLXBar::recvTimingSnoopResp(PacketPtr pkt, PortID cpu_side_port_id)
{
    // determine the source port based on the id
    ResponsePort* src_port = cpuSidePorts[cpu_side_port_id];

    if(pkt->getAddr() == 0xfac00000)
    {
        DPRINTF(CXLXBar, "%s: src %s packet %s\n", __func__,
            src_port->name(), pkt->print());
    }
    

    // get the destination
    const auto route_lookup = routeTo.find(pkt->req);
    assert(route_lookup != routeTo.end());
    const PortID dest_port_id = route_lookup->second;
    assert(dest_port_id != InvalidPortID);

    // determine if the response is from a snoop request we
    // created as the result of a normal request (in which case it
    // should be in the outstandingSnoop), or if we merely forwarded
    // someone else's snoop request
    const bool forwardAsSnoop = outstandingSnoop.find(pkt->req) ==
        outstandingSnoop.end();

    // test if the crossbar should be considered occupied for the
    // current port, note that the check is bypassed if the response
    // is being passed on as a normal response since this is occupying
    // the response layer rather than the snoop response layer
    if (forwardAsSnoop) {
        assert(dest_port_id < snoopLayers.size());
        if (!snoopLayers[dest_port_id]->tryTiming(src_port)) {
            DPRINTF(CXLXBar, "%s: src %s packet %s BUSY\n", __func__,
                    src_port->name(), pkt->print());
            return false;
        }
    } else {
        // get the memory-side port that mirrors this CPU-side port internally
        RequestPort* snoop_port = snoopRespPorts[cpu_side_port_id];
        assert(dest_port_id < respLayers.size());
        if (!respLayers[dest_port_id]->tryTiming(snoop_port)) {
            DPRINTF(CXLXBar, "%s: src %s packet %s BUSY\n", __func__,
                    snoop_port->name(), pkt->print());
            return false;
        }
    }

    DPRINTF(CXLXBar, "%s: src %s packet %s\n", __func__,
            src_port->name(), pkt->print());

    // store size and command as they might be modified when
    // forwarding the packet
    unsigned int pkt_size = pkt->hasData() ? pkt->getSize() : 0;
    unsigned int pkt_cmd = pkt->cmdToIndex();

    // responses are never express snoops
    assert(!pkt->isExpressSnoop());

    // a snoop response sees the snoop response latency, and if it is
    // forwarded as a normal response, the response latency
    Tick xbar_delay =
        (forwardAsSnoop ? snoopResponseLatency : responseLatency) *
        clockPeriod();

    // set the packet header and payload delay
    calcPacketTiming(pkt, xbar_delay);

    // determine how long to be crossbar layer is busy
    Tick packetFinishTime = clockEdge(headerLatency) + pkt->payloadDelay;

    // forward it either as a snoop response or a normal response
    if (forwardAsSnoop) {
        // this is a snoop response to a snoop request we forwarded,
        // e.g. coming from the L1 and going to the L2, and it should
        // be forwarded as a snoop response

        if (snoopFilter) {
            // update the probe filter so that it can properly track the line
            snoopFilter->updateSnoopForward(pkt,
                            *cpuSidePorts[cpu_side_port_id],
                            *memSidePorts[dest_port_id]);
        }

        [[maybe_unused]] bool success =
            memSidePorts[dest_port_id]->sendTimingSnoopResp(pkt);
        pktCount[cpu_side_port_id][dest_port_id]++;
        pktSize[cpu_side_port_id][dest_port_id] += pkt_size;
        assert(success);

        snoopLayers[dest_port_id]->succeededTiming(packetFinishTime);
    } else {
        // we got a snoop response on one of our CPU-side ports,
        // i.e. from a coherent requestor connected to the crossbar, and
        // since we created the snoop request as part of recvTiming,
        // this should now be a normal response again
        outstandingSnoop.erase(pkt->req);

        // this is a snoop response from a coherent requestor, hence it
        // should never go back to where the snoop response came from,
        // but instead to where the original request came from
        assert(cpu_side_port_id != dest_port_id);

        if (snoopFilter) {
            // update the probe filter so that it can properly track
            // the line
            snoopFilter->updateSnoopResponse(pkt,
                        *cpuSidePorts[cpu_side_port_id],
                        *cpuSidePorts[dest_port_id]);
        }

        DPRINTF(CXLXBar, "%s: src %s packet %s FWD RESP\n", __func__,
                src_port->name(), pkt->print());

        // as a normal response, it should go back to a requestor through
        // one of our CPU-side ports, we also pay for any outstanding
        // header latency
        Tick latency = pkt->headerDelay;
        pkt->headerDelay = 0;
        cpuSidePorts[dest_port_id]->schedTimingResp(pkt,
                                    curTick() + latency);

        respLayers[dest_port_id]->succeededTiming(packetFinishTime);
    }

    // remove the request from the routing table
    if (pkt->getAddr() >= 0x80000000)
        DPRINTF(CXLXBar, "CXLXBar::recvTimingResp Erase routeTO pkt=%s is_cxl=%d\n", pkt->print(), pkt->req->cxl_packet_type);
    routeTo.erase(route_lookup);

    // stats updates
    transDist[pkt_cmd]++;
    snoops++;
    snoopTraffic += pkt_size;

    return true;
}






// Port &
// CXLXBar::getPort(const std::string &if_name, PortID idx)
// {
//     if (if_name == "cxlmem_side_ports" && idx < memSidePorts.size()) {
//         // the memory-side ports index translates directly to the vector
//         // position
//         return *memSidePorts[idx];
//     } else  if (if_name == "cxldefault") {
//         return *memSidePorts[defaultPortID];
//     } else if (if_name == "cxlcpu_side_ports" && idx < cpuSidePorts.size()) {
//         // the CPU-side ports index translates directly to the vector position
//         return *cpuSidePorts[idx];
//     } else {
//         return ClockedObject::getPort(if_name, idx);
//     }
// }


}


