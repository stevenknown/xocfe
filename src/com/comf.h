/*@
XOC Release License

Copyright (c) 2013-2014, Alibaba Group, All rights reserved.

    compiler@aliexpress.com

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Su Zhenyu nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

author: Su Zhenyu
@*/
#ifndef _COMMON_FUNCTION_H_
#define _COMMON_FUNCTION_H_

namespace xcom {

class StrBuf;
class StrBufVec;
class Float;

//Forward Declaration
template <class T> class Vector;

//Compute the maximum unsigned integer value that type is ValueType.
//e.g: given bitwidth is 3, return 7 as result.
//Note ValueType should be unsigned integer type.
//NOTE: The direct way to write it is
//return ((((ValueType)1) << bitwidth) - 1);.
//However, this approach may lead to incorrect results on different
//architectures such as x86 and ARM.
//The shift operation `1 << bitwidth` places the `1` in the `bitwidth`-th
//position and then subtracting `1` gives us a number with all bits set to
//`1` up to that position. However, behavior on different architectures
//may vary:
//- On **x86** (32-bit systems), shifting beyond 31 bits wraps around
//(circular shift), which might lead to incorrect results if `bitwidth`
//is larger than the number of bits in the type.
//- On **ARM**, shifting beyond the number of available bits (e.g., 32
//bits for uint32_t) will discard the extra bits, which is the expected
//and correct behavior.
template <class ValueType>
inline ValueType computeUnsignedMaxValue(UINT bitwidth)
{
    return (ValueType)(~((ValueType)0)) >>
        (sizeof(ValueType) * BITS_PER_BYTE - bitwidth);
}


//Compute the maximum unsigned integer value that type is ValueType.
//e.g: given ValueType is UINT32, return 0xFFFFffff as result.
//Note ValueType should be unsigned integer type.
template <class ValueType>
inline ValueType computeUnsignedMaxValue()
{ return ~((ValueType)0); }

//Compute the maximum unsigned integer value
//based on the byte width of the type.
//e.g, given bytewidth is 1 (byte),
//return 255 as result for an 8-bit unsigned integer.
//Note: ValueType should be an unsigned integer type.
template <class ValueType>
inline ValueType computeMaxValueFromByteWidth(UINT bytewidth)
{
    return computeUnsignedMaxValue<ValueType>(bytewidth * BITS_PER_BYTE);
}

//Arrangement
//P(n,m)=n*(n-1)*...*(n-m+1)=n!/(n-m)!
UINT arra(UINT n, UINT m); //Arrangement

//Convert floating point string into binary words.
void af2i(IN CHAR * f, OUT BYTE * buf, INT buflen, bool is_double);

//Return the position in array if find v.
//array: sorted in incremental order.
//n: elements size of array.
//v: search v in array.
bool binsearch(INT array[], UINT n, INT v, MOD UINT * ppos);

//Combination
//C(n,m)=(n*(n-1)*...*(n-m+1))/m! = n!/m!(n-m)!
//Simplify:C(n,m)=(n*(n-1)*(m+1))/(n-m)!
UINT combin(UINT n, UINT m); //Combination

//Ceil rounding alignment.
//e.g  v=17 , align=4 , the result is 20.
inline LONGLONG ceil_align(LONGLONG v, LONGLONG align)
{
    if (align == 0 || align == 1) { return v; }
    if ((v % align) != 0) {
        v = (v / align + 1) * align;
    }
    return v;
}

//Compute the number of bits which biger enough to represent given value.
//value: the input value that to be check.
//e.g: given 00101, it needs at least 3 bits to hold the binary value 101.
//     and function return 3.
UINT computeMaxBitSizeForValue(ULONGLONG v);

//The function converts string content into set of hex and store in the 'buf'.
//Note the content in given buf must be string format of hex, that is the
//string can only contain "abcdefABCDEF0123456789".
//string: a string that represents bytes, where a byte consists of two hex.
//        e.g:ADBC5E demostrates three bytes, 0xAD, 0xBC, and 0x5E.
void charToByteHex(CHAR const* string, OUT BYTE * buf, UINT buflen);

//The function converts a character into hex.
//Note the content in given buf must be string format of hex, that is 'c'
//can only be "abcdefABCDEF0123456789".
BYTE charToHex(CHAR c);

//Return the number of leading zero.
//pos: record the bit position of the leading zero.
//e.g:given a is 0b00110, where the left size is the highest bit.
//the function return 2, indicates there are two leading zeros counting from
//high to low side about given number.
UINT countLeadingZero(UINT64 a);
UINT countLeadingZero(UINT32 a);
UINT countLeadingZero(UINT16 a);
UINT countLeadingZero(UINT8 a);

//Return true if found leading one.
//pos: record the bit position of the leading one.
//e.g:given a is 0b11110, where the left size is the highest bit.
//the function return 4, indicates there are four leading one counting from
//high to low side about given number.
UINT countLeadingOne(UINT64 a);
UINT countLeadingOne(UINT32 a);

//Count the one of val using `ByteOp::get_elem_count()`.
//e.g. given the value 0x1111 1111 1111 1111, the results are:
//countOne(UINT64) = 16
//countOne(UINT32) = 8
//countOne(UINT64) = 4
//countOne(UINT64) = 2
UINT countOne(UINT64 a);
UINT countOne(UINT32 a);
UINT countOne(UINT16 a);
UINT countOne(UINT8 a);

//Count the trailing zeros of val.
//e.g. given the value 0x1 0000 0000 0000, the results are:
//countTrailingZero(UINT64) = 48
//countTrailingZero(UINT64) = 32
//countTrailingZero(UINT64) = 16
//countTrailingZero(UINT64) = 8
UINT countTrailingZero(UINT64 val);
UINT countTrailingZero(UINT32 val);
UINT countTrailingZero(UINT16 val);
UINT countTrailingZero(UINT8 val);

//Extended Euclid Method.
//    ax + by = ay' + b(x' -floor(a/b)*y') = gcd(a,b) = gcd(b, a%b)
INT exgcd(INT a, INT b, OUT INT & x, OUT INT & y);

//Extract the right most sub-string which separated by 'separator' from string.
//e.g: Given string is a\b\c, separator is '\', return c;
CHAR const* extractRightMostSubString(CHAR const* string, CHAR separator);

//Extract the left most sub-string which separated by 'separator' from string.
//e.g: Given string is a\b\c, separator is '\', return a;
void extractLeftMostSubString(CHAR * tgt, CHAR const* string, CHAR separator);

//Extract the bit value from 'val' by given range that between 'start' bit
//position and 'end' bit position.
//The bit position start at 0.
//e.g: Given start is 4, end is 6, val is 0b110101111(0x03AF), return 0b010.
//       0b110101111
//     |end <-- start|
//NOTE: 'T' must be unsigned type.
template <typename T>
T extractBitRangeValue(T val, UINT start, UINT end)
{
    ASSERT0(start < sizeof(T) * BITS_PER_BYTE);
    ASSERT0(end < sizeof(T) * BITS_PER_BYTE);
    ASSERT0(start <= end);
    UINT lastbit = sizeof(T) * BITS_PER_BYTE - 1;
    UINT size = end - start;
    val = val << (lastbit - end);
    val = val >> (lastbit - size);
    return val;
}


//Extract the valid value according to the bitwidth.
//e.g: given value is 7 and bitwidth is 2, return 3 as result.
//Note ValueType should be unsigned integer type.
template <class ValueType>
inline ValueType extractValidValueViaBitWidth(
    ValueType value, UINT bitwidth)
{
    return (value & computeUnsignedMaxValue<ValueType>(bitwidth));
}

//Utility function to encode a ULEB128 value to an output stream. Returns
//the length in bytes of the encoded value.
//We can set a padding, for example,
//if we calculate that the number of bytes is two,
//we can supplement it to the number of bytes we set.
//ULEB128 (Unsigned Little Endian Base 128)
//and SLEB128 (Signed Little Endian Base 128) are
//variable-length encoding methods commonly used
//for encoding integers to save space.They are frequently used in DWARF.
//ULEB128 (Unsigned Little Endian Base 128) is used for variable-length
//encoding of non-negative integers.
//It encodes integers into one or more bytes,
//with each byte providing 7 bits to represent a portion of the value,
//and the highest 8th bit serves as a "continuation bit"
//indicating whether there are more bytes to follow.
//For example, if we have the number 1016, The encoding rules are as follows:
//1 The binary representation of 1016 is 001111111000.
//splitting the binary representation 0000_0011_1111_1000 into two groups of 7,
//we get 111_1000 and 000_0111.
//2 For the first group, 111_1000, we add its highest bit.
//Since there is more data following it,
//we set the highest bit to 1, making it 1111_1000.
//For the second group,
//000_0111, since there is no more data following it,
//we add a highest bit of 0, resulting in 0000_0111.
//3 Finally, we concatenate 1111_1000 and 0000_0111 to get 1111_1000_0000_0111.

//Here's the explanation for the parameters:
//value: This is the value that we want to encode,
//which serves as the input to the encoding process.
//os(output stream): This represents the encoded value,
//which is the output of the encoding process.
//After encoding, the result is stored in this variable.
//pad_to: This parameter specifies padding,
//meaning that even if the encoded value requires only
//a certain number of bytes (e.g., 2 bytes),
//we can forcefully pad it to a certain number of bytes.
//This ensures that the encoded value conforms to a specific byte length,
//regardless of the actual encoding size.
UINT32 encodeULEB128(UINT64 value, OUT Vector<CHAR> & os,
                     UINT32 pad_to = 0);

//Similar to the encodeULEB128 function.
UINT32 encodeSLEB128(INT64 value, OUT Vector<CHAR> & os,
                     UINT32 pad_to = 0);

//Factorial of n, namely, requiring n!.
UINT fact(UINT n);

//convert float to ESP64(64-bit extended single precision) format.
UINT64 float2ESP64(UINT64 val);

//Great common divisor for number of values.
INT gcdm(UINT num, ...);

//Great common divisor for values stored in vector.
INT gcdm(UINT num, Vector<INT> const& a);

//Compute the nearest power of 2 that not less than v.
//e.g: given v is 60, return 64.
inline UINT getNearestPowerOf2(UINT v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

//Compute the nearest power of 2 that not less than v.
//e.g: given v is 60, return 64.
inline ULONGLONG getNearestPowerOf2(ULONGLONG v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    v++;
    return v;
}

//Compute the number of 1.
//e.g: given 0xa, return 2.
UINT getLookupPopCount(ULONGLONG v);

//Compute the number of 1.
//e.g: given 0xa, return 2.
UINT getSparsePopCount(ULONGLONG v);

//Compute the power of 2, return the result.
//Note v must be power of 2.
//e.g: given v is 64, return 16.
UINT getPowerOf2(ULONGLONG v);

//Extract file suffix.
//e.g: Given a.foo, return foo.
CHAR * getFileSuffix(CHAR const* n, OUT CHAR * buf, UINT bufl);

//Extract file path.
//e.g: Given /xx/yy/zz.file, return /xx/yy
CHAR * getFilePath(CHAR const* n, OUT CHAR * buf, UINT bufl);

//Extract file name.
//e.g: Given /xx/yy/zz.foo, return zz.
CHAR * getFileName(CHAR const* n, OUT CHAR * buf, UINT bufl);

//Get current micro-second.
ULONGLONG getusec();
LONG getclockstart();
float getclockend(LONG start);

//Get the index of the first '1' start at most right side.
//e.g: given m=0x8, the first '1' index is 3.
INT getFirstOneAtRightSide(INT m);

//Get nbitnum consecutive bits starting at bit position mbitoffset from the val.
//a.given val: 0x12345678, nbitnum: 4 and mbitoffset: 8, it will return 0x6.
//b.given val: 0x12345678, nbitnum: 8 and mbitoffset: 8, it will return 0x56.
inline UINT64 getNBitValueAtMBitOffset(UINT64 val, UINT nbitnum,
                                       UINT mbitoffset)
{
    ASSERT0((nbitnum >= 1) && (nbitnum <= 64));
    ASSERT0((mbitoffset >= 0) && (mbitoffset < 64));
    ASSERT0((mbitoffset + nbitnum <= 64));
    UINT64 mask = (nbitnum == 64) ? UINT64(-1) : ((UINT64(1) << nbitnum) - 1);
    return (val >> mbitoffset) & mask;
}

//Get the sign bit of given bitsize.
//                      |
//                      V
//8bit integer:         0 00000000
//                        |<-7 ->|
//16bit integer:        0 000000000000000
//                        |<-   15    ->|
//32bit integer:        0 0000000000000000000000000000000
//                        |<-            31           ->|
//64bit integer:        0 0000000000....00000000000000000
//                        |<-            63           ->|
//128bit integer:       0 0000000000....00000000000000000
//                        |<-            127          ->|
//16bit floating point: 0 00000 0000000000 *f16*
//                        | 5 | |<- 10 ->|
//                      0 00000000 0000000 *bf16*
//                        |<- 8->| |<-7->|
//32bit floating point: 0 00000000 00000000000000000000000
//                        |<- 8->| |<-       23        ->|
//64bit floating point: 0 00000000000 0000000...0000000000
//                        |<-  11 ->| |<-     52       ->|
//128bit floating point:0 0000...0000 0000000...0000000000
//                        |<-  15 ->| |<-     112      ->|
inline UINT getSignBit(UINT bitsize)
{
    //Note that bitsize should be greater than or equal to 8 and be a
    //multiple of 8.
    ASSERTN(bitsize >= 8, ("Type bitsize is too small.\n"));
    ASSERTN(bitsize % 8 == 0, ("Type bitsize must be multiple of 8.\n"));
    return bitsize - 1;
}

//Get the signal bit of given value of bit-num. Note that the returned value
//is a binary value.
//For example:                              |
//                                          V
//(1) given 20-bit value: 0b 0000 0000 0000 1000 1001 0001 0010 0000
//(2) 1 shift 19-bit:     0b 0000 0000 0000 1000 0000 0000 0000 0000
//(3) & and shift:        0b 0000 0000 0000 0000 0000 0000 0000 0001
//
//Another example:                               |
//                                               V
//(1) given 16-bit value: 0b 0000 0000 0000 0000 0101 0010 0010 0010
//(2) 1 shift 15-bit:     0b 0000 0000 0000 0000 1000 0000 0000 0000
//(3) & and shift:        0b 0000 0000 0000 0000 0000 0000 0000 0000
inline UINT getSignBitOfNBitNum(UINT val, UINT bit_num)
{
    UINT s = 1;
    return (val & (s << (bit_num - s))) >> (bit_num - s);
}


//Get unsigned hign n-bit value of unsigned 32bit value.
inline UINT32 get32BitValueHighNBit(UINT32 val, UINT bitnum)
{
    ASSERT0((bitnum >= 1) && (bitnum <= 32));
    UINT32 shift = 32 - bitnum;
    return val >> shift;
}


//Get unsigned low n-bit value of unsigned 32bit value.
//For example:
//a.given val: 0x12345678 and bitnum: 4, it will return 0x8.
//b.given val: 0x12345678 and bitnum: 8, it will return 0x78.
inline UINT32 get32BitValueLowNBit(UINT32 val, UINT bitnum)
{
    ASSERT0((bitnum >= 1) && (bitnum <= 32));
    UINT32 shift = 32 - bitnum;
    return (val << shift) >> shift;
}

//Get signed low n-bit value of signal 32bit value.
//Note the function will signal extend the low-bit value.
//
//For example: get low 16-bit value of signed 32-bit value.
//Note the function will extend sign-bit of the low 16-bit to 32-bit value.
//For example:
//val:     0b 0100010......1000 0111 0101 1011
//                         &
//0xffff:  0b 0000000......1111 1111 1111 1111
//                         =
//         0b 0000000......1000 0111 0101 1011
//                         ^
//0x8000:  0b 0000000......1000 0000 0000 0000
//                         =
//         0b 0000000......0000 0111 0101 1011
//                         -
//0x8000:  0b 0000000......1000 0000 0000 0000
//                         =
//         0b 1111111......1000 0111 0101 1011
inline INT32 get32BitValueLowNBit(INT32 val, UINT bitnum)
{
    ASSERT0((bitnum >= 0) && (bitnum < 32));
    UINT64 const mask_0 = ((UINT64)(0x1) << bitnum) - 1;
    UINT64 const mask_1 = (UINT64)(0x1) << (bitnum - 1);
    return (INT32)(((val & mask_0) ^ mask_1) - mask_1);
}


//Get unsigned hign n-bit value of unsigned 64bit value.
//a.given val: 0x12345678 and bitnum: 4, it will return 0x1.
//b.given val: 0x12345678 and bitnum: 8, it will return 0x12.
inline UINT64 get64BitValueHighNBit(UINT64 val, UINT bitnum)
{
    ASSERT0((bitnum >= 1) && (bitnum <= 64));
    UINT shift = 64 - bitnum;
    return val >> shift;
}

//Get unsigned low n-bit value of unsigned 64bit value.
//For example:
//a.given val: 0x1122334455667788 and bitnum: 8, it will return 0x88.
//b.given val: 0x1122334455667788 and bitnum: 16, it will return 0x7788.
inline UINT64 get64BitValueLowNBit(UINT64 val, UINT bitnum)
{
    ASSERT0((bitnum >= 1) && (bitnum <= 64));
    UINT shift = 64 - bitnum;
    return (val << shift) >> shift;
}

//Get signal low n-bit value of signal 64bit value.
//Note the function will signal extend the low-bit value.
//The principle is similar to
//"INT32 get32BitValueLowNBit(INT32 val, UINT bitnum)".
inline INT64 get64BitValueLowNBit(INT64 val, UINT bitnum)
{
    ASSERT0((bitnum >= 0) && (bitnum < 64));
    UINT64 const mask_0 = ((UINT64)(0x1) << bitnum) - 1;
    UINT64 const mask_1 = (UINT64)(0x1) << (bitnum - 1);
    return (INT64)(((val & mask_0) ^ mask_1) - mask_1);
}

//Calculate an integer hash value according to 'n'.
inline UINT32 hash32bit(UINT32 n)
{
    n = (n+0x7ed55d16) + (n<<12);
    n = (n^0xc761c23c) ^ (n>>19);
    n = (n+0x165667b1) + (n<<5);
    n = (n+0xd3a2646c) ^ (n<<9);
    n = (n+0xfd7046c5) + (n<<3);
    n = (n^0xb55a4f09) ^ (n>>16);
    return n;
}

//convert half to EHP64(64-bit extended half-precision) format.
UINT64 half2EHP64(UINT64 val);

//Return true if *signed* val exceeds the range described by 'bitsize'.
bool isExceedBitWidth(LONGLONG val, UINT bitwidth);

//Return true if *unsigned* val exceeds the range described by 'bitsize'.
bool isExceedBitWidth(ULONGLONG val, UINT bitwidth);

//Insert 'in' value that from 'in_start' to 'in_end' index into the target
//position of value 'out' that determined by 'out_start' and 'out_end' index.
//e.g.: given out is '0x00F000F0', out_start is '12', out_end is '15';
//      in is '0x0F0', in_start is '4', in_end is '7';
//      and the result is '0x00F0F0F0'.
template <typename T>
void insertBitRangeValue(OUT T & out, T in, UINT out_start,
                         UINT out_end, UINT in_start, UINT in_end)
{
    UINT32 width = sizeof(T) * BITS_PER_BYTE;
    ASSERT0(!isExceedBitWidth((ULONGLONG)in, width));
    ASSERT0(out_start <= out_end && out_start < width && out_end < width);
    ASSERT0(in_start <= in_end && in_start < width && in_end < width);
    ASSERT0((in_end - in_start) <= (out_end - out_start));

    in = xcom::extractBitRangeValue(in, in_start, in_end);
    out |= (in << out_start);
}

//Return true if 'c' is an extended character.
//
//For example:
//  The special character "À" is expressed in unicode form as: \u00C0.
//  According to the following rules:
//
//  ---------------------------------------------------------------
//  |  Unicode hexadecimal  |            UTF-8 binary             |
//  |-------------------------------------------------------------|
//  | 0000 0000 ~ 0000 007F | 0xxxxxxx                            |
//  | 0000 0080 ~ 0000 07FF | 110xxxxx 10xxxxxx                   |
//  | 0000 0800 ~ 0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx          |
//  | 0001 0000 ~ 0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx |
//  ---------------------------------------------------------------
//
//  The special character "À" is expressed in UTF-8 form as: "0xC3 0x80".
inline bool xisextchar(CHAR c) { return (c & 0x80) != 0; }

//Judge if 'f' is integer conform to IEEE754 spec.
bool isIntegerF(float f);

//Judge if 'd' is integer conform to IEEE754 spec.
bool isIntegerD(double d);

//Return true if 'f' represents a finite floating-point value.
//f: is conform to IEEE754 spec.
bool isFiniteD(double f);

//Return true if 'f' represents a finite floating-point value.
//f: is conform to IEEE754 spec.
bool isFiniteF(float f);

//inline is necessary to avoid multiple define.
inline bool isPowerOf2(ULONGLONG x) { return (x != 0) & ((x & (x-1)) == 0); }
bool isPowerOf5(double f);

//Prime Factorization.
//e.g: 435234 = 251 * 17 * 17 * 3 * 2.
void prim(INT m, OUT INT * buf);

//Solve the integer power of 2.
//e.g: if 'x' is 2, return 4.
int powerXOf2(int x);

//Reverse a LONG type integer by lexicalgraph.
//e.g: if 'd' is 0x12345678, return 0x78563412.
inline LONG revlong(LONG d)
{
    CHAR * c = (CHAR*)&d, m;
    m = c[0], c[0] = c[3], c[3] = m;
    m = c[1], c[1] = c[2], c[2] = m;
    return d;
}

//Reverse the string.
CHAR * reverseString(CHAR * v);

//Compute the result of "val0 rotateleft val1".
//For example:
//val0: 0xFFFFFF0000001111
//va11: 0x3
//res:  (0xFFFFFF0000001111 << 3) | (0xFFFFFF0000001111 >> 61) =
//      0xFFFFF8000000888F
UINT rotateLeft(UINT val0, UINT val1);

//The function rotates string in buffer.
//e.g: given string "xyzmn", n is 2, after rotation,
//the 'str' will be "zmnxy".
//str: record the string.
//n: rotate times.
CHAR * rotateString(MOD CHAR * str, UINT n);

//Replace letters in 'n' to capital letter.
CHAR * upper(CHAR * n);
inline CHAR upper(CHAR n)
{ return ((n >= 'a') & (n <= 'z')) ? (CHAR)(n - 32) : n; }

//Replace letters in 'n' to lowercase letter.
CHAR * lower(CHAR * n);
inline CHAR lower(CHAR n)
{ return ((n >= 'A') & (n <= 'Z')) ? (CHAR)(n + 32) : n; }

//Replace the suffix of the file name(or path name) using given suffix name.
//For example:
//org_file_name: "xxx.c" or "/tmp/xxx.cpp"
//new_suffix: ".o"
//new_file_name: "xxx.o"
void replaceFileNameSuffix(CHAR const* org_file_name, CHAR const* new_suffix,
                           OUT StrBuf & new_file_name);

//Find the most significant bit (MSB) of given value with different types.
//e.g.: 0x4000 0000 4000 4040
//                |
//                V
//0b 01000000 00000000 00000000 00000000 01000000 00000000 01000000 01000000
//    ^                                   ^                 ^        ^
// UINT64                              UINT32            UINT16    UINT8
//findMostSignificantBit(UINT64) = 62
//findMostSignificantBit(UINT32) = 30
//findMostSignificantBit(UINT16) = 14
//findMostSignificantBit(UINT8) = 6
//Note: If the given value is negative, the result will be the sign bit
//corresponding to that type.
UINT findMostSignificantBit(UINT8 val);
UINT findMostSignificantBit(UINT16 val);
UINT findMostSignificantBit(UINT32 val);
UINT findMostSignificantBit(UINT64 val);
UINT findMostSignificantBit(INT8 val);
UINT findMostSignificantBit(INT16 val);
UINT findMostSignificantBit(INT32 val);
UINT findMostSignificantBit(INT64 val);

//Set nbitnum consecutive bits starting at bit position mbitoffset in orgval
//to bitval. The bitval can only be 0 or 1. If bitval is 1, the specified
//bits are set to all 1; if bitval is 0, they are set to all 0.
//a.given orgval: 0x12345678, nbitnum: 4, mbitoffset: 8 and bitval: 0,
//  The value of orgval will be set to 0x12345078.
//b.given orgval: 0x12345678, nbitnum: 8, mbitoffset: 8 and bitval: 1,
//  The value of orgval will be set to 0x1234FF78.
inline void setNBitValueAtMBitOffset(OUT UINT64 * orgval, UINT nbitnum,
                                     UINT mbitoffset, UINT64 bitval)
{
    ASSERT0((nbitnum >= 1) && (nbitnum <= 64));
    ASSERT0((mbitoffset >= 0) && (mbitoffset < 64));
    ASSERT0(mbitoffset + nbitnum <= 64);
    ASSERT0(bitval == 0 || bitval == 1);
    UINT64 mask = computeUnsignedMaxValue<UINT64>(nbitnum);
    mask <<= mbitoffset;
    *orgval = (*orgval & ~mask) | ((-(INT64)bitval) & mask);
}

//Calculate the Great Common Divisor of x and y.
INT sgcd(INT x, INT y);

//Calculate the Least Common Multiple of x and y.
INT slcm(INT x, INT y);

//According to the values in "bit_group_desc", split the m-bit value "val" into
//several parts and write the result to "bit_group_val".
//
// val:            the given m-bit value
// m:              the bit width of "val"
// bit_group_desc: the bit width of each part
// bit_group_val:  the result of UINT64 type
//
//For example:
//
// val:            0x123456
// m:              22        Given value is 0b 01 0010 0011 0100 0101 0110
// bit_group_desc: { 4, 5, 6, 7 }
// bit_group_val:  { 0x4, 0x11, 0x28, 0x56 }
//                 { 0b0100, 0b10001, 0b101000, 0b1010110 }
void splitMbitToNGroup(UINT64 val, UINT m, Vector<UINT> & bit_group_desc,
                       OUT Vector<UINT64> & bit_group_val);

//The function of this function is the same as
//
//    void splitMbitToNGroup(UINT64 val, UINT m,
//                           Vector<UINT> const& bit_group_desc,
//                           OUT Vector<UINT64> & bit_group_val).
//
//The difference is that the parameters passed in are pointers and lengths,
//not Vector references. It will be called by the function above.
void splitMbitToNGroup(UINT64 val, UINT m,
                       UINT const* bit_group_desc, UINT bit_group_desc_len,
                       OUT UINT64 * bit_group_val, UINT bit_group_val_len);

//According to the values in "bit_group_desc", split the m-bit value "val" into
//several parts and write the result to "bit_group_val".
//
//This function has similar functions to another function with the same name,
//except that the fourth parameter type is different. This function returns a
//Vector containing INT64 type, and the element value needs to be signal
//extended.
void splitMbitToNGroup(UINT64 val, UINT m, Vector<UINT> & bit_group_desc,
                       OUT Vector<INT64> & bit_group_val);

//The function of this function is the same as
//
//    void splitMbitToNGroup(UINT64 val, UINT m,
//                           Vector<UINT> const& bit_group_desc,
//                           OUT Vector<INT64> & bit_group_val).
//
//The difference is that the parameters passed in are pointers and lengths,
//not Vector references. It will be called by the function above.
void splitMbitToNGroup(UINT64 val, UINT m,
                       UINT const* bit_group_desc, UINT bit_group_desc_len,
                       OUT INT64 * bit_group_val, UINT bit_group_val_len);

//Shift a string to right side or left side.
//ofst: great than zero means shifting string to right side,
//   and the displacement is abs(ofst); negative
//   means shifting string to left.
void strshift(CHAR * src, INT ofst);

//The function produce output string according to 'info' as described below,
//and appends the output the 'buf'.
//Return a pointer to the resulting string 'buf'.
//buf: output string buffer.
//bufl: byte size of 'buf'.
//info: formatting string.
CHAR * xstrcat(CHAR * buf, size_t bufl, CHAR const* info, ...);

//Calculates the byte size of string 'p', excluding the terminating byte '\0'.
UINT xstrlen(CHAR const* p);

//Compare the first 'n' characters of two string.
//Return true if equal.
//n: byte length to compare.
//pos: it is optional and can be NULL. If it is not NULL then record the byte
//     position if the function return false.
//e.g: return true if p1 is "aaa", p2 is "aaab", which n is 3.
bool xstrcmp(CHAR const*RESTRICT p1, CHAR const*RESTRICT p2, INT n,
             OUT UINT * pos = nullptr);

//Format string and record in buf.
//'buf': output buffer record string.
//'stack_start': point to the first args.
CHAR * xsprintf(MOD CHAR * buf, UINT buflen, CHAR const* format, ...);

//Convert a string into long integer.
//e.g: cl = '1','2','3','4','5'
//return 12345.
//'is_oct': if true, nptr is octal digits.
LONGLONG xatoll(CHAR const* nptr, bool is_oct = false);

//Convert char value into binary.
//e.g: char p = ' '; p is blank.
INT xctoi(CHAR const* cl);

//Convert long to string.
CHAR * xltoa(LONG v, OUT CHAR * buf);

//Cacluates the smallest integral value that is not less than
//the division of a, b.
//e.g: xceiling(3,2) is 2.
INT xceiling(INT a, INT b);

//Cacluates the largest integral value that is not greater than
//the division of a, b.
//e.g: xceiling(3,2) is 1.
INT xfloor(INT a, INT b);

//Find partial string, return the subscript-index if substring found,
//otherwise return -1.
//src: input string.
//par: partial string.
LONG xstrstr(CHAR const* src, CHAR const* par);

//Split string by given separator, and return the number of substring.
//str: input string.
//ret: record each substring which separated by sep.
//sep: separator.
//Note caller is responsible for the free of each string memory in ret.
//e.g:given str "a/b/c", sep is '/', ret will store three StrBuf, "a", "b"
//and "c".
UINT xsplit(CHAR const* str, CHAR const* sep, OUT StrBufVec & ret);

//The function copies 'bytesize' of the string pointed by 'src'
//to destination StrBuf 'tgt', including the '\0'.
//The function also return buffer pointer of 'tgt'.
CHAR const* xstrcpy(CHAR const* src, size_t bytesize, OUT StrBuf & tgt);

//Return true if 'c' is blank space or TAB character.
inline bool xisspace(CHAR c) { return (c == ' ') | (c == '\t'); }

//Return true if char 'c' is decimal.
inline bool xisdigit(CHAR c) { return (c >= '0') & (c <= '9'); }

//Return true if string 'str' is decimal.
inline bool xisdigit(CHAR const* str)
{
    for (; *str != 0; str++) {
        if (!xisdigit(*str)) {
            return false;
        }
    }
    return true;
}

//Return true if 'c' is binary decimal, 0 or 1.
inline bool xisdigitbin(CHAR c) { return (c == '0') | (c == '1'); }

//Return true if 'c' is hex decimal in the range a-f or A-F.
inline bool xisdigithex_alpha(CHAR c)
{ return (upper(c) >= 'A') & (upper(c) <= 'F'); }

//Return true if 'c' is octal decimal in the range 0-7.
inline bool xisdigithex_octal(CHAR c) { return (c >= '0') & (c <= '7'); }

//Return true if 'c' is hex decimal.
//The hex decimal is character in the range of 0-9, a-f, and A-F.
inline bool xisdigithex(CHAR c) { return xisdigit(c) || xisdigithex_alpha(c); }

//Return true if 'c' is letter.
//The letter is character in the range of a-z and A-Z.
inline bool xisalpha(CHAR c) { return (upper(c) >= 'A') & (upper(c) <= 'Z'); }

//Return abs value of 'a'.
inline LONGLONG xabs(LONGLONG a) { return a >= LONGLONG(0) ? a : -a; }
inline float xfabs(float a) { return (a < float(0.0)) ? -a : a; }
inline double xfabs(double a) { return (a < double(0.0)) ? -a : a; }

//Return sqrt value of 'num'.
float xsqrt(float num);
double xsqrt(double num);

//The function utilizes a very fast algorithm that does not calculate
//iteratively to converge to the square root of 'n'.
//Note the algorithm's precision is a little bit low.
float xsqrtNonIter(float n);

//Return true if the nth-root of 'num' is in the range of real-number.
//num: the input real-number.
//nroot: the nth root.
//res: record the result if nth-root of 'num' is valid.
//e.g: given num is 8, root is 3, the function will calculate 8^(1/3).
//     and return 2.
//e.g2:given num is -4, root is 2, the function will return false.
//     Because the even root of negative number is undefined in the range of
//     real num.
bool xnroot(Float const& num, UINT nroot, OUT Float & res);

//Compute the logarithm of 'y' to the base of 'x': log_x(y).
//e.g: given x is 3, y is 27, the function return 3.0.
double xlog(double x, double y);

//Return true if the imm is valid for the limited bitsize.
bool isValidImmForBitsize(UINT bitsize, UINT64 imm);

//[IEEE 754] +0.0 == -0.0 are all floating-point zeros.
//+0.0: 0x0000000000000000
//-0.0: 0x8000000000000000
//Ignore the sign bit and only check if both the mantissa and exponent are
//zeros using bitwise operation.
bool isFPConstZero(UINT64 val);

//[IEEE 754] +0.0: 0x0000000000000000
//Therefore, floating-point values with all elements being zero belong to the
//category of positive floating-point numbers.
bool isFPConstZeroPositive(UINT64 val);

//Exported Data Structures
class ASCII {
public:
    UCHAR val;
    CHAR ch;
};
extern ASCII g_asc1[];

//Return true if the addr is bytesize-byte aligned.
//e.g: given addr is 0x8, bytesize is 4, the function will return true.
//e.g2:given addr is 0xc, bytesize is 8, the function will return false.
inline bool checkAligned(ULONGLONG addr, size_t bytesize)
{
    ASSERT0(bytesize > 0 && isPowerOf2((ULONGLONG)bytesize));
    return (addr & (bytesize - 1)) == 0;
}

} //namespace xcom

#endif  //_COMMON_FUNCTION_H_
