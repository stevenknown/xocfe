/*@
Copyright (c) 2013-2021, Su Zhenyu steven.known@gmail.com
All rights reserved.

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

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
@*/
#include "xcominc.h"

namespace xcom {

//Count of bits in all the one byte numbers.
BYTE const g_bit_count[256] = {
  0, //  0
  1, //  1
  1, //  2
  2, //  3
  1, //  4
  2, //  5
  2, //  6
  3, //  7
  1, //  8
  2, //  9
  2, // 10
  3, // 11
  2, // 12
  3, // 13
  3, // 14
  4, // 15
  1, // 16
  2, // 17
  2, // 18
  3, // 19
  2, // 20
  3, // 21
  3, // 22
  4, // 23
  2, // 24
  3, // 25
  3, // 26
  4, // 27
  3, // 28
  4, // 29
  4, // 30
  5, // 31
  1, // 32
  2, // 33
  2, // 34
  3, // 35
  2, // 36
  3, // 37
  3, // 38
  4, // 39
  2, // 40
  3, // 41
  3, // 42
  4, // 43
  3, // 44
  4, // 45
  4, // 46
  5, // 47
  2, // 48
  3, // 49
  3, // 50
  4, // 51
  3, // 52
  4, // 53
  4, // 54
  5, // 55
  3, // 56
  4, // 57
  4, // 58
  5, // 59
  4, // 60
  5, // 61
  5, // 62
  6, // 63
  1, // 64
  2, // 65
  2, // 66
  3, // 67
  2, // 68
  3, // 69
  3, // 70
  4, // 71
  2, // 72
  3, // 73
  3, // 74
  4, // 75
  3, // 76
  4, // 77
  4, // 78
  5, // 79
  2, // 80
  3, // 81
  3, // 82
  4, // 83
  3, // 84
  4, // 85
  4, // 86
  5, // 87
  3, // 88
  4, // 89
  4, // 90
  5, // 91
  4, // 92
  5, // 93
  5, // 94
  6, // 95
  2, // 96
  3, // 97
  3, // 98
  4, // 99
  3, //100
  4, //101
  4, //102
  5, //103
  3, //104
  4, //105
  4, //106
  5, //107
  4, //108
  5, //109
  5, //110
  6, //111
  3, //112
  4, //113
  4, //114
  5, //115
  4, //116
  5, //117
  5, //118
  6, //119
  4, //120
  5, //121
  5, //122
  6, //123
  5, //124
  6, //125
  6, //126
  7, //127
  1, //128
  2, //129
  2, //130
  3, //131
  2, //132
  3, //133
  3, //134
  4, //135
  2, //136
  3, //137
  3, //138
  4, //139
  3, //140
  4, //141
  4, //142
  5, //143
  2, //144
  3, //145
  3, //146
  4, //147
  3, //148
  4, //149
  4, //150
  5, //151
  3, //152
  4, //153
  4, //154
  5, //155
  4, //156
  5, //157
  5, //158
  6, //159
  2, //160
  3, //161
  3, //162
  4, //163
  3, //164
  4, //165
  4, //166
  5, //167
  3, //168
  4, //169
  4, //170
  5, //171
  4, //172
  5, //173
  5, //174
  6, //175
  3, //176
  4, //177
  4, //178
  5, //179
  4, //180
  5, //181
  5, //182
  6, //183
  4, //184
  5, //185
  5, //186
  6, //187
  5, //188
  6, //189
  6, //190
  7, //191
  2, //192
  3, //193
  3, //194
  4, //195
  3, //196
  4, //197
  4, //198
  5, //199
  3, //200
  4, //201
  4, //202
  5, //203
  4, //204
  5, //205
  5, //206
  6, //207
  3, //208
  4, //209
  4, //210
  5, //211
  4, //212
  5, //213
  5, //214
  6, //215
  4, //216
  5, //217
  5, //218
  6, //219
  5, //220
  6, //221
  6, //222
  7, //223
  3, //224
  4, //225
  4, //226
  5, //227
  4, //228
  5, //229
  5, //230
  6, //231
  4, //232
  5, //233
  5, //234
  6, //235
  5, //236
  6, //237
  6, //238
  7, //239
  4, //240
  5, //241
  5, //242
  6, //243
  5, //244
  6, //245
  6, //246
  7, //247
  5, //248
  6, //249
  6, //250
  7, //251
  6, //252
  7, //253
  7, //254
  8  //255
};


//Mapping from 8 bit unsigned integers to the index of the first one bit.
BYTE const g_first_one[256] = {
  0, //   no one
  0, //   1 the first bit in 0th pos
  1, //   2 the first bit in 1th pos
  0, //   3 the first bit in 0th pos
  2, //   4 the first bit in 2th pos
  0, //   5
  1, //   6
  0, //   7
  3, //   8
  0, //   9
  1, //  10
  0, //  11
  2, //  12
  0, //  13
  1, //  14
  0, //  15
  4, //  16
  0, //  17
  1, //  18
  0, //  19
  2, //  20
  0, //  21
  1, //  22
  0, //  23
  3, //  24
  0, //  25
  1, //  26
  0, //  27
  2, //  28
  0, //  29
  1, //  30
  0, //  31
  5, //  32
  0, //  33
  1, //  34
  0, //  35
  2, //  36
  0, //  37
  1, //  38
  0, //  39
  3, //  40
  0, //  41
  1, //  42
  0, //  43
  2, //  44
  0, //  45
  1, //  46
  0, //  47
  4, //  48
  0, //  49
  1, //  50
  0, //  51
  2, //  52
  0, //  53
  1, //  54
  0, //  55
  3, //  56
  0, //  57
  1, //  58
  0, //  59
  2, //  60
  0, //  61
  1, //  62
  0, //  63
  6, //  64
  0, //  65
  1, //  66
  0, //  67
  2, //  68
  0, //  69
  1, //  70
  0, //  71
  3, //  72
  0, //  73
  1, //  74
  0, //  75
  2, //  76
  0, //  77
  1, //  78
  0, //  79
  4, //  80
  0, //  81
  1, //  82
  0, //  83
  2, //  84
  0, //  85
  1, //  86
  0, //  87
  3, //  88
  0, //  89
  1, //  90
  0, //  91
  2, //  92
  0, //  93
  1, //  94
  0, //  95
  5, //  96
  0, //  97
  1, //  98
  0, //  99
  2, // 100
  0, // 101
  1, // 102
  0, // 103
  3, // 104
  0, // 105
  1, // 106
  0, // 107
  2, // 108
  0, // 109
  1, // 110
  0, // 111
  4, // 112
  0, // 113
  1, // 114
  0, // 115
  2, // 116
  0, // 117
  1, // 118
  0, // 119
  3, // 120
  0, // 121
  1, // 122
  0, // 123
  2, // 124
  0, // 125
  1, // 126
  0, // 127
  7, // 128
  0, // 129
  1, // 130
  0, // 131
  2, // 132
  0, // 133
  1, // 134
  0, // 135
  3, // 136
  0, // 137
  1, // 138
  0, // 139
  2, // 140
  0, // 141
  1, // 142
  0, // 143
  4, // 144
  0, // 145
  1, // 146
  0, // 147
  2, // 148
  0, // 149
  1, // 150
  0, // 151
  3, // 152
  0, // 153
  1, // 154
  0, // 155
  2, // 156
  0, // 157
  1, // 158
  0, // 159
  5, // 160
  0, // 161
  1, // 162
  0, // 163
  2, // 164
  0, // 165
  1, // 166
  0, // 167
  3, // 168
  0, // 169
  1, // 170
  0, // 171
  2, // 172
  0, // 173
  1, // 174
  0, // 175
  4, // 176
  0, // 177
  1, // 178
  0, // 179
  2, // 180
  0, // 181
  1, // 182
  0, // 183
  3, // 184
  0, // 185
  1, // 186
  0, // 187
  2, // 188
  0, // 189
  1, // 190
  0, // 191
  6, // 192
  0, // 193
  1, // 194
  0, // 195
  2, // 196
  0, // 197
  1, // 198
  0, // 199
  3, // 200
  0, // 201
  1, // 202
  0, // 203
  2, // 204
  0, // 205
  1, // 206
  0, // 207
  4, // 208
  0, // 209
  1, // 210
  0, // 211
  2, // 212
  0, // 213
  1, // 214
  0, // 215
  3, // 216
  0, // 217
  1, // 218
  0, // 219
  2, // 220
  0, // 221
  1, // 222
  0, // 223
  5, // 224
  0, // 225
  1, // 226
  0, // 227
  2, // 228
  0, // 229
  1, // 230
  0, // 231
  3, // 232
  0, // 233
  1, // 234
  0, // 235
  2, // 236
  0, // 237
  1, // 238
  0, // 239
  4, // 240
  0, // 241
  1, // 242
  0, // 243
  2, // 244
  0, // 245
  1, // 246
  0, // 247
  3, // 248
  0, // 249
  1, // 250
  0, // 251
  2, // 252
  0, // 253
  1, // 254
  0, // 255
};


BYTE const g_last_one[256] = {
  0, //  0
  0, //  1
  1, //  2
  1, //  3
  2, //  4
  2, //  5
  2, //  6
  2, //  7
  3, //  8
  3, //  9
  3, // 10
  3, // 11
  3, // 12
  3, // 13
  3, // 14
  3, // 15
  4, // 16
  4, // 17
  4, // 18
  4, // 19
  4, // 20
  4, // 21
  4, // 22
  4, // 23
  4, // 24
  4, // 25
  4, // 26
  4, // 27
  4, // 28
  4, // 29
  4, // 30
  4, // 31
  5, // 32
  5, // 33
  5, // 34
  5, // 35
  5, // 36
  5, // 37
  5, // 38
  5, // 39
  5, // 40
  5, // 41
  5, // 42
  5, // 43
  5, // 44
  5, // 45
  5, // 46
  5, // 47
  5, // 48
  5, // 49
  5, // 50
  5, // 51
  5, // 52
  5, // 53
  5, // 54
  5, // 55
  5, // 56
  5, // 57
  5, // 58
  5, // 59
  5, // 60
  5, // 61
  5, // 62
  5, // 63
  6, // 64
  6, // 65
  6, // 66
  6, // 67
  6, // 68
  6, // 69
  6, // 70
  6, // 71
  6, // 72
  6, // 73
  6, // 74
  6, // 75
  6, // 76
  6, // 77
  6, // 78
  6, // 79
  6, // 80
  6, // 81
  6, // 82
  6, // 83
  6, // 84
  6, // 85
  6, // 86
  6, // 87
  6, // 88
  6, // 89
  6, // 90
  6, // 91
  6, // 92
  6, // 93
  6, // 94
  6, // 95
  6, // 96
  6, // 97
  6, // 98
  6, // 99
  6, //100
  6, //101
  6, //102
  6, //103
  6, //104
  6, //105
  6, //106
  6, //107
  6, //108
  6, //109
  6, //110
  6, //111
  6, //112
  6, //113
  6, //114
  6, //115
  6, //116
  6, //117
  6, //118
  6, //119
  6, //120
  6, //121
  6, //122
  6, //123
  6, //124
  6, //125
  6, //126
  6, //127
  7, //128
  7, //129
  7, //130
  7, //131
  7, //132
  7, //133
  7, //134
  7, //135
  7, //136
  7, //137
  7, //138
  7, //139
  7, //140
  7, //141
  7, //142
  7, //143
  7, //144
  7, //145
  7, //146
  7, //147
  7, //148
  7, //149
  7, //150
  7, //151
  7, //152
  7, //153
  7, //154
  7, //155
  7, //156
  7, //157
  7, //158
  7, //159
  7, //160
  7, //161
  7, //162
  7, //163
  7, //164
  7, //165
  7, //166
  7, //167
  7, //168
  7, //169
  7, //170
  7, //171
  7, //172
  7, //173
  7, //174
  7, //175
  7, //176
  7, //177
  7, //178
  7, //179
  7, //180
  7, //181
  7, //182
  7, //183
  7, //184
  7, //185
  7, //186
  7, //187
  7, //188
  7, //189
  7, //190
  7, //191
  7, //192
  7, //193
  7, //194
  7, //195
  7, //196
  7, //197
  7, //198
  7, //199
  7, //200
  7, //201
  7, //202
  7, //203
  7, //204
  7, //205
  7, //206
  7, //207
  7, //208
  7, //209
  7, //210
  7, //211
  7, //212
  7, //213
  7, //214
  7, //215
  7, //216
  7, //217
  7, //218
  7, //219
  7, //220
  7, //221
  7, //222
  7, //223
  7, //224
  7, //225
  7, //226
  7, //227
  7, //228
  7, //229
  7, //230
  7, //231
  7, //232
  7, //233
  7, //234
  7, //235
  7, //236
  7, //237
  7, //238
  7, //239
  7, //240
  7, //241
  7, //242
  7, //243
  7, //244
  7, //245
  7, //246
  7, //247
  7, //248
  7, //249
  7, //250
  7, //251
  7, //252
  7, //253
  7, //254
  7, //255
};


BSIdx ByteOp::get_first_idx(BYTE const* ptr, UINT bytesize)
{
    for (UINT i = 0; i < bytesize; i++) {
        BYTE byte = ptr[i];
        if (byte != (BYTE)0) {
            return g_first_one[byte] + (MULBPB(i));
        }
    }
    return BS_UNDEF;
}


BSIdx ByteOp::get_next_idx(BYTE const* ptr, UINT bytesize, BSIdx idx)
{
    UINT first_byte = DIVBPB(idx);
    if (((UINT)first_byte + 1) > bytesize) {
        return BS_UNDEF;
    }
    BYTE byte = ptr[first_byte];
    UINT posv = MODBPB(idx) + 1; //index in 'first_byte'.
    byte = (BYTE)(byte >> posv); //Erase low 'v' bits.
    byte = (BYTE)(byte << posv);
    if (byte == 0) {
        //No elements in this byte.
        UINT i = first_byte + 1;
        while (i < bytesize) {
            byte = ptr[i];
            if (byte != 0) {
                first_byte = i;
                break;
            }
            i++;
        }
        if (byte == 0) {
            return BS_UNDEF;
        }
    }
    return g_first_one[byte] + (MULBPB(first_byte));
}


UINT ByteOp::get_elem_count(BYTE const* ptr, UINT bytesize)
{
    UINT count = 0;
    for (UINT i = 0; i < bytesize; i++) {
        count += g_bit_count[ptr[i]];
    }
    return count;
}

} //namespace xcom
