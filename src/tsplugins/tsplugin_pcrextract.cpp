//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Extract PCR's from TS packets.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsBinaryTable.h"
#include "tsSectionDemux.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsSpliceInformationTable.h"
#include "tsRegistrationDescriptor.h"
#include "tsSCTE35.h"
#include "tsFileUtils.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PCRExtractPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(PCRExtractPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Description of one PID carrying PCR, PTS or DTS.
        class PIDContext;
        using PIDContextPtr = std::shared_ptr<PIDContext>;
        using PIDContextMap = std::map<PID,PIDContextPtr>;

        // Description of one PID carrying SCTE 35 splice information.
        class SpliceContext;
        using SpliceContextPtr = std::shared_ptr<SpliceContext>;
        using SpliceContextMap = std::map<PID,SpliceContextPtr>;

        // Command line options:
        fs::path         _output_name {};         // Output file name (empty means stderr)
        PIDSet           _pids {};                // List of PID's to analyze
        UString          _separator {};           // Field separator
        bool             _all_pids = false;       // Analyze all PID's
        bool             _noheader = false;       // Suppress header
        bool             _good_pts_only = false;  // Keep "good" PTS only
        bool             _get_pcr = false;        // Get PCR
        bool             _get_opcr = false;       // Get OPCR
        bool             _get_pts = false;        // Get PTS
        bool             _get_dts = false;        // Get DTS
        bool             _csv_format = false;     // Output in CSV format
        bool             _log_format = false;     // Output in log format
        bool             _evaluate_pcr = false;   // Evaluate PCR offset for packets with PTS/DTS without PCR
        bool             _scte35 = false;         // Detect SCTE 35 PTS values
        bool             _input_time = false;     // Add an input timestamp of the TS packet

        // Working data:
        std::ofstream    _output_stream {};       // Output stream file
        std::ostream*    _output = nullptr;       // Reference to actual output stream file
        PIDContextMap    _stats {};               // Per-PID statistics
        SpliceContextMap _splices {};             // Per-PID splice information
        SectionDemux     _demux {duck, this};     // Section demux for service and SCTE 35 analysis

        // GCC complains that PCR, DTS, PTS shadow the global types of the same name.
        // However this is irrelevant because DataType is an enum class and the declared
        // identifiers must be prefixed. ts::DataType::PCR is an enum identifier, ts::PCR
        // is a type, without ambiguity.
        TS_PUSH_WARNING()
        TS_GCC_NOWARNING(shadow)

        // Types of time stamps.
        enum class DataType {PCR, OPCR, PTS, DTS};
        static const Names _type_names;

        TS_POP_WARNING()

        // Get the subfactor from PCR for a given data type.
        static uint32_t pcrSubfactor(DataType type)
        {
            return (type == DataType::PTS || type == DataType::DTS) ? SYSTEM_CLOCK_SUBFACTOR : 1;
        }

        // Get the number of ticks per millisecond for a given data type.
        static std::intmax_t ticksPerMS(DataType type)
        {
            return ((type == DataType::PTS || type == DataType::DTS) ? PTS::period::den : PCR::period::den) / 1000;
        }

        // Description of one type of data in a PID: PCR, OPCR, PTS, DTS.
        class PIDData
        {
            TS_NOBUILD_NOCOPY(PIDData);
        public:
            PIDData(DataType t) : type(t) {}
            const DataType type;                       // Data type.
            PacketCounter  count = 0;                  // Number of data of this type in this PID.
            uint64_t       first_value = INVALID_PCR;  // First data value of this type in this PID.
            uint64_t       last_value = INVALID_PCR;   // First data value of this type in this PID.
            PacketCounter  last_packet = 0;            // Packet index in TS of last value.
        };

        // Description of one PID carrying PCR, PTS or DTS.
        class PIDContext
        {
            TS_NOBUILD_NOCOPY(PIDContext);
        public:
            PIDContext(PID p) : pid(p) {}
            const PID     pid;                 // PID value.
            PacketCounter packet_count = 0;    // Number of packets in this PID.
            PID           pcr_pid = PID_NULL;  // PID containing PCR in the same service.
            uint64_t      last_good_pts = INVALID_PTS;
            PIDData       pcr {DataType::PCR};
            PIDData       opcr {DataType::OPCR};
            PIDData       pts {DataType::PTS};
            PIDData       dts {DataType::DTS};
        };

        // Description of one PID carrying SCTE 35 splice information.
        class SpliceContext
        {
            TS_NOCOPY(SpliceContext);
        public:
            SpliceContext() = default;
            PIDSet components {};  // All service components for this slice info PID.
        };

        // Implementation of TableHandlerInterface.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Process specific types of tables.
        void processPAT(const PAT&);
        void processPMT(const PMT&);
        void processSpliceCommand(PID pid, SpliceInformationTable&);

        // Get info context for a PID.
        PIDContextPtr getPIDContext(PID);
        SpliceContextPtr getSpliceContext(PID);

        // Report a value in csv or log format.
        void csvHeader();
        void processValue(PIDContext&, PIDData PIDContext::*, uint64_t value, uint64_t pcr, bool report_it, const TSPacketMetadata& mdata);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"pcrextract", ts::PCRExtractPlugin);


//----------------------------------------------------------------------------
// Plugin constructor
//----------------------------------------------------------------------------

ts::PCRExtractPlugin::PCRExtractPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Extracts PCR, OPCR, PTS, DTS from TS packet for analysis", u"[options]")
{
    option(u"csv", 'c');
    help(u"csv",
         u"Report data in CSV (comma-separated values) format. All values are reported "
         u"in decimal. This is the default output format. It is suitable for later "
         u"analysis using tools such as Microsoft Excel.");

    option(u"dts", 'd');
    help(u"dts",
         u"Report Decoding Time Stamps (DTS). By default, if none of --pcr, --opcr, "
         u"--pts, --dts is specified, report them all.");

    option(u"evaluate-pcr-offset", 'e');
    help(u"evaluate-pcr-offset",
         u"Evaluate the offset from the PCR to PTS/DTS for packets with PTS/DTS but without PCR. "
         u"This evaluation may be incorrect if the bitrate is not constant or incorrectly estimated. "
         u"By default, the offset is reported only for packets containing a PTS/DTS and a PCR.");

    option(u"good-pts-only", 'g');
    help(u"good-pts-only",
         u"Keep only \"good\" PTS, ie. PTS which have a higher value than the "
         u"previous good PTS. This eliminates PTS from out-of-sequence B-frames.");

    option(u"input-timestamp", 'i');
    help(u"input-timestamp",
         u"Add an input timestamp of the corresponding TS packet, if available. "
         u"This can be an RTP, SRT, kernel timestamp. It is always converted in PCR units.");

    option(u"log", 'l');
    help(u"log",
         u"Report data in \"log\" format through the standard tsp logging system. "
         u"All values are reported in hexadecimal.");

    option(u"noheader", 'n');
    help(u"noheader",
         u"Do not output initial header line in CSV format.");

    option(u"opcr");
    help(u"opcr",
         u"Report Original Program Clock References (OPCR). By default, if none of "
         u"--pcr, --opcr, --pts, --dts is specified, report them all.");

    option(u"output-file", 'o', FILENAME);
    help(u"output-file", u"filename",
         u"Output file name for CSV reporting (standard error by default).");

    option(u"pcr");
    help(u"pcr",
         u"Report Program Clock References (PCR). By default, if none of --pcr, "
         u"--opcr, --pts, --dts is specified, report them all.");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"Specifies PID's to analyze. By default, all PID's are analyzed. "
         u"Several --pid options may be specified.");

    option(u"pts");
    help(u"pts",
         u"Report Presentation Time Stamps (PTS). By default, if none of --pcr, "
         u"--opcr, --pts, --dts is specified, report them all.");

    option(u"scte35");
    help(u"scte35",
         u"Detect and report PTS in SCTE 35 commands. Imply --log and --pts. "
         u"If no --pid option is specified, detect all SCTE 35 PID's. "
         u"If some --pid option is specified, report only SCTE PID's "
         u"which are synchronized with the specified --pid options.");

    option(u"separator", 's', STRING);
    help(u"separator", u"string",
         u"Field separator string in CSV output (default: '" + UString(DEFAULT_CSV_SEPARATOR) + u"').");
}


//----------------------------------------------------------------------------
// Substructures constructors
//----------------------------------------------------------------------------

const ts::Names ts::PCRExtractPlugin::_type_names({
    {u"PCR",  DataType::PCR},
    {u"OPCR", DataType::OPCR},
    {u"DTS",  DataType::DTS},
    {u"PTS",  DataType::PTS}
});


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::PCRExtractPlugin::getOptions()
{
    // Get command line options.
    getIntValues(_pids, u"pid", true);
    getPathValue(_output_name, u"output-file");
    getValue(_separator, u"separator", DEFAULT_CSV_SEPARATOR);
    _all_pids = !present(u"pid");
    _noheader = present(u"noheader");
    _scte35 = present(u"scte35");
    _good_pts_only = present(u"good-pts-only");
    _get_pts = present(u"pts") || _scte35;
    _get_dts = present(u"dts");
    _get_pcr = present(u"pcr");
    _get_opcr = present(u"opcr");
    _evaluate_pcr = present(u"evaluate-pcr-offset");
    _csv_format = present(u"csv") || !_output_name.empty();
    _log_format = present(u"log") || _scte35;
    _input_time = present(u"input-timestamp");

    if (!_get_pts && !_get_dts && !_get_pcr && !_get_opcr) {
        // Report them all by default
        _get_pts = _get_dts = _get_pcr = _get_opcr = true;
    }

    if (!_csv_format && !_log_format) {
        // Use CSV format by default.
        _csv_format = true;
    }

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PCRExtractPlugin::start()
{
    // Reset state
    _stats.clear();
    _splices.clear();
    _demux.reset();
    _demux.addPID(PID_PAT);

    // Create the output file if there is one
    if (_output_name.empty()) {
        _output = &std::cerr;
    }
    else {
        _output = &_output_stream;
        _output_stream.open(_output_name);
        if (!_output_stream) {
            error(u"cannot create file %s", _output_name);
            return false;
        }
    }

    // Output header
    csvHeader();
    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::PCRExtractPlugin::stop()
{
    if (!_output_name.empty()) {
        _output_stream.close();
    }
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::PCRExtractPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();

    // Go through section demux.
    _demux.feedPacket(pkt);

    // When all PID's are analyzed, detect SCTE 35 is all PID's, regardless of PSI.
    if (_scte35 && _all_pids && !_demux.hasPID(pid) && pkt.getPUSI()) {
        // Check if this packet contains the start of an SCTE command.
        const size_t hs = pkt.getHeaderSize();
        // Index in packet of first table id (header plus pointer field):
        const size_t ti = hs + 1 + (hs < PKT_SIZE ? pkt.b[hs] : 0);
        if (ti < PKT_SIZE && pkt.b[ti] == TID_SCTE35_SIT) {
            // Make sure the splice informations are processed.
            getSpliceContext(pid);
        }
    }

    // Get context for this PID.
    PIDContext& pc(*getPIDContext(pid));

    // Get PCR from packet, if there is one.
    uint64_t pcr = pkt.getPCR();
    const bool has_pcr = pcr != INVALID_PCR;

    // Note that we must keep track in PCR in all PID's, not only PID's to display,
    // because a PID to display may need a PCR reference in another PID.
    if (!has_pcr && _evaluate_pcr && pc.pcr_pid != PID_NULL) {
        // No PCR in the packet, evaluate its theoretical value.
        // Get context of associated PCR PID.
        PIDContext& pcrpid(*getPIDContext(pc.pcr_pid));
        // Compute theoretical PCR at this point in the TS.
        // Note that NextPCR() return INVALID_PCR if last_pcr or bitrate is incorrect.
        pcr = NextPCR(pcrpid.pcr.last_value, tsp->pluginPackets() - pcrpid.pcr.last_packet, tsp->bitrate());
    }

    // Check if we must analyze and display this PID.
    if (_pids.test(pid)) {

        if (has_pcr) {
            processValue(pc, &PIDContext::pcr, pcr, INVALID_PCR, _get_pcr, pkt_data);
        }

        if (pkt.hasOPCR()) {
            processValue(pc, &PIDContext::opcr, pkt.getOPCR(), pcr, _get_opcr, pkt_data);
        }

        if (pkt.hasPTS()) {
            const uint64_t pts = pkt.getPTS();
            // Check if this is a "good" PTS, ie. greater than the last good PTS
            // (or wrapping around the max PTS value 2**33)
            const bool good_pts = pc.pts.count == 0 || SequencedPTS(pc.last_good_pts, pts);
            if (good_pts) {
                pc.last_good_pts = pts;
            }
            processValue(pc, &PIDContext::pts, pts, pcr, _get_pts && (good_pts || !_good_pts_only), pkt_data);
        }

        if (pkt.hasDTS()) {
            processValue(pc, &PIDContext::dts, pkt.getDTS(), pcr, _get_dts, pkt_data);
        }

        pc.packet_count++;
    }

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Report a CSV header. Must be consistent with processValue() below.
//----------------------------------------------------------------------------

void ts::PCRExtractPlugin::csvHeader()
{
    if (_csv_format && !_noheader) {
        *_output << "PID" << _separator
                 << "Packet index in TS" << _separator
                 << "Packet index in PID" << _separator
                 << "Type" << _separator
                 << "Count in PID" << _separator
                 << "Value" << _separator
                 << "Value offset in PID" << _separator
                 << "Offset from PCR";
        if (_input_time) {
            *_output << _separator
                     << "Input timestamp" << _separator
                     << "Input source" << _separator
                     << "Input offset";
        }
        *_output << std::endl;
    }
}


//----------------------------------------------------------------------------
// Report a value in CSV and/or log format.
//----------------------------------------------------------------------------

void ts::PCRExtractPlugin::processValue(PIDContext& ctx, PIDData PIDContext::* pdata, uint64_t value, uint64_t pcr, bool report_it, const TSPacketMetadata& mdata)
{
    PIDData& data(ctx.*pdata);
    const UString name(_type_names.name(data.type));
    const uint32_t pcr_subfactor = pcrSubfactor(data.type);
    const std::intmax_t ticks = ticksPerMS(data.type);

    // Count values and remember first value.
    if (data.count++ == 0) {
        data.first_value = value;
    }

    // Time offset since first value of this type in the PID.
    const uint64_t since_start = value - data.first_value;
    const int64_t since_previous = data.last_value == INVALID_PCR ? 0 : int64_t(value) - int64_t(data.last_value);

    // Report in CSV format.
    if (_csv_format && report_it) {
        *_output << ctx.pid << _separator
                 << tsp->pluginPackets() << _separator
                 << ctx.packet_count << _separator
                 << name << _separator
                 << data.count << _separator
                 << value << _separator
                 << since_start << _separator;
        if (pcr != INVALID_PCR) {
            *_output << (int64_t(value) - int64_t(pcr / pcr_subfactor));
        }
        if (_input_time) {
            *_output << _separator;
            if (mdata.hasInputTimeStamp()) {
                *_output << mdata.getInputTimeStamp().count();
            }
            *_output << _separator;
            if (mdata.hasInputTimeStamp()) {
                *_output << TimeSourceEnum().name(mdata.getInputTimeSource()).toLower();
            }
            *_output << _separator;
            if (mdata.hasInputTimeStamp()) {
                *_output << (int64_t(value) - int64_t(mdata.getInputTimeStamp().count() / pcr_subfactor));
            }
        }
        *_output << std::endl;
    }

    // Report in log format.
    if (_log_format && report_it) {
        UString trailer;
        if (_input_time && mdata.hasInputTimeStamp()) {
            trailer.format(u", input: 0x%011X", mdata.getInputTimeStamp().count());
        }
        // Number of hexa digits: 11 for PCR (42 bits) and 9 for PTS/DTS (33 bits).
        const size_t width = pcr_subfactor == 1 ? 11 : 9;
        info(u"PID: %n, %s: 0x%0*X, (0x%0*X, %'d ms from start of PID, %'d ms from previous)%s",
                  ctx.pid, name, width, value, width, since_start, since_start / ticks, since_previous / ticks, trailer);
    }

    // Remember last value.
    data.last_value = value;
    data.last_packet = tsp->pluginPackets();
}


//----------------------------------------------------------------------------
// Get or create PID context.
//----------------------------------------------------------------------------

ts::PCRExtractPlugin::PIDContextPtr ts::PCRExtractPlugin::getPIDContext(PID pid)
{
    PIDContextPtr& pc(_stats[pid]);
    if (pc == nullptr) {
        pc = std::make_shared<PIDContext>(pid);
    }
    return pc;
}


//----------------------------------------------------------------------------
// Get splice info context from the splice info PID.
//----------------------------------------------------------------------------

ts::PCRExtractPlugin::SpliceContextPtr ts::PCRExtractPlugin::getSpliceContext(PID pid)
{
    SpliceContextPtr& pc(_splices[pid]);
    if (pc == nullptr) {
        // Found a new splicing info PID.
        pc = std::make_shared<SpliceContext>();

        // Add this PID to the demux.
        _demux.addPID(pid);
        verbose(u"Found SCTE 35 info PID %n", pid);
    }
    return pc;
}


//----------------------------------------------------------------------------
// Implementation of TableHandlerInterface.
//----------------------------------------------------------------------------

void ts::PCRExtractPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {
        case TID_PAT: {
            const PAT pat(duck, table);
            if (pat.isValid()) {
                processPAT(pat);
            }
            break;
        }
        case TID_PMT: {
            const PMT pmt(duck, table);
            if (pmt.isValid()) {
                processPMT(pmt);
            }
            break;
        }
        case TID_SCTE35_SIT: {
            SpliceInformationTable sit(duck, table);
            if (sit.isValid()) {
                processSpliceCommand(table.sourcePID(), sit);
            }
            break;
        }
        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Process a PAT.
//----------------------------------------------------------------------------

void ts::PCRExtractPlugin::processPAT(const PAT& pat)
{
    // Add all PMT PID's to the demux.
    for (const auto& it : pat.pmts) {
        _demux.addPID(it.second);
    }
}


//----------------------------------------------------------------------------
// Process a PMT.
//----------------------------------------------------------------------------

void ts::PCRExtractPlugin::processPMT(const PMT& pmt)
{
    // SCTE 35 requests a registration descriptor in the program info loop.
    bool scte35_found = false;
    if (_scte35) {
        for (size_t index = pmt.descs.search(DID_MPEG_REGISTRATION); !scte35_found && index < pmt.descs.count(); index = pmt.descs.search(DID_MPEG_REGISTRATION, index + 1)) {
            const RegistrationDescriptor reg(duck, pmt.descs[index]);
            scte35_found = reg.isValid() && reg.format_identifier == SPLICE_ID_CUEI;
        }
    }

    // Detect all service PID's and all potential SCTE 35 PID's.
    PIDSet servicePIDs;
    PIDSet splicePIDs;
    for (const auto& it : pmt.streams) {
        const PID pid = it.first;

        // Associate a PCR PID with all PID's in the service.
        getPIDContext(pid)->pcr_pid = pmt.pcr_pid;

        // Track all components and splice information PID's in the service.
        if (_scte35) {
            if (it.second.stream_type == ST_SCTE35_SPLICE) {
                // This is a PID carrying splice information.
                splicePIDs.set(pid);
                scte35_found = true;
            }
            else {
                // This is a regular component of the service.
                servicePIDs.set(pid);
            }
        }
    }

    // Now, we know all components and all splice info PID's.
    if (scte35_found) {
        for (PID pid = 0; pid < splicePIDs.size(); ++pid) {
            if (splicePIDs.test(pid)) {
                // Add components which are associated with this splice info PID.
                getSpliceContext(pid)->components |= servicePIDs;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Process an SCTE 35 command
//----------------------------------------------------------------------------

void ts::PCRExtractPlugin::processSpliceCommand(PID pid, SpliceInformationTable& sit)
{
    // Adjust PTS values in splice command.
    sit.adjustPTS();

    // Get context for this splice PID.
    const SpliceContextPtr pc(getSpliceContext(pid));

    // Get the highest PTS from all associated components.
    uint64_t service_pts = INVALID_PTS;
    for (PID comp_pid = 0; comp_pid < pc->components.size(); ++comp_pid) {
        if (pc->components.test(comp_pid)) {
            const auto it = _stats.find(comp_pid);
            if (it != _stats.end()) {
                // PCR or PTS were found in this component.
                const uint64_t comp_pts = it->second->last_good_pts;
                if (comp_pts != 0 && (service_pts == INVALID_PTS || comp_pts > service_pts)) {
                    service_pts = comp_pts;
                }
            }
        }
    }

    // Get the lowest PTS in the splice command.
    const uint64_t command_pts = sit.splice_command_type == SPLICE_INSERT ? sit.splice_insert.lowestPTS() : INVALID_PTS;

    // Start of message.
    UString msg(UString::Format(u"PID: %n, SCTE 35 command %s", pid, NameFromSection(u"dtv", u"SpliceCommandType", sit.splice_command_type)));
    if (sit.splice_command_type == SPLICE_INSERT) {
        if (sit.splice_insert.canceled) {
            msg += u" canceled";
        }
        else {
            msg += sit.splice_insert.splice_out ? u" out" : u" in";
            if (sit.splice_insert.immediate) {
                msg += u" immediate";
            }
        }
    }
    // Add service PTS if there is one.
    if (service_pts != INVALID_PTS) {
        // No PTS in command but we know the last PTS in the service.
        msg += UString::Format(u", at PTS 0x%09X in service", service_pts);
    }

    // Add command PTS if there is one.
    if (command_pts != INVALID_PTS) {
        msg += UString::Format(u", exec at PTS 0x%09X", command_pts);
        if (service_pts != INVALID_PTS && service_pts < command_pts) {
            // Add real time difference.
            msg += u", in ";
            msg += UString::Chrono(cn::duration_cast<cn::milliseconds>(ts::PTS(command_pts - service_pts)), true);
        }
    }

    // Finally report the message.
    info(msg);
}
