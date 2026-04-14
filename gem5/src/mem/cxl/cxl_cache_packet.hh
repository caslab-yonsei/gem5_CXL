/*
 * Copyright (c) 2023 KIM.
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
 * Copyright (c) 2010,2015 Advanced Micro Devices, Inc.
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
 */

/**
 * @file
 * Declaration of the CXL.Cache protocol class.
 */

#ifndef __MEM_CXL_CXL_CACHE_PACKET_HH__
#define __MEM_CXL_CXL_CACHE_PACKET_HH__

#include <bitset>
#include <cassert>
#include <initializer_list>
#include <list>

#include "base/addr_range.hh"
#include "base/cast.hh"
#include "base/compiler.hh"
#include "base/flags.hh"
#include "base/logging.hh"
#include "base/printable.hh"
#include "base/types.hh"
#include "mem/htm.hh"
//#include "mem/request.hh"
#include "sim/byteswap.hh"

//#include "mem/packet.hh"


namespace gem5{

namespace cxl_cache_packet{

enum CXL_Cache_Types{
    _D2H_Req, _D2H_Resp, _D2H_Data,
    _H2D_Req, _H2D_Resp, _H2D_Data
};


enum linkType {B68, B256, PBR};

struct CXL_Cache{
    enum CXL_Cache_Types cxl_cache_type;
    enum linkType link_type = B256;        // PCIe Link Type (68B Flit, 256B Flit, PBR Flit)
    // CXL_Debug
    int last_resp = 1;
    bool last_data = true;

    CXL_Cache(CXL_Cache_Types cxl_cache_type)
    : cxl_cache_type(cxl_cache_type) {}
    virtual std::string print() {return "";};
    virtual int getSize_Bits() {return 0;};
    virtual Addr getAddr() {return 0;};
};


namespace d2h{

enum D2H_Req_OpCodes{
    RdCurr = 1,                             // Read
    RdOwn = 2,                              // Read
    RdShared = 3,                           // Read
    RdAny = 4,                              // Read
    RdOwnNoData = 5,                        // Read0
    ItoMWr = 6,                             // Read0-Write
    WrCur = 7,                              // Read0-Write
    CLFlush = 8,                            // Read0
    CleanEvict = 9,                         // Write
    DirtyEvict = 10,                        // Write
    CleanEvictNoData = 11,                  // Write
    WOWrInv = 12,                           // Write
    WOWrInvF = 13,                          // Write
    WrInv = 14,                             // Write
    CacheFlushed = 16                       // Read0
}; // Others are Reserved Codes

enum D2H_Resp_OpCodes{
    RspIHitI = 4,
    RspVHitV = 6,
    RspIHitSE = 5,
    RspSHitSE = 1,
    RspSFwdM = 7,
    RspIFwdM = 15,
    RspVFwdV = 22
}; // Others are Reserved Codes


struct D2H_Req : public CXL_Cache {


    bool valid = 1;                     // 1 Bit.
    
    enum D2H_Req_OpCodes Opcode;        // 5 Bits.

    uint16_t cqid;                      // 12 Bits.

    bool NT;                            // 1 Bits.
 
    uint8_t CacheID;                    // 4 Bits. 256B only.

    Addr Address;                       // 46 Bits.   
    
    uint16_t spid;                      // 12 Bit. PBR only.

    uint16_t dpid;                      // 12 Bit. PBR only.

    uint32_t rsvd;                      // 68B: 14 Bits, 256B/PBR: 7 Bits. Reserved.

    int getSize_Bits()
    {
        switch(link_type)
        {
        case B68:
            return 79;
        case B256:
            return 76;
        case PBR:
            return 96;
        default:
            return -1; // ERR
        }
    }

    D2H_Req(D2H_Req_OpCodes Opcode, uint16_t cqid, bool NT,
        uint8_t CacheID, Addr Address, uint16_t spid, uint16_t dpid,
        uint16_t rsvd)
    :   CXL_Cache(_D2H_Req), Opcode(Opcode), cqid(cqid), NT(NT),
        CacheID(CacheID), Address(Address), spid(spid), dpid(dpid),
        rsvd(rsvd) {}
    
    std::string print();

    Addr getAddr()
    {
        return Address;
    }

};

struct D2H_Resp : public CXL_Cache{   

    bool valid = 1;                     // 1 Bit.
    
    enum D2H_Resp_OpCodes Opcode;       // 5 Bits.
    
    uint16_t uqid;                      // 12 Bit.

    uint16_t dpid;                      // 12 Bit. PBR only.

    uint32_t rsvd;                      // 68B: 2 Bits, 256B/PBR: 6 Bits. Reserved.

    int getSize_Bits()
    { 
        switch(link_type)
        {
        case B68:
            return 20;
        case B256:
            return 24;
        case PBR:
            return 36;
        default:
            return -1; // ERR
        }
    }

    D2H_Resp(D2H_Resp_OpCodes Opcode, uint16_t uqid, uint16_t dpid,
        uint16_t rsvd)
    :   CXL_Cache(_D2H_Resp), Opcode(Opcode), uqid(uqid), dpid(dpid),
        rsvd(rsvd) {}
    
    std::string print();

};

struct D2H_Data : public CXL_Cache{

    bool valid = 1;                             // 1 Bit.

    uint16_t uqid;                              // 12 Bits.

    bool ChunkValid;                            // 1 Bit. 68B only.

    bool Bogus;                                 // 1 Bit.                 

    bool Poison;                                // 1 Bit.

    bool bep;                                   // 1 Bit. 256B, PBR only.

    uint16_t dpid;                              // 12 Bits. PBR only.

    uint16_t rsvd;                              // 68B: 1 Bits, 256B/PBR: 8 Bits. Reserved.

    int getSize_Bits()
    {
        switch(link_type)
        {
        case B68:
            return 17;
        case B256:
            return 24;
        case PBR:
            return 36;
        default:
            return -1; // ERR
        }
    }
    D2H_Data(uint16_t uqid,  bool ChunkValid, bool Bogus, bool Poison,
        bool bep, uint16_t dpid, uint16_t rsvd)
    :   CXL_Cache(_D2H_Data), uqid(uqid), ChunkValid(ChunkValid), Bogus(Bogus),
        Poison(Poison), bep(bep), dpid(dpid), rsvd(rsvd) {}
    std::string print();

};
}

namespace h2d{



enum H2D_Req_Opcodes{
    SnpData = 1,
    SnpInv = 2,
    SnpCur = 3
};

enum H2D_Resp_Opcodes{
    WritePull = 1,
    GO = 4,
    GO_WritePull = 5,
    ExtCmp = 6,
    GO_WritePull_Drop = 8,
    Reserved = 12,
    Fast_GO_writePull = 13,
    GO_ERR_WritePull = 15
};

enum RSP_PRE_Encoding{
    Host_CacheMiss_LocalCPU = 0,            // Host Cache Miss to local CPU socket memory
    Host_CacheHit = 1,                      // Host Cache Hit
    Host_CacheMiss_RemoteCPU = 2,           // Host Cache Miss to Remote CPU socket memory
    Reser = 3
};

enum RSP_Data_Encoding{
    Invalid = 3,
    Shared = 1,
    Exclusive = 2,
    Modified = 5,
    Error = 4
};


struct H2D_Req : public CXL_Cache
{

    bool valid = 1;                         // 1 Bit.

    enum H2D_Req_Opcodes opcode;            // 4 Bits.

    Addr Address;                           // 46 Bits.

    uint16_t uqid;                          // 12 Bits.

    uint8_t CacheID;                        // 4 Bits. 256B only.

    uint16_t spid;                          // 12 Bit. PBR only.

    uint16_t dpid;                          // 12 Bit. PBR only.

    uint32_t rsvd;                          // 68B: 2 Bits, 256B/PBR: 6 Bits. Reserved.

    int getSize_Bits()
    {
        switch(link_type)
        {
        case B68:
            return 64;
        case B256:
            return 72;
        case PBR:
            return 92;
        default:
            return -1; // ERR
        }
    }

    H2D_Req(H2D_Req_Opcodes opcode, Addr Address,
    uint16_t uqid, uint8_t CacheID,
    uint16_t spid, uint16_t dpid, uint16_t rsvd)
    :   CXL_Cache(_H2D_Req), opcode(opcode), Address(Address), uqid(uqid),
        CacheID(CacheID), spid(spid), dpid(dpid), rsvd(rsvd) {}

    std::string print();

    Addr getAddr()
    {
        return Address;
    }

};


struct H2D_Resp : public CXL_Cache
{

    bool valid;                             // 1 Bit.

    enum H2D_Resp_Opcodes opcode;           // 4 Bits.

    enum RSP_Data_Encoding RspData;         // 12 Bits.

    enum RSP_PRE_Encoding RSP_PRE;          // 2 Bits.

    uint16_t cqid;                          // 12 Bits.

    uint8_t CacheID;                        // 4 Bits. 256B only.    

    uint16_t dpid;                          // 12 Bits. PBR only.

    uint16_t rsvd;                          // 68B: 1 Bits, 256B/PBR: 5 Bits. Reserved.

    int getSize_Bits()
    {
        switch(link_type)
        {
        case B68:
            return 32;
        case B256:
            return 40;
        case PBR:
            return 40;
        default:
            return -1; // ERR
        }
    }
    H2D_Resp(H2D_Resp_Opcodes opcode, RSP_Data_Encoding RspData, enum RSP_PRE_Encoding RSP_PRE, 
        uint16_t cqid, uint8_t CacheID, uint16_t dpid, uint16_t rsvd)
    :   CXL_Cache(_H2D_Resp), opcode(opcode), RspData(RspData), RSP_PRE(RSP_PRE), cqid(cqid),
        CacheID(CacheID), dpid(dpid), rsvd(rsvd) {}

    std::string print();
};


struct H2D_Data : public CXL_Cache
{

    bool valid;                         // 1 Bit.

    uint16_t cqid;                      // 12 bits.

    bool ChunkValid;                    // 1 Bit. 68B only.

    bool Poison;                        // 1 Bit.

    bool GO_Err;                        // 1 Bit.    

    uint8_t CacheID;                    // 4 Bits. 256B only.

    uint16_t dpid;                      // 12 Bits. PBR only.

    uint16_t rsvd;                      // 68B: 8 Bits, 256B/PBR: 9 Bits. Reserved.

    int getSize_Bits()
    {
        switch(link_type)
        {
        case B68:
            return 24;
        case B256:
            return 28;
        case PBR:
            return 36;
        default:
            return -1; // ERR
        }
    }

    H2D_Data(uint16_t cqid,  bool ChunkValid, bool Poison, bool GO_Err,
        uint8_t CacheID, uint16_t dpid, uint16_t rsvd)
    :   CXL_Cache(_H2D_Data), cqid(cqid), ChunkValid(ChunkValid),
        Poison(Poison), GO_Err(GO_Err), CacheID(CacheID), dpid(dpid), rsvd(rsvd) {}

    std::string print();
};

}
/*
class CxlPacket : public Packet{
    CXL_Req* cxl_req;

    public:
    
    CxlPacket(const RequestPtr &_req, MemCmd _cmd);
    CxlPacket(const RequestPtr &_req, MemCmd _cmd, int _blkSize, PacketId _id);
    CxlPacket(const PacketPtr pkt, bool clear_flags, bool alloc_data);

    void SetCxlReq(enum CXL_Req_Types req_type, CXL_Req* req);
    
    ~CxlPacket();
};
*/
}
} // namespace gem5

#endif //__MEM_CXL_CXL_CACHE_PACKET_HH__