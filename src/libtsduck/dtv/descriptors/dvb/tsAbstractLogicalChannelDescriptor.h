//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract representation of a logical_channel_descriptor
//!  for different private data specifiers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Abstract representation of a logical_channel_descriptor for different private data specifiers.
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL AbstractLogicalChannelDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Service entry.
        //!
        struct TSDUCKDLL Entry
        {
            uint16_t service_id = 0;  //!< Service id.
            bool     visible = true;  //!< Service is visible. Not always defined, defaults to 1.
            uint16_t lcn = 0;         //!< Logical channel number.
        };

        //!
        //! List of service entries.
        //!
        using EntryList = std::list<Entry>;

        //!
        //! Maximum number of services entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 63;

        // AbstractLogicalChannelDescriptor public members:
        EntryList entries {};  //!< List of service entries.

        // Inherited methods
        DeclareDisplayDescriptor();
        virtual DescriptorDuplication duplicationMode() const override;
        virtual bool merge(const AbstractDescriptor& desc) override;

    protected:
        //!
        //! Protected constructor for subclasses.
        //! @param [in] edid Extended descriptor id.
        //! @param [in] xml_name Descriptor name, as used in XML structures.
        //! @param [in] xml_legacy_name Table or descriptor legacy XML name. Ignored if null pointer.
        //!
        AbstractLogicalChannelDescriptor(EDID edid, const UChar* xml_name, const UChar* xml_legacy_name = nullptr);

        //!
        //! Protected constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //! @param [in] edid Extended descriptor id.
        //! @param [in] xml_name Descriptor name, as used in XML structures.
        //! @param [in] xml_legacy_name Table or descriptor legacy XML name. Ignored if null pointer.
        //!
        AbstractLogicalChannelDescriptor(DuckContext& duck, const Descriptor& bin, EDID edid, const UChar* xml_name, const UChar* xml_legacy_name = nullptr);

        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        AbstractLogicalChannelDescriptor() = delete;
    };
}
