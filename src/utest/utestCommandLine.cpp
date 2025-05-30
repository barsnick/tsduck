//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for CommandLine class.
//
//----------------------------------------------------------------------------

#include "tsCommandLine.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class CommandLineTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Command);
    TSUNIT_DECLARE_TEST(CommandFile);
};

TSUNIT_REGISTER(CommandLineTest);


//----------------------------------------------------------------------------
// A command handler which logs its output in a string
//----------------------------------------------------------------------------

namespace {
    class TestCommand : public ts::CommandLineHandler
    {
        TS_NOBUILD_NOCOPY(TestCommand);
    public:
        // Everything is logged here.
        ts::UString output;

        // Constructor.
        TestCommand(ts::CommandLine& cmdline) : output()
        {
            ts::Args* args = cmdline.command(this, &TestCommand::cmd1, u"cmd1");
            args->option(u"foo");

            args = cmdline.command(this, &TestCommand::cmd2, u"cmd2");
            args->option(u"bar");
        }

        // Command handlers.
        ts::CommandStatus cmd1(const ts::UString& command, ts::Args& args)
        {
            output.format(u"[command:%s][--foo:%s]", command, args.present(u"foo"));
            return ts::CommandStatus::SUCCESS;
        }

        ts::CommandStatus cmd2(const ts::UString& command, ts::Args& args)
        {
            output.format(u"[command:%s][--bar:%s]", command, args.present(u"bar"));
            return ts::CommandStatus::SUCCESS;
        }
    };
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(Command)
{
    ts::CommandLine cmdline;
    TestCommand test(cmdline);

    test.output.clear();
    TSUNIT_EQUAL(ts::CommandStatus::SUCCESS, cmdline.processCommand(u"cmd1"));
    TSUNIT_EQUAL(test.output, u"[command:cmd1][--foo:false]");

    test.output.clear();
    TSUNIT_EQUAL(ts::CommandStatus::SUCCESS, cmdline.processCommand(u"cmd1 --foo"));
    TSUNIT_EQUAL(test.output, u"[command:cmd1][--foo:true]");

    test.output.clear();
    TSUNIT_EQUAL(ts::CommandStatus::SUCCESS, cmdline.processCommand(u"cmd2"));
    TSUNIT_EQUAL(test.output, u"[command:cmd2][--bar:false]");

    test.output.clear();
    TSUNIT_EQUAL(ts::CommandStatus::SUCCESS, cmdline.processCommand(u"cmd2 --bar"));
    TSUNIT_EQUAL(test.output, u"[command:cmd2][--bar:true]");
}

TSUNIT_DEFINE_TEST(CommandFile)
{
    ts::CommandLine cmdline;
    TestCommand test(cmdline);

    ts::UStringVector lines({
        u"cmd2",
        u" cmd1  --foo  ",
        u"cmd2 --bar"
    });

    TSUNIT_EQUAL(ts::CommandStatus::SUCCESS, cmdline.processCommands(lines));
    TSUNIT_EQUAL(test.output, u"[command:cmd2][--bar:false][command:cmd1][--foo:true][command:cmd2][--bar:true]");
}
