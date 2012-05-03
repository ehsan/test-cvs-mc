/* Copyright (C) 2003-2008 Jean-Marc Valin
   Copyright (C) 2007-2009 Xiph.Org Foundation */
/**
   @file fixed_debug.h
   @brief Fixed-point operations with debugging
*/
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef FIXED_DEBUG_H
#define FIXED_DEBUG_H

#include <stdio.h>

#ifdef CELT_C
long long celt_mips=0;
#else
extern long long celt_mips;
#endif

#define MULT16_16SU(a,b) ((opus_val32)(opus_val16)(a)*(opus_val32)(opus_uint16)(b))
#define MULT32_32_Q31(a,b) ADD32(ADD32(SHL32(MULT16_16(SHR32((a),16),SHR((b),16)),1), SHR32(MULT16_16SU(SHR32((a),16),((b)&0x0000ffff)),15)), SHR32(MULT16_16SU(SHR32((b),16),((a)&0x0000ffff)),15))

/** 16x32 multiplication, followed by a 16-bit shift right. Results fits in 32 bits */
#define MULT16_32_Q16(a,b) ADD32(MULT16_16((a),SHR32((b),16)), SHR32(MULT16_16SU((a),((b)&0x0000ffff)),16))

#define QCONST16(x,bits) ((opus_val16)(.5+(x)*(((opus_val32)1)<<(bits))))
#define QCONST32(x,bits) ((opus_val32)(.5+(x)*(((opus_val32)1)<<(bits))))

#define VERIFY_SHORT(x) ((x)<=32767&&(x)>=-32768)
#define VERIFY_INT(x) ((x)<=2147483647LL&&(x)>=-2147483648LL)
#define VERIFY_UINT(x) ((x)<=(2147483647LLU<<1))

#define SHR(a,b) SHR32(a,b)
#define PSHR(a,b) PSHR32(a,b)

static inline short NEG16(int x)
{
   int res;
   if (!VERIFY_SHORT(x))
   {
      fprintf (stderr, "NEG16: input is not short: %d\n", (int)x);
   }
   res = -x;
   if (!VERIFY_SHORT(res))
      fprintf (stderr, "NEG16: output is not short: %d\n", (int)res);
   celt_mips++;
   return res;
}
static inline int NEG32(long long x)
{
   long long res;
   if (!VERIFY_INT(x))
   {
      fprintf (stderr, "NEG16: input is not int: %d\n", (int)x);
   }
   res = -x;
   if (!VERIFY_INT(res))
      fprintf (stderr, "NEG16: output is not int: %d\n", (int)res);
   celt_mips+=2;
   return res;
}

#define EXTRACT16(x) EXTRACT16_(x, __FILE__, __LINE__)
static inline short EXTRACT16_(int x, char *file, int line)
{
   int res;
   if (!VERIFY_SHORT(x))
   {
      fprintf (stderr, "EXTRACT16: input is not short: %d in %s: line %d\n", x, file, line);
   }
   res = x;
   celt_mips++;
   return res;
}

#define EXTEND32(x) EXTEND32_(x, __FILE__, __LINE__)
static inline int EXTEND32_(int x, char *file, int line)
{
   int res;
   if (!VERIFY_SHORT(x))
   {
      fprintf (stderr, "EXTEND32: input is not short: %d in %s: line %d\n", x, file, line);
   }
   res = x;
   celt_mips++;
   return res;
}

#define SHR16(a, shift) SHR16_(a, shift, __FILE__, __LINE__)
static inline short SHR16_(int a, int shift, char *file, int line)
{
   int res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(shift))
   {
      fprintf (stderr, "SHR16: inputs are not short: %d >> %d in %s: line %d\n", a, shift, file, line);
   }
   res = a>>shift;
   if (!VERIFY_SHORT(res))
      fprintf (stderr, "SHR16: output is not short: %d in %s: line %d\n", res, file, line);
   celt_mips++;
   return res;
}
#define SHL16(a, shift) SHL16_(a, shift, __FILE__, __LINE__)
static inline short SHL16_(int a, int shift, char *file, int line)
{
   int res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(shift))
   {
      fprintf (stderr, "SHL16: inputs are not short: %d %d in %s: line %d\n", a, shift, file, line);
   }
   res = a<<shift;
   if (!VERIFY_SHORT(res))
      fprintf (stderr, "SHL16: output is not short: %d in %s: line %d\n", res, file, line);
   celt_mips++;
   return res;
}

static inline int SHR32(long long a, int shift)
{
   long long  res;
   if (!VERIFY_INT(a) || !VERIFY_SHORT(shift))
   {
      fprintf (stderr, "SHR32: inputs are not int: %d %d\n", (int)a, shift);
   }
   res = a>>shift;
   if (!VERIFY_INT(res))
   {
      fprintf (stderr, "SHR32: output is not int: %d\n", (int)res);
   }
   celt_mips+=2;
   return res;
}
static inline int SHL32(long long a, int shift)
{
   long long  res;
   if (!VERIFY_INT(a) || !VERIFY_SHORT(shift))
   {
      fprintf (stderr, "SHL32: inputs are not int: %d %d\n", (int)a, shift);
   }
   res = a<<shift;
   if (!VERIFY_INT(res))
   {
      fprintf (stderr, "SHL32: output is not int: %d\n", (int)res);
   }
   celt_mips+=2;
   return res;
}

#define PSHR32(a,shift) (celt_mips--,SHR32(ADD32((a),(((opus_val32)(1)<<((shift))>>1))),shift))
#define VSHR32(a, shift) (((shift)>0) ? SHR32(a, shift) : SHL32(a, -(shift)))

#define ROUND16(x,a) (celt_mips--,EXTRACT16(PSHR32((x),(a))))
#define HALF16(x)  (SHR16(x,1))
#define HALF32(x)  (SHR32(x,1))

//#define SHR(a,shift) ((a) >> (shift))
//#define SHL(a,shift) ((a) << (shift))

#define ADD16(a, b) ADD16_(a, b, __FILE__, __LINE__)
static inline short ADD16_(int a, int b, char *file, int line)
{
   int res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "ADD16: inputs are not short: %d %d in %s: line %d\n", a, b, file, line);
   }
   res = a+b;
   if (!VERIFY_SHORT(res))
   {
      fprintf (stderr, "ADD16: output is not short: %d+%d=%d in %s: line %d\n", a,b,res, file, line);
   }
   celt_mips++;
   return res;
}

#define SUB16(a, b) SUB16_(a, b, __FILE__, __LINE__)
static inline short SUB16_(int a, int b, char *file, int line)
{
   int res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "SUB16: inputs are not short: %d %d in %s: line %d\n", a, b, file, line);
   }
   res = a-b;
   if (!VERIFY_SHORT(res))
      fprintf (stderr, "SUB16: output is not short: %d in %s: line %d\n", res, file, line);
   celt_mips++;
   return res;
}

#define ADD32(a, b) ADD32_(a, b, __FILE__, __LINE__)
static inline int ADD32_(long long a, long long b, char *file, int line)
{
   long long res;
   if (!VERIFY_INT(a) || !VERIFY_INT(b))
   {
      fprintf (stderr, "ADD32: inputs are not int: %d %d in %s: line %d\n", (int)a, (int)b, file, line);
   }
   res = a+b;
   if (!VERIFY_INT(res))
   {
      fprintf (stderr, "ADD32: output is not int: %d in %s: line %d\n", (int)res, file, line);
   }
   celt_mips+=2;
   return res;
}

#define SUB32(a, b) SUB32_(a, b, __FILE__, __LINE__)
static inline int SUB32_(long long a, long long b, char *file, int line)
{
   long long res;
   if (!VERIFY_INT(a) || !VERIFY_INT(b))
   {
      fprintf (stderr, "SUB32: inputs are not int: %d %d in %s: line %d\n", (int)a, (int)b, file, line);
   }
   res = a-b;
   if (!VERIFY_INT(res))
      fprintf (stderr, "SUB32: output is not int: %d in %s: line %d\n", (int)res, file, line);
   celt_mips+=2;
   return res;
}

#undef UADD32
#define UADD32(a, b) UADD32_(a, b, __FILE__, __LINE__)
static inline unsigned int UADD32_(unsigned long long a, unsigned long long b, char *file, int line)
{
   long long res;
   if (!VERIFY_UINT(a) || !VERIFY_UINT(b))
   {
      fprintf (stderr, "UADD32: inputs are not int: %u %u in %s: line %d\n", (unsigned)a, (unsigned)b, file, line);
   }
   res = a+b;
   if (!VERIFY_UINT(res))
   {
      fprintf (stderr, "UADD32: output is not int: %u in %s: line %d\n", (unsigned)res, file, line);
   }
   celt_mips+=2;
   return res;
}

#undef USUB32
#define USUB32(a, b) USUB32_(a, b, __FILE__, __LINE__)
static inline unsigned int USUB32_(unsigned long long a, unsigned long long b, char *file, int line)
{
   long long res;
   if (!VERIFY_UINT(a) || !VERIFY_UINT(b))
   {
      /*fprintf (stderr, "USUB32: inputs are not int: %llu %llu in %s: line %d\n", (unsigned)a, (unsigned)b, file, line);*/
   }
   res = a-b;
   if (!VERIFY_UINT(res))
   {
      /*fprintf (stderr, "USUB32: output is not int: %llu - %llu = %llu in %s: line %d\n", a, b, res, file, line);*/
   }
   celt_mips+=2;
   return res;
}

/* result fits in 16 bits */
static inline short MULT16_16_16(int a, int b)
{
   int res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "MULT16_16_16: inputs are not short: %d %d\n", a, b);
   }
   res = a*b;
   if (!VERIFY_SHORT(res))
      fprintf (stderr, "MULT16_16_16: output is not short: %d\n", res);
   celt_mips++;
   return res;
}

#define MULT16_16(a, b) MULT16_16_(a, b, __FILE__, __LINE__)
static inline int MULT16_16_(int a, int b, char *file, int line)
{
   long long res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "MULT16_16: inputs are not short: %d %d in %s: line %d\n", a, b, file, line);
   }
   res = ((long long)a)*b;
   if (!VERIFY_INT(res))
      fprintf (stderr, "MULT16_16: output is not int: %d in %s: line %d\n", (int)res, file, line);
   celt_mips++;
   return res;
}

#define MAC16_16(c,a,b)     (celt_mips-=2,ADD32((c),MULT16_16((a),(b))))

#define MULT16_32_QX(a, b, Q) MULT16_32_QX_(a, b, Q, __FILE__, __LINE__)
static inline int MULT16_32_QX_(int a, long long b, int Q, char *file, int line)
{
   long long res;
   if (!VERIFY_SHORT(a) || !VERIFY_INT(b))
   {
      fprintf (stderr, "MULT16_32_Q%d: inputs are not short+int: %d %d in %s: line %d\n", Q, (int)a, (int)b, file, line);
   }
   if (ABS32(b)>=((opus_val32)(1)<<(15+Q)))
      fprintf (stderr, "MULT16_32_Q%d: second operand too large: %d %d in %s: line %d\n", Q, (int)a, (int)b, file, line);
   res = (((long long)a)*(long long)b) >> Q;
   if (!VERIFY_INT(res))
      fprintf (stderr, "MULT16_32_Q%d: output is not int: %d*%d=%d in %s: line %d\n", Q, (int)a, (int)b,(int)res, file, line);
   if (Q==15)
      celt_mips+=3;
   else
      celt_mips+=4;
   return res;
}

#define MULT16_32_Q15(a,b) MULT16_32_QX(a,b,15)
#define MAC16_32_Q15(c,a,b) (celt_mips-=2,ADD32((c),MULT16_32_Q15((a),(b))))

static inline int SATURATE(int a, int b)
{
   if (a>b)
      a=b;
   if (a<-b)
      a = -b;
   celt_mips+=3;
   return a;
}

static inline int MULT16_16_Q11_32(int a, int b)
{
   long long res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "MULT16_16_Q11: inputs are not short: %d %d\n", a, b);
   }
   res = ((long long)a)*b;
   res >>= 11;
   if (!VERIFY_INT(res))
      fprintf (stderr, "MULT16_16_Q11: output is not short: %d*%d=%d\n", (int)a, (int)b, (int)res);
   celt_mips+=3;
   return res;
}
static inline short MULT16_16_Q13(int a, int b)
{
   long long res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "MULT16_16_Q13: inputs are not short: %d %d\n", a, b);
   }
   res = ((long long)a)*b;
   res >>= 13;
   if (!VERIFY_SHORT(res))
      fprintf (stderr, "MULT16_16_Q13: output is not short: %d*%d=%d\n", a, b, (int)res);
   celt_mips+=3;
   return res;
}
static inline short MULT16_16_Q14(int a, int b)
{
   long long res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "MULT16_16_Q14: inputs are not short: %d %d\n", a, b);
   }
   res = ((long long)a)*b;
   res >>= 14;
   if (!VERIFY_SHORT(res))
      fprintf (stderr, "MULT16_16_Q14: output is not short: %d\n", (int)res);
   celt_mips+=3;
   return res;
}

#define MULT16_16_Q15(a, b) MULT16_16_Q15_(a, b, __FILE__, __LINE__)
static inline short MULT16_16_Q15_(int a, int b, char *file, int line)
{
   long long res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "MULT16_16_Q15: inputs are not short: %d %d in %s: line %d\n", a, b, file, line);
   }
   res = ((long long)a)*b;
   res >>= 15;
   if (!VERIFY_SHORT(res))
   {
      fprintf (stderr, "MULT16_16_Q15: output is not short: %d in %s: line %d\n", (int)res, file, line);
   }
   celt_mips+=1;
   return res;
}

static inline short MULT16_16_P13(int a, int b)
{
   long long res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "MULT16_16_P13: inputs are not short: %d %d\n", a, b);
   }
   res = ((long long)a)*b;
   res += 4096;
   if (!VERIFY_INT(res))
      fprintf (stderr, "MULT16_16_P13: overflow: %d*%d=%d\n", a, b, (int)res);
   res >>= 13;
   if (!VERIFY_SHORT(res))
      fprintf (stderr, "MULT16_16_P13: output is not short: %d*%d=%d\n", a, b, (int)res);
   celt_mips+=4;
   return res;
}
static inline short MULT16_16_P14(int a, int b)
{
   long long res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "MULT16_16_P14: inputs are not short: %d %d\n", a, b);
   }
   res = ((long long)a)*b;
   res += 8192;
   if (!VERIFY_INT(res))
      fprintf (stderr, "MULT16_16_P14: overflow: %d*%d=%d\n", a, b, (int)res);
   res >>= 14;
   if (!VERIFY_SHORT(res))
      fprintf (stderr, "MULT16_16_P14: output is not short: %d*%d=%d\n", a, b, (int)res);
   celt_mips+=4;
   return res;
}
static inline short MULT16_16_P15(int a, int b)
{
   long long res;
   if (!VERIFY_SHORT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "MULT16_16_P15: inputs are not short: %d %d\n", a, b);
   }
   res = ((long long)a)*b;
   res += 16384;
   if (!VERIFY_INT(res))
      fprintf (stderr, "MULT16_16_P15: overflow: %d*%d=%d\n", a, b, (int)res);
   res >>= 15;
   if (!VERIFY_SHORT(res))
      fprintf (stderr, "MULT16_16_P15: output is not short: %d*%d=%d\n", a, b, (int)res);
   celt_mips+=2;
   return res;
}

#define DIV32_16(a, b) DIV32_16_(a, b, __FILE__, __LINE__)

static inline int DIV32_16_(long long a, long long b, char *file, int line)
{
   long long res;
   if (b==0)
   {
      fprintf(stderr, "DIV32_16: divide by zero: %d/%d in %s: line %d\n", (int)a, (int)b, file, line);
      return 0;
   }
   if (!VERIFY_INT(a) || !VERIFY_SHORT(b))
   {
      fprintf (stderr, "DIV32_16: inputs are not int/short: %d %d in %s: line %d\n", (int)a, (int)b, file, line);
   }
   res = a/b;
   if (!VERIFY_SHORT(res))
   {
      fprintf (stderr, "DIV32_16: output is not short: %d / %d = %d in %s: line %d\n", (int)a,(int)b,(int)res, file, line);
      if (res>32767)
         res = 32767;
      if (res<-32768)
         res = -32768;
   }
   celt_mips+=35;
   return res;
}

#define DIV32(a, b) DIV32_(a, b, __FILE__, __LINE__)
static inline int DIV32_(long long a, long long b, char *file, int line)
{
   long long res;
   if (b==0)
   {
      fprintf(stderr, "DIV32: divide by zero: %d/%d in %s: line %d\n", (int)a, (int)b, file, line);
      return 0;
   }

   if (!VERIFY_INT(a) || !VERIFY_INT(b))
   {
      fprintf (stderr, "DIV32: inputs are not int/short: %d %d in %s: line %d\n", (int)a, (int)b, file, line);
   }
   res = a/b;
   if (!VERIFY_INT(res))
      fprintf (stderr, "DIV32: output is not int: %d in %s: line %d\n", (int)res, file, line);
   celt_mips+=70;
   return res;
}

#undef PRINT_MIPS
#define PRINT_MIPS(file) do {fprintf (file, "total complexity = %llu MIPS\n", celt_mips);} while (0);

#endif
