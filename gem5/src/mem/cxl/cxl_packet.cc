#include "cxl_packet.hh"

#include <string_view>
#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>

#include <charconv>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#define FMT_HEADER_ONLY
#include <fmt/core.h>




namespace gem5{

namespace cxl_packet{

std::string _str_CXL_ReQ_Type[] = {"M2S_Req", "M2S_RwD", "M2S_BIRsp", "S2M_NDR", "S2M_DRS", "S2M_BISnp"};
std::string _str_CXL_Link_Type[] = {"68B", "256B", "PBR"};
std::string _str_MetaFiled[] = {"Meta0_State", "UnDef", "UnDef", "No_Op"};
std::string _str_MetaValue[] = {"Invalied", "UnDef", "Any", "Shared"};

namespace m2s
{

std::string _str_M2S_Req_OpCodes[] = {"MemInv", "MemRd", "MemRdData", "MemRdFwd", "MemWrFwd",
                                "RESERVED1", "RESERVED2", "RESERVED3", "MemSpecRd", "MemSpecRd",
                                "MemInvNT", "MemClnEvct"};
std::string _str_M2S_RwD_OpCodes[] = {"UnDef", "MemWr", "MemWrPtl", "UnDef", "BIConflict"};
std::string _str_M2S_BIRsp_OpCodes[] = {"BIRspI", "BIRspS", "BIRspE", "UnDef", "BIRspIBlk", "BIRspSBlk", "BIRspEBlk"};
std::string _str_M2S_SnpTypes[] = {"No_Op", "SnpData", "SnpCur", "SnpInv"};
std::string _str_M2S_TC[] = {"Reserved"};

std::string 
M2S_Req::print(){
    std::string res_string = 
    fmt::format("{0}{1}{2}{3}{4}{5}{6}{7}{8}{9}{10}{11}{12}{13}",
    fmt::format("LinkType         : {}\r\n", _str_CXL_Link_Type[link_type]),
    fmt::format("Valid            : {}\r\n", valid ? "T" : "F"),
    fmt::format("memOpCode        : {:#X}\t{}\r\n", memOpcode, _str_M2S_Req_OpCodes[memOpcode]),
    fmt::format("snp_type         : {:#X}\t{}\r\n", snp_type, _str_M2S_SnpTypes[snp_type]),
    fmt::format("meta_filed       : {:#X}\t{}\r\n", meta_filed, _str_MetaFiled[meta_filed]),
    fmt::format("meta_value       : {:#X}\t{}\r\n", meta_value, _str_MetaValue[meta_value]),
    fmt::format("tag              : {:#X}\r\n", tag),
    fmt::format("address_51_6_5   : {:#X}\r\n", address_51_6_5),
    link_type == PBR ? "(68/256B only)\r\n" : fmt::format("LD-ID            : {:#X}\r\n", ld_id),
    link_type == PBR ? fmt::format("Source PBR        : {:#X}\r\n", spid) : "(PBR only)\r\n",
    link_type == PBR ? fmt::format("Destination PBR   : {:#X}\r\n", dpid) : "(PBR only)\r\n",
    fmt::format("Reserved         : {:#X}\r\n", rsvd),
    fmt::format("Traffic Class    : {:#X}\t{}\r\n", tc, _str_M2S_TC[tc]),
    fmt::format("Size             : {} Bits\r\n", getSize_Bits()));

    return res_string;
}

std::string
M2S_RwD::print(){
    // std::string test_string = 
    // link_type == PBR ? "(68/256B only)\r\n" : fmt::format("LD-ID            : {:#X}\r\n", ld_id);
    // return test_string;
    // std::string res_string = 
    //     fmt::format("{0}{1}{2}{3}{4}{5}{6}{7}{8}{9}{10}{11}{12}{13}{14}{15}",
    //     fmt::format("LinkType         : {}\r\n", _str_CXL_Link_Type[link_type]),
    //     fmt::format("Valid            : {}\r\n", valid ? "T" : "F"),
    //     fmt::format("memOpCode        : {:#X}\t{}\r\n", memOpcode, _str_M2S_RwD_OpCodes[memOpcode]),
    //     fmt::format("snp_type         : {:#X}\t{}\r\n", snp_type, _str_M2S_SnpTypes[snp_type]),
    //     fmt::format("meta_filed       : {:#X}\t{}\r\n", meta_filed, _str_MetaFiled[meta_filed]),
    //     fmt::format("meta_value       : {:#X}\t{}\r\n", meta_value, _str_MetaValue[meta_value]),
    //     fmt::format("tag              : {:#X}\r\n", tag),
    //     fmt::format("address_51_6     : {:#X}\r\n", address_51_6),
    //     fmt::format("Poision          : {}\r\n", poison ? "T" : "F"),
    //     link_type == B68 ? "(PBR/256B only)\r\n" : fmt::format("Byte-Enables Present  : {}\r\n", bep ? "T" : "F"),
    //     link_type == PBR ? "(68/256B only)\r\n" : fmt::format("LD-ID            : {:#X}\r\n", ld_id),
    //     link_type == PBR ? fmt::format("Source PBR        : {:#X}\r\n", spid) : "(PBR only)\r\n",
    //     link_type == PBR ? fmt::format("Destination PBR   : {:#X}\r\n", dpid) : "(PBR only)\r\n",
    //     fmt::format("Reserved         : {:#X}\r\n", rsvd),
    //     fmt::format("Traffic Class    : {:#X}\r\n", _str_M2S_TC[tc]),
    //     fmt::format("Size             : {} Bits\r\n", getSize_Bits()));
    // return res_string;

    std::string res_string = 
        fmt::format("{0}{1}{2}{3}{4}{5}{6}{7}{8}{9}{10}{11}{12}{13}{14}{15}",
        fmt::format("LinkType         : {}\r\n", _str_CXL_Link_Type[link_type]),
        fmt::format("Valid            : {}\r\n", valid ? "T" : "F"),
        fmt::format("memOpCode        : {:#X}\t{}\r\n", memOpcode, _str_M2S_RwD_OpCodes[memOpcode]),
        fmt::format("snp_type         : {:#X}\t{}\r\n", snp_type, _str_M2S_SnpTypes[snp_type]),
        fmt::format("meta_filed       : {:#X}\t{}\r\n", meta_filed, _str_MetaFiled[meta_filed]),
        fmt::format("meta_value       : {:#X}\t{}\r\n", meta_value, _str_MetaValue[meta_value]),
        fmt::format("tag              : {:#X}\r\n", tag),
        fmt::format("address_51_6     : {:#X}\r\n", address_51_6),
        fmt::format("Poision          : {}\r\n", poison ? "T" : "F"),
        link_type == B68 ? "(PBR/256B only)\r\n" : fmt::format("Byte-Enables Present  : {}\r\n", bep ? "T" : "F"),
        link_type == PBR ? "(68/256B only)\r\n" : fmt::format("LD-ID            : {:#X}\r\n", ld_id),
        link_type == PBR ? fmt::format("Source PBR        : {:#X}\r\n", spid) : "(PBR only)\r\n",
        link_type == PBR ? fmt::format("Destination PBR   : {:#X}\r\n", dpid) : "(PBR only)\r\n",
        fmt::format("Reserved         : {:#X}\r\n", rsvd),
        fmt::format("Traffic Class    : {:#X}\t{}\r\n", tc, _str_M2S_TC[tc]),
        fmt::format("Size             : {} Bits\r\n", getSize_Bits()));
    return res_string;
}

std::string
M2S_BIRsp::print(){
    std::string res_string = 
        fmt::format("{0}{1}{2}{3}{4}{5}{6}{7}",
        fmt::format("LinkType         : {}\r\n", _str_CXL_Link_Type[link_type]),
        fmt::format("Valid            : {}\r\n", valid ? "T" : "F"),
        link_type == B256 ? fmt::format("bi_id          : {:#X}\r\n", bi_id) : "(256B only)\r\n",
        fmt::format("biTag            : {:#X}\r\n", biTag),
        fmt::format("lowAddr          : {:#X}\r\n", lowAddr),
        link_type == PBR ? fmt::format("Destination PBR   : {:#X}\r\n", dpid) : "(PBR only)\r\n",
        fmt::format("Reserved         : {:#X}\r\n", rsvd),
        fmt::format("Size             : {} Bits\r\n", getSize_Bits()));
    return res_string;
}


} // namespace  m2s

namespace s2m
{
std::string _str_S2M_BISnp_Opcodes[] = {"BISnpCur", "BISnpData", "BISnpInv", "UnDef", "BISnpCurBlk", "Reserved",
                                    "BISnpDataBlk", "BISnpInvBlk"};
std::string _str_S2M_NDR_Opcodes[] = {"Cmp", "Cmp-S", "Cmp-E", "Reserved", "BI-ConflictAck"};
std::string _str_S2M_DRS_Opcodes[] = {"MemData", "MemData-NXM"};
std::string _str_S2M_BLK_Encoding[] = {"No_Op", "SnpData", "SnpCur", "SnpInv"};
std::string _str_DevLoad[] = {"Reserved"};

std::string
S2M_BISnp::print(){
    std::string res_string = 
        fmt::format("{0}{1}{2}{3}{4}{5}{6}{7}{8}{9}",
        fmt::format("LinkType         : {}\r\n", _str_CXL_Link_Type[link_type]),
        fmt::format("Valid            : {}\r\n", valid ? "T" : "F"),
        fmt::format("OpCode           : {:#X}\t{}\r\n", opcode, _str_S2M_BISnp_Opcodes[opcode]),
        link_type == B256 ? fmt::format("bi_id          : {:#X}\r\n", bi_id) : "(256B only)\r\n",
        fmt::format("biTag            : {:#X}\t{}\r\n", biTag),
        fmt::format("address_51_6     : {:#X}\r\n", address_51_6),
        link_type == PBR ? fmt::format("Source PBR        : {:#X}\r\n", spid) : "(PBR only)\r\n",
        link_type == PBR ? fmt::format("Destination PBR   : {:#X}\r\n", dpid) : "(PBR only)\r\n",
        fmt::format("Reserved         : {:#X}\r\n", rsvd),
        fmt::format("Size             : {} Bits\r\n", getSize_Bits()));
    return res_string;
}

std::string
S2M_NDR::print(){
    std::string res_string = 
        fmt::format("{0}{1}{2}{3}{4}{5}{6}{7}{8}{9}{10}",
        fmt::format("LinkType         : {}\r\n", _str_CXL_Link_Type[link_type]),
        fmt::format("Valid            : {}\r\n", valid ? "T" : "F"),
        fmt::format("opCode           : {:#X}\t{}\r\n", opcode, _str_S2M_NDR_Opcodes[opcode]),
        fmt::format("meta_filed       : {:#X}\t{}\r\n", metafiled, _str_MetaFiled[metafiled]),
        fmt::format("meta_value       : {:#X}\t{}\r\n", metavalue, _str_MetaValue[metavalue]),
        fmt::format("tag              : {:#X}\r\n", tag),
        link_type == PBR ? "(68/256B only)\r\n" : fmt::format("LD-ID            : {:#X}\r\n", ld_id),
        fmt::format("dev load         : {:#X}\r\n", dev_load),
        link_type == PBR ? fmt::format("Destination PBR   : {:#X}\r\n", dpid) : "(PBR only)\r\n",
        fmt::format("Reserved         : {:#X}\r\n", rsvd),
        fmt::format("Size             : {} Bits\r\n", getSize_Bits()));
    return res_string;
}

std::string
S2M_DRS::print(){
    std::string res_string = 
        // fmt::format("{0}{1}{2}{3}",
        // fmt::format("L1\n"),
        // fmt::format("L2\n"),
        fmt::format("{0}{1}{2}{3}{4}{5}{6}{7}{8}{9}{10}{11}",
        fmt::format("LinkType         : {}\r\n", _str_CXL_Link_Type[link_type]),
        fmt::format("Valid            : {}\r\n", valid ? "T" : "F"),
        fmt::format("opCode           : {:#X}\t{}\r\n", opcode, _str_S2M_DRS_Opcodes[opcode]),
        fmt::format("meta_filed       : {:#X}\t{}\r\n", metafiled, _str_MetaFiled[metafiled]),
        fmt::format("meta_value       : {:#X}\t{}\r\n", metavalue, _str_MetaValue[metavalue]),
        fmt::format("tag              : {:#X}\r\n", tag),

        fmt::format("Poision          : {}\r\n", poison),
        link_type == PBR ? "(68/256B only)\r\n" : fmt::format("LD-ID            : {:#X}\r\n", ld_id),
        fmt::format("dev load         : {:#X}\r\n", dev_load),
        link_type == PBR ? fmt::format("Destination PBR   : {:#X}\r\n", dpid) : "(PBR only)\r\n",
        fmt::format("Reserved         : {:#X}\r\n", rsvd),
        fmt::format("Size             : {} Bits\r\n", getSize_Bits()));
        // );
    return res_string;
}

}

// CxlPacket::CxlPacket(const RequestPtr &_req, MemCmd _cmd)
// :Packet(_req, _cmd)
// {
//     cxl_req = nullptr;
// }

// CxlPacket::CxlPacket(const RequestPtr &_req, MemCmd _cmd, int _blkSize, PacketId _id = 0)
// :Packet(_req, _cmd, _blkSize, _id)
// {
//     cxl_req = nullptr;
// }

// CxlPacket::CxlPacket(const PacketPtr pkt, bool clear_flags, bool alloc_data)
// :Packet(pkt, clear_flags, alloc_data)
// {
//     cxl_req = nullptr;
// }

// void 
// CxlPacket::SetCxlReq(enum CXL_Req_Types req_type, CXL_Req* req)
// {
//     cxl_req = req;
// }

// CxlPacket::~CxlPacket()
// {
//     delete cxl_req;
//     /* Packet::~Packet();*/
// }

}

}