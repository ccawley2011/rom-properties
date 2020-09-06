/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoDS.hpp: Nintendo DS(i) ROM reader. (ROM operations)             *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "NintendoDS.hpp"
#include "NintendoDS_p.hpp"
#include "ndscrypt.hpp"

// librpbase
using LibRpBase::RomData;

// C++ STL classes.
using std::unique_ptr;
using std::vector;

namespace LibRomData {

/** NintendoDSPrivate **/

/**
 * Check the NDS security data.
 *
 * $1000-$3FFF is normally unreadable on hardware, so this
 * area is usually blank in dumped ROMs. However, this area
 * normally has precomputed Blowfish tables and other data,
 * which are used as part of the NDS security system.
 * DSiWare and Wii U VC SRLs, as well as SRLs generated by
 * the DS SDK, will have actual data here.
 *
 * @return NDS security data flags.
 */
uint32_t NintendoDSPrivate::checkNDSSecurityData(void)
{
	// TODO: Verify the entire area. (Not sure if we can calculate the
	// Random data correctly...)
	uint32_t ret = 0;
	if (!file || !file->isOpen()) {
		// File was closed...
		return ret;
	}

	uint32_t security_data[0x3000/4];
	size_t size = file->seekAndRead(0x1000, security_data, sizeof(security_data));
	if (size != sizeof(security_data)) {
		// Seek and/or read error.
		return 0;
	}

	// Check the S-Box and Blowfish tables for non-zero data.
	// S-Box: 0x1600
	// Blowfish: 0x1C00
	if (security_data[0x0600/4] != 0 && security_data[0x0C00/4] != 0) {
		ret |= NDS_SECDATA_BLOWFISH;
	}

	// Check the static area.
	if (security_data[0x2000/4] == be32_to_cpu(0xFF00FF00) &&
	    security_data[0x2004/4] == be32_to_cpu(0xAA55AA55) &&
	    security_data[0x2008/4] == be32_to_cpu(0x08090A0B) &&
	    security_data[0x200C/4] == be32_to_cpu(0x0C0D0E0F) &&
	    security_data[0x2200/4] == be32_to_cpu(0xFFFEFDFC) &&
	    security_data[0x2204/4] == be32_to_cpu(0xFBFAF9F8) &&
	    security_data[0x2400/4] == be32_to_cpu(0x00000000) &&
	    security_data[0x2600/4] == be32_to_cpu(0xFFFFFFFF) &&
	    security_data[0x2800/4] == be32_to_cpu(0x0F0F0F0F) &&
	    security_data[0x2A00/4] == be32_to_cpu(0xF0F0F0F0) &&
	    security_data[0x2C00/4] == be32_to_cpu(0x55555555) &&
	    security_data[0x2E00/4] == be32_to_cpu(0xAAAAAAAA))
	{
		// Static area appears to be valid.
		ret |= NDS_SECDATA_STATIC;
	}

	// Check for random values at other areas.
	if (security_data[0x0000/4] != 0 ||
	    security_data[0x0700/4] != 0 ||
	    security_data[0x1C00/4] != 0)
	{
		// Non-zero value in one of these areas.
		// Random data is present.
		ret |= NDS_SECDATA_RANDOM;
	}

	return ret;
}

/**
 * Check the NDS Secure Area type.
 * @return Secure area type.
 */
NintendoDSPrivate::NDS_SecureArea NintendoDSPrivate::checkNDSSecureArea(void)
{
	if (!file || !file->isOpen()) {
		// File was closed...
		return NDS_SECAREA_UNKNOWN;
	}

	// Read the start of the Secure Area.
	// NOTE: We only need to check the first two DWORDs, but
	// we're reading the first four because CIAReader only
	// supports multiples of 16 bytes right now.
	uint32_t secure_area[4];
	size_t size = file->seekAndRead(0x4000, secure_area, sizeof(secure_area));
	if (size != sizeof(secure_area)) {
		// Seek and/or read error.
		return NDS_SECAREA_UNKNOWN;
	}

	// Reference: https://github.com/devkitPro/ndstool/blob/master/source/header.cpp#L39

	NDS_SecureArea ret;
	//bool needs_encryption = false;	// TODO
	if (le32_to_cpu(romHeader.arm9.rom_offset) < 0x4000) {
		// ARM9 secure area is not present.
		// This is only valid for homebrew.
		ret = NDS_SECAREA_HOMEBREW;
	} else if (secure_area[0] == cpu_to_le32(0x00000000) && secure_area[1] == cpu_to_le32(0x00000000)) {
		// Secure area is empty. (DS Download Play)
		ret = NDS_SECAREA_MULTIBOOT;
	} else if (secure_area[0] == cpu_to_le32(0xE7FFDEFF) && secure_area[1] == cpu_to_le32(0xE7FFDEFF)) {
		// Secure area is decrypted.
		// Probably dumped using wooddumper or Decrypt9WIP.
		ret = NDS_SECAREA_DECRYPTED;
		//needs_encryption = true;	// CRC requires encryption.
	} else {
		// Secure area is encrypted.
		ret = NDS_SECAREA_ENCRYPTED;
	}

	// TODO: Verify the CRC?
	// For decrypted ROMs, this requires re-encrypting the secure area.
	return ret;
}

/** NintendoDS **/

/**
 * Get the list of operations that can be performed on this ROM.
 * Internal function; called by RomData::romOps().
 * @return List of operations.
 */
vector<RomData::RomOps> NintendoDS::romOps_int(void) const
{
	// Determine if the ROM is trimmed and/or encrypted.
	// TODO: Cache the vector?
	vector<RomOps> ops;
#ifdef ENABLE_DECRYPTION
	ops.resize(2);
#else /* !ENABLE_DECRYPTION */
	ops.resize(1);
#endif /* ENABLE_DECRYPTION */

	RP_D(NintendoDS);
	uint32_t flags;

	// Trim/Untrim ROM
	bool showUntrim = false;
	if (likely(d->romSize > 0)) {
		// Determine if the ROM is trimmed.

		const uint32_t total_used_rom_size = d->totalUsedRomSize();
		if (total_used_rom_size == d->romSize && isPow2(d->romSize)) {
			// ROM is technically trimmed, but it's already a power of two.
			// Cannot trim/untrim, so show the "Trim ROM" option but disabled.
			showUntrim = false;
			flags = 0;
		} else {
			showUntrim = !(total_used_rom_size < d->romSize);
			flags = RomOps::ROF_ENABLED;
		}
	} else {
		// Empty ROM?
		flags = 0;
	}
	ops[0].desc = showUntrim
		? C_("NintendoDS|RomOps", "&Untrim ROM")
		: C_("NintendoDS|RomOps", "&Trim ROM");
	ops[0].flags = flags;

#ifdef ENABLE_DECRYPTION
	// Encrypt/Decrypt ROM
	bool showEncrypt = false;
	switch (d->secArea) {
		case NintendoDSPrivate::NDS_SECAREA_DECRYPTED:
			showEncrypt = true;
			flags = RomOps::ROF_ENABLED;
			break;
		case NintendoDSPrivate::NDS_SECAREA_ENCRYPTED:
			showEncrypt = false;
			flags = RomOps::ROF_ENABLED;
			break;
		default:
			showEncrypt = false;
			flags = 0;
			break;
	}
	ops[1].desc = showEncrypt
		? C_("NintendoDS|RomOps", "Encrypt ROM")
		: C_("NintendoDS|RomOps", "Decrypt ROM");
	ops[1].flags = flags;
#endif /* ENABLE_DECRYPTION */

	return ops;
}

/**
 * Perform a ROM operation.
 * Internal function; called by RomData::doRomOp().
 * @param id Operation index.
 * @return 0 on success; negative POSIX error code on error.
 */
int NintendoDS::doRomOp_int(int id)
{
	RP_D(NintendoDS);
	int ret = 0;

	printf("DO OP: %d\n", id);
	switch (id) {
		case 0: {
			// Trim/untrim ROM.
			// Trim = reduce ROM to minimum size as indicated by header.
			// Untrim = expand to power of 2 size, filled with 0xFF.
			const uint32_t total_used_rom_size = d->totalUsedRomSize();
			if (!(total_used_rom_size < d->romSize)) {
				// ROM is trimmed. Untrim it.
				const off64_t next_pow2 = (1LL << (uilog2(total_used_rom_size) + 1));
				assert(next_pow2 > total_used_rom_size);
				if (next_pow2 <= total_used_rom_size) {
					// Something screwed up here...
					return -EIO;
				}

#define UNTRIM_BLOCK_SIZE (64*1024)
				unique_ptr<uint8_t[]> ff_block(new uint8_t[UNTRIM_BLOCK_SIZE]);
				memset(ff_block.get(), 0xFF, UNTRIM_BLOCK_SIZE);

				off64_t pos = static_cast<off64_t>(total_used_rom_size);
				int ret = d->file->seek(pos);
				if (ret != 0) {
					// Seek error.
					int err = d->file->lastError();
					if (err == 0) {
						err = EIO;
					}
					return -err;
				}

				// If we're not aligned to the untrim block size,
				// write a partial block.
				const unsigned int partial = static_cast<unsigned int>(pos % UNTRIM_BLOCK_SIZE);
				if (partial != 0) {
					const unsigned int toWrite = UNTRIM_BLOCK_SIZE - partial;
					size_t size = d->file->write(ff_block.get(), toWrite);
					if (size != toWrite) {
						// Write error.
						int err = d->file->lastError();
						if (err == 0) {
							err = EIO;
						}
						return -err;
					}
					pos += toWrite;
				}

				// Write remaining full blocks.
				for (; pos < next_pow2; pos += UNTRIM_BLOCK_SIZE) {
					size_t size = d->file->write(ff_block.get(), UNTRIM_BLOCK_SIZE);
					if (size != UNTRIM_BLOCK_SIZE) {
						// Write error.
						int err = d->file->lastError();
						if (err == 0) {
							err = EIO;
						}
						return -err;
					}
				}

				// ROM untrimmed.
				d->file->flush();
				d->romSize = d->file->size();
			} else if (total_used_rom_size < d->romSize) {
				// ROM is untrimmed. Trim it.
				d->file->truncate(total_used_rom_size);
				d->file->flush();
				d->romSize = d->file->size();
			} else {
				// ROM can't be trimmed or untrimmed...
				assert(!"ROM can't be trimmed or untrimmed; menu option should have been disabled.");
			}
			break;
		}

#ifdef ENABLE_DECRYPTION
		case 1: {
			// Encrypt/Decrypt ROM.
			bool doEncrypt;
			if (d->secArea == NintendoDSPrivate::NDS_SECAREA_DECRYPTED) {
				// Encrypt the secure area.
				doEncrypt = true;
			} else if (d->secArea == NintendoDSPrivate::NDS_SECAREA_ENCRYPTED) {
				// Decrypt the secure area.
				doEncrypt = false;
				break;
			} else {
				// Cannot perform this ROM operation.
				assert(!"Secure area cannot be adjusted.");
				ret = -ENOTSUP;
				break;
			}

			// Make sure nds-blowfish.bin is loaded.
			int ret = ndscrypt_load_nds_blowfish_bin();
			if (ret != 0) {
				// TODO: Show an error message.
				ret = -EIO;
				break;
			}

			// Load the first 32 KB.
#define SEC_AREA_SIZE (32*1024)
			unique_ptr<uint8_t[]> buf(new uint8_t[SEC_AREA_SIZE]);
			d->file->rewind();
			size_t size = d->file->read(buf.get(), SEC_AREA_SIZE);
			if (size != SEC_AREA_SIZE) {
				// Seek and/or read error.
				// TODO: Show an error message.
				ret = -d->file->lastError();
				if (ret == 0) {
					ret = -EIO;
				}
				break;
			}

			// Run the actual encryption.
			// TODO: Decrypt function.
			ret = doEncrypt
				? ndscrypt_encrypt_secure_area(buf.get(), SEC_AREA_SIZE)
				: -ENOTSUP;
			if (ret != 0) {
				// Error encrypting/decrypting.
				// TODO: Show an error message.
				break;
			}

			// Write the data back to the ROM.
			d->file->rewind();
			size = d->file->write(buf.get(), SEC_AREA_SIZE);
			if (size != SEC_AREA_SIZE) {
				// Seek and/or write error.
				// TODO: Show an error message.
				ret = -d->file->lastError();
				if (ret == 0) {
					ret = -EIO;
				}
				break;
			}

			// Update the secData and secArea values.
			d->secData = d->checkNDSSecurityData();
			d->secArea = d->checkNDSSecureArea();
			break;
		}
#endif /* ENABLE_DECRYPTION */
	}

	return ret;
}

}
