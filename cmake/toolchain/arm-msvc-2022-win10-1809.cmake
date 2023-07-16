# CMake Toolchain file for Windows 10 on ARM
# Requires MSVC 2022 with ARM toolchain and Windows 10 v1809 SDK.

# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)
SET(CMAKE_SYSTEM_PROCESSOR ARM)
SET(CMAKE_SYSTEM_VERSION 10.0.17763.0)
SET(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION 10.0.17763.0)

# MSVC host binaries directory
SET(MSVC_HOST_BIN_DIR "$ENV{ProgramFiles}/Microsoft Visual Studio/2022")
FOREACH(_msvc_edition Enterprise Professional Community)
	IF(EXISTS "${MSVC_HOST_BIN_DIR}/${_msvc_edition}" AND IS_DIRECTORY "${MSVC_HOST_BIN_DIR}/${_msvc_edition}")
		SET(MSVC_HOST_BIN_DIR "${MSVC_HOST_BIN_DIR}/${_msvc_edition}")
		BREAK()
	ENDIF()
ENDFOREACH(_msvc_edition)
# TODO: Allow different Host architectures?
SET(MSVC_HOST_BIN_DIR "${MSVC_HOST_BIN_DIR}/VC/Tools/MSVC/14.36.32532/bin/Hostx64/arm")

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER	"${MSVC_HOST_BIN_DIR}/cl.exe")
SET(CMAKE_CXX_COMPILER	"${MSVC_HOST_BIN_DIR}/cl.exe")
# NOTE: rc.exe is part of the SDK, not the compiler toolchain.
#SET(CMAKE_RC_COMPILER	"${MSVC_HOST_BIN_DIR}/rc.exe")
