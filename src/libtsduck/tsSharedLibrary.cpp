//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  Shared library handling (.so on UNIX, DLL on Windows)
//
//----------------------------------------------------------------------------

#include "tsSharedLibrary.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor: Load a shared library
//----------------------------------------------------------------------------

ts::SharedLibrary::SharedLibrary(const UString& filename, bool permanent, Report& report) :
    _report(report),
    _filename(),
    _error(),
    _is_loaded(false),
    _permanent(permanent),
#if defined(TS_WINDOWS)
    _module(0)
#else
    _dl(0)
#endif
{
    if (!filename.empty()) {
        load(filename);
    }
}


//----------------------------------------------------------------------------
// Destructor: Unload the shared library
//----------------------------------------------------------------------------

ts::SharedLibrary::~SharedLibrary()
{
    if (!_permanent) {
        unload();
    }
}


//----------------------------------------------------------------------------
// Try to load an alternate file if the shared library is not yet loaded.
//----------------------------------------------------------------------------

void ts::SharedLibrary::load(const UString& filename)
{
    if (_is_loaded) {
        return; // already loaded
    }

    _filename = filename;
    _report.debug(u"trying to load " + _filename);

#if defined(TS_WINDOWS)
    // Flawfinder: ignore: LoadLibraryEx: Ensure that the full path to the library is specified
    _module = ::LoadLibraryExW(_filename.wc_str(), NULL, 0);
    _is_loaded = _module != 0;
    if (!_is_loaded) {
        _error = ErrorCodeMessage();
    }
#else
    _dl = ::dlopen(_filename.toUTF8().c_str(), RTLD_NOW | RTLD_GLOBAL);
    _is_loaded = _dl != 0;
    if (!_is_loaded) {
        _error = UString::FromUTF8(dlerror());
    }
#endif

    // Normalize error messages
    if (!_is_loaded) {
        if (_error.empty()) {
            _error = u"error loading " + filename;
        }
        else if (_error.find(filename) == UString::NPOS) {
            _error = filename + u": " + _error;
        }
        _report.debug(_error);
    }
}


//----------------------------------------------------------------------------
// Force unload, even if permanent
//----------------------------------------------------------------------------

void ts::SharedLibrary::unload()
{
    if (_is_loaded) {
#if defined(TS_WINDOWS)
        ::FreeLibrary(_module);
#else
        ::dlclose(_dl);
#endif
        _is_loaded = false;
    }
}


//----------------------------------------------------------------------------
// Get the value of a symbol. Return 0 on error.
//----------------------------------------------------------------------------

void* ts::SharedLibrary::getSymbol(const std::string& name) const
{
    if (!_is_loaded) {
        return 0;
    }
    else {
        void* result = 0;
#if defined(TS_WINDOWS)
        result = ::GetProcAddress(_module, name.c_str());
#else
        result = ::dlsym(_dl, name.c_str());
#endif
        if (result == 0) {
            _report.debug(u"symbol %s not found in %s", {name, _filename});
        }
        return result;
    }
}
