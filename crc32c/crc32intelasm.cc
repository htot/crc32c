// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

// Implementations adapted from Intel's Slicing By 8 Sourceforge Project
// http://sourceforge.net/projects/slicing-by-8/
/*
 * Copyright 2016 Ferry Toth, Exalon Delft BV, The Netherlands
 *
 *
 * This software program is licensed subject to the BSD License,
 * available at http://www.opensource.org/licenses/bsd-license.html.
 *
 * Abstract:
 *
 *  This file is just a C wrapper around Intels assembly optimized crc_pcl
 */

#include "logging/crc32c.h"

namespace logging
{
extern "C" unsigned int crc_pcl ( unsigned char * buffer, int len, unsigned int crc_init );

uint32_t crc32cIntelAsm ( uint32_t crc, const void *buf, size_t len )
{
        return ( unsigned int ) crc_pcl ( ( unsigned char * ) buf, ( int ) len, ( unsigned int ) crc );
}
}
// kate: indent-mode cstyle; indent-width 8; replace-tabs on; 
