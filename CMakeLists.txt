cmake_minimum_required(VERSION 3.16)
find_program(CMAKE_C_COMPILER NAMES $ENV{CC} clang PATHS ENV PATH NO_DEFAULT_PATH)
find_program(CMAKE_CXX_COMPILER NAMES $ENV{CXX} clang++ PATHS ENV PATH NO_DEFAULT_PATH)

project(OrionUO)

#
# Custom Build Options
#
option(ENABLE_LTO "Enables Link Time Optimization" OFF)
option(ORION_BUILD_MODULES "Enables Modular Project Structure" OFF)
option(ORION_DLL_STATIC "Use Builtin Orion Crypto Library (Windows option)" ON)
option(ORION_WISP "Use WISP pure Win32 API Implementation (Windows option)" ON)
# cotire is unmaintained and its stale-prefix-header detection breaks on every
# header edit. CMake has had native precompiled headers since 3.16, so use those
# by default and keep cotire behind an opt-in for the legacy unity build.
option(ORION_USE_COTIRE "Use cotire for precompiled/unity builds (legacy)" OFF)

list(APPEND CMAKE_MODULE_PATH
  ${CMAKE_SOURCE_DIR}/CMake
)

# Enable folders for IDE
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Libraries to link
set(LIBS)

# CMake Customizations
include(cotire)
include(CheckAndAddFlag)
include(CompileDefinitions)
include(Compiler)
include(CheckCCompilerFlag)
include(CCache)
include(Revision)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Build type (Release/Debug/RelWithDebInfo/MinSizeRel)" FORCE)
endif()

#
# Dependencies
#

#include(CheckLib)
include(CheckCXXSourceRuns)

if(ORION_WINDOWS)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")

	# Architecture used to be part of the generator name - "Visual Studio 15 2017
	# Win64" - and this matched on that. CMake 3.1 moved it to CMAKE_GENERATOR_PLATFORM,
	# set by -A, so on any current generator the old test silently reported 32 bit
	# and the build then looked for x64 libraries in Dependencies/lib/Win32.
	# CMAKE_SIZEOF_VOID_P is the reliable answer and covers both conventions.
	if("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)" OR
	   "${CMAKE_GENERATOR_PLATFORM}" MATCHES "(x64|X64|ARM64)" OR
	   CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(_WIN_TARGET "x64")
		set(ORION_64BITS 1)
	else()
		set(_WIN_TARGET "Win32")
		set(ORION_64BITS 0)
	endif()
	message(STATUS "Windows target: ${_WIN_TARGET}")

	set(DEP_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/Dependencies/include")
	set(DEP_LIB_DIR "${CMAKE_SOURCE_DIR}/Dependencies/lib/${_WIN_TARGET}/")

	set(ZLIB_INCLUDE_DIRS "${DEP_INCLUDE_DIR}")
	set(ZLIB_LIBRARIES "${DEP_LIB_DIR}zlib.lib")

	set(FREEIMAGE_INCLUDE_DIR "${DEP_INCLUDE_DIR}")
	set(FREEIMAGE_LIBRARIES "${DEP_LIB_DIR}freeimage.lib")

	#set(OPENGL_INCLUDE_DIR "${DEP_INCLUDE_DIR}")
	#set(OPENGL_LIBRARIES "${DEP_LIB_DIR}")

	set(GLEW_INCLUDE_DIR "${DEP_INCLUDE_DIR}")
	set(GLEW_LIBRARIES "${DEP_LIB_DIR}glew32.lib")

	set(SDL2_INCLUDE_DIRS "${DEP_INCLUDE_DIR}/SDL2")
	set(SDL2_LIBRARIES "${DEP_LIB_DIR}sdl2.lib" "${DEP_LIB_DIR}sdl2main.lib")

	#set(SDL2_IMAGE_INCLUDE_DIRS "${DEP_INCLUDE_DIR}/SDL2")
	#set(SDL2_IMAGE_LIBRARIES "${DEP_LIB_DIR}")

	# temporary hijacking to use other libs
	set(SDL2_LIBRARIES ${SDL2_LIBRARIES} "${DEP_LIB_DIR}bass.lib" "${DEP_LIB_DIR}bassmidi.lib" "${DEP_LIB_DIR}psapi.lib")
else()
	set(ORION_DLL_STATIC ON)
	set(ORION_WISP OFF)	
	find_package(ZLIB REQUIRED)
	find_package(SDL2 REQUIRED)
	find_package(SDL2_image REQUIRED)
	find_package(SDL2_mixer REQUIRED)
	# The bundled FindSDL2_mixer module predates find_package_handle_standard_args:
	# it sets SDL2MIXER_* and never fails, so check it and normalise the names.
	if(NOT SDL2MIXER_FOUND)
		message(FATAL_ERROR "SDL2_mixer not found (needed for audio)")
	endif()
	set(SDL2_MIXER_LIBRARIES ${SDL2MIXER_LIBRARY})
	set(SDL2_MIXER_INCLUDE_DIRS ${SDL2MIXER_INCLUDE_DIR})
	find_package(FreeImage REQUIRED)
	find_package(GLEW REQUIRED)
	# Modern glew ships a config-module exporting only the GLEW::GLEW target,
	# leaving the legacy GLEW_LIBRARIES/GLEW_LIBRARY variables unset.
	if(TARGET GLEW::GLEW)
		set(GLEW_LIBRARIES GLEW::GLEW)
	endif()
	# Set the preferred opengl library to GLVND if there is GLVND and Legacy.
	# Only if there is no GLVND Legacy will be used.
	# The that is the default with cmake-3.11. Legacy is deprecated.
	# See cmake --help-policy CMP0072
	set (OpenGL_GL_PREFERENCE GLVND)
endif()

find_package(OpenGL REQUIRED)

include_directories(${OPENGL_INCLUDE_DIR})
include_directories(${ZLIB_INCLUDE_DIRS})
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${SDL2_IMAGE_INCLUDE_DIR})
include_directories(${SDL2_MIXER_INCLUDE_DIRS})
include_directories(${FREEIMAGE_INCLUDE_DIR})
include_directories(${GLEW_INCLUDE_DIR})
#
# Project Files
#

configure_file(
  "${PROJECT_SOURCE_DIR}/OrionUO/GitRevision.h.in"
  "${PROJECT_BINARY_DIR}/GitRevision.h"
)
include_directories("${PROJECT_BINARY_DIR}/")

add_subdirectory(OrionUO)
