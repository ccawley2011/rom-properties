/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * XAttrReader_p.hpp: Extended Attribute reader (PRIVATE CLASS)            *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPFILE_XATTRREADER_P_HPP__
#define __ROMPROPERTIES_LIBRPFILE_XATTRREADER_P_HPP__

// Common macros
#include "common.h"
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

// C++ includes
#include <list>
#include <string>
#include <utility>

namespace LibRpFile {

class XAttrReaderPrivate
{
	public:
		XAttrReaderPrivate(const char *filename);
#ifdef _WIN32
		XAttrReaderPrivate(const wchar_t *filename);
#endif /* _WIN32 */
	private:
		void init(int fd);

	private:
		RP_DISABLE_COPY(XAttrReaderPrivate)

	public:
		/**
		 * Load Linux attributes, if available.
		 * @param fd File descriptor of the open file
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadLinuxAttrs(int fd);

		/**
		 * Load MS-DOS attributes, if available.
		 * @param fd File descriptor of the open file
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadDosAttrs(int fd);

		/**
		 * Load generic xattrs, if available.
		 * (POSIX xattr on Linux; ADS on Windows)
		 * @param fd File descriptor of the open file
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadGenericXattrs(int fd);

	public:
		int lastError;

		bool hasLinuxAttributes;
		bool hasDosAttributes;
		bool hasGenericXAttrs;

		int linuxAttributes;
		unsigned int dosAttributes;
		XAttrReader::XAttrList genericXAttrs;
};

}

#endif /* __ROMPROPERTIES_LIBRPFILE_IRPFILE_HPP__ */
