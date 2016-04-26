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

#ifndef __LP64__
#define CRC_NATIVE uint32_t
#else
#define CRC_NATIVE uint64_t
#endif

#ifndef __LP64__
#define CRCtriplet(crc, buf, offset) \
    crc ## 0 = __builtin_ia32_crc32si(crc ## 0, *((uint32_t*) buf ## 0 + 2 * offset)); \
    crc ## 1 = __builtin_ia32_crc32si(crc ## 1, *((uint32_t*) buf ## 1 + 2 * offset)); \
    crc ## 2 = __builtin_ia32_crc32si(crc ## 2, *((uint32_t*) buf ## 2 + 2 * offset)); \
    crc ## 0 = __builtin_ia32_crc32si(crc ## 0, *((uint32_t*) buf ## 0 + 1 + 2 * offset)); \
    crc ## 1 = __builtin_ia32_crc32si(crc ## 1, *((uint32_t*) buf ## 1 + 1 + 2 * offset)); \
    crc ## 2 = __builtin_ia32_crc32si(crc ## 2, *((uint32_t*) buf ## 2 + 1 + 2 * offset));
#else
#define CRCtriplet(crc, buf, offset) \
    crc ## 0 = __builtin_ia32_crc32di(crc ## 0, *(buf ## 0 + offset)); \
    crc ## 1 = __builtin_ia32_crc32di(crc ## 1, *(buf ## 1 + offset)); \
    crc ## 2 = __builtin_ia32_crc32di(crc ## 2, *(buf ## 2 + offset));
#endif

#ifndef __LP64__
#define CRCduplet(crc, buf, offset) \
    crc ## 0 = __builtin_ia32_crc32si(crc ## 0, *((uint32_t*) buf ## 0 + 2 * offset)); \
    crc ## 1 = __builtin_ia32_crc32si(crc ## 1, *((uint32_t*) buf ## 1 + 2 * offset)); \
    crc ## 0 = __builtin_ia32_crc32si(crc ## 0, *((uint32_t*) buf ## 0 + 1 + 2 * offset)); \
    crc ## 1 = __builtin_ia32_crc32si(crc ## 1, *((uint32_t*) buf ## 1 + 1 + 2 * offset));
#else
#define CRCduplet(crc, buf, offset) \
    crc ## 0 = __builtin_ia32_crc32di(crc ## 0, *(buf ## 0 + offset)); \
    crc ## 1 = __builtin_ia32_crc32di(crc ## 1, *(buf ## 1 + offset));
#endif


#ifndef __LP64__
#define CRCsinglet(crc, buf, offset) \
    crc = __builtin_ia32_crc32si(crc, *(uint32_t*)(buf + offset)); \
    crc = __builtin_ia32_crc32si(crc, *(uint32_t*)(buf + offset + sizeof(uint32_t)));
#else
#define CRCsinglet(crc, buf, offset) crc = __builtin_ia32_crc32di(crc, *(uint64_t*)(buf + offset));
#endif


/*
 * CombineCRC performs pclmulqdq multiplication of 2 partial CRC's and a well chosen constant 
 * and xor's these with the remaining CRC. I (Ferry Toth) could not find a way to implement this in
 * C, so the 64bit code following here is from Intel. As that code runs only on 64 bit (due to movq
 * instructions), I am providing a 32bit variant that does the same but using movd. The 32bit
 * version keeps intermediate results longer in the xmm registers to do the 2nd xor, then moves the
 * longs in 2 steps for the final crc32l
 * 
*/

#ifndef __LP64__
#define CombineCRC()\
asm volatile (\
"movdqu (%3), %%xmm0\n\t"\
"movd %0, %%xmm1\n\t"\
"pclmullqlqdq %%xmm0, %%xmm1\n\t"\
"movd %2, %%xmm2\n\t"\
"pclmullqhqdq %%xmm0, %%xmm2\n\t"\
"pxor %%xmm2, %%xmm1\n\t"\
"movdqu (%4), %%xmm2\n\t"\
"pxor %%xmm2, %%xmm1\n\t"\
"movd %%xmm1, %0\n\t"\
"crc32l %0, %5\n\t"\
"pextrd $1, %%xmm1, %1\n\t"\
"crc32l %1, %5\n\t"\
"movl %5, %0"\
: "=r" ( crc0 )\
: "0" ( crc0 ), "r" ( crc1 ), "r" ( K + block_size - 1 ), "r" ( ( uint64_t* ) next2 - 1 ), "r" ( crc2 )\
: "%xmm0", "%xmm1", "%xmm2"\
);
#else
#define CombineCRC()\
asm volatile (\
"movdqa (%3), %%xmm0\n\t"\
"movq %0, %%xmm1\n\t"\
"pclmullqlqdq %%xmm0, %%xmm1\n\t"\
"movq %2, %%xmm2\n\t"\
"pclmullqhqdq %%xmm0, %%xmm2\n\t"\
"pxor %%xmm2, %%xmm1\n\t"\
"movq %%xmm1, %0"\
: "=r" ( crc0 ) \
: "0" ( crc0 ), "r" ( crc1 ), "r" ( K + block_size - 1 ) \
: "%xmm0", "%xmm1", "%xmm2"\
); \
crc0 = crc0 ^ * ( ( uint64_t* ) next2 - 1 );\
crc2 = __builtin_ia32_crc32di ( crc2, crc0 );\
crc0 = crc2;
#endif
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
