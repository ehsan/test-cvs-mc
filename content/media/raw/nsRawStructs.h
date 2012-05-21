/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined(nsRawStructs_h_)
#define nsRawStructs_h_

static const PRUint32 RAW_ID = 0x595556;

struct nsRawVideo_PRUint24 {
  operator PRUint32() const { return value[2] << 16 | value[1] << 8 | value[0]; }
  nsRawVideo_PRUint24& operator= (const PRUint32& rhs)
  { value[2] = (rhs & 0x00FF0000) >> 16;
    value[1] = (rhs & 0x0000FF00) >> 8;
    value[0] = (rhs & 0x000000FF);
    return *this; }
private:
  PRUint8 value[3];
};

struct nsRawPacketHeader {
  typedef nsRawVideo_PRUint24 PRUint24;
  PRUint8 packetID;
  PRUint24 codecID;
};

// This is Arc's draft from wiki.xiph.org/OggYUV
struct nsRawVideoHeader {
  typedef nsRawVideo_PRUint24 PRUint24;
  PRUint8 headerPacketID;          // Header Packet ID (always 0)
  PRUint24 codecID;                // Codec identifier (always "YUV")
  PRUint8 majorVersion;            // Version Major (breaks backwards compat)
  PRUint8 minorVersion;            // Version Minor (preserves backwards compat)
  PRUint16 options;                // Bit 1: Color (false = B/W)
                                   // Bits 2-4: Chroma Pixel Shape
                                   // Bit 5: 50% horizontal offset for Cr samples
                                   // Bit 6: 50% vertical ...
                                   // Bits 7-8: Chroma Blending
                                   // Bit 9: Packed (false = Planar)
                                   // Bit 10: Cr Staggered Horizontally
                                   // Bit 11: Cr Staggered Vertically
                                   // Bit 12: Unused (format is always little endian)
                                   // Bit 13: Interlaced (false = Progressive)
                                   // Bits 14-16: Interlace options (undefined)

  PRUint8 alphaChannelBpp;
  PRUint8 lumaChannelBpp;
  PRUint8 chromaChannelBpp;
  PRUint8 colorspace;

  PRUint24 frameWidth;
  PRUint24 frameHeight;
  PRUint24 aspectNumerator;
  PRUint24 aspectDenominator;

  PRUint32 framerateNumerator;
  PRUint32 framerateDenominator;
};

#endif // nsRawStructs_h_