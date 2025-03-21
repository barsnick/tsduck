//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractDuplicateRemapPlugin.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::AbstractDuplicateRemapPlugin::AbstractDuplicateRemapPlugin(bool remap, TSP* tsp_, const UString& description, const UString& syntax) :
    ProcessorPlugin(tsp_, description, syntax),
    _remap(remap),
    _noun(remap ? u"remapping" : u"duplication"),
    _verb(remap ? u"remap" : u"duplicate"),
    _verbed(remap ? u"remapped" : u"duplicated"),
    _verbing(remap ? u"remapping" : u"duplicating")
{

    option(u"");
    help(u"",
         u"Each " + _noun + u" is specified as \"pid=newpid\" or \"pid1-pid2=newpid\" "
         u"(all PID's can be specified as decimal or hexadecimal values). "
         u"In the first form, the PID \"pid\" is " + _verbed + u" to \"newpid\". "
         u"In the latter form, all PID's within the range \"pid1\" to \"pid2\" "
         u"(inclusive) are respectively " + _verbed + u" to \"newpid\", \"newpid\"+1, etc. "
         u"This behaviour can be changed using option --single. "
         u"The null PID 0x1FFF cannot be " + _verbed + u".");

    option(u"single", 's');
    help(u"single",
         u"When a " + _noun + u" is in the form \"pid1-pid2=newpid\", " + _verb + u" all input PID's "
         u"to the same \"newpid\" value, not \"newpid\", \"newpid\"+1, etc. "
         u"This option forces --unchecked since distinct PID's are " + _verbed + u" to the same one.");

    option(u"unchecked", 'u');
    help(u"unchecked",
         u"Do not perform any consistency checking while " + _verbing + u" PID's; " +
         _verbing + u" two PID's to the same PID or to a PID which is "
         u"already present in the input is accepted. "
         u"Note that this option should be used with care since the "
         u"resulting stream can be illegal or inconsistent.");

    option(u"set-label", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketLabelSet::MAX);
    help(u"set-label", u"label1[-label2]",
         u"Set the specified labels on the " + _verbed + u" packets. "
         u"Several --set-label options may be specified.");

    option(u"reset-label", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketLabelSet::MAX);
    help(u"reset-label", u"label1[-label2]",
         u"Clear the specified labels on the " + _verbed + u" packets. "
         u"Several --reset-label options may be specified.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::AbstractDuplicateRemapPlugin::getOptions()
{
    const bool single = present(u"single");
    _unchecked = single || present(u"unchecked");
    getIntValues(_setLabels, u"set-label");
    getIntValues(_resetLabels, u"reset-label");

    _pidMap.clear();
    _newPIDs.reset();

    // Decode all PID duplications/remappings.
    for (size_t i = 0; i < count(u""); ++i) {

        // Get parameter: pid[-pid]=newpid
        const UString param(value(u"", u"", i));

        // Decode PID values
        PID pid1 = PID_NULL;
        PID pid2 = PID_NULL;
        PID newpid = PID_NULL;

        if (param.scan(u"%d=%d", &pid1, &newpid)) {
            // Simple form.
            pid2 = pid1;
        }
        else if (!param.scan(u"%d-%d=%d", &pid1, &pid2, &newpid)) {
            error(u"invalid PID %s specification: %s", _noun, param);
            return false;
        }

        if (pid1 > pid2 || pid2 >= PID_NULL || newpid > PID_NULL || (!single && newpid + pid2 - pid1 > PID_NULL)) {
            error(u"invalid PID %s values in %s", _noun, param);
            return false;
        }

        // Skip void remapping (duplication is never void).
        if (_remap && pid1 == newpid && (pid2 == pid1 || !single)) {
            continue;
        }

        // Remember each PID remapping/duplication.
        while (pid1 <= pid2) {
            debug(u"%s PID %n to %n", _verbing, pid1, newpid);

            // Check that we don't remap/duplicate the same PID twice on distinct taget PID's.
            // Ignore --unchecked since this is always inconsistent.
            const auto it = _pidMap.find(pid1);
            if (it != _pidMap.end() && it->second != newpid) {
                error(u"PID %n %s twice", pid1, _verbed);
                return false;
            }

            // Remember PID mapping.
            _pidMap.insert(std::make_pair(pid1, newpid));

            // Remember output PID's
            if (!_unchecked && _newPIDs.test(newpid)) {
                error(u"duplicated output PID %n", newpid);
                return false;
            }
            _newPIDs.set(newpid);

            ++pid1;
            if (!single) {
                ++newpid;
            }
        }
    }

    return true;
}
