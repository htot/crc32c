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
  1.0  07 May 2016  Ferry Toth - This file just copies the data table from Intels assembly optimized crc_pcl
*/

#include "logging/crc32c.h"
#include <x86intrin.h>

namespace logging
{
__v2di K[] = {
        {0x14cd00bd6, 0x105ec76f0},
        {0x0ba4fc28e, 0x14cd00bd6},
        {0x1d82c63da, 0x0f20c0dfe},
        {0x09e4addf8, 0x0ba4fc28e},
        {0x039d3b296, 0x1384aa63a},
        {0x102f9b8a2, 0x1d82c63da},
        {0x14237f5e6, 0x01c291d04},
        {0x00d3b6092, 0x09e4addf8},
        {0x0c96cfdc0, 0x0740eef02},
        {0x18266e456, 0x039d3b296},
        {0x0daece73e, 0x0083a6eec},
        {0x0ab7aff2a, 0x102f9b8a2},
        {0x1248ea574, 0x1c1733996},
        {0x083348832, 0x14237f5e6},
        {0x12c743124, 0x02ad91c30},
        {0x0b9e02b86, 0x00d3b6092},
        {0x018b33a4e, 0x06992cea2},
        {0x1b331e26a, 0x0c96cfdc0},
        {0x17d35ba46, 0x07e908048},
        {0x1bf2e8b8a, 0x18266e456},
        {0x1a3e0968a, 0x11ed1f9d8},
        {0x0ce7f39f4, 0x0daece73e},
        {0x061d82e56, 0x0f1d0f55e},
        {0x0d270f1a2, 0x0ab7aff2a},
        {0x1c3f5f66c, 0x0a87ab8a8},
        {0x12ed0daac, 0x1248ea574},
        {0x065863b64, 0x08462d800},
        {0x11eef4f8e, 0x083348832},
        {0x1ee54f54c, 0x071d111a8},
        {0x0b3e32c28, 0x12c743124},
        {0x0064f7f26, 0x0ffd852c6},
        {0x0dd7e3b0c, 0x0b9e02b86},
        {0x0f285651c, 0x0dcb17aa4},
        {0x010746f3c, 0x018b33a4e},
        {0x1c24afea4, 0x0f37c5aee},
        {0x0271d9844, 0x1b331e26a},
        {0x08e766a0c, 0x06051d5a2},
        {0x093a5f730, 0x17d35ba46},
        {0x06cb08e5c, 0x11d5ca20e},
        {0x06b749fb2, 0x1bf2e8b8a},
        {0x1167f94f2, 0x021f3d99c},
        {0x0cec3662e, 0x1a3e0968a},
        {0x19329634a, 0x08f158014},
        {0x0e6fc4e6a, 0x0ce7f39f4},
        {0x08227bb8a, 0x1a5e82106},
        {0x0b0cd4768, 0x061d82e56},
        {0x13c2b89c4, 0x188815ab2},
        {0x0d7a4825c, 0x0d270f1a2},
        {0x10f5ff2ba, 0x105405f3e},
        {0x00167d312, 0x1c3f5f66c},
        {0x0f6076544, 0x0e9adf796},
        {0x026f6a60a, 0x12ed0daac},
        {0x1a2adb74e, 0x096638b34},
        {0x19d34af3a, 0x065863b64},
        {0x049c3cc9c, 0x1e50585a0},
        {0x068bce87a, 0x11eef4f8e},
        {0x1524fa6c6, 0x19f1c69dc},
        {0x16cba8aca, 0x1ee54f54c},
        {0x042d98888, 0x12913343e},
        {0x1329d9f7e, 0x0b3e32c28},
        {0x1b1c69528, 0x088f25a3a},
        {0x02178513a, 0x0064f7f26},
        {0x0e0ac139e, 0x04e36f0b0},
        {0x0170076fa, 0x0dd7e3b0c},
        {0x141a1a2e2, 0x0bd6f81f8},
        {0x16ad828b4, 0x0f285651c},
        {0x041d17b64, 0x19425cbba},
        {0x1fae1cc66, 0x010746f3c},
        {0x1a75b4b00, 0x18db37e8a},
        {0x0f872e54c, 0x1c24afea4},
        {0x01e41e9fc, 0x04c144932},
        {0x086d8e4d2, 0x0271d9844},
        {0x160f7af7a, 0x052148f02},
        {0x05bb8f1bc, 0x08e766a0c},
        {0x0a90fd27a, 0x0a3c6f37a},
        {0x0b3af077a, 0x093a5f730},
        {0x04984d782, 0x1d22c238e},
        {0x0ca6ef3ac, 0x06cb08e5c},
        {0x0234e0b26, 0x063ded06a},
        {0x1d88abd4a, 0x06b749fb2},
        {0x04597456a, 0x04d56973c},
        {0x0e9e28eb4, 0x1167f94f2},
        {0x07b3ff57a, 0x19385bf2e},
        {0x0c9c8b782, 0x0cec3662e},
        {0x13a9cba9e, 0x0e417f38a},
        {0x093e106a4, 0x19329634a},
        {0x167001a9c, 0x14e727980},
        {0x1ddffc5d4, 0x0e6fc4e6a},
        {0x00df04680, 0x0d104b8fc},
        {0x02342001e, 0x08227bb8a},
        {0x00a2a8d7e, 0x05b397730},
        {0x168763fa6, 0x0b0cd4768},
        {0x1ed5a407a, 0x0e78eb416},
        {0x0d2c3ed1a, 0x13c2b89c4},
        {0x0995a5724, 0x1641378f0},
        {0x19b1afbc4, 0x0d7a4825c},
        {0x109ffedc0, 0x08d96551c},
        {0x0f2271e60, 0x10f5ff2ba},
        {0x00b0bf8ca, 0x00bf80dd2},
        {0x123888b7a, 0x00167d312},
        {0x1e888f7dc, 0x18dcddd1c},
        {0x002ee03b2, 0x0f6076544},
        {0x183e8d8fe, 0x06a45d2b2},
        {0x133d7a042, 0x026f6a60a},
        {0x116b0f50c, 0x1dd3e10e8},
        {0x05fabe670, 0x1a2adb74e},
        {0x130004488, 0x0de87806c},
        {0x000bcf5f6, 0x19d34af3a},
        {0x18f0c7078, 0x014338754},
        {0x017f27698, 0x049c3cc9c},
        {0x058ca5f00, 0x15e3e77ee},
        {0x1af900c24, 0x068bce87a},
        {0x0b5cfca28, 0x0dd07448e},
        {0x0ded288f8, 0x1524fa6c6},
        {0x059f229bc, 0x1d8048348},
        {0x06d390dec, 0x16cba8aca},
        {0x037170390, 0x0a3e3e02c},
        {0x06353c1cc, 0x042d98888},
        {0x0c4584f5c, 0x0d73c7bea},
        {0x1f16a3418, 0x1329d9f7e},
        {0x0531377e2, 0x185137662},
        {0x1d8d9ca7c, 0x1b1c69528},
        {0x0b25b29f2, 0x18a08b5bc},
        {0x19fb2a8b0, 0x02178513a},
        {0x1a08fe6ac, 0x1da758ae0},
        {0x045cddf4e, 0x0e0ac139e},
        {0x1a91647f2, 0x169cf9eb0},
        {0x1a0f717c4, 0x0170076fa}
};

}
// kate: indent-mode cstyle; indent-width 8; replace-tabs on; 
