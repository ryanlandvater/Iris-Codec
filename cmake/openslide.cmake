include(FindPkgConfig)
pkg_check_modules(OPENSLIDE openslide)
if (OPENSLIDE_FOUND)
    # Best case: .pc loaded and gives up the package vars
    find_library (OPENSLIDE_LIB openslide PATHS  ${OPENSLIDE_LIBDIR})
    set (OPENSLIDE_DIR ${OPENSLIDE_INCLUDE_DIRS})
else ()
    # Maybe the pc didn't work but we can try to just find the files
    find_library (OPENSLIDE_LIB openslide)
    find_path(OPENSLIDE_DIR openslide/openslide.h)
endif()

# OpenSlide was not found on the system, so pull the prebuilt binaries. It is
# deliberately not built from source — its Meson build has proven buggy and
# frustrating. The version comes from Iris-Headers' cmake/dependencies.cmake, and the release
# URL and the extracted directory are BOTH derived from it, so they can never
# disagree (they used to: the log said 4.0.0.6 while the download fetched
# 4.0.0.5, across four platform branches each spelling the version by hand).
set(IRIS_OPENSLIDE_BIN_DIR "openslide-bin-${IRIS_DEP_OPENSLIDE_BIN}")
set(IRIS_OPENSLIDE_BIN_URL
    "https://github.com/openslide/openslide-bin/releases/download/v${IRIS_DEP_OPENSLIDE_BIN}")

# Fetch one prebuilt openslide-bin archive and point the OpenSlide find
# variables at its extracted tree. @_platform is the archive's platform suffix
# (e.g. "linux-x86_64"); @_ext is "tar.xz" or "zip".
macro(iris_fetch_openslide_bin _platform _ext)
    set(_name    "${IRIS_OPENSLIDE_BIN_DIR}-${_platform}")
    set(_archive "${CMAKE_CURRENT_BINARY_DIR}/_deps/openslide.${_ext}")
    message(STATUS "Downloading ${IRIS_OPENSLIDE_BIN_URL}/${_name}.${_ext}...")
    file(DOWNLOAD "${IRIS_OPENSLIDE_BIN_URL}/${_name}.${_ext}" "${_archive}")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar -xf "${_archive}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/_deps/
    )
    find_library(OPENSLIDE_LIB openslide HINTS ${CMAKE_CURRENT_BINARY_DIR}/_deps/${_name}/lib)
    find_path(OPENSLIDE_DIR openslide/openslide.h HINTS ${CMAKE_CURRENT_BINARY_DIR}/_deps/${_name}/include)
endmacro()

if (NOT OPENSLIDE_LIB OR NOT OPENSLIDE_DIR)
    message(STATUS "FAILED to find OpenSlide on your system. We will pull precompiled binaries for openslide - it cannot be built into the system.")
    message(WARNING "Downloading OpenSlide precompiled binaries. It is up to you to ENSURE the numerous openslide dependencies are present at runtime.")
    if (WIN32)
        iris_fetch_openslide_bin("windows-x64" zip)
    elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        iris_fetch_openslide_bin("macos-arm64-x86_64" tar.xz)
    elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "ARM64")
        iris_fetch_openslide_bin("linux-aarch64" tar.xz)
    else()
        iris_fetch_openslide_bin("linux-x86_64" tar.xz)
    endif()
endif(NOT OPENSLIDE_LIB OR NOT OPENSLIDE_DIR)
if (NOT OPENSLIDE_LIB)
    message(FATAL_ERROR "Failed to find OpenSlide. Disable encoder functionality (-DIRIS_BUILD_ENCODER=OFF) to skip OpenSlide requirement.")
endif()
