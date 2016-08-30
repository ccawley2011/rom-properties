/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * DialogBuilder.hpp: DLGTEMPLATEEX builder class.                         *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_WIN32_DIALOGBUILDER_HPP__
#define __ROMPROPERTIES_WIN32_DIALOGBUILDER_HPP__

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <cstring>

// rp_string
#include "libromdata/TextFuncs.hpp"

// Standard window classes.
// These macros use the ordinal value, which saves
// space in the generated dialog resource.
#define WC_ORD_BUTTON		MAKEINTRESOURCE(0x0080)
#define WC_ORD_EDIT		MAKEINTRESOURCE(0x0081)
#define WC_ORD_STATIC		MAKEINTRESOURCE(0x0082)
#define WC_ORD_LISTBOX		MAKEINTRESOURCE(0x0083)
#define WC_ORD_SCROLLBAR	MAKEINTRESOURCE(0x0084)
#define WC_ORD_COMBOBOX		MAKEINTRESOURCE(0x0085)

class DialogBuilder
{
	public:
		DialogBuilder();
		~DialogBuilder();

	private:
		DialogBuilder(const DialogBuilder &);
		DialogBuilder &operator=(const DialogBuilder &);

	private:
		/** DLGTEMPLATEEX helper functions. **/
		inline void write_word(WORD w);
		inline void write_wstr(LPCWSTR wstr);
		inline void write_wstr_ord(LPCWSTR wstr);

		inline void align_dword(void);

	public:
		/**
		 * Initialize the DLGTEMPLATEEX.
		 *
		 * DS_SETFONT will always be added to dwStyle,
		 * and the appropriate dialog font will be
		 * added to the dialog structure.
		 *
		 * NOTE: Help ID, menu, and custom dialog classes
		 * are not supported.
		 *
		 * @param lpTemplate	[in] DLGTEMPLATE.
		 * @param lpszTitle	[in, opt] Dialog title.
		 */
		void init(LPCDLGTEMPLATE lpTemplate, LPCWSTR lpszTitle);

		/**
		 * Add a control to the dialog.
		 * @param lpItemTemplate	[in] DLGITEMTEMPLATE.
		 * @param lpszWindowClass	[in] Window class. (May be an ordinal value.)
		 * @param lpszWindowText	[in, opt] Window text. (May be an ordinal value.)
		 */
		void add(const DLGITEMTEMPLATE *lpItemTemplate, LPCWSTR lpszWindowClass, LPCWSTR lpszWindowText);

		/**
		 * Add a control to the dialog.
		 * @param lpItemTemplate	[in] DLGITEMTEMPLATE.
		 * @param lpszWindowClass	[in] Window class. (May be an ordinal value.)
		 * @param lpszWindowText	[in, opt] Window text.
		 */
		void add(const DLGITEMTEMPLATE *lpItemTemplate, LPCWSTR lpszWindowClass, const rp_char *lpszWindowText)
		{
#ifdef RP_UTF8
			add(lpItemTemplate, lpszWindowClass,
				reinterpret_cast<LPCWSTR>(LibRomData::rp_string_to_utf16(
					lpszWindowText, wcslen(lpszWindowText)).c_str()));
#endif
#ifdef RP_UTF16
			add(lpItemTemplate, lpszWindowClass, reinterpret_cast<LPCWSTR>(lpszWindowText));
#endif
		}

		/**
		 * Add a control to the dialog.
		 * @param lpItemTemplate	[in] DLGITEMTEMPLATE.
		 * @param lpszWindowClass	[in] Window class. (May be an ordinal value.)
		 * @param lpszWindowText	[in, opt] Window text.
		 */
		void add(const DLGITEMTEMPLATE *lpItemTemplate, LPCWSTR lpszWindowClass, const LibRomData::rp_string &lpszWindowText)
		{
#ifdef RP_UTF8
			add(lpItemTemplate, lpszWindowClass,
				reinterpret_cast<LPCWSTR>(LibRomData::rp_string_to_utf16(lpszWindowText).c_str()));
#endif
#ifdef RP_UTF16
			add(lpItemTemplate, lpszWindowClass, reinterpret_cast<LPCWSTR>(lpszWindowText.c_str()));
#endif
		}

		/**
		 * Get a pointer to the created DLGTEMPLATEEX.
		 * @return DLGTEMPLATE.
		 */
		LPCDLGTEMPLATE get(void) const;

	protected:
		// DLGTEMPLATEEX data.
		// TODO: Smaller maximum size and/or dynamic allocation?
		uint8_t m_DlgBuf[32768];

		// Current pointer into m_DlgBuf.
		uint8_t *m_pDlgBuf;

		// Pointer to DLGTEMPLATEEX's cDlgItems.
		LPWORD m_pcDlgItems;
};

#endif /* __ROMPROPERTIES_WIN32_DIALOGBUILDER_HPP__ */
