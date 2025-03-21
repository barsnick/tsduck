//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::TSProcessor
//
//----------------------------------------------------------------------------

#include "tsTSProcessor.h"
#include "tsPluginRepository.h"
#include "tsCerrReport.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TSProcessorTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Processing);
};

TSUNIT_REGISTER(TSProcessorTest);


//----------------------------------------------------------------------------
// Testing the use of plugin specific data type during event signalling.
// Probably not useful in many applications, but must be tested.
//----------------------------------------------------------------------------

namespace {
    class TestPluginData : public ts::Object
    {
    public:
        // Public fields
        int data;

        // Constructor
        TestPluginData(int d = 0) :
            data(d)
        {
        }
    };
}


//----------------------------------------------------------------------------
// Internal packet processing plugin class.
// The start and stop methods signal an event.
// The packet processing method signals an even every N packets.
//----------------------------------------------------------------------------

namespace {
    class TestPlugin : ts::ProcessorPlugin
    {
    public:
        // Constructor.
        TestPlugin(ts::TSP*);

        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(ts::TSPacket&, ts::TSPacketMetadata&) override;

        // A factory static method which creates an instance of that class.
        static ts::ProcessorPlugin* CreateInstance(ts::TSP*);

        // Plugin-specific event codes.
        static constexpr uint32_t EVENT_START  = 0xBEEF0001;
        static constexpr uint32_t EVENT_STOP   = 0xBEEF0002;
        static constexpr uint32_t EVENT_PACKET = 0xBEEF0003;

    private:
        // Command line options:
        ts::PacketCounter _count;
    };
}

// Factory method.
ts::ProcessorPlugin* TestPlugin::CreateInstance(ts::TSP* t)
{
    return new TestPlugin(t);
}

// Constructor.
TestPlugin::TestPlugin(ts::TSP* t) :
    ts::ProcessorPlugin(t, u"Test plugin", u"[options]"),
    _count(0)
{
    option(u"count", 'c', POSITIVE);
    help(u"count", u"Send an event every that number of packets.");
}

bool TestPlugin::getOptions()
{
    _count = intValue<ts::PacketCounter>(u"count", 100);
    return true;
}

bool TestPlugin::start()
{
    TestPluginData data(-1);
    tsp->signalPluginEvent(EVENT_START, &data);
    return true;
}

bool TestPlugin::stop()
{
    TestPluginData data(-2);
    tsp->signalPluginEvent(EVENT_STOP, &data);
    return true;
}

TestPlugin::Status TestPlugin::processPacket(ts::TSPacket& pkt, ts::TSPacketMetadata& metadata)
{
    if (tsp->pluginPackets() % _count == 0) {
        TestPluginData data(int(tsp->pluginPackets() / _count));
        tsp->signalPluginEvent(EVENT_PACKET, &data);
    }
    return TSP_OK;
}


//----------------------------------------------------------------------------
// A test plugin event handler.
// We don't do the TSUNIT assertions in the event handler (called in plugin
// thread, under global mutex, exceptions ignored). All events are logged
// into an internal public vector for later test. The assertions are made
// on the log after completion of the processing.
//----------------------------------------------------------------------------

namespace {
    class TestEventHandler : public ts::PluginEventHandlerInterface
    {
    public:
        TestEventHandler();
        virtual void handlePluginEvent(const ts::PluginEventContext& context) override;

        class LogEntry
        {
        public:
            uint32_t          code;
            int               data;
            ts::UString       name;
            size_t            index;
            size_t            count;
            ts::PacketCounter packets;
        };

        std::vector<LogEntry> logs;
    };
}

TestEventHandler::TestEventHandler() :
    logs()
{
    logs.reserve(100);
}

void TestEventHandler::handlePluginEvent(const ts::PluginEventContext& ctx)
{
    // We cannot assert here, we log a debug entry in a buffer.
    // In case of error, messages will not be logged or logged in the wrong order,
    // the post-processing assertions will fail and the problem will be reported at that time.

    if (ctx.pluginData() == nullptr) {
        std::cerr << "***** TestEventHandler::handlePluginEvent: ctx.pluginData() == nullptr";
        return;
    }

    TestPluginData* data = dynamic_cast<TestPluginData*>(ctx.pluginData());
    if (data == nullptr) {
        std::cerr << "***** TestEventHandler::handlePluginEvent: data == nullptr";
        return;
    }

    LogEntry log{ctx.eventCode(), data->data, ctx.pluginName(), ctx.pluginIndex(), ctx.pluginCount(), ctx.pluginPackets()};
    logs.push_back(log);
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(Processing)
{
    // Register our custom plugin with the name "test1".
    ts::PluginRepository::Instance().registerProcessor(u"test1", TestPlugin::CreateInstance);

    // List of preregistered plugins.
    debug() << "TSProcessorTest: pre-registered plugins: " << std::endl
            << "  input: " << ts::UString::Join(ts::PluginRepository::Instance().inputNames()) << std::endl
            << "  output: " << ts::UString::Join(ts::PluginRepository::Instance().outputNames()) << std::endl
            << "  processor names: " << ts::UString::Join(ts::PluginRepository::Instance().processorNames()) << std::endl;

    // Build tsp options.
    ts::TSProcessorArgs opt;
    opt.app_name = u"TSProcessorTest::testProcessing";
    opt.input = {u"null", {u"26"}};
    opt.plugins = {
        {u"test1", {u"--count", u"10"}},
    };
    opt.output = {u"drop"};

    // The TS processing is performed into this object.
    ts::TSProcessor tsproc(CERR);

    // Event handlers.
    TestEventHandler handler1;
    TestEventHandler handler2;

    ts::TSProcessor::Criteria crit;
    crit.event_code = TestPlugin::EVENT_STOP;

    tsproc.registerEventHandler(&handler1);        // all events
    tsproc.registerEventHandler(&handler2, crit);  // stop events only

    // TS processing.
    TSUNIT_ASSERT(tsproc.start(opt));
    tsproc.waitForTermination();

    // All events were reported to handler1.
    TSUNIT_EQUAL(5, handler1.logs.size());

    TSUNIT_EQUAL(0xBEEF0001, handler1.logs[0].code);
    TSUNIT_EQUAL(-1,         handler1.logs[0].data);
    TSUNIT_EQUAL(u"test1",   handler1.logs[0].name);
    TSUNIT_EQUAL(1,          handler1.logs[0].index);
    TSUNIT_EQUAL(3,          handler1.logs[0].count);
    TSUNIT_EQUAL(0,          handler1.logs[0].packets);

    TSUNIT_EQUAL(0xBEEF0003, handler1.logs[1].code);
    TSUNIT_EQUAL(0,          handler1.logs[1].data);
    TSUNIT_EQUAL(u"test1",   handler1.logs[1].name);
    TSUNIT_EQUAL(1,          handler1.logs[1].index);
    TSUNIT_EQUAL(3,          handler1.logs[1].count);
    TSUNIT_EQUAL(0,          handler1.logs[1].packets);

    TSUNIT_EQUAL(0xBEEF0003, handler1.logs[2].code);
    TSUNIT_EQUAL(1,          handler1.logs[2].data);
    TSUNIT_EQUAL(u"test1",   handler1.logs[2].name);
    TSUNIT_EQUAL(1,          handler1.logs[2].index);
    TSUNIT_EQUAL(3,          handler1.logs[2].count);
    TSUNIT_EQUAL(10,         handler1.logs[2].packets);

    TSUNIT_EQUAL(0xBEEF0003, handler1.logs[3].code);
    TSUNIT_EQUAL(2,          handler1.logs[3].data);
    TSUNIT_EQUAL(u"test1",   handler1.logs[3].name);
    TSUNIT_EQUAL(1,          handler1.logs[3].index);
    TSUNIT_EQUAL(3,          handler1.logs[3].count);
    TSUNIT_EQUAL(20,         handler1.logs[3].packets);

    TSUNIT_EQUAL(0xBEEF0002, handler1.logs[4].code);
    TSUNIT_EQUAL(-2,         handler1.logs[4].data);
    TSUNIT_EQUAL(u"test1",   handler1.logs[4].name);
    TSUNIT_EQUAL(1,          handler1.logs[4].index);
    TSUNIT_EQUAL(3,          handler1.logs[4].count);
    TSUNIT_EQUAL(26,         handler1.logs[4].packets);

    // Only stop events were reported to handler2.
    TSUNIT_EQUAL(1, handler2.logs.size());

    TSUNIT_EQUAL(0xBEEF0002, handler2.logs[0].code);
    TSUNIT_EQUAL(-2,         handler2.logs[0].data);
    TSUNIT_EQUAL(u"test1",   handler2.logs[0].name);
    TSUNIT_EQUAL(1,          handler2.logs[0].index);
    TSUNIT_EQUAL(3,          handler2.logs[0].count);
    TSUNIT_EQUAL(26,         handler2.logs[0].packets);
}
