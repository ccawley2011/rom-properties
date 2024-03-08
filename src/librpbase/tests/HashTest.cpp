/***************************************************************************
 * ROM Properties Page shell extension. (librpbase/tests)                  *
 * HashTest.cpp: Hash class test.                                          *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"

// Hash
#include "../crypto/Hash.hpp"

// C includes. (C++ namespace)
#include <cstdio>

// C++ includes.
#include <iostream>
#include <sstream>
#include <string>
using std::ostringstream;
using std::string;

namespace LibRpBase { namespace Tests {

struct HashTest_mode
{
	const char *str;		// String to hash
	const uint8_t *hash;		// Hash data
	unsigned int hash_len;		// Length of hash data
	Hash::Algorithm algorithm;	// Algorithm

	HashTest_mode(const char *str, const uint8_t *hash, size_t hash_len, Hash::Algorithm algorithm)
		: str(str)
		, hash(hash)
		, hash_len(static_cast<unsigned int>(hash_len))
		, algorithm(algorithm)
	{}
};

class HashTest : public ::testing::TestWithParam<HashTest_mode>
{
	public:
		/**
		 * Compare two byte arrays.
		 * The byte arrays are converted to hexdumps and then
		 * compared using EXPECT_EQ().
		 * @param expected	[in] Expected data.
		 * @param actual	[in] Actual data.
		 * @param size		[in] Size of both arrays.
		 * @param data_type	[in] Data type.
		 */
		void CompareByteArrays(
			const uint8_t *expected,
			const uint8_t *actual,
			size_t size,
			const char *data_type);
};

/**
 * Compare two byte arrays.
 * The byte arrays are converted to hexdumps and then
 * compared using EXPECT_EQ().
 * @param expected	[in] Expected data.
 * @param actual	[in] Actual data.
 * @param size		[in] Size of both arrays.
 * @param data_type	[in] Data type.
 */
void HashTest::CompareByteArrays(
	const uint8_t *expected,
	const uint8_t *actual,
	size_t size,
	const char *data_type)
{
	// Output format: (assume ~64 bytes per line)
	// 0000: 01 23 45 67 89 AB CD EF  01 23 45 67 89 AB CD EF
	const size_t bufSize = ((size / 16) + !!(size % 16)) * 64;
	char printf_buf[16];
	string s_expected, s_actual;
	s_expected.reserve(bufSize);
	s_actual.reserve(bufSize);

	const uint8_t *pE = expected, *pA = actual;
	for (size_t i = 0; i < size; i++, pE++, pA++) {
		if (i % 16 == 0) {
			// New line.
			if (i > 0) {
				// Append newlines.
				s_expected += '\n';
				s_actual += '\n';
			}

			snprintf(printf_buf, sizeof(printf_buf), "%04X: ", static_cast<unsigned int>(i));
			s_expected += printf_buf;
			s_actual += printf_buf;
		}

		// Print the byte.
		snprintf(printf_buf, sizeof(printf_buf), "%02X", *pE);
		s_expected += printf_buf;
		snprintf(printf_buf, sizeof(printf_buf), "%02X", *pA);
		s_actual += printf_buf;

		if (i % 16 == 7) {
			s_expected += "  ";
			s_actual += "  ";
		} else if (i % 16  < 15) {
			s_expected += ' ';
			s_actual += ' ';
		}
	}

	// Compare the byte arrays, and
	// print the strings on failure.
	EXPECT_EQ(0, memcmp(expected, actual, size)) <<
		"Expected " << data_type << ":" << '\n' << s_expected << '\n' <<
		"Actual " << data_type << ":" << '\n' << s_actual << '\n';
}

/**
 * Run a Hash test.
 */
TEST_P(HashTest, hashTest)
{
	const HashTest_mode &mode = GetParam();

	// Maximum hash length is 64 (SHA-512)
	uint8_t hash[64];

	Hash hashObj(mode.algorithm);
	EXPECT_EQ(0, hashObj.process(mode.str, strlen(mode.str)));
	EXPECT_EQ(0, hashObj.getHash(hash, mode.hash_len));

	// Compare the hash to the expected hash.
	CompareByteArrays(mode.hash, hash, mode.hash_len, "hash");

	if (mode.hash_len == 4) {
		// Verify the uint32_t hash.
		const uint32_t expected_hash = (mode.hash[0] << 24) | (mode.hash[1] << 16) | (mode.hash[2] << 8) | mode.hash[3];
		EXPECT_EQ(expected_hash, hashObj.getHash32());
	}
}

/** CRC32 hash tests **/

static const uint8_t crc32_exp[][4] = {
	{0x00,0x00,0x00,0x00},
	{0x51,0x90,0x25,0xE9},
	{0xCF,0xFA,0xFB,0xFD},
	{0x47,0x4B,0x75,0x6F},
	{0xDE,0x90,0xA6,0x7F},
	{0x52,0x5B,0x47,0x99},
};

INSTANTIATE_TEST_SUITE_P(CRC32StringHashTest, HashTest,
	::testing::Values(
		HashTest_mode("", crc32_exp[0], 4, Hash::Algorithm::CRC32),
		HashTest_mode("The quick brown fox jumps over the lazy dog.", crc32_exp[1], 4, Hash::Algorithm::CRC32),
		HashTest_mode("▁▂▃▄▅▆▇█▉▊▋▌▍▎▏", crc32_exp[2], 4, Hash::Algorithm::CRC32),
		HashTest_mode("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.", crc32_exp[3], 4, Hash::Algorithm::CRC32),
		HashTest_mode("ＳＰＹＲＯ　ＴＨＥ　ＤＲＡＧＯＮ", crc32_exp[4], 4, Hash::Algorithm::CRC32),
		HashTest_mode("ソニック カラーズ", crc32_exp[5], 4, Hash::Algorithm::CRC32)
		)
	);

/** MD5 hash tests **/

static const uint8_t md5_exp[][16] = {
	{0xD4,0x1D,0x8C,0xD9,0x8F,0x00,0xB2,0x04,
	 0xE9,0x80,0x09,0x98,0xEC,0xF8,0x42,0x7E},

	{0xE4,0xD9,0x09,0xC2,0x90,0xD0,0xFB,0x1C,
	 0xA0,0x68,0xFF,0xAD,0xDF,0x22,0xCB,0xD0},

	{0x39,0xA1,0x08,0x7C,0x44,0x3E,0xAB,0xCB,
	 0x69,0xBF,0x9F,0xC4,0xD8,0xA3,0x49,0x96},

	{0x81,0x8C,0x6E,0x60,0x1A,0x24,0xF7,0x27,
	 0x50,0xDA,0x0F,0x6C,0x9B,0x8E,0xBE,0x28},

	{0xB6,0xBD,0x99,0xD7,0xB2,0x10,0xAB,0x3B,
	 0x09,0x89,0xD1,0x12,0x8D,0xF9,0x26,0x47},

	{0xFE,0x96,0x0B,0x7E,0x81,0xAE,0x74,0xF0,
	 0xC1,0x05,0xE9,0x0A,0x88,0x40,0x77,0xA0},
};

INSTANTIATE_TEST_SUITE_P(MD5StringHashTest, HashTest,
	::testing::Values(
		HashTest_mode("", md5_exp[0], 16, Hash::Algorithm::MD5),
		HashTest_mode("The quick brown fox jumps over the lazy dog.", md5_exp[1], 16, Hash::Algorithm::MD5),
		HashTest_mode("▁▂▃▄▅▆▇█▉▊▋▌▍▎▏", md5_exp[2], 16, Hash::Algorithm::MD5),
		HashTest_mode("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.", md5_exp[3], 16, Hash::Algorithm::MD5),
		HashTest_mode("ＳＰＹＲＯ　ＴＨＥ　ＤＲＡＧＯＮ", md5_exp[4], 16, Hash::Algorithm::MD5),
		HashTest_mode("ソニック カラーズ", md5_exp[5], 16, Hash::Algorithm::MD5)
		)
	);

/** SHA-1 hash tests **/

static const uint8_t sha1_exp[][20] = {
	{0xDA,0x39,0xA3,0xEE,0x5E,0x6B,0x4B,0x0D,
	 0x32,0x55,0xBF,0xEF,0x95,0x60,0x18,0x90,
	 0xAF,0xD8,0x07,0x09},

	{0x40,0x8D,0x94,0x38,0x42,0x16,0xF8,0x90,
	 0xFF,0x7A,0x0C,0x35,0x28,0xE8,0xBE,0xD1,
	 0xE0,0xB0,0x16,0x21},

	{0x66,0x9B,0x35,0x68,0xF8,0xCE,0x8C,0xE1,
	 0xBD,0xA9,0x91,0x39,0x27,0x10,0xA8,0x1E,
	 0x87,0x95,0xCB,0x81},

	{0xCC,0xA0,0x87,0x1E,0xCB,0xE2,0x00,0x37,
	 0x9F,0x0A,0x1E,0x4B,0x46,0xDE,0x17,0x7E,
	 0x2D,0x62,0xE6,0x55},

	{0x88,0x7B,0x40,0x5B,0x2B,0xFB,0x35,0x58,
	 0x58,0x57,0xC8,0x8A,0xA0,0xDA,0xF8,0x61,
	 0x8A,0x17,0x5E,0x88},

	{0xFC,0x11,0xA3,0x90,0x85,0x41,0xC3,0x09,
	 0xBB,0xEA,0x23,0x64,0x5F,0x4D,0x36,0x5C,
	 0x11,0xDF,0x50,0xC8},
};

INSTANTIATE_TEST_SUITE_P(SHA1StringHashTest, HashTest,
	::testing::Values(
		HashTest_mode("", sha1_exp[0], 20, Hash::Algorithm::SHA1),
		HashTest_mode("The quick brown fox jumps over the lazy dog.", sha1_exp[1], 20, Hash::Algorithm::SHA1),
		HashTest_mode("▁▂▃▄▅▆▇█▉▊▋▌▍▎▏", sha1_exp[2], 20, Hash::Algorithm::SHA1),
		HashTest_mode("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.", sha1_exp[3], 20, Hash::Algorithm::SHA1),
		HashTest_mode("ＳＰＹＲＯ　ＴＨＥ　ＤＲＡＧＯＮ", sha1_exp[4], 20, Hash::Algorithm::SHA1),
		HashTest_mode("ソニック カラーズ", sha1_exp[5], 20, Hash::Algorithm::SHA1)
		)
	);

/** SHA-256 hash tests **/

static const uint8_t sha256_exp[][32] = {
	{0xE3,0xB0,0xC4,0x42,0x98,0xFC,0x1C,0x14,
	 0x9A,0xFB,0xF4,0xC8,0x99,0x6F,0xB9,0x24,
	 0x27,0xAE,0x41,0xE4,0x64,0x9B,0x93,0x4C,
	 0xA4,0x95,0x99,0x1B,0x78,0x52,0xB8,0x55},

	{0xEF,0x53,0x7F,0x25,0xC8,0x95,0xBF,0xA7,
	 0x82,0x52,0x65,0x29,0xA9,0xB6,0x3D,0x97,
	 0xAA,0x63,0x15,0x64,0xD5,0xD7,0x89,0xC2,
	 0xB7,0x65,0x44,0x8C,0x86,0x35,0xFB,0x6C},

	{0x0C,0x60,0xAA,0x79,0x98,0xFD,0x2A,0x75,
	 0x67,0xEA,0x22,0xBA,0xBE,0xD7,0xEC,0x6E,
	 0x09,0xDC,0xA2,0x59,0xED,0xF3,0x0A,0x77,
	 0x4D,0xDA,0xFB,0xAE,0x4E,0x14,0xBE,0x9C},

	{0x97,0x31,0x53,0xF8,0x6E,0xC2,0xDA,0x17,
	 0x48,0xE6,0x3F,0x0C,0xF8,0x5B,0x89,0x83,
	 0x5B,0x42,0xF8,0xEE,0x80,0x18,0xC5,0x49,
	 0x86,0x8A,0x13,0x08,0xA1,0x9F,0x6C,0xA3},

	{0x06,0x90,0x62,0x5B,0x8A,0x7E,0x76,0x0F,
	 0xFC,0x5B,0xBF,0x6E,0x41,0x15,0x20,0xA2,
	 0x51,0xDC,0x41,0x63,0x53,0x50,0x56,0xF0,
	 0x41,0xBC,0xAE,0xAC,0x79,0xF9,0x8F,0xFD},

	{0x91,0xA9,0x25,0xD3,0x4D,0x0E,0xFD,0xC6,
	 0xA0,0x04,0x0B,0xE0,0x93,0x57,0x42,0x3A,
	 0x91,0x05,0xC3,0x5A,0x87,0xA7,0xC1,0x27,
	 0xAE,0x82,0x8B,0xBB,0x20,0xE8,0x66,0x05},
};

INSTANTIATE_TEST_SUITE_P(SHA256StringHashTest, HashTest,
	::testing::Values(
		HashTest_mode("", sha256_exp[0], 32, Hash::Algorithm::SHA256),
		HashTest_mode("The quick brown fox jumps over the lazy dog.", sha256_exp[1], 32, Hash::Algorithm::SHA256),
		HashTest_mode("▁▂▃▄▅▆▇█▉▊▋▌▍▎▏", sha256_exp[2], 32, Hash::Algorithm::SHA256),
		HashTest_mode("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.", sha256_exp[3], 32, Hash::Algorithm::SHA256),
		HashTest_mode("ＳＰＹＲＯ　ＴＨＥ　ＤＲＡＧＯＮ", sha256_exp[4], 32, Hash::Algorithm::SHA256),
		HashTest_mode("ソニック カラーズ", sha256_exp[5], 32, Hash::Algorithm::SHA256)
		)
	);

/** SHA-512 hash tests **/

static const uint8_t sha512_exp[][64] = {
	{0xCF,0x83,0xE1,0x35,0x7E,0xEF,0xB8,0xBD,
	 0xF1,0x54,0x28,0x50,0xD6,0x6D,0x80,0x07,
	 0xD6,0x20,0xE4,0x05,0x0B,0x57,0x15,0xDC,
	 0x83,0xF4,0xA9,0x21,0xD3,0x6C,0xE9,0xCE,
	 0x47,0xD0,0xD1,0x3C,0x5D,0x85,0xF2,0xB0,
	 0xFF,0x83,0x18,0xD2,0x87,0x7E,0xEC,0x2F,
	 0x63,0xB9,0x31,0xBD,0x47,0x41,0x7A,0x81,
	 0xA5,0x38,0x32,0x7A,0xF9,0x27,0xDA,0x3E},

	{0x91,0xEA,0x12,0x45,0xF2,0x0D,0x46,0xAE,
	 0x9A,0x03,0x7A,0x98,0x9F,0x54,0xF1,0xF7,
	 0x90,0xF0,0xA4,0x76,0x07,0xEE,0xB8,0xA1,
	 0x4D,0x12,0x89,0x0C,0xEA,0x77,0xA1,0xBB,
	 0xC6,0xC7,0xED,0x9C,0xF2,0x05,0xE6,0x7B,
	 0x7F,0x2B,0x8F,0xD4,0xC7,0xDF,0xD3,0xA7,
	 0xA8,0x61,0x7E,0x45,0xF3,0xC4,0x63,0xD4,
	 0x81,0xC7,0xE5,0x86,0xC3,0x9A,0xC1,0xED},

	{0xB6,0x60,0x0C,0xFD,0xF9,0x26,0x52,0xAC,
	 0xE2,0x2B,0xC4,0xE9,0x5C,0x1E,0xC9,0x0B,
	 0xB6,0x2B,0x38,0x47,0xC3,0x8B,0x64,0x87,
	 0x86,0xFC,0x3D,0xE6,0x11,0x36,0x1B,0x79,
	 0xE5,0xEA,0x05,0xF6,0x95,0xCE,0xF2,0xFD,
	 0xAC,0xB2,0x82,0x0C,0x1A,0x5E,0x43,0xAF,
	 0xC3,0x34,0x67,0x4F,0x84,0x17,0x59,0x4F,
	 0x70,0xD9,0xAA,0xA9,0x84,0x4F,0x1B,0xEB},

	{0x83,0xCD,0x88,0x66,0xBE,0x23,0x8E,0xDA,
	 0x44,0x7C,0xB0,0xEE,0x94,0xA6,0xBF,0xA6,
	 0x24,0x81,0x09,0x34,0x6B,0x1C,0xE3,0xC7,
	 0x5F,0x8A,0x67,0xD3,0x5F,0x3D,0x8A,0xB1,
	 0x69,0x7B,0x46,0x70,0x30,0x65,0xC0,0x94,
	 0xFC,0xC7,0xD3,0xA6,0x1A,0xCC,0x1E,0x8E,
	 0xE8,0x5A,0x4F,0x30,0x6F,0x13,0xCC,0x1A,
	 0x7A,0xEA,0x76,0x51,0x78,0x11,0x99,0xB3},

	{0x17,0x31,0xF8,0x9E,0x4C,0x42,0x8A,0xD1,
	 0xFE,0x2A,0x09,0x8F,0x34,0xE1,0x12,0x9D,
	 0xA3,0xEA,0xB2,0xF3,0xBD,0xA9,0xE8,0xAF,
	 0xEC,0x05,0x7A,0x29,0x24,0xB0,0x1A,0xA8,
	 0xF3,0x9E,0xE6,0x96,0xD9,0xA2,0xDF,0xDA,
	 0x3F,0x5E,0x27,0x6E,0x33,0x46,0x56,0xB7,
	 0x80,0x33,0xF5,0xD6,0xBE,0x61,0xED,0x8B,
	 0x0A,0xEC,0x59,0xDE,0xA0,0x04,0xFD,0x12},

	{0x31,0x12,0x0A,0xEA,0xB6,0x07,0x21,0xFC,
	 0x5D,0x43,0x9A,0x57,0xD4,0x5E,0x23,0x06,
	 0x13,0x9A,0xCB,0x61,0xC7,0x8C,0xC7,0x10,
	 0x37,0xD4,0x47,0xA0,0x09,0xB6,0xF8,0x7F,
	 0x13,0x9C,0xAC,0x9B,0x8D,0x49,0x3C,0x72,
	 0x20,0x64,0xBB,0xC7,0xE4,0x07,0xAA,0x20,
	 0x19,0x91,0xB0,0xC1,0x91,0xA1,0xE6,0xD0,
	 0x42,0x0C,0x0A,0x2B,0xE3,0x5C,0xD3,0x82},
};

INSTANTIATE_TEST_SUITE_P(SHA512StringHashTest, HashTest,
	::testing::Values(
		HashTest_mode("", sha512_exp[0], 64, Hash::Algorithm::SHA512),
		HashTest_mode("The quick brown fox jumps over the lazy dog.", sha512_exp[1], 64, Hash::Algorithm::SHA512),
		HashTest_mode("▁▂▃▄▅▆▇█▉▊▋▌▍▎▏", sha512_exp[2], 64, Hash::Algorithm::SHA512),
		HashTest_mode("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.", sha512_exp[3], 64, Hash::Algorithm::SHA512),
		HashTest_mode("ＳＰＹＲＯ　ＴＨＥ　ＤＲＡＧＯＮ", sha512_exp[4], 64, Hash::Algorithm::SHA512),
		HashTest_mode("ソニック カラーズ", sha512_exp[5], 64, Hash::Algorithm::SHA512)
		)
	);

} }
