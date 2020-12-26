/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Archimedes.cpp: Acorn Archimedes disc image reader.                     *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/


#include "stdafx.h"
#include "Archimedes.hpp"
#include "jfd_structs.h"

// librpbase, librpfile
using namespace LibRpBase;
using LibRpFile::IRpFile;

// C++ STL classes.
using std::string;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(Archimedes)

class ArchimedesPrivate final : public RomDataPrivate
{
	public:
		ArchimedesPrivate(Archimedes *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(ArchimedesPrivate)

	public:
		// JFD header.
		JFD_DiscHeader jfdHeader;
};

/** ArchimedesPrivate **/

ArchimedesPrivate::ArchimedesPrivate(Archimedes *q, IRpFile *file)
	: super(q, file)
{
	// Clear the JFD header struct.
	memset(&jfdHeader, 0, sizeof(jfdHeader));
}

/** Archimedes **/

/**
 * Read an Acorn Archimedes disk image
 *
 * A ROM file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM file.
 */
Archimedes::Archimedes(IRpFile *file)
	: super(new ArchimedesPrivate(this, file))
{
	RP_D(Archimedes);
	d->className = "Archimedes";
	d->fileType = FileType::DiskImage;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Seek to the beginning of the header.
	d->file->rewind();

	// Read the JFD header. [0x13C bytes]
	uint8_t header[0x13C];
	size_t size = d->file->read(header, sizeof(header));
	if (size != sizeof(header)) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if this ROM is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(header);
	info.header.pData = header;
	info.ext = nullptr;	// Not needed for Archimedes.
	info.szFile = 0;	// Not needed for Archimedes.
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (d->isValid) {
		// Save the header for later.
		memcpy(&d->jfdHeader, header, sizeof(d->jfdHeader));
	} else {
		d->file->unref();
		d->file = nullptr;
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Archimedes::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < 0x13C)
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check the magic number.
	const JFD_DiscHeader *const jfdHeader =
		reinterpret_cast<const JFD_DiscHeader*>(info->header.pData);
	if (jfdHeader->magic == cpu_to_be32(JFD_MAGIC)) {
		// Found a JFD disc image.
		return 0;
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *Archimedes::systemName(unsigned int type) const
{
	RP_D(const Archimedes);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	static_assert(SYSNAME_TYPE_MASK == 3,
		"Archimedes::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char *const sysNames[4] = {
		"Acorn Archimedes", "Archimedes", "ARC", nullptr,
	};

	unsigned int idx = (type & SYSNAME_TYPE_MASK);
	if (idx >= ARRAY_SIZE(sysNames)) {
		// Invalid index...
		idx &= SYSNAME_TYPE_MASK;
	}

	return sysNames[idx];
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions include the leading dot,
 * e.g. ".bin" instead of "bin".
 *
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *Archimedes::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".jfd",
		nullptr
	};
	return exts;
}

/**
 * Get a list of all supported MIME types.
 * This is to be used for metadata extractors that
 * must indicate which MIME types they support.
 *
 * NOTE: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *Archimedes::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		"application/adffs",

		nullptr
	};
	return mimeTypes;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Archimedes::loadFieldData(void)
{
	RP_D(Archimedes);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	// JFD header.
	const JFD_DiscHeader *const jfdHeader = &d->jfdHeader;
	d->fields->reserve(9);	// Maximum of 9 fields.

	d->fields->addField_string(C_("RomData", "Title"),
		latin1_to_utf8(jfdHeader->disc_title, sizeof(jfdHeader->disc_title)));

	d->fields->addField_string_numeric(C_("RomData", "Game ID"),
		le32_to_cpu(jfdHeader->game_id));

	uint32_t image_version = le32_to_cpu(jfdHeader->image_version);
	d->fields->addField_string_numeric(C_("Archimedes", "Release"),
		image_version >> 20);

	d->fields->addField_string(C_("Archimedes", "Modified"),
		rp_sprintf_p(C_("Archimedes|ImageVer", "%1$u time%2$s"),
			(image_version & 0xFFFFF), (image_version & 0xFFFFF) == 1 ? "" : "s"));

	uint32_t disc_sequence = le32_to_cpu(jfdHeader->disc_sequence);
	d->fields->addField_string(C_("Archimedes", "Disc #"),
		// tr: Disc X of Y (for multi-disc games)
		rp_sprintf_p(C_("Archimedes|Disc", "%1$u of %2$u"),
			(disc_sequence & 0xFFFF), (disc_sequence >> 16)));

	if (le32_to_cpu(jfdHeader->min_version) >= 204) {
		uint32_t bit_flags = le32_to_cpu(jfdHeader->bit_flags);
		// General flags.
		static const char *const flags_bitfield_names[] = {
			NOP_C_("Archimedes|Flags", "Write protected"),
			NOP_C_("Archimedes|Flags", "Read-Write"),
			NOP_C_("Archimedes|Flags", "Protect CMOS"),
			NOP_C_("Archimedes|Flags", "Protect Modules"),
			NOP_C_("Archimedes|Flags", "Hide Hourglass"),
			NOP_C_("Archimedes|Flags", "Requires Shift-Break"),
			NOP_C_("Archimedes|Flags", "Remap video memory"),
		};
		vector<string> *const v_flags_bitfield_names = RomFields::strArrayToVector_i18n(
			"Archimedes|Flags", flags_bitfield_names, ARRAY_SIZE(flags_bitfield_names));
		d->fields->addField_bitfield(C_("Archimedes", "Flags"),
			v_flags_bitfield_names, 3, bit_flags);

		// Supported ARM versions.
		static const char *const cpu_bitfield_names[] = {
			NOP_C_("Archimedes|CPU", "ARM3"),
			NOP_C_("Archimedes|CPU", "ARM250"),
			NOP_C_("Archimedes|CPU", "ARM610/710"),
			NOP_C_("Archimedes|CPU", "ARM7500"),
			NOP_C_("Archimedes|CPU", "StrongARM"),
			NOP_C_("Archimedes|CPU", "ARMv5/v6/v7"),
		};
		vector<string> *const v_cpu_bitfield_names = RomFields::strArrayToVector_i18n(
			"Archimedes|CPU", cpu_bitfield_names, ARRAY_SIZE(cpu_bitfield_names));
		d->fields->addField_bitfield(C_("Archimedes", "Supported CPUs"),
			v_cpu_bitfield_names, 3, bit_flags >> 8);

		// Supported RISC OS versions.
		static const char *const os_version_bitfield_names[] = {
			NOP_C_("Archimedes|OSVer", "2"),
			NOP_C_("Archimedes|OSVer", "3.1"),
			NOP_C_("Archimedes|OSVer", "3.5"),
			NOP_C_("Archimedes|OSVer", "3.7"),
			NOP_C_("Archimedes|OSVer", "3.8 / 4.x"),
			NOP_C_("Archimedes|OSVer", "5.x"),
			NOP_C_("Archimedes|OSVer", "6.x"),
		};
		vector<string> *const v_os_version_bitfield_names = RomFields::strArrayToVector_i18n(
			"Archimedes|OSVer", os_version_bitfield_names, ARRAY_SIZE(os_version_bitfield_names));
		d->fields->addField_bitfield(C_("Archimedes", "Supported RISC OS versions"),
			v_os_version_bitfield_names, 3, bit_flags >> 16);

		d->fields->addField_string_numeric(C_("Archimedes", "Frames per second"),
			jfdHeader->fps / 2);
	}

	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int Archimedes::loadMetaData(void)
{
	RP_D(Archimedes);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();

	// JFD header.
	const JFD_DiscHeader *const jfdHeader = &d->jfdHeader;
	d->metaData->reserve(3);	// Maximum of 3 metadata properties.

	// Title.
	if (jfdHeader->disc_title[0] != 0) {
		d->metaData->addMetaData_string(Property::Title,
			latin1_to_utf8(jfdHeader->disc_title, sizeof(jfdHeader->disc_title)));
	}

	// Disc number. (multiple disc sets only)
	uint32_t disc_sequence = le32_to_cpu(jfdHeader->disc_sequence);
	if ((disc_sequence & 0xFFFF) != 0 && (disc_sequence >> 16) > 1) {
		d->metaData->addMetaData_integer(Property::DiscNumber, (disc_sequence & 0xFFFF));
	}

	if (le32_to_cpu(jfdHeader->min_version) >= 204) {
		// Frame rate.
		if (jfdHeader->fps != 0) {
			d->metaData->addMetaData_integer(Property::FrameRate, jfdHeader->fps / 2);
		}
	}

	// Finished reading the metadata.
	return static_cast<int>(d->fields->count());
}

}
