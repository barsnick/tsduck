//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Electronic Code Book (ECB) mode.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBlockCipher.h"

namespace ts {
    //!
    //! Electronic Code Book (ECB) mode.
    //! @ingroup libtscore crypto
    //!
    //! No padding is performed. The plain text and cipher text sizes must be
    //! multiples of the block size of the underlying block cipher.
    //!
    //! @tparam CIPHER A subclass of ts::BlockCipher, the underlying block cipher.
    //!
    template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
    class ECB: public CIPHER
    {
        TS_NOCOPY(ECB);
    public:
        //!
        //! Constructor.
        //!
        ECB();

    protected:
        //! Properties of this algorithm.
        //! @return A constant reference to the properties.
        static const BlockCipherProperties& Properties();

        //! Constructor for subclasses which add some properties, such as fixed IV.
        //! @param [in] props Constant reference to a block of properties of this block cipher.
        ECB(const BlockCipherProperties& props);

        // Implementation of BlockCipher interface.
        //! @cond nodoxygen
        virtual bool encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length) override;
        virtual bool decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length) override;
        //! @endcond
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

#if !defined(DOXYGEN)

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
const ts::BlockCipherProperties& ts::ECB<CIPHER>::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(CIPHER::Properties(), u"ECB", false, CIPHER::BLOCK_SIZE, 0, 0);
    return props;
}

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
ts::ECB<CIPHER>::ECB() : CIPHER(ECB::Properties())
{
}

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
ts::ECB<CIPHER>::ECB(const BlockCipherProperties& props) : CIPHER(props)
{
    props.assertCompatibleChaining(ECB::Properties());
}


//----------------------------------------------------------------------------
// Encryption in ECB mode.
// The algorithm is safe with overlapping buffers.
//----------------------------------------------------------------------------

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
bool ts::ECB<CIPHER>::encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
    const size_t bsize = this->properties.block_size;

    if (plain_length % bsize != 0 || cipher_maxsize < plain_length) {
        return false;
    }
    if (cipher_length != nullptr) {
        *cipher_length = plain_length;
    }

    const uint8_t* pt = reinterpret_cast<const uint8_t*>(plain);
    uint8_t* ct = reinterpret_cast<uint8_t*>(cipher);

    while (plain_length > 0) {
        if (!CIPHER::encryptImpl(pt, bsize, ct, bsize, nullptr)) {
            return false;
        }
        ct += bsize;
        pt += bsize;
        plain_length -= bsize;
    }

    return true;
}


//----------------------------------------------------------------------------
// Decryption in ECB mode.
// The algorithm is safe with overlapping buffers.
//----------------------------------------------------------------------------

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
bool ts::ECB<CIPHER>::decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    const size_t bsize = this->properties.block_size;

    if (cipher_length % bsize != 0 || plain_maxsize < cipher_length) {
        return false;
    }
    if (plain_length != nullptr) {
        *plain_length = cipher_length;
    }

    const uint8_t* ct = reinterpret_cast<const uint8_t*>(cipher);
    uint8_t* pt = reinterpret_cast<uint8_t*>(plain);

    while (cipher_length > 0) {
        if (!CIPHER::decryptImpl(ct, bsize, pt, bsize, nullptr)) {
            return false;
        }
        ct += bsize;
        pt += bsize;
        cipher_length -= bsize;
    }

    return true;
}

#endif
