#include "cxl_cache_packet.hh"

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

namespace cxl_cache_packet{

std::string _str_CXL_Cache_Type[] = {"D2H_Req", "D2H_Resp", "D2H_Data","H2D_Req", "H2D_Resp", "H2D_Data"};
std::string _str_CXL_Link_Type[] = {"68B", "256B", "PBR"};

namespace d2h
{

std::string _str_D2H_Req_OpCodes[] = {"No_OP","RdCurr", "RdOwn", "RdShared", "RdAny", "RdOwnNoData", "ItoMWr",
                                "WrCur", "CLFlush", "CleanEvict", "DirtyEvict", "CleanEvictNoData", "WOWrInv",
                                "WOWrInvF", "WrInv", "No_OP","CacheFlushed"};
std::string _str_D2H_Resp_OpCodes[] = {"No_OP","RspSHitSE","No_OP","No_OP","RspIHitI", "RspIHitSE", "RspVHitV", "RspSFwdM",
                                "No_OP","No_OP","No_OP","No_OP","No_OP","No_OP","No_OP","RspIFwdM",
                                "No_OP","No_OP","No_OP","No_OP","No_OP","No_OP","RspVFwdV"};


std::string 
D2H_Req::print(){
    std::string res_string = 
        fmt::format("{0}{1}{2}{3}{4}{5}{6}{7}{8}{9}{10}",
        fmt::format("LinkType         : {}\r\n", _str_CXL_Link_Type[link_type]),
        fmt::format("Valid            : {}\r\n", valid ? "T" : "F"),
        fmt::format("OpCode           : {:#X}\t{}\r\n", Opcode, _str_D2H_Req_OpCodes[Opcode]),
        fmt::format("cqid             : {:#X}\r\n", cqid),
        fmt::format("NT               : {}\r\n", NT ? "T" : "F"),
        link_type == B256 ? fmt::format("CacheID          : {:#X}\r\n", CacheID) : "(256B only)\r\n",
        fmt::format("Address          : {:#X}\r\n", Address),
        link_type == PBR ? fmt::format("Source PBR       : {:#X}\r\n", spid) : "(PBR only)\r\n",
        link_type == PBR ? fmt::format("Destination PBR  : {:#X}\r\n", dpid) : "(PBR only)\r\n",
        fmt::format("Reserved         : {:#X}\r\n", rsvd),
        fmt::format("Size             : {} Bits\r\n", getSize_Bits()));

    return res_string;
}

std::string
D2H_Resp::print(){

    std::string res_string = 
        fmt::format("{0}{1}{2}{3}{4}{5}{6}",
        fmt::format("LinkType         : {}\r\n", _str_CXL_Link_Type[link_type]),
        fmt::format("Valid            : {}\r\n", valid ? "T" : "F"),
        fmt::format("OpCode           : {:#X}\t{}\r\n", Opcode, _str_D2H_Resp_OpCodes[Opcode]),
        fmt::format("uqid             : {:#X}\r\n", uqid),
        link_type == PBR ? fmt::format("Destination PBR  : {:#X}\r\n", dpid) : "(PBR only)\r\n",
        fmt::format("Reserved         : {:#X}\r\n", rsvd),
        fmt::format("Size             : {} Bits\r\n", getSize_Bits()));
    return res_string;
}

std::string
D2H_Data::print(){
    std::string res_string = 
        fmt::format("{0}{1}{2}{3}{4}{5}{6}{7}{8}{9}",
        fmt::format("LinkType         : {}\r\n", _str_CXL_Link_Type[link_type]),
        fmt::format("Valid            : {}\r\n", valid ? "T" : "F"),
        fmt::format("uqid             : {:#X}\r\n", uqid),
        link_type == B68 ? fmt::format("ChunkValid       : {}\r\n", ChunkValid ? "T" : "F") : "(68B only)\r\n",
        fmt::format("Bogus            : {}\r\n", Bogus ? "T" : "F"),
        fmt::format("Poison           : {}\r\n", Poison ? "T" : "F"),
        link_type == B68 ? "(256B, PBR only)\r\n" : fmt::format("bep              : {}\r\n", bep ? "T" : "F"),
        link_type == PBR ? fmt::format("Destination PBR   : {:#X}\r\n", dpid) : "(PBR only)\r\n",
        fmt::format("Reserved         : {:#X}\r\n", rsvd),
        fmt::format("Size             : {} Bits\r\n", getSize_Bits()));
    return res_string;
}


} // namespace  d2h

namespace h2d
{
std::string _str_H2D_Req_OpCodes[] = {"No_op", "SnpData", "SnpInv", "SnpCur"};
std::string _str_H2D_Resp_OpCodes[] = {"No_op", "WritePull", "No_op", "No_op","GO",
                            "GO_WritePull", "ExtCmp", "No_op", "GO_WritePull_Drop",
                            "No_op", "No_op", "No_op",
                            "Reserved", "Fast_GO_WritePull", "No_op", "GO_ERR_WritePull"};
std::string _str_RSP_PRE_Encoding[] = {"Host_CacheMiss_LocalCPU", "Host_CacheHit", "Host_CacheMiss_RemoteCPU", "Reser"};
std::string _str_RSP_Data_Encoding[] = {"No_op", "Shared", "Exclusive", "Invalid", "Error", "Modified"};

std::string
H2D_Req::print(){
    std::string res_string = 
        fmt::format("{0}{1}{2}{3}{4}{5}{6}{7}{8}{9}",
        fmt::format("LinkType         : {}\r\n", _str_CXL_Link_Type[link_type]),
        fmt::format("Valid            : {}\r\n", valid ? "T" : "F"),
        fmt::format("opcode           : {:#X}\t{}\r\n", opcode, _str_H2D_Req_OpCodes[opcode]),
        fmt::format("Address          : {:#X}\r\n", Address),
        fmt::format("uqid             : {:#X}\r\n", uqid),
        link_type == B256 ? fmt::format("CacheID          : {:#X}\r\n", CacheID) : "(256B only)\r\n",
        link_type == PBR ? fmt::format("Source PBR       : {:#X}\r\n", spid) : "(PBR only)\r\n",
        link_type == PBR ? fmt::format("Destination PBR  : {:#X}\r\n", dpid) : "(PBR only)\r\n",
        fmt::format("Reserved         : {:#X}\r\n", rsvd),
        fmt::format("Size             : {} Bits\r\n", getSize_Bits()));
    return res_string;
}

std::string
H2D_Resp::print(){
    std::string res_string = 
        fmt::format("{0}{1}{2}{3}{4}{5}{6}{7}{8}{9}",
        fmt::format("LinkType         : {}\r\n", _str_CXL_Link_Type[link_type]),
        fmt::format("Valid            : {}\r\n", valid ? "T" : "F"),
        fmt::format("opcode           : {:#X}\t{}\r\n", opcode, _str_H2D_Resp_OpCodes[opcode]),
        fmt::format("RspData          : {:#X}\t{}\r\n", RspData, _str_RSP_Data_Encoding[RspData]),
        fmt::format("RSP_PRE          : {:#X}\t{}\r\n", RSP_PRE, _str_RSP_PRE_Encoding[RSP_PRE]),
        fmt::format("cqid             : {:#X}\r\n", cqid),
        link_type == B256 ? fmt::format("CacheID          : {:#X}\r\n", CacheID) : "(256B only)\r\n",
        link_type == PBR ? fmt::format("Destination PBR  : {:#X}\r\n", dpid) : "(PBR only)\r\n",
        fmt::format("Reserved         : {:#X}\r\n", rsvd),
        fmt::format("Size             : {} Bits\r\n", getSize_Bits()));
    return res_string;
}

std::string
H2D_Data::print(){
    std::string res_string = 
        fmt::format("{0}{1}{2}{3}{4}{5}{6}{7}{8}{9}",
        fmt::format("LinkType         : {}\r\n", _str_CXL_Link_Type[link_type]),
        fmt::format("Valid            : {}\r\n", valid ? "T" : "F"),
        fmt::format("cqid             : {:#X}\r\n", cqid),
        link_type == B68 ? fmt::format("ChunkValid       : {}\r\n", ChunkValid ? "T" : "F") : "(68B only)\r\n",
        fmt::format("Poison           : {}\r\n", Poison ? "T" : "F"),
        fmt::format("GO_Err            : {}\r\n", GO_Err ? "T" : "F"),
        link_type == B256 ? fmt::format("CacheID          : {:#X}\r\n", CacheID) : "(256B only)\r\n",
        link_type == PBR ? fmt::format("Destination PBR  : {:#X}\r\n", dpid) : "(PBR only)\r\n",
        fmt::format("Reserved         : {:#X}\r\n", rsvd),
        fmt::format("Size             : {} Bits\r\n", getSize_Bits()));
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