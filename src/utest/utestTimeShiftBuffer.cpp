//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::TimeShiftBuffer
//
//----------------------------------------------------------------------------

#include "tsTimeShiftBuffer.h"
#include "tsCerrReport.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TimeShiftBufferTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Minimum);
    TSUNIT_DECLARE_TEST(Memory);
    TSUNIT_DECLARE_TEST(File);

private:
    void testCommon(uint8_t total, uint8_t memory);
};

TSUNIT_REGISTER(TimeShiftBufferTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void TimeShiftBufferTest::testCommon(uint8_t total, uint8_t memory)
{
    ts::TimeShiftBuffer buf(total);
    TSUNIT_ASSERT(buf.setMemoryPackets(memory));
    TSUNIT_ASSERT(!buf.isOpen());
    TSUNIT_ASSERT(buf.open(CERR));
    TSUNIT_ASSERT(buf.isOpen());
    TSUNIT_EQUAL(total, buf.size());
    TSUNIT_EQUAL(0, buf.count());
    TSUNIT_ASSERT(buf.empty());
    TSUNIT_ASSERT(!buf.full());
    TSUNIT_EQUAL(memory >= total, buf.memoryResident());

    ts::TSPacket pkt;
    ts::TSPacketMetadata mdata;
    size_t in_label = 0;
    size_t out_label = 0;

    // Fill the buffer, return null packets.
    for (uint8_t i = 0; i < total; i++) {

        pkt.init(i, i, i);
        mdata.reset();
        mdata.setLabel(in_label);
        in_label = (in_label + 1) % ts::TSPacketLabelSet::SIZE;

        TSUNIT_EQUAL(184, pkt.getPayloadSize());
        TSUNIT_EQUAL(i, pkt.getPID());
        TSUNIT_EQUAL(i, *pkt.getPayload());
        TSUNIT_EQUAL(i, buf.count());
        TSUNIT_ASSERT(!buf.full());

        TSUNIT_ASSERT(buf.shift(pkt, mdata, CERR));

        TSUNIT_EQUAL(ts::PID_NULL, pkt.getPID());
        TSUNIT_ASSERT(mdata.getInputStuffing());
        TSUNIT_ASSERT(!mdata.hasAnyLabel());
    }
    TSUNIT_ASSERT(buf.full());

    // Actual time shift by 'total' packets.
    for (uint8_t i = total; i < 3 * total; i++) {

        pkt.init(i, i, i);
        mdata.reset();
        mdata.setLabel(in_label);
        in_label = (in_label + 1) % ts::TSPacketLabelSet::SIZE;

        TSUNIT_EQUAL(184, pkt.getPayloadSize());
        TSUNIT_EQUAL(i, pkt.getPID());
        TSUNIT_EQUAL(i, *pkt.getPayload());
        TSUNIT_EQUAL(total, buf.count());
        TSUNIT_ASSERT(buf.full());

        TSUNIT_ASSERT(buf.shift(pkt, mdata, CERR));

        TSUNIT_EQUAL(184, pkt.getPayloadSize());
        TSUNIT_EQUAL(i - total, pkt.getPID());
        TSUNIT_EQUAL(i - total, *pkt.getPayload());
        TSUNIT_ASSERT(!mdata.getInputStuffing());
        TSUNIT_ASSERT(mdata.hasAnyLabel());
        TSUNIT_ASSERT(mdata.hasLabel(out_label));
        out_label = (out_label + 1) % ts::TSPacketLabelSet::SIZE;
        TSUNIT_ASSERT(!mdata.hasLabel(out_label));
    }

    TSUNIT_ASSERT(buf.close(CERR));
    TSUNIT_ASSERT(!buf.isOpen());
}

TSUNIT_DEFINE_TEST(Minimum)
{
    testCommon(2, 2);
}

TSUNIT_DEFINE_TEST(Memory)
{
    testCommon(10, 16);
}

TSUNIT_DEFINE_TEST(File)
{
    testCommon(20, 4);
}
