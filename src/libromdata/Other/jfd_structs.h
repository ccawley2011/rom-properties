/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * jfd_structs.h: JFD data structures.                                     *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * Copyright (c) 2018 by Cameron Cawley.                                   *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_JFD_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_JFD_STRUCTS_H__

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * JFD disc image header.
 * This matches the disc image header format exactly.
 * Reference:
 * - https://forums.jaspp.org.uk/forum/viewtopic.php?f=14&t=476
 *
 * All fields are little-endian,
 * except for the magic number.
 */
#define JFD_MAGIC 'JFDI'
typedef struct _JFD_DiscHeader {
	uint32_t magic;			// [0x000] 'JFDI' (big-endian)
	uint32_t min_version;		// [0x004]
	uint32_t filesize;		// [0x008]
	uint32_t disc_sequence;		// [0x00C]
	uint32_t game_id;		// [0x010]
	uint32_t image_version;		// [0x014]
	uint32_t track_offset;		// [0x018]
	uint32_t sector_offset;		// [0x01C]
	uint32_t data_offset;		// [0x020]
	uint32_t delta_track_offset;	// [0x024]
	uint32_t delta_sector_offset;	// [0x028]
	uint32_t delta_data_offset;	// [0x02C]
	char disc_title[256];		// [0x030]
	uint32_t bit_flags;		// [0x130]
	uint8_t fps;			// [0x134]
	uint8_t reserved[3];		// [0x135]
	uint32_t obey_length;		// [0x138]
} JFD_DiscHeader;
ASSERT_STRUCT(JFD_DiscHeader, 316);

// Rotation values.
typedef enum {
	JFD_WRITE_PROTECTED     = 1 << 0,
	JFD_READ_WRITE          = 1 << 1,
	JFD_PROTECT_CMOS        = 1 << 2,
	JFD_PROTECT_MODULES     = 1 << 3,
	JFD_HIDE_HOURGLASS      = 1 << 4,
	JFD_REQUIRE_SHIFT_BREAK = 1 << 5,
	JFD_REMAP_VIDEO_MEMORY  = 1 << 6,

	JFD_ARM3_COMPATIBLE     = 1 << 8,
	JFD_ARM250_COMPATIBLE   = 1 << 9,
	JFD_ARM610_COMPATIBLE   = 1 << 10,
	JFD_ARM7500_COMPATIBLE  = 1 << 11,
	JFD_SA_COMPATIBLE       = 1 << 12,
	JFD_ARMv5_COMPATIBLE    = 1 << 13,

	JFD_RISCOS2_COMPATIBLE  = 1 << 16,
	JFD_RISCOS31_COMPATIBLE = 1 << 17,
	JFD_RISCOS35_COMPATIBLE = 1 << 18,
	JFD_RISCOS37_COMPATIBLE = 1 << 19,
	JFD_RISCOS38_COMPATIBLE = 1 << 20,
	JFD_RISCOS5_COMPATIBLE  = 1 << 21,
	JFD_RISCOS6_COMPATIBLE  = 1 << 22
} JFD_Flags;

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_JFD_STRUCTS_H__ */
