//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsPESOneShotPacketizer.h"
#include "tsNullReport.h"
#include "tsTSPacket.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::PESOneShotPacketizer::PESOneShotPacketizer(const DuckContext& duck, PID pid, Report* report) :
    PESStreamPacketizer(duck, pid, report)
{
}

ts::PESOneShotPacketizer::~PESOneShotPacketizer()
{
}


//----------------------------------------------------------------------------
// Get complete cycle as one list of packets
//----------------------------------------------------------------------------

void ts::PESOneShotPacketizer::getPackets(TSPacketVector& packets)
{
    packets.clear();
    while (!empty()) {
        packets.resize(packets.size() + 1);
        PESStreamPacketizer::getNextPacket(packets[packets.size() - 1]);
    }
}


//----------------------------------------------------------------------------
// Hidden methods
//----------------------------------------------------------------------------

bool ts::PESOneShotPacketizer::getNextPacket(TSPacket&)
{
    return false;
}
