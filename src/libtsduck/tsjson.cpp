//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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

#include "tsjson.h"
#include "tsjsonNull.h"
#include "tsjsonTrue.h"
#include "tsjsonFalse.h"
#include "tsjsonValue.h"
#include "tsjsonNumber.h"
#include "tsjsonString.h"
#include "tsjsonObject.h"
#include "tsjsonArray.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Parse a JSON value (typically an object or array).
//----------------------------------------------------------------------------

bool ts::json::Parse(ValuePtr& value, const UStringList& lines, Report& report)
{
    TextParser parser(lines, report);
    return Parse(value, parser, true, report);
}

bool ts::json::Parse(ValuePtr& value, const UString& text, Report& report)
{
    TextParser parser(text, report);
    return Parse(value, parser, true, report);
}

bool ts::json::Parse(ValuePtr& value, TextParser& parser, bool jsonOnly, Report& report)
{
    value.clear();

    UString str;
    int64_t intVal;

    // Leading spaces are ignored.
    parser.skipWhiteSpace();

    // Look for one of the seven possible forms or JSON value.
    if (parser.match(u"null", true)) {
        value = new Null;
    }
    else if (parser.match(u"true", true)) {
        value = new True;
    }
    else if (parser.match(u"false", true)) {
        value = new False;
    }
    else if (parser.parseJSONStringLiteral(str)) {
        value = new String(str);
    }
    else if (parser.parseNumericLiteral(str, false, true)) {
        if (str.toInteger(intVal)) {
            value = new Number(intVal);
        }
        else {
            // Invalid integer,
            report.error(u"line %d: JSON floating-point numbers not yet supported, using \"null\" instead", {parser.lineNumber()});
            value = new Null;
        }
    }
    else if (parser.match(u"{", true)) {
        // Parse an object.
        value = new Object;
        // Loop on all fields of the object.
        for (;;) {
            parser.skipWhiteSpace();
            // Exit at end of object
            if (parser.match(u"}", true)) {
                break;
            }
            ValuePtr element;
            if (!parser.parseJSONStringLiteral(str) ||
                !parser.skipWhiteSpace() ||
                !parser.match(u":", true) ||
                !parser.skipWhiteSpace() ||
                !Parse(element, parser, false, report))
            {
                return false;
            }
            // Found field.
            value->add(str, element);
            parser.skipWhiteSpace();
            // Exit at end of object
            if (parser.match(u"}", true)) {
                break;
            }
            // Expect a comma before next field.
            if (!parser.match(u",", true)) {
                report.error(u"line %d: syntax error in JSON object, missing ','", {parser.lineNumber()});
                return false;
            }
        }
    }
    else if (parser.match(u"[", true)) {
        // Parse an array.
        value = new Array;
        // Loop on all elements of the array.
        for (;;) {
            parser.skipWhiteSpace();
            // Exit at end of array.
            if (parser.match(u"]", true)) {
                break;
            }
            ValuePtr element;
            if (!Parse(element, parser, false, report)) {
                return false;
            }
            // Found an element.
            value->set(element);
            parser.skipWhiteSpace();
            // Exit at end of array.
            if (parser.match(u"]", true)) {
                break;
            }
            // Expect a comma before next element
            if (!parser.match(u",", true)) {
                report.error(u"line %d: syntax error in JSON array, missing ','", {parser.lineNumber()});
                return false;
            }
        }
    }
    else {
        report.error(u"line %d: not a valid JSON value", {parser.lineNumber()});
        return false;
    }

    // Process text after the JSON value.
    if (jsonOnly) {
        // Nothing is allowed after the JSON value.
        parser.skipWhiteSpace();
        if (parser.eof()) {
            return true;
        }
        else {
            report.error(u"line %d: extraneous text after JSON value", {parser.lineNumber()});
            return false;
        }
    }
    else {
        // Do not parse further.
        return true;
    }
}
