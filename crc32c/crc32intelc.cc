// Copyright 2016 Ferry Toth, Exalon Delft BV, The Netherlands
/*
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Ferry Toth
  ftoth@exalondelft.nl
*/

/* Use hardware CRC instruction on Intel SSE 4.2 processors.  This computes a
  CRC-32C, *not* the CRC-32 used by Ethernet and zip, gzip, etc. Where efficient
  3 crc32q instructions are used which a single core can execute in parallel.
  This compensates for the latency of a single crc32q instruction. Combining the 
  3 CRC-32C bytes is done using the pclmulqdq instruction, which has overhead of
  its own, and makes this code path only efficient for buffer sizes above 216 bytes. 
  All code requiring a crc32q instruction is done inside a macro, for which alternative
  code is generated in case of a 32 bit platform.
  
  This code is a port of Intels crc_iscsi_v_pcl.asm assembly code (which is part of
  this project as well as in a modified form the linux kernel) and reaches the same 
  throughput on 64bit platforms. The main advantage of this port is that it was
  relatively easy to port to 32bit platforms (like Intel Edison which currently has
  only 32bit support). Being written in C it is of course easier to maintain and possibly
  optimize further */

/* Version history:
  1.0  07 May 2016  Ferry Toth - First version
*/

#include "logging/crc32c.h"
#include "logging/crc32intelc.h"
#include <x86intrin.h>

namespace logging
{

extern __v2di K[];

/* Compute CRC-32C using the Intel hardware instruction. */
uint32_t crc32cIntelC ( uint32_t crc, const void *buf, size_t len )
{
        const unsigned char *next = ( const unsigned char * ) buf;
        unsigned long count;
        CRC_NATIVE crc0, crc1, crc2;
        crc0 = crc;

        if ( len >= 8 ) {
                // if len > 216 then align and use triplets
                if ( len > 216 ) {
                        {
                                uint32_t crc32bit = crc0;                                       // create this block actually prevent 2 asignments
                                unsigned long align = ( 8 - ( uintptr_t ) next ) % 8;           // byte to boundary
                                len -= align;
                                if ( align & 0x04 ) {
                                        crc32bit = __builtin_ia32_crc32si ( crc32bit, * ( uint32_t* ) next );
                                        next += sizeof(uint32_t);
                                };
                                if ( align & 0x02 ) {
                                        crc32bit = __builtin_ia32_crc32hi ( crc32bit, * ( uint16_t* ) next );
                                        next += sizeof(uint16_t);
                                };

                                if ( align & 0x01 ) {
                                        crc32bit = __builtin_ia32_crc32qi ( crc32bit, * ( next ) );
                                        next++;
                                };
                                crc0 = crc32bit;
                        };

                        // use Duff's device, a for() loop inside a switch() statement. This is Legal
                        // needs to execute at least once, round len down to nearast triplet multiple
                        count = len / 24;			// number of triplets
                        len %= 24;				// bytes remaining
                        unsigned long n = count / 128;		// #blocks = first block + full blocks
                        unsigned long block_size = count % 128;
                        if ( block_size == 0 ) {
                                block_size = 128;
                        } else {
                                n++;
                        };
                        const uint64_t *next0 = ( uint64_t* ) next + block_size; // points to the first byte of the next block
                        const uint64_t *next1 = next0 + block_size;
                        const uint64_t *next2 = next1 + block_size;

                        crc1 = crc2 = 0;
                        switch ( block_size ) {
                        case 128:
                                do {
                                        CRCtriplet ( crc, next, -128 );	// jumps here for a full block of len 128
                                case 127:
                                        CRCtriplet ( crc, next, -127 );	// jumps here or below for the first block smaller
                                case 126:
                                        CRCtriplet ( crc, next, -126 );	// than 128
                                case 125:
                                        CRCtriplet ( crc, next, -125 );
                                case 124:
                                        CRCtriplet ( crc, next, -124 );
                                case 123:
                                        CRCtriplet ( crc, next, -123 );
                                case 122:
                                        CRCtriplet ( crc, next, -122 );
                                case 121:
                                        CRCtriplet ( crc, next, -121 );
                                case 120:
                                        CRCtriplet ( crc, next, -120 );
                                case 119:
                                        CRCtriplet ( crc, next, -119 );
                                case 118:
                                        CRCtriplet ( crc, next, -118 );
                                case 117:
                                        CRCtriplet ( crc, next, -117 );
                                case 116:
                                        CRCtriplet ( crc, next, -116 );
                                case 115:
                                        CRCtriplet ( crc, next, -115 );
                                case 114:
                                        CRCtriplet ( crc, next, -114 );
                                case 113:
                                        CRCtriplet ( crc, next, -113 );
                                case 112:
                                        CRCtriplet ( crc, next, -112 );
                                case 111:
                                        CRCtriplet ( crc, next, -111 );
                                case 110:
                                        CRCtriplet ( crc, next, -110 );
                                case 109:
                                        CRCtriplet ( crc, next, -109 );
                                case 108:
                                        CRCtriplet ( crc, next, -108 );
                                case 107:
                                        CRCtriplet ( crc, next, -107 );
                                case 106:
                                        CRCtriplet ( crc, next, -106 );
                                case 105:
                                        CRCtriplet ( crc, next, -105 );
                                case 104:
                                        CRCtriplet ( crc, next, -104 );
                                case 103:
                                        CRCtriplet ( crc, next, -103 );
                                case 102:
                                        CRCtriplet ( crc, next, -102 );
                                case 101:
                                        CRCtriplet ( crc, next, -101 );
                                case 100:
                                        CRCtriplet ( crc, next, -100 );
                                case 99:
                                        CRCtriplet ( crc, next, -99 );
                                case 98:
                                        CRCtriplet ( crc, next, -98 );
                                case 97:
                                        CRCtriplet ( crc, next, -97 );
                                case 96:
                                        CRCtriplet ( crc, next, -96 );
                                case 95:
                                        CRCtriplet ( crc, next, -95 );
                                case 94:
                                        CRCtriplet ( crc, next, -94 );
                                case 93:
                                        CRCtriplet ( crc, next, -93 );
                                case 92:
                                        CRCtriplet ( crc, next, -92 );
                                case 91:
                                        CRCtriplet ( crc, next, -91 );
                                case 90:
                                        CRCtriplet ( crc, next, -90 );
                                case 89:
                                        CRCtriplet ( crc, next, -89 );
                                case 88:
                                        CRCtriplet ( crc, next, -88 );
                                case 87:
                                        CRCtriplet ( crc, next, -87 );
                                case 86:
                                        CRCtriplet ( crc, next, -86 );
                                case 85:
                                        CRCtriplet ( crc, next, -85 );
                                case 84:
                                        CRCtriplet ( crc, next, -84 );
                                case 83:
                                        CRCtriplet ( crc, next, -83 );
                                case 82:
                                        CRCtriplet ( crc, next, -82 );
                                case 81:
                                        CRCtriplet ( crc, next, -81 );
                                case 80:
                                        CRCtriplet ( crc, next, -80 );
                                case 79:
                                        CRCtriplet ( crc, next, -79 );
                                case 78:
                                        CRCtriplet ( crc, next, -78 );
                                case 77:
                                        CRCtriplet ( crc, next, -77 );
                                case 76:
                                        CRCtriplet ( crc, next, -76 );
                                case 75:
                                        CRCtriplet ( crc, next, -75 );
                                case 74:
                                        CRCtriplet ( crc, next, -74 );
                                case 73:
                                        CRCtriplet ( crc, next, -73 );
                                case 72:
                                        CRCtriplet ( crc, next, -72 );
                                case 71:
                                        CRCtriplet ( crc, next, -71 );
                                case 70:
                                        CRCtriplet ( crc, next, -70 );
                                case 69:
                                        CRCtriplet ( crc, next, -69 );
                                case 68:
                                        CRCtriplet ( crc, next, -68 );
                                case 67:
                                        CRCtriplet ( crc, next, -67 );
                                case 66:
                                        CRCtriplet ( crc, next, -66 );
                                case 65:
                                        CRCtriplet ( crc, next, -65 );
                                case 64:
                                        CRCtriplet ( crc, next, -64 );
                                case 63:
                                        CRCtriplet ( crc, next, -63 );
                                case 62:
                                        CRCtriplet ( crc, next, -62 );
                                case 61:
                                        CRCtriplet ( crc, next, -61 );
                                case 60:
                                        CRCtriplet ( crc, next, -60 );
                                case 59:
                                        CRCtriplet ( crc, next, -59 );
                                case 58:
                                        CRCtriplet ( crc, next, -58 );
                                case 57:
                                        CRCtriplet ( crc, next, -57 );
                                case 56:
                                        CRCtriplet ( crc, next, -56 );
                                case 55:
                                        CRCtriplet ( crc, next, -55 );
                                case 54:
                                        CRCtriplet ( crc, next, -54 );
                                case 53:
                                        CRCtriplet ( crc, next, -53 );
                                case 52:
                                        CRCtriplet ( crc, next, -52 );
                                case 51:
                                        CRCtriplet ( crc, next, -51 );
                                case 50:
                                        CRCtriplet ( crc, next, -50 );
                                case 49:
                                        CRCtriplet ( crc, next, -49 );
                                case 48:
                                        CRCtriplet ( crc, next, -48 );
                                case 47:
                                        CRCtriplet ( crc, next, -47 );
                                case 46:
                                        CRCtriplet ( crc, next, -46 );
                                case 45:
                                        CRCtriplet ( crc, next, -45 );
                                case 44:
                                        CRCtriplet ( crc, next, -44 );
                                case 43:
                                        CRCtriplet ( crc, next, -43 );
                                case 42:
                                        CRCtriplet ( crc, next, -42 );
                                case 41:
                                        CRCtriplet ( crc, next, -41 );
                                case 40:
                                        CRCtriplet ( crc, next, -40 );
                                case 39:
                                        CRCtriplet ( crc, next, -39 );
                                case 38:
                                        CRCtriplet ( crc, next, -38 );
                                case 37:
                                        CRCtriplet ( crc, next, -37 );
                                case 36:
                                        CRCtriplet ( crc, next, -36 );
                                case 35:
                                        CRCtriplet ( crc, next, -35 );
                                case 34:
                                        CRCtriplet ( crc, next, -34 );
                                case 33:
                                        CRCtriplet ( crc, next, -33 );
                                case 32:
                                        CRCtriplet ( crc, next, -32 );
                                case 31:
                                        CRCtriplet ( crc, next, -31 );
                                case 30:
                                        CRCtriplet ( crc, next, -30 );
                                case 29:
                                        CRCtriplet ( crc, next, -29 );
                                case 28:
                                        CRCtriplet ( crc, next, -28 );
                                case 27:
                                        CRCtriplet ( crc, next, -27 );
                                case 26:
                                        CRCtriplet ( crc, next, -26 );
                                case 25:
                                        CRCtriplet ( crc, next, -25 );
                                case 24:
                                        CRCtriplet ( crc, next, -24 );
                                case 23:
                                        CRCtriplet ( crc, next, -23 );
                                case 22:
                                        CRCtriplet ( crc, next, -22 );
                                case 21:
                                        CRCtriplet ( crc, next, -21 );
                                case 20:
                                        CRCtriplet ( crc, next, -20 );
                                case 19:
                                        CRCtriplet ( crc, next, -19 );
                                case 18:
                                        CRCtriplet ( crc, next, -18 );
                                case 17:
                                        CRCtriplet ( crc, next, -17 );
                                case 16:
                                        CRCtriplet ( crc, next, -16 );
                                case 15:
                                        CRCtriplet ( crc, next, -15 );
                                case 14:
                                        CRCtriplet ( crc, next, -14 );
                                case 13:
                                        CRCtriplet ( crc, next, -13 );
                                case 12:
                                        CRCtriplet ( crc, next, -12 );
                                case 11:
                                        CRCtriplet ( crc, next, -11 );
                                case 10:
                                        CRCtriplet ( crc, next, -10 );
                                case 9:
                                        CRCtriplet ( crc, next, -9 );
                                case 8:
                                        CRCtriplet ( crc, next, -8 );
                                case 7:
                                        CRCtriplet ( crc, next, -7 );
                                case 6:
                                        CRCtriplet ( crc, next, -6 );
                                case 5:
                                        CRCtriplet ( crc, next, -5 );
                                case 4:
                                        CRCtriplet ( crc, next, -4 );
                                case 3:
                                        CRCtriplet ( crc, next, -3 );
                                case 2:
                                        CRCtriplet ( crc, next, -2 );
                                case 1:
                                        CRCduplet ( crc, next, -1 );		        // the final triplet is actually only 2
                                        CombineCRC();
                                        if ( --n > 0 ) {
                                                crc1 = crc2 = 0;
                                                block_size = 128;
                                                next0 = next2 + 128;			// points to the first byte of the next block
                                                next1 = next0 + 128;			// from here on all blocks are 128 long
                                                next2 = next1 + 128;
                                        };
                                case 0:
                                        ;
                                } while ( n > 0 );
                        };
                        next = ( const unsigned char* ) next2;
                };
                unsigned count = len / 8;                                               // 216 of less bytes is 27 or less singlets
                len %= 8;
                next += ( count * 8 );
                switch ( count ) {
                case 27:
                        CRCsinglet ( crc0, next, -27 * 8 );
                case 26:
                        CRCsinglet ( crc0, next, -26 * 8 );
                case 25:
                        CRCsinglet ( crc0, next, -25 * 8 );
                case 24:
                        CRCsinglet ( crc0, next, -24 * 8 );
                case 23:
                        CRCsinglet ( crc0, next, -23 * 8 );
                case 22:
                        CRCsinglet ( crc0, next, -22 * 8 );
                case 21:
                        CRCsinglet ( crc0, next, -21 * 8 );
                case 20:
                        CRCsinglet ( crc0, next, -20 * 8 );
                case 19:
                        CRCsinglet ( crc0, next, -19 * 8 );
                case 18:
                        CRCsinglet ( crc0, next, -18 * 8 );
                case 17:
                        CRCsinglet ( crc0, next, -17 * 8 );
                case 16:
                        CRCsinglet ( crc0, next, -16 * 8 );
                case 15:
                        CRCsinglet ( crc0, next, -15 * 8 );
                case 14:
                        CRCsinglet ( crc0, next, -14 * 8 );
                case 13:
                        CRCsinglet ( crc0, next, -13 * 8 );
                case 12:
                        CRCsinglet ( crc0, next, -12 * 8 );
                case 11:
                        CRCsinglet ( crc0, next, -11 * 8 );
                case 10:
                        CRCsinglet ( crc0, next, -10 * 8 );
                case 9:
                        CRCsinglet ( crc0, next, -9 * 8 );
                case 8:
                        CRCsinglet ( crc0, next, -8 * 8 );
                case 7:
                        CRCsinglet ( crc0, next, -7 * 8 );
                case 6:
                        CRCsinglet ( crc0, next, -6 * 8 );
                case 5:
                        CRCsinglet ( crc0, next, -5 * 8 );
                case 4:
                        CRCsinglet ( crc0, next, -4 * 8 );
                case 3:
                        CRCsinglet ( crc0, next, -3 * 8 );
                case 2:
                        CRCsinglet ( crc0, next, -2 * 8 );
                case 1:
                        CRCsinglet ( crc0, next, -1 * 8 );
                case 0:
                        ;
                };

        };
        {
                uint32_t crc32bit = crc0;
                // less than 8 bytes remain
                /* compute the crc for up to seven trailing bytes */
                if ( len & 0x04 ) {
                        crc32bit = __builtin_ia32_crc32si ( crc32bit, * ( uint32_t* ) next );
                        next += 4;
                };
                if ( len & 0x02 ) {
                        crc32bit = __builtin_ia32_crc32hi ( crc32bit, * ( uint16_t* ) next );
                        next += 2;
                };

                if ( len & 0x01 ) {
                        crc32bit = __builtin_ia32_crc32qi ( crc32bit, * ( next ) );
                };
                return ( uint32_t ) crc32bit;
        };
};

}  // namespace logging
// kate: indent-mode cstyle; indent-width 8; replace-tabs on; 
