/*
 * Copyright (c) 2023 SHIN.
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
 * Declaration of the CXL.MEM protocol class.
 */

#ifndef __MEM_CXL_CXL_PACKET_HH__
#define __MEM_CXL_CXL_PACKET_HH__

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
// #include "mem/request.hh"
#include "sim/byteswap.hh"

//#include "mem/packet.hh"


namespace gem5{

namespace cxl_packet{

enum CXL_Req_Types{
    _M2S_Req, _M2S_RwD, _M2S_BIRsp,
    _S2M_NDR, _S2M_DRS, _S2M_BISnp
};



enum linkType {B68, B256, PBR};

struct CXL_Req{
    enum CXL_Req_Types cxl_req_type;
    enum linkType link_type = B256;        // PCIe Link Type (68B Flit, 256B Flit, PBR Flit)
    // CXL_Debug
    int last_resp = 1;

    CXL_Req(CXL_Req_Types cxl_req_type)
    : cxl_req_type(cxl_req_type) {}

    virtual std::string print() {return "";};
    virtual int getSize_Bits() {return 0;};
    virtual Addr getAddr() {return 0;};
};

enum MetaFiled{
    Meta0_State = 0,
    No_Op = 3
}; // Others are Reserved
enum MetaValue{
    Invalied = 0,
    Any = 2,
    Shared = 3
}; // Others are Reserved

namespace m2s{



enum M2S_Req_OpCodes{
    MemInv = 0,
    MemRd = 1,
    MemRdData = 2,
    MemRdFwd = 3,
    MemWrFwd = 4,
    RESERVED1 = 5,
    RESERVED2 = 6,
    RESERVED3 = 7,
    MemSpecRd = 8,
    MemInvNT = 9,
    MemClnEvct = 10
}; // Others are Reserved Codes

enum M2S_RwD_OpCodes{
    MemWr = 1,
    MemWrPtl = 2,
    BIConflict = 4
}; // Others are Reserved Codes

enum M2S_BIRsp_OpCodes{
    BIRspI = 0,
    BIRspS = 1,
    BIRspE = 2,
    BIRspIBlk = 4,
    BIRspSBlk = 5,
    BIRspEBlk = 6
}; // Others are Reserved Codes

enum M2S_SnpTypes{
    No_Op = 0,
    SnpData = 1,
    SnpCur = 2,
    SnpInv = 3,
}; // Others are Reserved


enum M2S_TC{
    Reserved
};

struct M2S_Req : public CXL_Req {     // The Req message class generically contains reads, invalidates, and signals going from
                    // the Master to the Subordinate.

    bool valid = 1;                     // 1 Bit. The valid signal indicates that this is a valid request
    
    enum M2S_Req_OpCodes memOpcode;     // 4 Bits.
                                    // Memory Operation – This specifies which, if any, operation
                                    // needs to be performed on the data and associated information.
                                    // Details in Table 3-29.
    
    enum M2S_SnpTypes snp_type;     // 3 Bits.
                                    // Snoop Type - This specifies what snoop type, if any, needs to
                                    // be issued by the DCOH and the minimum coherency state
                                    // required by the Host. Details in Table 3-32.

    enum MetaFiled meta_filed;  // 2 Bits.
                                    // Meta Data Field – Up to 3 Meta Data Fields can be addressed.
                                    // This specifies which, if any, Meta Data Field needs to be
                                    // updated. Details of Meta Data Field in Table 3-30. If the
                                    // Subordinate does not support memory with Meta Data, this
                                    // field will still be used by the DCOH for interpreting Host
                                    // commands as described in Table 3-31.

    enum MetaValue meta_value;  // 2 Bits.
                                    // Meta Data Value - When MetaField is not No-Op, this specifies
                                    // the value the field needs to be updated to. Details in
                                    // Table 3-31. If the Subordinate does not support memory with
                                    // Meta Data, this field will still be used by the device coherence
                                    // engine for interpreting Host commands as described in
                                    // Table 3-31.

    uint16_t tag;                   // 16 Bits.
                                    // The Tag field is used to specify the source entry in the Master
                                    // which is pre-allocated for the duration of the CXL.mem
                                    // transaction. This value needs to be reflected with the response
                                    // from the Subordinate so the response can be routed
                                    // appropriately. The exceptions are the MemRdFwd and
                                    // MemWrFwd opcodes as described in Table 3-29.
                                    // Note: The Tag field has no explicit requirement to be unique

    //Addr address_5;                 // 1 Bit for 68B, 0 Bit for others
                                    // Address[5] is provisioned for future usages such as critical
                                    // chunk first for 68B flit, but this is not included in a 256B flit.

    Addr address_51_6_5;              // 46 Bits
                                    // This field specifies the Host Physical Address associated with
                                    // the MemOpcode. 

    uint8_t ld_id;                  // LD-ID[3:0]. 4 Bits for 68B/256B, 0 Bit for others
                                    // Logical Device Identifier - This identifies a Logical Device within
                                    // a Multiple-Logical Device. Not applicable in PBR mode where
                                    // SPID infers this field.
    
    uint16_t spid;                  // 12 Bit. PBR only. Source PBR ID
    uint16_t dpid;                  // 12 Bit. PBR only. Destination PBR ID
    uint32_t rsvd;                  // 68B: 6 Bits, 256B/PBR: 20 Bits. Reserved
    
    enum M2S_TC tc;                 // 2 Bits.
                                    // Traffic Class - This can be used by the Master to specify the
                                    // Quality of Service associated with the request. This is reserved
                                    // for future usage.

    int getSize_Bits()
    {
        switch(link_type)
        {
        case B68:
            return 87;
        case B256:
            return 100;
        case PBR:
            return 120;
        default:
            return -1; // ERR
        }
    }

    M2S_Req(M2S_Req_OpCodes memOpcode, M2S_SnpTypes snp_type,
        MetaFiled meta_filed, MetaValue meta_value, uint16_t tag,
        Addr address_51_6_5, uint8_t ld_id, uint16_t spid, uint16_t dpid,
        uint16_t rsvd, M2S_TC tc)
    :   CXL_Req(_M2S_Req), memOpcode(memOpcode), snp_type(snp_type),
        meta_filed(meta_filed), meta_value(meta_value), tag(tag),
        address_51_6_5(address_51_6_5), ld_id(ld_id), spid(spid), dpid(dpid),
        rsvd(rsvd), tc(tc) {}
    
    std::string print();

    Addr getAddr()
    {
        return address_51_6_5;
    }

};

struct M2S_RwD : public CXL_Req{     // The Request with Data (RwD) message class generally contains writes from the Master
                    // to the Subordinate.

    bool valid = 1;                     // 1 Bit. The valid signal indicates that this is a valid request
    
    enum M2S_RwD_OpCodes memOpcode;     // 4 Bits.
    
    enum M2S_SnpTypes snp_type;     // 3 Bits.

    enum MetaFiled meta_filed;  // 2 Bits.

    enum MetaValue meta_value;  // 2 Bits.

    uint16_t tag;                   // 16 Bits.

    Addr address_51_6;              // 46 Bits

    bool poison;                    // 1 Bit.
                                    // This indicates that the data contains an error. The handling of
                                    // poisoned data is device specific. Please refer to Chapter 12.0 for
                                    // more details.

    bool bep;                       // 265B/PBR only. 1 Bit.
                                    // Byte-Enables Present - Indication that 5 data slots are included in
                                    // the message where the 5th data slot carries the 64-bit Byte
                                    // Enables. This field is carried as part of the Flit header bits in 68B
                                    // Flit mode.

    uint8_t ld_id;                  // LD-ID[3:0]. 4 Bits for 68B/256B, 0 Bit for others
    
    uint16_t spid;                  // 12 Bit. PBR only. Source PBR ID
    uint16_t dpid;                  // 12 Bit. PBR only. Destination PBR ID
    uint32_t rsvd;                  // 68B: 6 Bits, 256B/PBR: 22 Bits. Reserved
    
    enum M2S_TC tc;                 // 2 Bits.

    int getSize_Bits()
    { 
        switch(link_type)
        {
        case B68:
            return 87;
        case B256:
            return 104;
        case PBR:
            return 124;
        default:
            return -1; // ERR
        }
    }

    M2S_RwD(M2S_RwD_OpCodes memOpcode, M2S_SnpTypes snp_type,
        MetaFiled meta_filed, MetaValue meta_value, uint16_t tag,
        Addr address_51_6, bool poison, bool bep, uint8_t ld_id, uint16_t spid, uint16_t dpid,
        uint16_t rsvd, M2S_TC tc)
    :   CXL_Req(_M2S_RwD), memOpcode(memOpcode), snp_type(snp_type),
        meta_filed(meta_filed), meta_value(meta_value), tag(tag),
        address_51_6(address_51_6), poison(poison), bep(bep), ld_id(ld_id), spid(spid), dpid(dpid),
        rsvd(rsvd), tc(tc) {}
    
    std::string print();

    Addr getAddr()
    {
        return address_51_6;
    }

};

struct M2S_BIRsp : public CXL_Req{       // The Back-Invalidate Response (BIRsp) message class contains response messages
                        // from the Master to the Subordinate as a result of Back-Invalidate Snoops. This
                        // message class is not supported in 68B Flit mode.


    bool valid = 1;                             // 1 Bit
    enum M2S_BIRsp_OpCodes opcode;          // 4 Bits
    uint16_t bi_id;                         // 256B: 12 Bits
    uint16_t biTag;                         // 12 Bits
    uint8_t lowAddr;                        // 2 Bits. (for BLK)
    uint16_t dpid;                          // 12 Bits. (PBR only)
    uint16_t rsvd;                          // 9 Bits. reserved

    int getSize_Bits()
    {
        switch(link_type)
        {
        case B68:
            return -1; // ERR
        case B256:
            return 40;
        case PBR:
            return 40;
        default:
            return -1; // ERR
        }
    }
    M2S_BIRsp(M2S_BIRsp_OpCodes opcode, uint16_t bi_id, uint16_t biTag,
        uint8_t lowAddr, uint16_t dpid, uint16_t rsvd)
    :   CXL_Req(_M2S_BIRsp), opcode(opcode), bi_id(bi_id), biTag(biTag),
        lowAddr(lowAddr), dpid(dpid), rsvd(rsvd) {}

    std::string print();

};
}

namespace s2m{



enum S2M_BISnp_Opcodes{
    BISnpCur = 0,
    BISnpData = 1,
    BISnpInv = 2,
    BISnpCurBlk = 4,
    BISnpDataBlk = 5,
    BISnpInvBlk = 6
};

enum S2M_NDR_Opcodes{
    Cmp = 0,            // Completions for Writebacks, Reads and Invalidates
    Cmp_S = 1,          // Indication from the DCOH to the Host for Shared state
    Cmp_E = 2,          // Indication from the DCOH to the Host for Exclusive ownership
    BI_ConflictAck = 4  // Completion of the Back-Invalidation conflict handshake
};

enum S2M_DRS_Opcodes{
    MemData = 0,
    MemData_NXM = 1
};

enum S2M_BLK_Encoding{
    Reserved = 0,
    Valid_Lower_128B = 1,
    Valid_Upper_128B = 2,
    Valid_256B = 3
};

enum DevLoad{
    LightLoad = 0,
    OptimalLoad = 1,
    ModerateOverload = 2,
    SevereOverload = 3
};



// The Back-Invalidate Snoop (BISnp) message class contains Snoop messages from the
// Subordinate to the Master. This message class is not supported in 68B Flit mode.
struct S2M_BISnp : public CXL_Req
{

    bool valid = 1;                         // 1 Bit
    enum S2M_BISnp_Opcodes opcode;      // 4 Bits
    uint16_t bi_id;                     // 12 Bits. 256B only
    uint16_t biTag;                     // 12 Bits
    Addr address_51_6;                  // 46 Bits. Host Physical address
    uint16_t spid;                      // 12 Bit. PBR only. Source PBR ID
    uint16_t dpid;                      // 12 Bit. PBR only. Destination PBR ID
    uint32_t rsvd;                      // 9 Bits. Reserved

    int getSize_Bits()
    {
        switch(link_type)
        {
        case B68:
            return -1; // ERR
        case B256:
            return 84;
        case PBR:
            return 96;
        default:
            return -1; // ERR
        }
    }

    S2M_BLK_Encoding getValidBlk(){
        uint8_t valid_bits = address_51_6 & (3 << 6);
        switch (valid_bits)
        {
        case Valid_Lower_128B:
            return Valid_Lower_128B;
        case Valid_Upper_128B:
            return Valid_Upper_128B;
        case Valid_256B:
            return Valid_256B;
        }
    }

    S2M_BISnp(S2M_BISnp_Opcodes opcode, uint16_t bi_id, uint16_t biTag,
        Addr address_51_6, uint16_t spid, uint16_t dpid, uint16_t rsvd)
    :   CXL_Req(_S2M_BISnp), opcode(opcode), bi_id(bi_id), biTag(biTag),
        address_51_6(address_51_6), spid(spid), dpid(dpid), rsvd(rsvd) {}

    std::string print();

    Addr getAddr()
    {
        return address_51_6;
    }


};


// The NDR message class contains completions and indications from the Subordinate to
// the Master.
struct S2M_NDR : public CXL_Req
{

    bool valid;                     // 1
    enum S2M_NDR_Opcodes opcode;    // 3
    enum MetaFiled metafiled;       // 2
    enum MetaValue metavalue;       // 2
    uint16_t tag;                   // 16
    uint8_t ld_id;                  // 4 | 4 | 0    
    enum DevLoad dev_load;          // 2
    uint16_t dpid;                  // 0 | 0| 12
    uint16_t rsvd;                  // 0 | 10 | 10

    int getSize_Bits()
    {
        switch(link_type)
        {
        case B68:
            return 30; // ERR
        case B256:
            return 40;
        case PBR:
            return 48;
        default:
            return -1; // ERR
        }
    }
    S2M_NDR(S2M_NDR_Opcodes opcode, MetaFiled metafiled, MetaValue metavalue, 
        uint16_t tag, uint8_t ld_id, DevLoad dev_load, uint16_t dpid, uint16_t rsvd)
    :   CXL_Req(_S2M_NDR), opcode(opcode), metafiled(metafiled), metavalue(metavalue), tag(tag),
        ld_id(ld_id), dev_load(dev_load), dpid(dpid), rsvd(rsvd) {}

    std::string print();

};

// The DRS message class contains memory read data from the Subordinate to the
// Master.
struct S2M_DRS : public CXL_Req
{

    bool valid;                     // 1
    enum S2M_DRS_Opcodes opcode;    // 3
    enum MetaFiled metafiled;       // 2
    enum MetaValue metavalue;       // 2
    uint16_t tag;                   // 16
    bool poison;                    // 1
    uint8_t ld_id;                  // 4 | 4 | 0    
    enum DevLoad dev_load;          // 2
    uint16_t dpid;                  // 0 | 0| 12
    uint16_t rsvd;                  // 0 | 10 | 10

    int getSize_Bits()
    {
        switch(link_type)
        {
        case B68:
            return 40; // ERR
        case B256:
            return 40;
        case PBR:
            return 48;
        default:
            return -1; // ERR
        }
    }

    S2M_DRS(S2M_DRS_Opcodes opcode, MetaFiled metafiled, MetaValue metavalue, uint16_t tag, 
        bool poison, uint8_t ld_id, DevLoad dev_load, uint16_t dpid, uint16_t rsvd)
    :   CXL_Req(_S2M_DRS), opcode(opcode), metafiled(metafiled), metavalue(metavalue), tag(tag),
        poison(poison), ld_id(ld_id), dev_load(dev_load), dpid(dpid), rsvd(rsvd) {}

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

#endif //__MEM_CXL_CXL_MEM_HH__