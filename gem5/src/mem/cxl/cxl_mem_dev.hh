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

#ifndef __MEM_CXL_CXL_MEM_DEV_HH__
#define __MEM_CXL_CXL_MEM_DEV_HH__

#include <bitset>
#include <cassert>
#include <initializer_list>
#include <list>

// #include "base/addr_range.hh"
// #include "base/cast.hh"
// #include "base/compiler.hh"
// #include "base/flags.hh"
// #include "base/logging.hh"
// #include "base/printable.hh"
// #include "base/types.hh"
// #include "mem/htm.hh"
// #include "mem/request.hh"
// #include "sim/byteswap.hh"

#include "mem/packet.hh"

#include "mem/xbar.hh"
#include "mem/noncoherent_xbar.hh"
#include "mem/coherent_xbar.hh"

#include "params/CxlMemOnlyDev.hh"

namespace gem5{
namespace memory{

class CxlMemOnlyDev : public ClockedObject
{
    CoherentXBar* internal_xbar;
    CoherentXBar* external_xbar;

    Port* to_internal_xbar_port; 
    //Port from_internal_xbar_port; 



  public:
    CxlMemOnlyDev(const CxlMemOnlyDevParams &p);
    Port& getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;
};

}
}

#endif