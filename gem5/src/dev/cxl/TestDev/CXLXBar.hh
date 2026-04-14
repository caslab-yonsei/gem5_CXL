#ifndef __CXLXBAR_HH__
#define __CXLXBAR_HH__

#include "mem/coherent_xbar.hh"
#include "params/CXLXBar.hh"
#include "mem/packet.hh"
#include "base/types.hh"

namespace gem5
{

class CXLXBar : public CoherentXBar
{
    public:

        class CXLXBarResponsePort : public CoherentXBar::CoherentXBarResponsePort {
            public:
                CXLXBarResponsePort(const std::string &_name,
                             CXLXBar &_cxlxbar, PortID _id)
                    : CoherentXBar::CoherentXBarResponsePort(_name, _cxlxbar, _id), parent(_cxlxbar)
                {}

                CXLXBar &parent;

                Tick
                recvAtomic(PacketPtr pkt) override
                {
                    return parent.recvAtomicBackdoor(pkt, id);
                }

                bool recvTimingReq(PacketPtr pkt) override;


        };

        class CXLXBarRequestPort : public CoherentXBar::CoherentXBarRequestPort {
            public:
                CXLXBarRequestPort(const std::string &_name,
                                CXLXBar &_cxlxbar, PortID _id)
                    : CoherentXBar::CoherentXBarRequestPort(_name, _cxlxbar, _id), parent(_cxlxbar)
                {}

                CXLXBar &parent;

                Tick sendBiasAtomic(Addr addr, Addr originAddr, int* bias_mode, bool get_set=0);
        };


        CXLXBar(const CXLXBarParams &p);
    
        ~CXLXBar();

        std::vector<PacketPtr> PendingPacketList;

        void init();

        Tick CheckBias(PacketPtr pkt, Addr originAddr, int* bias_mode);
        
        Tick recvAtomicBackdoor(PacketPtr pkt, PortID cpu_side_port_id,
                                    MemBackdoorPtr *backdoor=nullptr);

        bool recvTimingReq(PacketPtr pkt, PortID cpu_side_port_id);

        bool recvTimingResp(PacketPtr pkt, PortID mem_side_port_id);

        bool recvTimingSnoopResp(PacketPtr pkt, PortID cpu_side_port_id);
        

        std::pair<MemCmd, Tick>
        forwardAtomic(PacketPtr pkt, PortID exclude_cpu_side_port_id)
        {
            return forwardAtomic(pkt, exclude_cpu_side_port_id, InvalidPortID,
                                snoopPorts);
        }


        std::pair<MemCmd, Tick> forwardAtomic(PacketPtr pkt, PortID exclude_cpu_side_port_id,
                        PortID source_mem_side_port_id,
                        const std::vector<QueuedResponsePort*>& dests);

        

        
        // Port &CXLXBar::getPort(const std::string &if_name, PortID idx=InvalidPortID) override;

};




}


#endif