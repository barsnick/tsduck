//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::TSPacket
//
//----------------------------------------------------------------------------

#include "tsTSPacket.h"
#include "tsByteBlock.h"
#include "tsMemory.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TSPacketTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Packet);
    TSUNIT_DECLARE_TEST(Construction);
    TSUNIT_DECLARE_TEST(Init);
    TSUNIT_DECLARE_TEST(CreatePCR);
    TSUNIT_DECLARE_TEST(AFStuffingSize);
    TSUNIT_DECLARE_TEST(SetPayloadSize);
    TSUNIT_DECLARE_TEST(Flags);
    TSUNIT_DECLARE_TEST(PrivateData);
    TSUNIT_DECLARE_TEST(BitRate);
    TSUNIT_DECLARE_TEST(PCR);
};

TSUNIT_REGISTER(TSPacketTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(Packet)
{
    ts::TSPacket::SanityCheck();

    ts::NullPacket.display(debug(), ts::TSPacket::DUMP_TS_HEADER | ts::TSPacket::DUMP_RAW);

    ts::TSPacket packets[7];
    TS_ZERO(packets); // to avoid unreferenced or uninitialized warning

    TSUNIT_EQUAL(7 * ts::PKT_SIZE, sizeof(packets));
}

TSUNIT_DEFINE_TEST(Construction)
{
    // Test aggregate initialization.
    ts::TSPacket p1 = {{
        // Header: PID 0x1FFF
        0x47, 0x1F, 0xFF, 0x10,
        // Payload: 184 bytes
        4, 5, 6, 7, 8, 9,
        10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
        30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
        50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
        70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
        90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
        110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
        130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
        150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
        170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183
    }};

    TSUNIT_ASSERT(p1.hasValidSync());
    TSUNIT_ASSERT(p1.hasPayload());
    TSUNIT_EQUAL(184, p1.getPayloadSize());
    for (uint8_t i = 4; i < 184; i++) {
        TSUNIT_EQUAL(i, p1.b[i]);
    }

    // Test copy constructor (implicit).
    ts::TSPacket p2(p1);
    TSUNIT_ASSERT(p2.hasValidSync());
    TSUNIT_ASSERT(p2.hasPayload());
    TSUNIT_EQUAL(184, p2.getPayloadSize());
    for (uint8_t i = 4; i < 184; i++) {
        TSUNIT_EQUAL(i, p2.b[i]);
    }

    // Test assignment operator (implicit).
    ts::TSPacket p3;
    p3 = p1;
    TSUNIT_ASSERT(p3.hasValidSync());
    TSUNIT_ASSERT(p3.hasPayload());
    TSUNIT_EQUAL(184, p3.getPayloadSize());
    for (uint8_t i = 4; i < 184; i++) {
        TSUNIT_EQUAL(i, p3.b[i]);
    }
}

TSUNIT_DEFINE_TEST(Init)
{
    ts::TSPacket pkt;
    pkt.init(0x1ABC, 7, 0x35);
    TSUNIT_ASSERT(pkt.hasValidSync());
    TSUNIT_ASSERT(!pkt.hasAF());
    TSUNIT_ASSERT(pkt.hasPayload());
    TSUNIT_EQUAL(7, pkt.getCC());
    TSUNIT_EQUAL(0x1ABC, pkt.getPID());
    TSUNIT_EQUAL(184, pkt.getPayloadSize());
    for (size_t i = 4; i < ts::PKT_SIZE; ++i) {
        TSUNIT_EQUAL(0x35, pkt.b[i]);
    }
}

TSUNIT_DEFINE_TEST(CreatePCR)
{
    ts::TSPacket pkt;
    pkt.init(0x1ABC);

    TSUNIT_ASSERT(pkt.hasValidSync());
    TSUNIT_ASSERT(!pkt.hasAF());
    TSUNIT_ASSERT(pkt.hasPayload());
    TSUNIT_EQUAL(0x1ABC, pkt.getPID());
    TSUNIT_EQUAL(184, pkt.getPayloadSize());
    TSUNIT_ASSERT(!pkt.hasPCR());
    TSUNIT_EQUAL(ts::INVALID_PCR, pkt.getPCR());

    TSUNIT_ASSERT(!pkt.setPCR(0x000000126789ABCD, false));

    TSUNIT_ASSERT(!pkt.hasAF());
    TSUNIT_EQUAL(184, pkt.getPayloadSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_ASSERT(!pkt.hasPCR());
    TSUNIT_EQUAL(ts::INVALID_PCR, pkt.getPCR());

    TSUNIT_ASSERT(pkt.setPCR(0x000000126789ABCD, true));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(8, pkt.getAFSize());
    TSUNIT_ASSERT(pkt.hasPayload());
    TSUNIT_EQUAL(176, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.hasPCR());
    TSUNIT_EQUAL(0x000000126789ABCD, pkt.getPCR());

    TSUNIT_ASSERT(pkt.setPCR(0x0000023456789ABC, true));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(8, pkt.getAFSize());
    TSUNIT_ASSERT(pkt.hasPayload());
    TSUNIT_EQUAL(176, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.hasPCR());
    TSUNIT_EQUAL(0x0000023456789ABC, pkt.getPCR());

    pkt.removePCR();

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(176, pkt.getPayloadSize());
    TSUNIT_EQUAL(8, pkt.getAFSize());
    TSUNIT_EQUAL(6, pkt.getAFStuffingSize());
    TSUNIT_ASSERT(!pkt.hasPCR());

    TSUNIT_ASSERT(pkt.setPCR(0x00000089642CA4F7, true));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(8, pkt.getAFSize());
    TSUNIT_ASSERT(pkt.hasPayload());
    TSUNIT_EQUAL(176, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.hasPCR());
    TSUNIT_ASSERT(!pkt.hasOPCR());
    TSUNIT_ASSERT(!pkt.hasSpliceCountdown());
    TSUNIT_EQUAL(0x00000089642CA4F7, pkt.getPCR());
    TSUNIT_EQUAL(ts::INVALID_PCR, pkt.getOPCR());
    TSUNIT_EQUAL(0, pkt.getSpliceCountdown());

    TSUNIT_ASSERT(!pkt.setSpliceCountdown(23, false));
    TSUNIT_ASSERT(pkt.setSpliceCountdown(-97, true));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(9, pkt.getAFSize());
    TSUNIT_ASSERT(pkt.hasPayload());
    TSUNIT_EQUAL(175, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.hasPCR());
    TSUNIT_ASSERT(!pkt.hasOPCR());
    TSUNIT_ASSERT(pkt.hasSpliceCountdown());
    TSUNIT_EQUAL(0x00000089642CA4F7, pkt.getPCR());
    TSUNIT_EQUAL(ts::INVALID_PCR, pkt.getOPCR());
    TSUNIT_EQUAL(-97, pkt.getSpliceCountdown());

    TSUNIT_ASSERT(pkt.setOPCR(0x000000B964FEA456, true));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(15, pkt.getAFSize());
    TSUNIT_ASSERT(pkt.hasPayload());
    TSUNIT_EQUAL(169, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.hasPCR());
    TSUNIT_ASSERT(pkt.hasOPCR());
    TSUNIT_ASSERT(pkt.hasSpliceCountdown());
    TSUNIT_EQUAL(0x00000089642CA4F7, pkt.getPCR());
    TSUNIT_EQUAL(0x000000B964FEA456, pkt.getOPCR());
    TSUNIT_EQUAL(-97, int(pkt.getSpliceCountdown()));
}

TSUNIT_DEFINE_TEST(AFStuffingSize)
{
    ts::TSPacket pkt;

    pkt.init();
    TSUNIT_ASSERT(!pkt.hasAF());
    TSUNIT_EQUAL(0, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());

    TSUNIT_ASSERT(pkt.setPCR(0, true));
    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(8, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());

    pkt.b[4] += 25;
    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(33, pkt.getAFSize());
    TSUNIT_EQUAL(25, pkt.getAFStuffingSize());
}

TSUNIT_DEFINE_TEST(SetPayloadSize)
{
    ts::TSPacket pkt;

    pkt.init();
    TSUNIT_ASSERT(!pkt.hasAF());
    TSUNIT_EQUAL(0, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(184, pkt.getPayloadSize());

    TSUNIT_ASSERT(pkt.setPayloadSize(100));
    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(84, pkt.getAFSize());
    TSUNIT_EQUAL(82, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(100, pkt.getPayloadSize());

    TSUNIT_ASSERT(pkt.setPayloadSize(130));
    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(54, pkt.getAFSize());
    TSUNIT_EQUAL(52, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(130, pkt.getPayloadSize());

    TSUNIT_ASSERT(!pkt.setPayloadSize(190));
    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(54, pkt.getAFSize());
    TSUNIT_EQUAL(52, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(130, pkt.getPayloadSize());

    pkt.init();
    TSUNIT_ASSERT(pkt.setPCR(0, true));
    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(8, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(176, pkt.getPayloadSize());

    TSUNIT_ASSERT(pkt.setPayloadSize(100));
    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(84, pkt.getAFSize());
    TSUNIT_EQUAL(76, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(100, pkt.getPayloadSize());

    // Test shift of payload.
    uint8_t* pl = pkt.getPayload();
    pl[0] = 0x10;
    pl[1] = 0x11;
    pl[2] = 0x12;
    pl[3] = 0x13;
    pl[4] = 0x14;
    pl[5] = 0x15;

    TSUNIT_ASSERT(pkt.setPayloadSize(99, true));
    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(85, pkt.getAFSize());
    TSUNIT_EQUAL(77, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(99, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.getPayload() == pl + 1);
    TSUNIT_EQUAL(0x10, pkt.getPayload()[0]);
    TSUNIT_EQUAL(0x11, pkt.getPayload()[1]);
    TSUNIT_EQUAL(0x12, pkt.getPayload()[2]);
    TSUNIT_EQUAL(0x13, pkt.getPayload()[3]);
    TSUNIT_EQUAL(0x14, pkt.getPayload()[4]);
    TSUNIT_EQUAL(0x15, pkt.getPayload()[5]);

    TSUNIT_ASSERT(pkt.setPayloadSize(98, false));
    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(86, pkt.getAFSize());
    TSUNIT_EQUAL(78, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(98, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.getPayload() == pl + 2);
    TSUNIT_EQUAL(0x11, pkt.getPayload()[0]);
    TSUNIT_EQUAL(0x12, pkt.getPayload()[1]);
    TSUNIT_EQUAL(0x13, pkt.getPayload()[2]);
    TSUNIT_EQUAL(0x14, pkt.getPayload()[3]);
    TSUNIT_EQUAL(0x15, pkt.getPayload()[4]);
}

TSUNIT_DEFINE_TEST(Flags)
{
    ts::TSPacket pkt;
    pkt.init();

    uint8_t* pl = pkt.getPayload();
    pl[0] = 0x10;
    pl[1] = 0x11;
    pl[2] = 0x12;
    pl[3] = 0x13;
    pl[4] = 0x14;
    pl[5] = 0x15;

    TSUNIT_ASSERT(!pkt.hasAF());
    TSUNIT_EQUAL(0, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(184, pkt.getPayloadSize());

    TSUNIT_ASSERT(!pkt.getDiscontinuityIndicator());
    TSUNIT_ASSERT(!pkt.getRandomAccessIndicator());
    TSUNIT_ASSERT(!pkt.getESPI());

    TSUNIT_ASSERT(!pkt.setDiscontinuityIndicator(false));
    TSUNIT_ASSERT(!pkt.hasAF());
    TSUNIT_EQUAL(0, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(184, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.getPayload() == pl);
    TSUNIT_EQUAL(0x10, pkt.getPayload()[0]);

    TSUNIT_ASSERT(!pkt.getDiscontinuityIndicator());
    TSUNIT_ASSERT(!pkt.getRandomAccessIndicator());
    TSUNIT_ASSERT(!pkt.getESPI());

    TSUNIT_ASSERT(pkt.setDiscontinuityIndicator(true));
    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(2, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(182, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.getPayload() == pl + 2);
    TSUNIT_EQUAL(0x10, pkt.getPayload()[0]);

    TSUNIT_ASSERT(pkt.getDiscontinuityIndicator());
    TSUNIT_ASSERT(!pkt.getRandomAccessIndicator());
    TSUNIT_ASSERT(!pkt.getESPI());

    pkt.clearDiscontinuityIndicator();
    TSUNIT_ASSERT(pkt.setRandomAccessIndicator(true));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(2, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(182, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.getPayload() == pl + 2);
    TSUNIT_EQUAL(0x10, pkt.getPayload()[0]);

    TSUNIT_ASSERT(!pkt.getDiscontinuityIndicator());
    TSUNIT_ASSERT(pkt.getRandomAccessIndicator());
    TSUNIT_ASSERT(!pkt.getESPI());

    pkt.clearRandomAccessIndicator();
    TSUNIT_ASSERT(pkt.setESPI(true));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_EQUAL(2, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(182, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.getPayload() == pl + 2);
    TSUNIT_EQUAL(0x10, pkt.getPayload()[0]);

    TSUNIT_ASSERT(!pkt.getDiscontinuityIndicator());
    TSUNIT_ASSERT(!pkt.getRandomAccessIndicator());
    TSUNIT_ASSERT(pkt.getESPI());
}

TSUNIT_DEFINE_TEST(PrivateData)
{
    ts::ByteBlock data;
    ts::TSPacket pkt;
    pkt.init();

    TSUNIT_ASSERT(!pkt.hasAF());
    TSUNIT_ASSERT(!pkt.hasPrivateData());
    TSUNIT_EQUAL(0, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getPrivateDataSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(184, pkt.getPayloadSize());

    const ts::ByteBlock refPayload({0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29});
    const ts::ByteBlock refPrivate1({0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59});
    const ts::ByteBlock refPrivate2({0x60, 0x61, 0x62});

    uint8_t* pl = pkt.getPayload();
    ts::MemCopy(pl, refPayload.data(), refPayload.size());

    TSUNIT_ASSERT(!pkt.setPrivateData(refPrivate1, false));

    TSUNIT_ASSERT(!pkt.hasAF());
    TSUNIT_ASSERT(!pkt.hasPrivateData());
    TSUNIT_EQUAL(0, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getPrivateDataSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(184, pkt.getPayloadSize());

    TSUNIT_ASSERT(pkt.setPrivateData(refPrivate1, true));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_ASSERT(pkt.hasPrivateData());
    TSUNIT_EQUAL(13, pkt.getAFSize());
    TSUNIT_EQUAL(10, pkt.getPrivateDataSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(171, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.getPayload() == pl + 13);
    TSUNIT_EQUAL(0, ts::MemCompare(pkt.getPayload(), refPayload.data(), refPayload.size()));
    TSUNIT_EQUAL(ts::UString::Dump(refPrivate1, ts::UString::SINGLE_LINE),
                 ts::UString::Dump(pkt.getPrivateData(), pkt.getPrivateDataSize(), ts::UString::SINGLE_LINE));
    TSUNIT_EQUAL(0, ts::MemCompare(pkt.getPrivateData(), refPrivate1.data(), refPrivate1.size()));
    pkt.getPrivateData(data);
    TSUNIT_ASSERT(data == refPrivate1);

    TSUNIT_ASSERT(pkt.setPrivateData(refPrivate2, false));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_ASSERT(pkt.hasPrivateData());
    TSUNIT_EQUAL(13, pkt.getAFSize());
    TSUNIT_EQUAL(3, pkt.getPrivateDataSize());
    TSUNIT_EQUAL(7, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(171, pkt.getPayloadSize());
    TSUNIT_ASSERT(pkt.getPayload() == pl + 13);
    TSUNIT_EQUAL(0, ts::MemCompare(pkt.getPayload(), refPayload.data(), refPayload.size()));
    TSUNIT_EQUAL(ts::UString::Dump(refPrivate2, ts::UString::SINGLE_LINE),
                 ts::UString::Dump(pkt.getPrivateData(), pkt.getPrivateDataSize(), ts::UString::SINGLE_LINE));
    TSUNIT_EQUAL(0, ts::MemCompare(pkt.getPrivateData(), refPrivate2.data(), refPrivate2.size()));
    pkt.getPrivateData(data);
    TSUNIT_ASSERT(data == refPrivate2);

    TSUNIT_ASSERT(pkt.setPCR(0x000000126789ABCD, false));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_ASSERT(pkt.hasPCR());
    TSUNIT_ASSERT(pkt.hasPrivateData());
    TSUNIT_EQUAL(13, pkt.getAFSize());
    TSUNIT_EQUAL(3, pkt.getPrivateDataSize());
    TSUNIT_EQUAL(1, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(171, pkt.getPayloadSize());
    TSUNIT_EQUAL(0x000000126789ABCD, pkt.getPCR());
    TSUNIT_ASSERT(pkt.getPayload() == pl + 13);
    TSUNIT_EQUAL(0, ts::MemCompare(pkt.getPayload(), refPayload.data(), refPayload.size()));
    TSUNIT_EQUAL(ts::UString::Dump(refPrivate2, ts::UString::SINGLE_LINE),
                 ts::UString::Dump(pkt.getPrivateData(), pkt.getPrivateDataSize(), ts::UString::SINGLE_LINE));
    TSUNIT_EQUAL(0, ts::MemCompare(pkt.getPrivateData(), refPrivate2.data(), refPrivate2.size()));
    data.clear();
    pkt.getPrivateData(data);
    TSUNIT_ASSERT(data == refPrivate2);

    TSUNIT_ASSERT(!pkt.setOPCR(0x000000AB67925678, false));
    TSUNIT_ASSERT(pkt.setOPCR(0x000000AB67925678, true));

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_ASSERT(pkt.hasPCR());
    TSUNIT_ASSERT(pkt.hasOPCR());
    TSUNIT_ASSERT(pkt.hasPrivateData());
    TSUNIT_EQUAL(18, pkt.getAFSize());
    TSUNIT_EQUAL(3, pkt.getPrivateDataSize());
    TSUNIT_EQUAL(0, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(166, pkt.getPayloadSize());
    TSUNIT_EQUAL(0x000000126789ABCD, pkt.getPCR());
    TSUNIT_EQUAL(0x000000AB67925678, pkt.getOPCR());
    TSUNIT_ASSERT(pkt.getPayload() == pl + 18);
    TSUNIT_EQUAL(0, ts::MemCompare(pkt.getPayload(), refPayload.data(), refPayload.size()));
    TSUNIT_EQUAL(ts::UString::Dump(refPrivate2, ts::UString::SINGLE_LINE),
                                  ts::UString::Dump(pkt.getPrivateData(), pkt.getPrivateDataSize(), ts::UString::SINGLE_LINE));
    TSUNIT_EQUAL(0, ts::MemCompare(pkt.getPrivateData(), refPrivate2.data(), refPrivate2.size()));
    data.clear();
    pkt.getPrivateData(data);
    TSUNIT_ASSERT(data == refPrivate2);

    pkt.removePrivateData();

    TSUNIT_ASSERT(pkt.hasAF());
    TSUNIT_ASSERT(pkt.hasPCR());
    TSUNIT_ASSERT(pkt.hasOPCR());
    TSUNIT_ASSERT(!pkt.hasPrivateData());
    TSUNIT_EQUAL(18, pkt.getAFSize());
    TSUNIT_EQUAL(0, pkt.getPrivateDataSize());
    TSUNIT_EQUAL(4, pkt.getAFStuffingSize());
    TSUNIT_EQUAL(166, pkt.getPayloadSize());
    TSUNIT_EQUAL(0x000000126789ABCD, pkt.getPCR());
    TSUNIT_EQUAL(0x000000AB67925678, pkt.getOPCR());
    TSUNIT_ASSERT(pkt.getPayload() == pl + 18);
    TSUNIT_EQUAL(0, ts::MemCompare(pkt.getPayload(), refPayload.data(), refPayload.size()));
    pkt.getPrivateData(data);
    TSUNIT_ASSERT(data.empty());
}

TSUNIT_DEFINE_TEST(BitRate)
{
    TSUNIT_EQUAL(8 * 188 * 1000, ts::PacketBitRate(1000, cn::seconds(1)).toInt64());
    TSUNIT_EQUAL(8 * 188 * 1000, ts::PacketBitRate(1000, cn::milliseconds(1000)).toInt64());

    TSUNIT_EQUAL(1000, ts::PacketDistance(8 * 188 * 1000, cn::seconds(1)));
    TSUNIT_EQUAL(1000, ts::PacketDistance(8 * 188 * 1000, cn::milliseconds(1000)));

    using tq = cn::duration<std::intmax_t, std::ratio<3,4>>;
    TSUNIT_EQUAL(8 * 188 * 1000, ts::PacketBitRate(3000, tq(4)).toInt64());
    TSUNIT_EQUAL(3000, ts::PacketDistance(8 * 188 * 1000, tq(4)));

    cn::milliseconds ms = cn::milliseconds(2500);
    TSUNIT_EQUAL(25, std::chrono::duration_cast<ts::deciseconds>(ms).count());

    ts::BitRate br = 14'800'000;
    ts::PacketCounter pk = 200;
    debug() << "TSPacketTest::testBitRate: intervals: "
            << ts::PacketInterval<ts::PCR>(br, pk).count() << "  "
            << ts::PacketInterval(br, pk).count()
            << std::endl;
}

TSUNIT_DEFINE_TEST(PCR)
{
    TSUNIT_EQUAL(1100, ts::AddPCR(1000, 100));
    TSUNIT_EQUAL(900, ts::AddPCR(1000, -100));
    TSUNIT_EQUAL(10, ts::AddPCR(ts::PCR_SCALE - 90, 100));
    TSUNIT_EQUAL(ts::PCR_SCALE - 90, ts::AddPCR(10, -100));
    TSUNIT_EQUAL(ts::INVALID_PCR, ts::AddPCR(ts::PCR_SCALE, 100));
}
