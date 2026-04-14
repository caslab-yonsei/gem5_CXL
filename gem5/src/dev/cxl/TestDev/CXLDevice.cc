#include "dev/pci/device.hh"
#include "params/CXLDevice.hh"
#include "dev/cxl/TestDev/CXLDevice.hh"
#include "debug/CXLDevice.hh"
#include "debug/DCOH.hh"
#include "debug/AddrRanges.hh"
#include "mem/packet.hh"
#include "mem/request.hh"
#include "mem/cxl/cxl_packet.hh"
#include "mem/cxl/cxl_cache_packet.hh"
#include "dev/dma_device.hh"


namespace gem5 {



// bool
// CXLPioPort<CXLDevice>::recvTimingReq(PacketPtr pkt)
// {
//     return device->recvTimingReq(pkt);
// }

// AddrRangeList
// CXLPioPort::getAddrRanges() const
// {
//     AddrRangeList ranges;
//     ranges.push_back(device->getAddrRanges());
//     ranges.push_back(RangeSize((Addr)0x16000000L, (Addr)0x1000000));
//     return ranges;
// }

// CoherentPort::CoherentPort(CXLDevice *dev)
// : RequestPort(dev->name() + ".in_port", dev), device(dev)
// {}

InternalPort::InternalPort(CXLDevice *dev)
: RequestPort(dev->name() + ".in_port", dev), device(dev)
{}

Tick
InternalPort::memAction(PacketPtr pkt)
{
    DPRINTF(CXLDevice, "(CXL_Debug) InternalPort::memAction call\n");
    DPRINTF(CXLDevice, "(CXL_Debug) pkt_addr=%llx, is_req=%d\n",pkt->getAddr(), pkt->isRequest());
    DPRINTF(CXLDevice, "(CXL_Debug) Access CXL Memory!\n");

    if (device->sys->isAtomicMode()) {
        Tick latency = sendAtomic(pkt);

        return latency;
    }
    else {
        panic("Is AtomicMode but call memAction\n");
    }

}

bool
InternalPort::memActionTiming(PacketPtr pkt, Tick latency)
{
    DPRINTF(CXLDevice, "(CXL_Debug) InternalPort::memActionTiming call\n");
    DPRINTF(CXLDevice, "(CXL_Debug) pkt_addr=%llx, is_req=%d\n",pkt->getAddr(), pkt->isRequest());
    DPRINTF(CXLDevice, "(CXL_Debug) Access CXL Memory!\n");

    if (device->sys->isTimingMode()) {
        if (pkt->req->cxl_packet_type != CXLPacketType::HostBiasCheck)
            pkt->req->cxl_packet_type = CXLPacketType::CXLcacheH2DResp;
        bool success = sendTimingReq(pkt);

        return success;
    }
    else {
        panic("Is TimingMode but call memActionTiming\n");
    }
}

bool
InternalPort::recvTimingResp(PacketPtr pkt)
{
    DPRINTF(CXLDevice, "(CXL_Debug) InternalPort::recvTimingResp call\n");
    return device->recvTimingResp(pkt);
} 

// AddrRangeList
// InternalPort::getAddrRanges() const
// {
//     AddrRangeList ranges;
//     ranges = device->getAddrRanges();
//     ranges.pop_back();
//     ranges.push_back(RangeSize((Addr)0x16000000L, (Addr)0x1000000));
//     DPRINTF(CXLDevice, "(CXL_Debug) SSSSSSSSS InternalPort_cxl::getAddrRanges()\n");
//     for (const auto &range : ranges) {
//         DPRINTF(CXLDevice, "Range: Start = 0x%lx, End = 0x%lx\n",
//                range.start(), range.end());
//     }
//     return ranges;
// }

ExternalPort_cxl::ExternalPort_cxl(CXLDevice *dev, CXLDevice &d)
: QueuedResponsePort(dev->name() + ".ex_port", queue), queue(d, *this), device(dev)
{}

Tick
ExternalPort_cxl::recvAtomic(PacketPtr pkt)
{
    DPRINTF(CXLDevice, "(CXL_Debug) ExternalPort_cxl::recvAtomic from CXLProc pkt->cmd = %s\n",pkt->cmdString());
    // if (pkt->cmd == MemCmd::WriteReq) {
    //     DPRINTF(CXLDevice, "(CXL_Debug) ExternalPort_cxl::recvAtomic from CXLProc pkt->cmd = MemCmd::WriteReq\n");
    // }
    // else if (pkt->cmd == MemCmd::ReadReq) {
    //     DPRINTF(CXLDevice, "(CXL_Debug) ExternalPort_cxl::recvAtomic from CXLProc pkt->cmd = MemCmd::ReadReq\n");
    // }
    // else {
    //     fatal("ExternalPort_cxl::recvAtomic unknown cmd!!\n");
    // }
    device->hostSend(pkt);
    return 0;
}

bool
ExternalPort_cxl::recvTimingReq(PacketPtr pkt)
{
    return device->hostSendTiming(pkt);
};

Tick
ExternalPort_cxl::recvBiasAtomic(Addr addr, Addr originAddr, int* bias_mode, bool get_set)
{
    DPRINTF(CXLDevice, "call ExternalPort_cxl::recvBiasAtomic\n");

    if (get_set == 0) {
        if ((device->dcoh).is_host_bias_mode(originAddr) == true) {
            *bias_mode = 1;
        }
        else {
            *bias_mode = 2;
        }
    }
    else {
        if (*bias_mode == 1) {
            (device->dcoh).set_bias_mode(originAddr, true);
        }
        else if (*bias_mode == 2) {
            (device->dcoh).set_bias_mode(originAddr, false);
        }
        else {
            panic("undefined bias_mode!!\n");
        }
        
    }

    return 1000;
}

AddrRangeList
ExternalPort_cxl::getAddrRanges() const
{
    DPRINTF(AddrRanges, "CXLDevice.ex_port connected, calling getAddrRanges\n");
    AddrRangeList ranges;
    ranges = device->getAddrRanges();
    ranges.pop_back();
    // ranges.push_back(RangeSize((Addr)0x80000000L, (Addr)0x40000000L));
    ranges.push_back(RangeSize((Addr)0xA0000000L, (Addr)0x20000000L));
    DPRINTF(CXLDevice, "(CXL_Debug) SSSSSSSSS ExternalPort_cxl::getAddrRanges()\n");
    for (const auto &range : ranges) {
        DPRINTF(CXLDevice, "Range: Start = 0x%lx, End = 0x%lx\n",
               range.start(), range.end());
    }
    return ranges;
}



CXLDevice::CXLDevice(const CXLDeviceParams &p)
: PcieDevice(p), pioAddr(p.pio_addr), sys(p.system), 
    pioSize(p.pio_size), pioDelay(p.pio_delay), 
    sendTestEvent([this]{ send_host(); }, name()),
    FinishReqEvent([this]{ finish_req(); }, name()),
    DataSendEvent([this]{ host_data(); }, name()),
    FinishDataEvent([this]{ finish_data(); }, name()),
    waitEvent([this]{ wait_cxl(); }, name()),
    Test_value(123), in_port(this), ex_port(this, *this), is_cxl(1), dcoh(this)
{
    // pioPort = CXLPioPort<CXLDevice>(this);
    pioPort.is_cxl_dev = true;
    Test_value2 = (uint8_t*)(malloc(1024));
    is_cxl_dev = true;
    for (int i = 0; i < 1024; i++) {
        Test_value2[i] = 1;
    }
}

void
CXLDevice::init()
{
    PcieDevice::init();

    if (!in_port.isConnected())
        panic("in_port of %s not connected to anything!", name());
    if (!ex_port.isConnected())
        panic("ex_port of %s not connected to anything!", name());
    ex_port.sendRangeChange();
    DPRINTF(CXLDevice, "(CXL_Debug) Init CXLDevice\n");
    // schedule(sendTestEvent, 1355698371L+clockEdge(Cycles(1)));
}

void
CXLDevice::wait_cxl()
{}

void
CXLDevice::finish_data()
{
    schedule(waitEvent, 100000L+clockEdge(Cycles(1)));
    DPRINTF(CXLDevice, "(CXL_Debug) CXL data process done!!\n");
}

Tick
CXLDevice::read(PacketPtr pkt)
{
    Tick latency = callDCOH(pkt);
    return latency;
}

Tick
CXLDevice::write(PacketPtr pkt)
{
    Tick latency = callDCOH(pkt);
    return latency;
}

Tick
CXLDevice::callDCOH(PacketPtr pkt)
{
    if (sys->isTimingMode()) {
        if (pkt->req->cxl_packet_type == CXLPacketType::HostBiasCheck) {
            return dcoh.recvCoherentCheckReq(pkt);
        }
        return dcoh.recvTimingReq(pkt);
    }
    else if (sys->isAtomicMode()) {
        return dcoh.recvReq(pkt);
    }
}

Tick
CXLDevice::ReadData(PacketPtr pkt)
{
    DPRINTF(CXLDevice, "(CXL_Debug) CXLDevice Read Call\n");
    DPRINTF(CXLDevice, "(CXL_Debug) pkt Addr=%llx\n", pkt->getAddr());
    // if ((pkt->getAddr() >= 0x16000000L) && (pkt->getAddr() < 0x17000000L)) {
    if ((pkt->getAddr() >= 0x80000000L) && (pkt->getAddr() < 0xA0000000L)) {
        DPRINTF(CXLDevice, "(CXL_Debug) read req's dest is CXLMEM\n");
        Tick latency = in_port.memAction(pkt);
        return latency;
    }  
    else if (pkt->req->cxl_packet_type == CXLPacketType::CXLcacheH2DReq) {
        DPRINTF(CXLDevice, "(CXL_Debug) snoop req's dest is CXL XBar\n");
        Tick latency = in_port.memAction(pkt);
        return latency;
    }
    else {
        pkt->makeAtomicResponse();
        return 5780;
    }
}

Tick
CXLDevice::WriteData(PacketPtr pkt)
{
    DPRINTF(CXLDevice, "(CXL_Debug) CXLDevice Write Call\n");
    DPRINTF(CXLDevice, "(CXL_Debug) pkt Addr=%llx\n", pkt->getAddr());

    // if ((pkt->getAddr() >= 0x16000000L) && (pkt->getAddr() < 0x17000000L)) {
    if ((pkt->getAddr() >= 0x80000000L) && (pkt->getAddr() < 0xA0000000L)) {
        DPRINTF(CXLDevice, "(CXL_Debug) write req's dest is CXLMEM\n");
        Tick latency = in_port.memAction(pkt);
        return latency;
    }
    else {
        panic("ReadData: incorrect pkt\n");
    }
}

bool
CXLDevice::ReadDataTiming(PacketPtr pkt, Tick latency)
{
    DPRINTF(CXLDevice, "(CXL_Debug) CXLDevice Read Call\n");
    DPRINTF(CXLDevice, "(CXL_Debug) pkt Addr=%llx\n", pkt->getAddr());
    // if ((pkt->getAddr() >= 0x16000000L) && (pkt->getAddr() < 0x17000000L)) {
    if ((pkt->getAddr() >= 0x80000000L) && (pkt->getAddr() < 0xA0000000L)) {
        DPRINTF(CXLDevice, "(CXL_Debug) read req's dest is CXLMEM\n");
        bool success = in_port.memActionTiming(pkt, latency);
        return success;
    }  
    else if (pkt->req->cxl_packet_type == CXLPacketType::CXLcacheH2DReq) {
        DPRINTF(CXLDevice, "(CXL_Debug) snoop req's dest is CXL XBar\n");
        bool success = in_port.memActionTiming(pkt, latency);
        return success;
    }
    else {
        panic("ReadDataTiming: incorrect pkt\n");
    }
}

bool
CXLDevice::WriteDataTiming(PacketPtr pkt, Tick latency)
{
    DPRINTF(CXLDevice, "(CXL_Debug) CXLDevice Write Call\n");
    DPRINTF(CXLDevice, "(CXL_Debug) pkt Addr=%llx\n", pkt->getAddr());
    // if ((pkt->getAddr() >= 0x16000000L) && (pkt->getAddr() < 0x17000000L)) {
    if ((pkt->getAddr() >= 0x80000000L) && (pkt->getAddr() < 0xA0000000L)) {
        DPRINTF(CXLDevice, "(CXL_Debug) write req's dest is CXLMEM\n");
        bool success = in_port.memActionTiming(pkt, latency);
        return success;
    }  
    else if (pkt->req->cxl_packet_type == CXLPacketType::CXLcacheH2DReq) {
        DPRINTF(CXLDevice, "(CXL_Debug) snoop req's dest is CXL XBar\n");
        bool success = in_port.memActionTiming(pkt, latency);
        return success;
    }
    else {
        panic("ReadDataTiming: incorrect pkt\n");
    }
}

Tick
CXLDevice::LoadData(PacketPtr pkt)
{
    DPRINTF(CXLDevice, "(CXL_Debug) CXLDevice loadData Call\n");
    DPRINTF(CXLDevice, "(CXL_Debug) pkt Addr=%llx\n", pkt->getAddr());
    pkt->makeAtomicResponse();

    return 0;
}

//
Tick
CXLDevice::SnpProcess(PacketPtr pkt)
{
    DPRINTF(CXLDevice, "(CXL_Debug) CXLDevice::SnpProcess Call\n");
}

void
CXLDevice::hostSend(PacketPtr pkt)
{
    DPRINTF(CXLDevice, "(CXL_Debug) CXLDevice hostSend Call\n");
    if (pkt->req->cxl_packet_type != CXLPacketType::CXLcacheH2DReq)
        makeRequest(pkt);
    dmaPort.sendAtomic(pkt);

    if (pkt->req->cxl_packet_type == CXLPacketType::HostBiasCheck) {
        cxl_packet::m2s::M2S_SnpTypes req_snoop_type;
        cxl_packet::MetaValue req_meta_value;
        cxl_packet::MetaFiled req_meta_field;
        cxl_packet::m2s::M2S_Req_OpCodes op;

        op = ((cxl_packet::m2s::M2S_Req *)(pkt->req->cxl_req))->memOpcode;
        req_snoop_type = ((cxl_packet::m2s::M2S_Req *)(pkt->req->cxl_req))->snp_type;
        req_meta_value = ((cxl_packet::m2s::M2S_Req *)(pkt->req->cxl_req))->meta_value;
        req_meta_field = ((cxl_packet::m2s::M2S_Req *)(pkt->req->cxl_req))->meta_filed;

        if ((op == cxl_packet::m2s::M2S_Req_OpCodes::MemRdFwd) || (op == cxl_packet::m2s::M2S_Req_OpCodes::MemWrFwd)) {
            DPRINTF(CXLDevice, "(CXL_Debug) CXLDevice: No Response MemRdFwd/MemWrFwd!! -> well operate internal read/write\n");
            
            if (req_meta_field == cxl_packet::MetaFiled::Meta0_State) {
                if (req_meta_value == cxl_packet::MetaValue::Any) {
                    dcoh.set_bias_mode(pkt->getAddr(), 1);
                }
                else if (req_meta_value == cxl_packet::MetaValue::Shared) {
                    dcoh.set_bias_mode(pkt->getAddr(), 1);
                }
                else if (req_meta_value == cxl_packet::MetaValue::Invalied) {
                    dcoh.set_bias_mode(pkt->getAddr(), 0);
                }
            }
        }
    }



    // else {
    //     cxl_cache_packet::CXL_Cache_Types protocol_type = pkt->req->cxl_cache->cxl_cache_type;
    //     if (protocol_type == cxl_cache_packet::CXL_Cache_Types::_H2D_Resp) {
    //         cxl_cache_packet::h2d::H2D_Resp_Opcodes op;
    //         cxl_cache_packet::h2d::RSP_Data_Encoding rsp_data;

    //         op = ((cxl_cache_packet::h2d::H2D_Resp *)(pkt->req->cxl_cache))->opcode;
    //         rsp_data = ((cxl_cache_packet::h2d::H2D_Resp *)(pkt->req->cxl_cache))->RspData;

    //         if ((rsp_data == cxl_cache_packet::h2d::RSP_Data_Encoding::Exclusive) || (rsp_data == cxl_cache_packet::h2d::RSP_Data_Encoding::Modified)) {
    //             dcoh.set_bias_mode(pkt->getAddr(), 0);
    //         }
    //         else {
    //             dcoh.set_bias_mode(pkt->getAddr(), 1);
    //         }
    //     }

    // }

}

bool
CXLDevice::hostSendTiming(PacketPtr pkt)
{
    DPRINTF(CXLDevice, "(CXL_Debug) CXLDevice hostSendTiming Call\n");
    if (pkt->req->cxl_packet_type == CXLPacketType::CXLcacheH2DResp) {
        dcoh.makeSnoopResp(pkt);
    }
    else {
        makeRequest(pkt);
    }    
    bool success = dmaPort.sendTimingReq(pkt);

    return success;


    // if (pkt->req->cxl_packet_type == CXLPacketType::HostBiasCheck) {
    //     cxl_packet::m2s::M2S_SnpTypes req_snoop_type;
    //     cxl_packet::MetaValue req_meta_value;
    //     cxl_packet::MetaFiled req_meta_field;
    //     cxl_packet::m2s::M2S_Req_OpCodes op;

    //     op = ((cxl_packet::m2s::M2S_Req *)(pkt->req->cxl_req))->memOpcode;
    //     req_snoop_type = ((cxl_packet::m2s::M2S_Req *)(pkt->req->cxl_req))->snp_type;
    //     req_meta_value = ((cxl_packet::m2s::M2S_Req *)(pkt->req->cxl_req))->meta_value;
    //     req_meta_field = ((cxl_packet::m2s::M2S_Req *)(pkt->req->cxl_req))->meta_filed;

    //     if ((op == cxl_packet::m2s::M2S_Req_OpCodes::MemRdFwd) || (op == cxl_packet::m2s::M2S_Req_OpCodes::MemWrFwd)) {
    //         DPRINTF(CXLDevice, "(CXL_Debug) CXLDevice: No Response MemRdFwd/MemWrFwd!! -> well operate internal read/write\n");
            
    //         if (req_meta_field == cxl_packet::MetaFiled::Meta0_State) {
    //             if (req_meta_value == cxl_packet::MetaValue::Any) {
    //                 dcoh.set_bias_mode(pkt->getAddr(), 1);
    //             }
    //             else if (req_meta_value == cxl_packet::MetaValue::Shared) {
    //                 dcoh.set_bias_mode(pkt->getAddr(), 1);
    //             }
    //             else if (req_meta_value == cxl_packet::MetaValue::Invalied) {
    //                 dcoh.set_bias_mode(pkt->getAddr(), 0);
    //             }
    //         }
    //     }
    // }

}

void
CXLDevice::hostWrite(Addr addr, int size, Event *event, uint8_t *data, Tick delay)
{
    DPRINTF(CXLDevice, "(CXL_Debug) CXLDevice hostWrite Call\n");
    // 패킷 수정을 어디서???
    // dmaWrite(addr, size, event, data);
    DPRINTF(CXLDevice, "event= %llx\n", event);
    dmaPort.dmaAction(MemCmd::WriteReq, addr, size, event, data, dmaPort.defaultSid, dmaPort.defaultSSid, 0, 1);

}

void
CXLDevice::hostRead(Addr addr, int size, Event *event, uint8_t *data, Tick delay)
{
    DPRINTF(CXLDevice, "(CXL_Debug) CXLDevice hostRead Call\n");
    // dmaRead(addr, size, event, data);
    dmaPort.dmaAction(MemCmd::ReadReq, addr, size, event, data, dmaPort.defaultSid, dmaPort.defaultSSid, delay, 0, 1);
}

void
CXLDevice::host_data()
{
    Addr _addr = dcoh.data_dest_addr;
    // int _size = 4;
    int _size = 1024;

    DPRINTF(CXLDevice, "(CXL_Debug) CXLDevice send write req to host(0x90000000)\n");
    // hostData(_addr, _size, &FinishDataEvent, &Test_value);
    hostData(_addr, _size, &FinishDataEvent, Test_value2);
}

void
CXLDevice::hostData(Addr addr, int size, Event *event, uint8_t *data, Tick delay)
{
    DPRINTF(CXLDevice, "(CXL_Debug) CXLDevice hostData Call\n");
    dmaPort.dmaAction(MemCmd::WriteReq, addr, size, event, data, dmaPort.defaultSid, dmaPort.defaultSSid, delay, 0, 1);
}

void
CXLDevice::send_host()
{
    Addr _addr = 0x90000000L;
    int _size = 4;
    
    DPRINTF(CXLDevice, "(CXL_Debug) CXLDevice send write req to host(0x90000000)\n");
    DPRINTF(CXLDevice, "event= %llx\n", &FinishReqEvent);
    // hostRead(_addr, _size, &FinishReqEvent, &Test_value);
    
    hostWrite(_addr, _size, &FinishReqEvent, &Test_value);
}

void
CXLDevice::finish_req()
{
    DPRINTF(CXLDevice, "(CXL_Debug) CXLDevice recieve resp from host\n");
    if (dcoh.need_data) {
        schedule(DataSendEvent, 100000L+clockEdge(Cycles(1)));
    }

}

// void
// CXLDevice::processResponse(PacketPtr pkt) {
//     DPRINTF(DCOH, "(CXL_Debug) CXLDevice::processResponse call\n");
//     ex_port.schedTimingResp(pkt);
// }

void
CXLDevice::makeRequest(PacketPtr pkt)
{
    dcoh.makeCXLrequest(pkt);
}

void
CXLDevice::makeDataPkt(PacketPtr pkt, bool lastpkt)
{
    dcoh.makeCXLData(pkt, lastpkt);
}

void
CXLDevice::ReservedData(PacketPtr pkt)
{
    DPRINTF(DCOH, "(CXL_Debug) CXLDevice::ReservedData call\n");
    bool writepull = false;
    cxl_cache_packet::h2d::H2D_Resp_Opcodes op;
    op = ((cxl_cache_packet::h2d::H2D_Resp *)(pkt->req->cxl_cache))->opcode;
    if (op == cxl_cache_packet::h2d::H2D_Resp_Opcodes::Fast_GO_writePull)
        writepull = true;
    else if (op == cxl_cache_packet::h2d::H2D_Resp_Opcodes::GO_WritePull)
        writepull = true;
    else if (op == cxl_cache_packet::h2d::H2D_Resp_Opcodes::GO_WritePull_Drop)
        writepull = true;
    else if (op == cxl_cache_packet::h2d::H2D_Resp_Opcodes::WritePull)
        writepull = true;

    if (writepull){
        DPRINTF(DCOH, "(CXL_Debug) CXLDevice::ReservedData need_data true setting\n");
        dcoh.need_data = true;
        dcoh.data_dest_addr = pkt->getAddr();
    }
    else {
        DPRINTF(DCOH, "(CXL_Debug) CXLDevice::ReservedData need_data NO set\n");
    }
}

AddrRangeList
CXLDevice::getAddrRanges() const
{
    assert(pioSize != 0);
    AddrRangeList ranges;
    DPRINTF(DCOH, "(CXL_Debug) CXLDevice getAddrRanges call\n");

    //장치 자체 주소
    ranges.push_back(RangeSize((Addr)0x15000000L, (Addr)0x40000));

    // CXL mem 주소
    // ranges.push_back(RangeSize((Addr)0x16000000L, (Addr)0x1000000));
    ranges.push_back(RangeSize((Addr)0x80000000L, (Addr)0x20000000));
    DPRINTF(CXLDevice, "(CXL_Debug) SSSSSSSSS CXLDevice::getAddrRanges()\n");
    for (const auto &range : ranges) {
        DPRINTF(CXLDevice, "Range: Start = 0x%lx, End = 0x%lx\n",
               range.start(), range.end());
    }
    return ranges;
}

CXLDevice::DCOH::DCOH(CXLDevice* parent_ptr)
: parent(parent_ptr), _name(parent_ptr->name()+".dcoh"), need_data(false)
{}

// Tick
// CXLDevice::DCOH::sendReq(PacketPtr pkt) {
//     DPRINTF(DCOH, "(CXL_Debug) DCOH::sendReq Recieve Req from CXLTestMem \n");
//     DPRINTF(DCOH, "(CXL_Debug) pkt Addr=%llx\n", pkt->getAddr());

//     assert(pkt->req->cxl_cache == nullptr);

//     makeCXLrequest(pkt);
//     Tick latency = RequestOutsidePort.ReqSend(pkt);

//     assert(pkt->req->cxl_cache != nullptr);
    
//     return latency;
// }

Tick
CXLDevice::DCOH::recvReq(PacketPtr pkt) {
    DPRINTF(DCOH, "(CXL_Debug) CXLDevice::DCOH::recvReq call\n");
    Tick delay = 0;
    if (pkt->req->cxl_packet_type == CXLPacketType::CXLcacheH2DReq) {
        DPRINTF(DCOH, "(CXL_Debug) DCOH::recvReq Recieve snoop Request snnoop_pkt=%s\n", pkt->print());
        if (pkt->req->cxl_cache != nullptr)
            DPRINTF(DCOH, "(CXL_Debug) DCOH::recvReq Snoop Request Packet \n%s\n", pkt->req->cxl_cache->print());
        delay =
            pkt->isRead() ? parent->ReadData(pkt) : parent->WriteData(pkt);

        makeSnoopResp(pkt);

        return delay;
    }

    if (pkt->req->cxl_req != nullptr)
        DPRINTF(DCOH, "(CXL_Debug) DCOH::recvReq Request Packet \n%s\n", pkt->req->cxl_req->print());
    
    assert(pkt->req->cxl_req != nullptr);

    makeCXLresponse(pkt);

    delay =
        pkt->isRead() ? parent->ReadData(pkt) : parent->WriteData(pkt);

    // makeCXLresponse(pkt);

    if (pkt->req->cxl_req->last_resp == 0) {

        // if (sys->isAtomicMode()) {
            makeMemDataAtomic(pkt);
        // }
        assert(pkt->req->cxl_data_atomic != nullptr);
    }

    return delay;
}

void
CXLDevice::DCOH::makeCXLrequest(PacketPtr pkt)
{
    DPRINTF(DCOH, "(CXL_Debug) DCOH::makeCXLrequest call\n");

    // -------- test case (Read) -----------
    bool ReadCur = 0;
    bool ReadShared = 1;
    bool ReadAny = 1;
    bool ReadOwn = 1;
    bool CLFlush = 1;
    bool ReadOwnNoData = 0;
    // -------------------------------------

    // -------- test case (Write) -----------
    bool WOWr = 1;
    // --------------------------------------

    cxl_cache_packet::d2h::D2H_Req_OpCodes req_opcode;

    if (pkt->cmd == MemCmd::ReadExReq) {
        DPRINTF(DCOH, "(CXL_Debug) make cxl req %s -> RdOwn\n", pkt->print());
        if (ReadOwn) {
            req_opcode = cxl_cache_packet::d2h::D2H_Req_OpCodes::RdOwn;
        }
    }
    else if (pkt->cmd == MemCmd::ReadSharedReq) {
        DPRINTF(DCOH, "(CXL_Debug) make cxl req %s -> RdShared\n", pkt->print());
        if (ReadShared) {
            req_opcode = cxl_cache_packet::d2h::D2H_Req_OpCodes::RdShared;
        }
    }
    else if (pkt->cmd == MemCmd::FlushReq) {
        if (CLFlush) {
            DPRINTF(DCOH, "(CXL_Debug) make cxl req %s -> CLFlush\n", pkt->print());
            req_opcode = cxl_cache_packet::d2h::D2H_Req_OpCodes::CLFlush;
        }
    }
    else {
        if (ReadCur) {
            DPRINTF(DCOH, "(CXL_Debug) make cxl req %s -> RdCurr\n", pkt->print());
            req_opcode = cxl_cache_packet::d2h::D2H_Req_OpCodes::RdCurr;
        }
        else if (ReadAny) {
            DPRINTF(DCOH, "(CXL_Debug) make cxl req %s -> RdAny\n", pkt->print());
            req_opcode = cxl_cache_packet::d2h::D2H_Req_OpCodes::RdAny;
        }
        else if (ReadOwnNoData) {
            DPRINTF(DCOH, "(CXL_Debug) make cxl req %s -> RdOwnNoData\n", pkt->print());
            req_opcode = cxl_cache_packet::d2h::D2H_Req_OpCodes::RdOwnNoData;
        }
        else {
            fatal("DCOH::makeCXLrequest unknown cmd!!\n");
        }
    }

    cxl_cache_packet::d2h::D2H_Req* d2h_req;

    // if (pkt->req->cxl_packet_type == CXLPacketType::HostBiasCheck) {

    //     // // Process HostMode
    //     // PacketPtr coh_pkt = new Packet(pkt->req, pkt->cmd);
    //     // // CXL_Debug
    //     // if (pkt->req->cxl_req != nullptr) {
    //     //     coh_pkt->req->cxl_req = pkt->req->cxl_req;
            
    //     // }
    //     // if (pkt->req->cxl_data_atomic != nullptr) {
    //     //     coh_pkt->req->cxl_data_atomic = pkt->req->cxl_data_atomic;
            
    //     // }
    //     // if (pkt->req->cxl_cache != nullptr) {
    //     //     coh_pkt->req->cxl_cache = pkt->req->cxl_cache;
            
    //     // }
    //     // if (pkt->req->cxl_data_atomic2 != nullptr) {
    //     //     coh_pkt->req->cxl_data_atomic2 = pkt->req->cxl_data_atomic2;
            
    //     // }
    //     // coh_pkt->req->cxl_packet_type = pkt->req->cxl_packet_type;
    //     // coh_pkt->req->coherent_check_dest = pkt->req->coherent_check_dest;
    //     // coh_pkt->req->from_cxl_proc = pkt->req->from_cxl_proc;

    //     // coh_pkt->setAddr(0xa0000000)

    //     // parent->dmaPort.sendAtomic(coh_pkt);


    //     d2h_req = new cxl_cache_packet::d2h::D2H_Req(
    //         req_opcode, 1, false, 0, pkt->req->coherent_check_dest, 0, 0, 0);
    // }
    // else {
        d2h_req = new cxl_cache_packet::d2h::D2H_Req(
            req_opcode, 1, false, 0, pkt->getAddr(), 0, 0, 0);
    // }
   

    //test!!
    // d2h_req = new cxl_cache_packet::d2h::D2H_Req(
    //     req_opcode, 1, false, 0, 0x16000000L, 0, 0, 0);
    
    if (pkt->req->cxl_packet_type != CXLPacketType::HostBiasCheck) {
        pkt->req->cxl_packet_type = CXLPacketType::CXLcachePacket;
    }
    pkt->req->cxl_cache = d2h_req;
    

    //test!!
    // pkt->req->cxl_packet_type = CXLPacketType::HostBiasCheck;
    if (pkt->req->cxl_cache != nullptr)
        DPRINTF(DCOH, "(CXL_Debug) DCOH::makeCXLrequest Request Packet \n%s\n", pkt->req->cxl_cache->print());
}

void
CXLDevice::DCOH::makeCXLData(PacketPtr pkt, bool lastpkt)
{
    DPRINTF(DCOH, "(CXL_Debug) DCOH::makeCXLData call\n");
    cxl_cache_packet::d2h::D2H_Data* d2h_data;
    d2h_data = new cxl_cache_packet::d2h::D2H_Data(
            0, false, false, false, 0, 0, 0);
    pkt->req->cxl_packet_type = CXLPacketType::CXLcacheDataPacket;
    pkt->req->cxl_data_atomic2 = d2h_data;
    if (lastpkt == 0) {
        pkt->req->cxl_data_atomic2->last_data = 0;
    }
    else {
        pkt->req->cxl_data_atomic2->last_data = 1;
    }  
    if (pkt->req->cxl_data_atomic2 != nullptr)  
        DPRINTF(DCOH, "(CXL_Debug) DCOH::makeCXLData Data Packet \n%s\n", pkt->req->cxl_data_atomic2->print());
}

void
CXLDevice::DCOH::makeCXLresponse(PacketPtr pkt) {
    cxl_packet::CXL_Req_Types cxl_req_type = pkt->req->cxl_req->cxl_req_type;
    cxl_packet::m2s::M2S_SnpTypes req_snoop_type;
    cxl_packet::MetaValue req_meta_value;
    cxl_packet::MetaFiled req_meta_field;

    cxl_packet::s2m::S2M_NDR* s2m_ndr;
    cxl_packet::m2s::M2S_Req_OpCodes op;
    cxl_packet::m2s::M2S_RwD_OpCodes op2;
    
    DPRINTF(DCOH, "(CXL_Debug) DCOH::makeCXLresponse call\n");
    // DPRINTF(DCOH, "(CXL_Debug) DCOH::makeCXLresponse Request Packet \n%s\n", pkt->req->cxl_req->print());

    switch (cxl_req_type)
    {
    case cxl_packet::CXL_Req_Types::_M2S_Req:

      

      op = ((cxl_packet::m2s::M2S_Req *)(pkt->req->cxl_req))->memOpcode;
      req_snoop_type = ((cxl_packet::m2s::M2S_Req *)(pkt->req->cxl_req))->snp_type;
      req_meta_value = ((cxl_packet::m2s::M2S_Req *)(pkt->req->cxl_req))->meta_value;
      req_meta_field = ((cxl_packet::m2s::M2S_Req *)(pkt->req->cxl_req))->meta_filed;

      if ((op == cxl_packet::m2s::M2S_Req_OpCodes::MemRdFwd) || (op == cxl_packet::m2s::M2S_Req_OpCodes::MemWrFwd)) {
        DPRINTF(DCOH, "(CXL_Debug) DCOH::makeCXLresponse No Response MemRdFwd/MemWrFwd!! -> well operate internal read/write\n");
        
        // if (req_meta_field == cxl_packet::MetaFiled::Meta0_State) {
        //     if (req_meta_value == cxl_packet::MetaValue::Any) {
        //         set_bias_mode(pkt->getAddr(), 1);
        //     }
        //     else if (req_meta_value == cxl_packet::MetaValue::Shared) {
        //         set_bias_mode(pkt->getAddr(), 1);
        //     }
        //     else if (req_meta_value == cxl_packet::MetaValue::Invalied) {
        //         set_bias_mode(pkt->getAddr(), 0);
        //     }
        // }
        // delete pkt->req->cxl_req;
        return;
      }

      if ((op == cxl_packet::m2s::M2S_Req_OpCodes::MemInv) || (op == cxl_packet::m2s::M2S_Req_OpCodes::MemInvNT))
      {
        if (req_meta_field == cxl_packet::MetaFiled::No_Op) {
            if (req_snoop_type == cxl_packet::m2s::M2S_SnpTypes::SnpInv) {
                // 호스트는 장치가 캐시에서 해당 라인을 Invalidate하기를 원합니다.
                s2m_ndr = new cxl_packet::s2m::S2M_NDR(
                    cxl_packet::s2m::S2M_NDR_Opcodes::Cmp,
                    cxl_packet::MetaFiled::No_Op,
                    cxl_packet::MetaValue::Invalied,
                    0, 0, cxl_packet::s2m::DevLoad::LightLoad, 0, 0);
            }
            else {
                panic("invaild request recive device (MemInv/NT, MF:no)\n");
            }
        }
        else if (req_meta_field == cxl_packet::MetaFiled::Meta0_State) {
            if (req_snoop_type == cxl_packet::m2s::M2S_SnpTypes::SnpInv) {
                if (req_meta_value == cxl_packet::MetaValue::Invalied) {
                    // 호스트가 장치의 캐시를 포함한 모든 캐시에서 장치 메모리로 라인을 플러시
                    // 따라서 Matavalue는 00b(Invalied)이여야 하며 SnpInv이여야 함
                    // 장치는 캐시를 플러시 해야하며 Cmp를 호스트로 응답해야함
                    s2m_ndr = new cxl_packet::s2m::S2M_NDR(
                        cxl_packet::s2m::S2M_NDR_Opcodes::Cmp,
                        cxl_packet::MetaFiled::No_Op,
                        cxl_packet::MetaValue::Invalied,
                        0, 0, cxl_packet::s2m::DevLoad::LightLoad, 0, 0);
                }
                else if (req_meta_value == cxl_packet::MetaValue::Any) {
                    // 호스트가 데이터는 없어도 되지만 ownership을 가져오려고 함
                    s2m_ndr = new cxl_packet::s2m::S2M_NDR(
                        cxl_packet::s2m::S2M_NDR_Opcodes::Cmp_E,
                        cxl_packet::MetaFiled::No_Op,
                        cxl_packet::MetaValue::Any,
                        0, 0, cxl_packet::s2m::DevLoad::LightLoad, 0, 0);
                    set_bias_mode(pkt->getAddr(), 1);
                }
                else {
                    panic("invaild request recive device (MemInv/NT, MF:M0S, SnpInv)\n");
                }
            }
            else if (req_snoop_type == cxl_packet::m2s::M2S_SnpTypes::SnpData) {
                if (req_meta_value == cxl_packet::MetaValue::Shared) {
                    // 호스트는 장치가 캐시에서 S로 저하되기를 원하고, 캐시라인에 대한 공유 상태(데이터 없음)를 원합니다.
                    s2m_ndr = new cxl_packet::s2m::S2M_NDR(
                        cxl_packet::s2m::S2M_NDR_Opcodes::Cmp_S,
                        cxl_packet::MetaFiled::No_Op,
                        cxl_packet::MetaValue::Any,
                        0, 0, cxl_packet::s2m::DevLoad::LightLoad, 0, 0);
                    set_bias_mode(pkt->getAddr(), 1);
                }
                else {
                    panic("invaild request recive device (MemInv/NT, MF:M0S, SnpData)\n");
                }
            }
            else {
                panic("invaild request recive device (MemInv/NT, MF:M0S)\n");
            }
        }
        else
        {
            panic("invaild request recive device (MemInv/NT)\n");
        }
        s2m_ndr->last_resp = 1; // 추가 응답 필요 없음
        pkt->req->cxl_packet_type = CXLPacketType::CXLmemPacket;
        pkt->req->cxl_req = s2m_ndr;
      }

      else if (op == cxl_packet::m2s::M2S_Req_OpCodes::MemRd)
      {
        // 호스트로부터 읽기 요청

        if (req_meta_field == cxl_packet::MetaFiled::Meta0_State) {
            if (req_meta_value == cxl_packet::MetaValue::Any) {
                if (req_snoop_type == cxl_packet::m2s::M2S_SnpTypes::SnpInv) {
                    // 호스트는 해당 라인의 독점 사본을 원합니다.
                    s2m_ndr = new cxl_packet::s2m::S2M_NDR(
                        cxl_packet::s2m::S2M_NDR_Opcodes::Cmp_E,
                        cxl_packet::MetaFiled::No_Op,
                        cxl_packet::MetaValue::Any,
                        0, 0, cxl_packet::s2m::DevLoad::LightLoad, 0, 0);
                    set_bias_mode(pkt->getAddr(), 1);
                }
                else {
                    panic("invaild request recive device (MemRd, MF:M2S Any)\n");
                }
            }
            else if (req_meta_value == cxl_packet::MetaValue::Shared) {
                if (req_snoop_type == cxl_packet::m2s::M2S_SnpTypes::SnpData) {
                    // 호스트가 라인의 공유 사본을 요청하지만 Rsp 유형은 장치가 S 또는 E 상태를 호스트로 반환할 수 있도록 합니다.
                    // 장치가 이 상태를 요청하지 않았기 때문에 Cmp-E 응답은 권장되지 않습니다.
                    s2m_ndr = new cxl_packet::s2m::S2M_NDR(
                        cxl_packet::s2m::S2M_NDR_Opcodes::Cmp_S,
                        cxl_packet::MetaFiled::No_Op,
                        cxl_packet::MetaValue::Any,
                        0, 0, cxl_packet::s2m::DevLoad::LightLoad, 0, 0);
                    set_bias_mode(pkt->getAddr(), 1);
                }
                else {
                    panic("invaild request recive device (MemRd, MF:M2S Shared)\n");
                }
            }
            else if (req_meta_value == cxl_packet::MetaValue::Invalied) {
                if (req_snoop_type == cxl_packet::m2s::M2S_SnpTypes::SnpInv) {
                    // 호스트가 캐시할 수 없지만 현재 값을 요청하고 장치가 캐시를 플러시하도록 강제합니다.
                    s2m_ndr = new cxl_packet::s2m::S2M_NDR(
                        cxl_packet::s2m::S2M_NDR_Opcodes::Cmp,
                        cxl_packet::MetaFiled::No_Op,
                        cxl_packet::MetaValue::Any,
                        0, 0, cxl_packet::s2m::DevLoad::LightLoad, 0, 0);
                }
                else if (req_snoop_type == cxl_packet::m2s::M2S_SnpTypes::SnpCur) {
                    // 호스트가 장치의 캐시에 데이터를 남겨두고 캐시할 수 없지만 현재 값을 요청합니다.
                    s2m_ndr = new cxl_packet::s2m::S2M_NDR(
                        cxl_packet::s2m::S2M_NDR_Opcodes::Cmp,
                        cxl_packet::MetaFiled::No_Op,
                        cxl_packet::MetaValue::Any,
                        0, 0, cxl_packet::s2m::DevLoad::LightLoad, 0, 0);
                }
                else {
                    panic("invaild request recive device (MemRd, MF:M2S Invalid)\n");
                }
            }
            else {
                panic("invaild request recive device (MemRd, MF:M2S)\n");
            }
        }
        else if (req_meta_field == cxl_packet::MetaFiled::No_Op) {
            if (req_snoop_type == cxl_packet::m2s::M2S_SnpTypes::SnpInv) {
                // 호스트는 호스트 캐시에 있는 상태를 변경하지 않고 줄을 읽으려고 하며, 장치는 캐시에서 line을 무효화해야 합니다.
                s2m_ndr = new cxl_packet::s2m::S2M_NDR(
                        cxl_packet::s2m::S2M_NDR_Opcodes::Cmp,
                        cxl_packet::MetaFiled::No_Op,
                        cxl_packet::MetaValue::Any,
                        0, 0, cxl_packet::s2m::DevLoad::LightLoad, 0, 0);
            }
            else if (req_snoop_type == cxl_packet::m2s::M2S_SnpTypes::SnpCur) {
                // 호스트는 호스트 캐시에서 예상되는 상태를 변경하지 않고 line의 현재 값을 원합니다.
                s2m_ndr = new cxl_packet::s2m::S2M_NDR(
                        cxl_packet::s2m::S2M_NDR_Opcodes::Cmp,
                        cxl_packet::MetaFiled::No_Op,
                        cxl_packet::MetaValue::Any,
                        0, 0, cxl_packet::s2m::DevLoad::LightLoad, 0, 0);
            }
            else if (req_snoop_type == cxl_packet::m2s::M2S_SnpTypes::No_Op) {
                // 호스트는 장치 캐시를 스누핑하지 않고 호스트 캐시에서 예상되는 캐시 상태를 변경하지 않고 메모리 위치의 값을 원합니다.
                // 이를 위한 사용 사례는 호스트가 데이터 없이 E 또는 S 상태를 포함하여 데이터만 요청하고 캐시 상태를 변경하지 않으려 하며
                // E 또는 S 상태가 있으므로 장치 캐시를 스누핑할 필요가 없다는 것을 알 수 있는 경우입니다.
                s2m_ndr = new cxl_packet::s2m::S2M_NDR(
                        cxl_packet::s2m::S2M_NDR_Opcodes::Cmp,
                        cxl_packet::MetaFiled::No_Op,
                        cxl_packet::MetaValue::Any,
                        0, 0, cxl_packet::s2m::DevLoad::LightLoad, 0, 0);
            }
            else {
                panic("invaild request recive device (MemRd, MF:M2S Invalid)\n");
            }
        }
        s2m_ndr->last_resp = 0; // MemData 필요
        pkt->req->cxl_packet_type = CXLPacketType::CXLmemPacket;
        pkt->req->cxl_req = s2m_ndr;
      }

      else if (op == cxl_packet::m2s::M2S_Req_OpCodes::MemClnEvct)
      {
        if (req_meta_field == cxl_packet::MetaFiled::No_Op) {
            panic("invaild request recive device (MemClnEvct req but MataField worng)\n");
        }
        if (req_snoop_type == cxl_packet::m2s::M2S_SnpTypes::No_Op) {
            if (req_meta_value == cxl_packet::MetaValue::Any) {
                // 호스트에서 캐시의 E 또는 S 상태를 삭제하고 해당 line을 I상태로 유지
                // 장치는 스누프 필터 (또는 bias table) clean 가능
                s2m_ndr = new cxl_packet::s2m::S2M_NDR(
                    cxl_packet::s2m::S2M_NDR_Opcodes::Cmp,
                    cxl_packet::MetaFiled::No_Op,
                    cxl_packet::MetaValue::Invalied,
                    0, 0, cxl_packet::s2m::DevLoad::LightLoad, 0, 0);
            }
            else {
                panic("invaild request recive device (MemClnEvct req but MetaValue worng)\n");
            }
        }
        else {
            panic("invaild request recive device (MemClnEvct req but SnpType worng)\n");
        }
        s2m_ndr->last_resp = 1; // 추가 응답 필요 없음
        pkt->req->cxl_packet_type = CXLPacketType::CXLmemPacket;
        pkt->req->cxl_req = s2m_ndr;
      }

      else if (op == cxl_packet::m2s::M2S_Req_OpCodes::MemRdData) {
        if (req_snoop_type == cxl_packet::m2s::M2S_SnpTypes::SnpData) {
            // 호스트는 독점 상태 또는 공유 상태에서 캐시 가능한 사본을 원합니다.
            s2m_ndr = new cxl_packet::s2m::S2M_NDR(
                cxl_packet::s2m::S2M_NDR_Opcodes::Cmp_E,
                cxl_packet::MetaFiled::Meta0_State,
                cxl_packet::MetaValue::Invalied,
                0, 0, cxl_packet::s2m::DevLoad::LightLoad, 0, 0);
            set_bias_mode(pkt->getAddr(), 1);

        }
        else {
            panic("invaild request recive device (MemRdData)\n");
        }
        s2m_ndr->last_resp = 0; // 추가 응답 필요
        pkt->req->cxl_packet_type = CXLPacketType::CXLmemPacket;
        pkt->req->cxl_req = s2m_ndr;
      }
      pkt->req->cxl_req->cxl_req_type = cxl_packet::CXL_Req_Types::_S2M_NDR;
      break;


    case cxl_packet::CXL_Req_Types::_M2S_RwD:

      op2 = ((cxl_packet::m2s::M2S_RwD *)(pkt->req->cxl_req))->memOpcode;
      req_snoop_type = ((cxl_packet::m2s::M2S_RwD *)(pkt->req->cxl_req))->snp_type;
      req_meta_value = ((cxl_packet::m2s::M2S_RwD *)(pkt->req->cxl_req))->meta_value;
      req_meta_field = ((cxl_packet::m2s::M2S_RwD *)(pkt->req->cxl_req))->meta_filed;

      if ((op2 == cxl_packet::m2s::M2S_RwD_OpCodes::MemWr) || (op2 == cxl_packet::m2s::M2S_RwD_OpCodes::MemWrPtl))
      {
        if (req_snoop_type == cxl_packet::MetaFiled::Meta0_State) {
            if (req_meta_value == cxl_packet::MetaValue::Any) {
                if (req_snoop_type == cxl_packet::m2s::M2S_SnpTypes::No_Op) {
                    // 호스트는 메모리를 업데이트하고 라인의 독점 사본을 보관하려고 합니다.
                    s2m_ndr = new cxl_packet::s2m::S2M_NDR(
                        cxl_packet::s2m::S2M_NDR_Opcodes::Cmp,
                        cxl_packet::MetaFiled::No_Op,
                        cxl_packet::MetaValue::Invalied,
                        0, 0, cxl_packet::s2m::DevLoad::LightLoad, 0, 0);
                    set_bias_mode(pkt->getAddr(), 1);
                }
                else {
                    panic("invaild request recive device (MemWr, MF:M0S Any)\n");
                }
            }
            else if (req_meta_value == cxl_packet::MetaValue::Shared) {
                if (req_snoop_type == cxl_packet::m2s::M2S_SnpTypes::No_Op) {
                    // 호스트는 메모리를 업데이트하고 라인의 공유 사본을 보관하려고 합니다.
                    s2m_ndr = new cxl_packet::s2m::S2M_NDR(
                        cxl_packet::s2m::S2M_NDR_Opcodes::Cmp,
                        cxl_packet::MetaFiled::No_Op,
                        cxl_packet::MetaValue::Invalied,
                        0, 0, cxl_packet::s2m::DevLoad::LightLoad, 0, 0);
                    set_bias_mode(pkt->getAddr(), 1);
                }
                else {
                    panic("invaild request recive device (MemWr, MF:M0S Shared)\n");
                }
                
            }
            else if (req_meta_value == cxl_packet::MetaValue::Invalied) {
                if (req_snoop_type == cxl_packet::m2s::M2S_SnpTypes::SnpInv) {
                    // 호스트는 라인을 메모리에 writeback 싶어하고 캐시 가능한 사본을 보관하지 않습니다.
                    // 또한 호스트는 이 쓰기를 하기 전에 라인의 소유권을 얻지 못했고
                    // 메모리에 다시 쓰기를 하기 전에 장치가 캐시를 무효화해야 합니다.
                    s2m_ndr = new cxl_packet::s2m::S2M_NDR(
                        cxl_packet::s2m::S2M_NDR_Opcodes::Cmp,
                        cxl_packet::MetaFiled::No_Op,
                        cxl_packet::MetaValue::Invalied,
                        0, 0, cxl_packet::s2m::DevLoad::LightLoad, 0, 0);
                }
                else if (req_snoop_type == cxl_packet::m2s::M2S_SnpTypes::No_Op) {
                    // 호스트는 메모리를 업데이트하려고 하며 호스트 캐시를 I-상태로 종료합니다.
                    // 호스트에서 Dirty(M-상태) 캐시 제거에 사용합니다.
                    s2m_ndr = new cxl_packet::s2m::S2M_NDR(
                        cxl_packet::s2m::S2M_NDR_Opcodes::Cmp,
                        cxl_packet::MetaFiled::No_Op,
                        cxl_packet::MetaValue::Invalied,
                        0, 0, cxl_packet::s2m::DevLoad::LightLoad, 0, 0);
                }
                else {
                    panic("invaild request recive device (MemWr, MF:M0S Invalid)\n");
                }
            }
            else {
                panic("invaild request recive device (MemWr, MF:M0S)\n");
            }
        }
        else {
            panic("invaild request recive device (MemWr)\n");
        }

      s2m_ndr->last_resp = 1;  // 추가 응답 필요 없음
      pkt->req->cxl_packet_type = CXLPacketType::CXLmemPacket;
      pkt->req->cxl_req = s2m_ndr;
      pkt->req->cxl_req->cxl_req_type = cxl_packet::CXL_Req_Types::_S2M_NDR;
      break;
      }
    

    default:
      break;
    }
    if (pkt->req->cxl_req != nullptr)
        DPRINTF(DCOH, "(CXL_Debug) DCOH::makeCXLresponse Response Packet \n%s\n", pkt->req->cxl_req->print());
}

void
CXLDevice::DCOH::makeMemDataAtomic(PacketPtr pkt)
{
    DPRINTF(DCOH, "(CXL_Debug) DCOH::makeMemDataAtomic call\n");
    cxl_packet::s2m::S2M_DRS* s2m_drs = 
        new cxl_packet::s2m::S2M_DRS(
        cxl_packet::s2m::S2M_DRS_Opcodes::MemData,
        ((cxl_packet::s2m::S2M_NDR *)(pkt->req->cxl_req))->metafiled,
        ((cxl_packet::s2m::S2M_NDR *)(pkt->req->cxl_req))->metavalue,
        0, 0, 0, cxl_packet::s2m::DevLoad::LightLoad, 0, 0);

    // DPRINTF(DCOH, "(CXL_Debug) meta_filed=%d, meta_value=%d\n", ((cxl_packet::m2s::M2S_RwD *)(pkt->req->cxl_req))->meta_filed, ((cxl_packet::m2s::M2S_RwD *)(pkt->req->cxl_req))->meta_value);
    
    pkt->req->cxl_packet_type = CXLPacketType::CXLmemDataPacket;                    
    pkt->req->cxl_data_atomic = s2m_drs;
    pkt->req->cxl_req->cxl_req_type = cxl_packet::CXL_Req_Types::_S2M_DRS;
    pkt->req->cxl_data_atomic->last_resp = 1;
    pkt->req->cxl_req->last_resp = 1;
    if (pkt->req->cxl_data_atomic != nullptr)  
        DPRINTF(DCOH, "(CXL_Debug) DCOH::makeMemDataAtomic MemData Response Packet \n%s\n", pkt->req->cxl_data_atomic->print());
}

void
CXLDevice::DCOH::makeSnoopResp(PacketPtr pkt)
{
    DPRINTF(DCOH, "(CXL_Debug) DCOH::makeSnoopResp call\n");
    cxl_cache_packet::d2h::D2H_Resp* d2h_resp;
    if (pkt->req->old_state == 0b000) {
        d2h_resp =
            new cxl_cache_packet::d2h::D2H_Resp(
                 cxl_cache_packet::d2h::D2H_Resp_OpCodes::RspIHitI, 0, 0, 0);
    }
    else if (((pkt->req->new_state == 0b001) || (pkt->req->new_state == 0b011)) && (pkt->req->old_state == 0b111)) {
        d2h_resp =
            new cxl_cache_packet::d2h::D2H_Resp(
                 cxl_cache_packet::d2h::D2H_Resp_OpCodes::RspSFwdM, 0, 0, 0);
    }
    else if ((pkt->req->new_state == 0b000) && (pkt->req->old_state == 0b111)) {
        d2h_resp =
            new cxl_cache_packet::d2h::D2H_Resp(
                 cxl_cache_packet::d2h::D2H_Resp_OpCodes::RspIFwdM, 0, 0, 0);
    }
    else if ((pkt->req->new_state == 0b000) && ((pkt->req->old_state == 0b001) || (pkt->req->old_state == 0b011) || (pkt->req->old_state == 0b101))) {
        d2h_resp =
            new cxl_cache_packet::d2h::D2H_Resp(
                 cxl_cache_packet::d2h::D2H_Resp_OpCodes::RspIHitSE, 0, 0, 0);
    }
    else if ((pkt->req->new_state == 0b001) && ((pkt->req->old_state == 0b001) || (pkt->req->old_state == 0b011) || (pkt->req->old_state == 0b101))) {
        d2h_resp =
            new cxl_cache_packet::d2h::D2H_Resp(
                 cxl_cache_packet::d2h::D2H_Resp_OpCodes::RspSHitSE, 0, 0, 0);
    }
    else if ((pkt->req->old_state == pkt->req->new_state) && !(sinkPacket(pkt))) {
        d2h_resp =
            new cxl_cache_packet::d2h::D2H_Resp(
                 cxl_cache_packet::d2h::D2H_Resp_OpCodes::RspVHitV, 0, 0, 0);
    }
    else if ((pkt->req->old_state == pkt->req->new_state) && (sinkPacket(pkt))) {
        d2h_resp =
            new cxl_cache_packet::d2h::D2H_Resp(
                 cxl_cache_packet::d2h::D2H_Resp_OpCodes::RspVFwdV, 0, 0, 0);
    }
    else {
        panic("no resp snoop\n");
    }

    delete pkt->req->cxl_cache;
    pkt->req->cxl_cache = d2h_resp;
    pkt->req->cxl_cache->cxl_cache_type = cxl_cache_packet::CXL_Cache_Types::_D2H_Resp;

}



std::string
CXLDevice::DCOH::name() const
{
    return _name;
}

Tick
CXLDevice::DCOH::recvSnpReq(PacketPtr pkt)
{
    Tick snp_latency = parent->SnpProcess(pkt);

    DPRINTF(DCOH, "(CXL_Debug) CXLDevice::DCOH::recvSnpReq call\n");
    cxl_cache_packet::d2h::D2H_Resp* d2h_resp = 
        new cxl_cache_packet::d2h::D2H_Resp(
            cxl_cache_packet::d2h::D2H_Resp_OpCodes::RspIHitI,
            0, 0, 0);
    pkt->req->cxl_packet_type = CXLPacketType::CXLcachePacket;
    pkt->req->cxl_cache = d2h_resp;
    pkt->req->cxl_cache->last_resp = 1;


    return snp_latency;
}

void
CXLDevice::DCOH::set_bias_mode(Addr addr, bool bias_bit)
{
    assert((addr >= 0x80000000) && (addr < 0xa0000000));
    Addr page_num = addr >> 12;
    if ((Bias_Table.count(page_num) > 0) && (Bias_Table[page_num] == bias_bit)) {
        DPRINTF(DCOH, "Setting Addr(%llx) bias mode is already %s bias\n", addr, bias_bit ? "Host" : "Device");
    }
    else {
        DPRINTF(DCOH, "New setting Addr(%llx) bias mode -> %s\n", addr, bias_bit ? "Host" : "Device");
        Bias_Table[page_num] = bias_bit;
    }
}


bool
CXLDevice::DCOH::is_host_bias_mode(Addr addr)
{
    assert((addr >= 0x80000000) && (addr < 0xa0000000));
    DPRINTF(DCOH, "call CXLDevice::DCOH::is_host_bias_mode\n");
    Addr page_num = addr >> 12;
    if (Bias_Table.count(page_num) > 0) {
        bool bias_state = Bias_Table[page_num];
        DPRINTF(DCOH, "CXLDevice::DCOH::is_host_bias_mode Addr %lld is %s Bias mode\n", addr,  bias_state ? "host" : "Device");
        return bias_state;
    }
    else {
        // set_bias_mode(addr, 0); //set device mode
        // DPRINTF(DCOH, "CXLDevice::DCOH::is_host_bias_mode : Bias mode is not set, so Addr %llx setting Device mode\n", addr);
        // return 0;
        set_bias_mode(addr, 1); //set host mode
        DPRINTF(DCOH, "CXLDevice::DCOH::is_host_bias_mode : Bias mode is not set, so Addr %llx setting Host mode\n", addr);
        return 1;
    }
}

Port &
CXLDevice::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "in_port") {
      return in_port;
    }
    else if (if_name == "ex_port") {
      return ex_port;
    }
    // else if (if_name == "coh_port") {
    //   return coh_port;
    // }
    return PciDevice::getPort(if_name, idx);
}

bool
CXLDevice::recvTimingReq(PacketPtr pkt)
{
    return dcoh.recvTimingReq(pkt);
}

bool
CXLDevice::recvTimingResp(PacketPtr pkt)
{
    DPRINTF(CXLDevice, "(CXL_Debug) CXLDevice::recvTimingResp call pkt=%s, is_cxl=%d\n", pkt->print(), pkt->req->cxl_packet_type);
    Addr pkt_addr = pkt->getAddr();
    if (pkt->req->cxl_packet_type != CXLPacketType::HostBiasCheck) {
        DPRINTF(CXLDevice, "Default Resp from Host\n");
        if ((pkt_addr >= 0x80000000) && (pkt_addr < 0xa0000000)) {
            DPRINTF(CXLDevice, "dest is CXL Mem\n");
            dcoh.makeCXLresponse(pkt);

            if (pkt->req->cxl_req->last_resp == 0) {

                // if (sys->isAtomicMode()) {
                    dcoh.makeMemDataAtomic(pkt);
                // }
                assert(pkt->req->cxl_data_atomic != nullptr);
            }
        }
        else if (pkt_addr >= 0xa0000000) {
            DPRINTF(CXLDevice, "recv Resp CXLProc->HostMem access\n");
        }
    }
    
    if (pkt->req->cxl_packet_type == CXLPacketType::HostBiasCheck) {
        DPRINTF(CXLDevice, "Mem Fwd Resp from Host \n%s\n", pkt->req->cxl_req->print());
        cxl_packet::m2s::M2S_SnpTypes req_snoop_type;
        cxl_packet::MetaValue req_meta_value;
        cxl_packet::MetaFiled req_meta_field;
        cxl_packet::m2s::M2S_Req_OpCodes op;

        op = ((cxl_packet::m2s::M2S_Req *)(pkt->req->cxl_req))->memOpcode;
        req_snoop_type = ((cxl_packet::m2s::M2S_Req *)(pkt->req->cxl_req))->snp_type;
        req_meta_value = ((cxl_packet::m2s::M2S_Req *)(pkt->req->cxl_req))->meta_value;
        req_meta_field = ((cxl_packet::m2s::M2S_Req *)(pkt->req->cxl_req))->meta_filed;

        if ((op == cxl_packet::m2s::M2S_Req_OpCodes::MemRdFwd) || (op == cxl_packet::m2s::M2S_Req_OpCodes::MemWrFwd)) {
            DPRINTF(CXLDevice, "(CXL_Debug) CXLDevice: No Response MemRdFwd/MemWrFwd!! -> well operate internal read/write\n");
            
            if (req_meta_field == cxl_packet::MetaFiled::Meta0_State) {
                if (req_meta_value == cxl_packet::MetaValue::Any) {
                    dcoh.set_bias_mode(pkt->getAddr(), 1);
                }
                else if (req_meta_value == cxl_packet::MetaValue::Shared) {
                    dcoh.set_bias_mode(pkt->getAddr(), 1);
                }
                else if (req_meta_value == cxl_packet::MetaValue::Invalied) {
                    dcoh.set_bias_mode(pkt->getAddr(), 0);
                }
            }
        }
    }

    if ((pkt->req->cxl_packet_type == CXLPacketType::CXLmemPacket ) || (pkt->req->cxl_packet_type == CXLPacketType::CXLmemDataPacket)) {
        DPRINTF(CXLDevice, "Resp dest is Host\n");
        pioPort.schedTimingResp(pkt, curTick()+100);
    }
    else if ((pkt->req->cxl_packet_type == CXLPacketType::CXLcachePacket) || (pkt->req->cxl_packet_type == CXLPacketType::CXLcacheDataPacket)) {
        DPRINTF(CXLDevice, "Resp dest is CXLXBar\n");
        ex_port.schedTimingResp(pkt, curTick()+100);
    }
    else if (pkt->req->cxl_packet_type == CXLPacketType::HostBiasCheck) {
        DPRINTF(CXLDevice, "Resp dest is CXLXBar\n");
        ex_port.schedTimingResp(pkt, curTick()+100);
    }
    else if (pkt->req->cxl_packet_type == CXLPacketType::CXLcacheH2DResp) {
        DPRINTF(CXLDevice, "make Snoop Resp \n");
        dcoh.makeSnoopResp(pkt);
        DPRINTF(CXLDevice, "Resp dest is Host\n");
        pioPort.schedTimingResp(pkt, curTick()+100);
    }
    else {
        panic("incorrect resp\n");
    }

    return true;
    
}

bool
CXLDevice::DCOH::recvTimingReq(PacketPtr pkt)
{
    Addr pkt_addr = pkt->getAddr();
    DPRINTF(DCOH, "(CXL_Debug) CXLDevice::DCOH::recvTimingReq call\n");
    if (pkt->req->cxl_req != nullptr) {
        DPRINTF(DCOH, "(CXL_Debug) DCOH::recvTimingReq Request Packet \n%s\n", pkt->req->cxl_req->print());
    }
    else if (pkt->req->cxl_cache != nullptr)  {
        DPRINTF(DCOH, "(CXL_Debug) DCOH::recvTimingReq Request Packet \n%s\n", pkt->req->cxl_cache->print());
    }
    bool success;

    if (pkt->req->cxl_packet_type == CXLPacketType::CXLcacheH2DReq) {
        DPRINTF(DCOH, "(CXL_Debug) DCOH::recvTimingReq Recieve snoop Request snnoop_pkt=%s\n", pkt->print());
        if (pkt->req->cxl_cache != nullptr)
            DPRINTF(DCOH, "(CXL_Debug) DCOH::recvTimingReq Snoop Request Packet\n");
        success =
            pkt->isRead() ? parent->ReadDataTiming(pkt, 1) : parent->WriteDataTiming(pkt, 1);

        return success;
    }

    if ((pkt_addr >= 0xa0000000) || (pkt->req->cxl_packet_type == CXLPacketType::HostBiasCheck)) {
        parent->makeRequest(pkt);
        bool success = (parent->dmaPort).sendTimingReq(pkt);

        return success;
    }
    else if ((pkt_addr >= 0x80000000) && (pkt_addr < 0xa0000000)) {
        assert(pkt->req->cxl_req != nullptr);
        success =
            pkt->isRead() ? parent->ReadDataTiming(pkt, 1) : parent->WriteDataTiming(pkt, 1);


        return success;
    }
    
}

bool
CXLDevice::DCOH::recvCoherentCheckReq(PacketPtr pkt)
{
    DPRINTF(DCOH, "CXLDevice::DCOH::recvCoherentCheckReq call\n");
    if (pkt->req->cxl_req != nullptr) 
        DPRINTF(DCOH, "(CXL_Debug) DCOH::recvCoherentCheckReq Request Packet \n%s\n", pkt->req->cxl_req->print());
    bool success =
        pkt->isRead() ? parent->ReadDataTiming(pkt, 1) : parent->WriteDataTiming(pkt, 1);
    
    // DPRINTF(CXLDevice, "CCC\n");

    return success;
}


}
