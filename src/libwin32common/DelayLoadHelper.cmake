# DelayLoadHelper macros.
MACRO(SET_DELAY_LOAD_FLAGS)
IF(MSVC)
	UNSET(DL_DLLS)
	IF(NOT USE_INTERNAL_ZLIB OR USE_INTERNAL_ZLIB_DLL)
		SET(DL_DLLS ${DL_DLLS} zlib1)
	ENDIF(NOT USE_INTERNAL_ZLIB OR USE_INTERNAL_ZLIB_DLL)
	IF(NOT USE_INTERNAL_PNG OR USE_INTERNAL_PNG_DLL)
		SET(DL_DLLS ${DL_DLLS} libpng16)
	ENDIF(NOT USE_INTERNAL_PNG OR USE_INTERNAL_PNG_DLL)
	IF(NOT USE_INTERNAL_JPEG OR USE_INTERNAL_JPEG_DLL)
		SET(DL_DLLS ${DL_DLLS} jpeg62)
	ENDIF(NOT USE_INTERNAL_JPEG OR USE_INTERNAL_JPEG_DLL)
	IF(NOT USE_INTERNAL_XML OR USE_INTERNAL_XML_DLL)
		SET(DL_DLLS ${DL_DLLS} tinyxml2)
	ENDIF(NOT USE_INTERNAL_XML OR USE_INTERNAL_XML_DLL)

	SET(DL_DEBUG_FLAGS "/ignore:4199")
	SET(DL_RELEASE_FLAGS "/ignore:4199")
	FOREACH(_dll ${DL_DLLS})
		SET(DL_DEBUG_FLAGS "${DL_DEBUG_FLAGS} /DELAYLOAD:${_dll}d.dll")
		SET(DL_RELEASE_FLAGS "${DL_RELEASE_FLAGS} /DELAYLOAD:${_dll}.dll")
	ENDFOREACH()

	# libgnuintl-8.dll is precompiled. (Release build only)
	IF(ENABLE_NLS)
		SET(DL_DEBUG_FLAGS "${DL_DEBUG_FLAGS} /DELAYLOAD:libgnuintl-8.dll")
		SET(DL_RELEASE_FLAGS "${DL_RELEASE_FLAGS} /DELAYLOAD:libgnuintl-8.dll")
	ENDIF(ENABLE_NLS)

	SET(CMAKE_EXE_LINKER_FLAGS_DEBUG    "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${DL_DEBUG_FLAGS}")
	SET(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} ${DL_DEBUG_FLAGS}")
	SET(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} ${DL_DEBUG_FLAGS}")

	SET(CMAKE_EXE_LINKER_FLAGS_RELEASE    "${CMAKE_EXE_LINKER_FLAGS_RELEASE} ${DL_RELEASE_FLAGS}")
	SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} ${DL_RELEASE_FLAGS}")
	SET(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} ${DL_RELEASE_FLAGS}")

	UNSET(DL_DEBUG_FLAGS)
	UNSET(DL_RELEASE_FLAGS)
	UNSET(DL_DLLS)
ENDIF(MSVC)
ENDMACRO(SET_DELAY_LOAD_FLAGS)
