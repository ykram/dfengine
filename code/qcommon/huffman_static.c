/*
===========================================================================
Copyright (C) 2009 Cyril Gantin

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================

Quake 3 uses a prerendered huffman tree to compress netchan messages.

This version is more efficient - eats up less memory, initializes faster - because it uses
a static code instead of feeding the adaptative huffman algorithm with predefined values.

*/

#include "../game/q_shared.h"
#include "qcommon.h"

typedef struct {
	int code:16;
	int nbits:16;
} huff_code_t;

static huff_code_t huff_q3code[] = {
	{ 0x002, 2 },		// 0
	{ 0x01b, 5 },		// 1
	{ 0x048, 7 },		// 2
	{ 0x06c, 7 },		// 3
	{ 0x0a1, 8 },		// 4
	{ 0x011, 8 },		// 5
	{ 0x010, 7 },		// 6
	{ 0x03f, 6 },		// 7
	{ 0x015, 5 },		// 8
	{ 0x034, 7 },		// 9
	{ 0x069, 7 },		// 10
	{ 0x00b, 7 },		// 11
	{ 0x013, 7 },		// 12
	{ 0x02d, 6 },		// 13
	{ 0x039, 8 },		// 14
	{ 0x0ac, 9 },		// 15
	{ 0x025, 7 },		// 16
	{ 0x058, 9 },		// 17
	{ 0x1f0, 9 },		// 18
	{ 0x1f8, 9 },		// 19
	{ 0x1dd, 10 },		// 20
	{ 0x3f3, 10 },		// 21
	{ 0x22b, 10 },		// 22
	{ 0x323, 10 },		// 23
	{ 0x0f4, 9 },		// 24
	{ 0x18d, 10 },		// 25
	{ 0x0ab, 10 },		// 26
	{ 0x363, 10 },		// 27
	{ 0x1eb, 10 },		// 28
	{ 0x043, 8 },		// 29
	{ 0x04f, 9 },		// 30
	{ 0x0d4, 8 },		// 31
	{ 0x037, 6 },		// 32
	{ 0x0d3, 10 },		// 33
	{ 0x044, 9 },		// 34
	{ 0x2cd, 10 },		// 35
	{ 0x3c5, 10 },		// 36
	{ 0x3f9, 10 },		// 37
	{ 0x30d, 10 },		// 38
	{ 0x3cd, 10 },		// 39
	{ 0x094, 9 },		// 40
	{ 0x1ac, 10 },		// 41
	{ 0x033, 10 },		// 42
	{ 0x014, 10 },		// 43
	{ 0x271, 10 },		// 44
	{ 0x2f0, 10 },		// 45
	{ 0x1f4, 9 },		// 46
	{ 0x078, 8 },		// 47
	{ 0x027, 7 },		// 48
	{ 0x0c3, 8 },		// 49
	{ 0x0ef, 8 },		// 50
	{ 0x197, 9 },		// 51
	{ 0x053, 8 },		// 52
	{ 0x0b1, 8 },		// 53
	{ 0x00d, 9 },		// 54
	{ 0x161, 9 },		// 55
	{ 0x007, 9 },		// 56
	{ 0x0f1, 9 },		// 57
	{ 0x199, 9 },		// 58
	{ 0x191, 10 },		// 59
	{ 0x123, 10 },		// 60
	{ 0x0bc, 9 },		// 61
	{ 0x144, 9 },		// 62
	{ 0x1f3, 10 },		// 63
	{ 0x0cf, 8 },		// 64
	{ 0x050, 7 },		// 65
	{ 0x07c, 7 },		// 66
	{ 0x004, 7 },		// 67
	{ 0x021, 8 },		// 68
	{ 0x051, 8 },		// 69
	{ 0x080, 9 },		// 70
	{ 0x070, 9 },		// 71
	{ 0x13d, 9 },		// 72
	{ 0x063, 10 },		// 73
	{ 0x2d7, 10 },		// 74
	{ 0x371, 10 },		// 75
	{ 0x19d, 9 },		// 76
	{ 0x2ab, 10 },		// 77
	{ 0x1c7, 10 },		// 78
	{ 0x333, 10 },		// 79
	{ 0x12c, 9 },		// 80
	{ 0x09d, 10 },		// 81
	{ 0x16b, 10 },		// 82
	{ 0x36b, 10 },		// 83
	{ 0x1d3, 10 },		// 84
	{ 0x171, 10 },		// 85
	{ 0x1e3, 10 },		// 86
	{ 0x233, 10 },		// 87
	{ 0x0d7, 10 },		// 88
	{ 0x2cb, 10 },		// 89
	{ 0x170, 9 },		// 90
	{ 0x0a8, 9 },		// 91
	{ 0x0c7, 9 },		// 92
	{ 0x105, 9 },		// 93
	{ 0x0eb, 9 },		// 94
	{ 0x0d8, 8 },		// 95
	{ 0x0f3, 9 },		// 96
	{ 0x03c, 8 },		// 97
	{ 0x1ab, 9 },		// 98
	{ 0x18f, 9 },		// 99
	{ 0x097, 9 },		// 100
	{ 0x030, 7 },		// 101
	{ 0x041, 8 },		// 102
	{ 0x14f, 9 },		// 103
	{ 0x01c, 6 },		// 104
	{ 0x028, 8 },		// 105
	{ 0x0bd, 9 },		// 106
	{ 0x0c4, 9 },		// 107
	{ 0x098, 8 },		// 108
	{ 0x08f, 9 },		// 109
	{ 0x00c, 8 },		// 110
	{ 0x0b3, 8 },		// 111
	{ 0x085, 8 },		// 112
	{ 0x08c, 8 },		// 113
	{ 0x047, 8 },		// 114
	{ 0x079, 8 },		// 115
	{ 0x059, 7 },		// 116
	{ 0x040, 7 },		// 117
	{ 0x017, 8 },		// 118
	{ 0x019, 8 },		// 119
	{ 0x04b, 8 },		// 120
	{ 0x0e1, 8 },		// 121
	{ 0x0a3, 8 },		// 122
	{ 0x073, 8 },		// 123
	{ 0x06f, 8 },		// 124
	{ 0x068, 7 },		// 125
	{ 0x008, 7 },		// 126
	{ 0x065, 7 },		// 127
	{ 0x01f, 6 },		// 128
	{ 0x029, 7 },		// 129
	{ 0x04c, 7 },		// 130
	{ 0x07d, 7 },		// 131
	{ 0x00f, 8 },		// 132
	{ 0x083, 8 },		// 133
	{ 0x001, 8 },		// 134
	{ 0x087, 8 },		// 135
	{ 0x067, 8 },		// 136
	{ 0x0e7, 8 },		// 137
	{ 0x057, 8 },		// 138
	{ 0x074, 8 },		// 139
	{ 0x1cb, 9 },		// 140
	{ 0x1c4, 9 },		// 141
	{ 0x081, 9 },		// 142
	{ 0x04d, 9 },		// 143
	{ 0x131, 9 },		// 144
	{ 0x163, 10 },		// 145
	{ 0x180, 9 },		// 146
	{ 0x3d7, 10 },		// 147
	{ 0x02b, 10 },		// 148
	{ 0x145, 10 },		// 149
	{ 0x06b, 10 },		// 150
	{ 0x03d, 10 },		// 151
	{ 0x32b, 10 },		// 152
	{ 0x0f9, 10 },		// 153
	{ 0x0e3, 10 },		// 154
	{ 0x245, 10 },		// 155
	{ 0x12b, 10 },		// 156
	{ 0x031, 10 },		// 157
	{ 0x3eb, 10 },		// 158
	{ 0x1b9, 10 },		// 159
	{ 0x114, 9 },		// 160
	{ 0x1f9, 10 },		// 161
	{ 0x133, 10 },		// 162
	{ 0x02c, 10 },		// 163
	{ 0x2dd, 10 },		// 164
	{ 0x1c1, 10 },		// 165
	{ 0x31d, 10 },		// 166
	{ 0x1d1, 10 },		// 167
	{ 0x138, 9 },		// 168
	{ 0x061, 10 },		// 169
	{ 0x2e3, 10 },		// 170
	{ 0x345, 10 },		// 171
	{ 0x26b, 10 },		// 172
	{ 0x0cd, 10 },		// 173
	{ 0x0cb, 10 },		// 174
	{ 0x14d, 10 },		// 175
	{ 0x038, 9 },		// 176
	{ 0x3c1, 10 },		// 177
	{ 0x23d, 10 },		// 178
	{ 0x3bc, 10 },		// 179
	{ 0x0c5, 10 },		// 180
	{ 0x3ac, 10 },		// 181
	{ 0x3e3, 10 },		// 182
	{ 0x299, 10 },		// 183
	{ 0x3d3, 10 },		// 184
	{ 0x214, 10 },		// 185
	{ 0x203, 10 },		// 186
	{ 0x1bc, 10 },		// 187
	{ 0x29d, 10 },		// 188
	{ 0x381, 10 },		// 189
	{ 0x263, 10 },		// 190
	{ 0x08d, 10 },		// 191
	{ 0x054, 8 },		// 192
	{ 0x103, 9 },		// 193
	{ 0x05d, 8 },		// 194
	{ 0x020, 6 },		// 195
	{ 0x009, 7 },		// 196
	{ 0x3c7, 10 },		// 197
	{ 0x307, 10 },		// 198
	{ 0x0b8, 8 },		// 199
	{ 0x1f1, 9 },		// 200
	{ 0x22c, 10 },		// 201
	{ 0x045, 10 },		// 202
	{ 0x003, 10 },		// 203
	{ 0x11d, 10 },		// 204
	{ 0x1c5, 10 },		// 205
	{ 0x34d, 10 },		// 206
	{ 0x01d, 10 },		// 207
	{ 0x000, 9 },		// 208
	{ 0x3b9, 10 },		// 209
	{ 0x0dd, 10 },		// 210
	{ 0x181, 10 },		// 211
	{ 0x10d, 10 },		// 212
	{ 0x0b9, 10 },		// 213
	{ 0x1cd, 10 },		// 214
	{ 0x394, 10 },		// 215
	{ 0x1bd, 10 },		// 216
	{ 0x194, 10 },		// 217
	{ 0x38d, 10 },		// 218
	{ 0x158, 10 },		// 219
	{ 0x3bd, 10 },		// 220
	{ 0x0c1, 10 },		// 221
	{ 0x3dd, 10 },		// 222
	{ 0x0f8, 10 },		// 223
	{ 0x0d1, 9 },		// 224
	{ 0x091, 9 },		// 225
	{ 0x099, 10 },		// 226
	{ 0x2f8, 10 },		// 227
	{ 0x023, 10 },		// 228
	{ 0x071, 10 },		// 229
	{ 0x2d3, 10 },		// 230
	{ 0x391, 10 },		// 231
	{ 0x049, 7 },		// 232
	{ 0x231, 10 },		// 233
	{ 0x107, 10 },		// 234
	{ 0x261, 10 },		// 235
	{ 0x223, 10 },		// 236
	{ 0x018, 8 },		// 237
	{ 0x205, 10 },		// 238
	{ 0x2c1, 10 },		// 239
	{ 0x1d7, 10 },		// 240
	{ 0x0f0, 10 },		// 241
	{ 0x2c5, 10 },		// 242
	{ 0x300, 10 },		// 243
	{ 0x3d1, 10 },		// 244
	{ 0x3a8, 10 },		// 245
	{ 0x21d, 10 },		// 246
	{ 0x500, 11 },		// 247
	{ 0x005, 10 },		// 248
	{ 0x358, 10 },		// 249
	{ 0x2f9, 10 },		// 250
	{ 0x1a8, 10 },		// 251
	{ 0x2b9, 10 },		// 252
	{ 0x28d, 10 },		// 253
	{ 0x02f, 7 },		// 254
	{ 0x024, 6 }		// 255
};

// adapted from huffman.c
static void add_bit(char bit, byte *fout, int *offset) {
	if ((*offset&7) == 0) {
		fout[(*offset>>3)] = 0;
	}
	fout[(*offset>>3)] |= bit << (*offset&7);
	(*offset)++;
}

// adapted from huffman.c
static int get_bit(byte *fin, int *offset) {
	int t;
	t = (fin[(*offset>>3)] >> (*offset&7)) & 0x1;
	(*offset)++;
	return t;
}

static int getSymbolForCode(huff_code_t c) {
	int i;
	for (i=0; i<256; i++) {
		if (huff_q3code[i].nbits == c.nbits && huff_q3code[i].code == c.code) {
			return i;
		}
	}
	return -1;
}
void Huff_offsetReceive(int *ch, byte *fin, int *offset) {
	huff_code_t c;
	memset(&c, 0, sizeof(c));
	c.code |= get_bit(fin, offset) << c.nbits++;
	c.code |= get_bit(fin, offset) << c.nbits++;
	while ((*ch = getSymbolForCode(c)) == -1) {
		c.code |= get_bit(fin, offset) << c.nbits++;
		if (c.nbits	> 11) {
			*ch = 0;
			return;	/* code depth reached */
		}
	}
}

void Huff_offsetTransmit(int ch, byte *fout, int *offset) {
	const huff_code_t *c;
	int i;

	if (ch < 0 || ch > 255) {
		Com_Error(ERR_FATAL, "Huffman compression: invalid symbol 0x%x.", ch);
	}
	c = huff_q3code+ch;
	for (i= 0; i<c->nbits; i++) {
		add_bit((c->code>>i) &1, fout, offset);
	}
}
