# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Iris-Headers and Iris-File-Extension — the repositories we own.
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Included by both CMake projects (./CMakeLists.txt and javascript/), so the
# native and WASM builds always fetch the same pair. The JavaScript build once
# carried its own copy hard-coded to IrisDigitalPathology, and a fork's WASM job
# then paired the fork's Iris-File-Extension with the Org's older Iris-Headers.
#
# Both are tracked at their tips (GIT_TAG origin/main) and fetched from THIS
# repository's own owner, so a fork proves a coordinated change across the three
# repositories in isolation before it reaches the Org — which keeps building
# the tips it already depends on. The owner comes from GITHUB_REPOSITORY in CI,
# else this checkout's `origin`; override with -DIRIS_DEPS_OWNER=<owner> or a
# full -DIRIS_HEADERS_REPOSITORY / -DIRIS_FILE_EXTENSION_REPOSITORY. There is no
# fallback to IrisDigitalPathology: a fork builds against its own forks, and a
# source tree that cannot name its owner (no GitHub `origin`) must be told. The
# PyPI sdist is: distribute-pypi.yml records the owner in its CMake arguments.
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
include(FetchContent)

set(IRIS_DEPS_OWNER "" CACHE STRING
    "GitHub owner to fetch Iris-Headers / Iris-File-Extension from (empty: origin)")
if (NOT IRIS_DEPS_OWNER)
    if (DEFINED ENV{GITHUB_REPOSITORY})
        # GitHub Actions sets owner/repo, which is authoritative in CI.
        string(REGEX REPLACE "/.*$" "" IRIS_DEPS_OWNER "$ENV{GITHUB_REPOSITORY}")
    else()
        # This file's own directory, so either including project resolves the
        # same checkout.
        execute_process(
            COMMAND git remote get-url origin
            WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
            OUTPUT_VARIABLE _iris_origin OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET RESULT_VARIABLE _iris_origin_rc)
        if (_iris_origin_rc EQUAL 0 AND _iris_origin MATCHES "github\\.com[:/]([^/]+)/")
            set(IRIS_DEPS_OWNER "${CMAKE_MATCH_1}")
        endif()
    endif()
endif()
set(IRIS_HEADERS_REPOSITORY "" CACHE STRING "Iris-Headers git URL (empty: from owner)")
set(IRIS_FILE_EXTENSION_REPOSITORY "" CACHE STRING "Iris-File-Extension git URL (empty: from owner)")
if (NOT IRIS_DEPS_OWNER AND (NOT IRIS_HEADERS_REPOSITORY OR NOT IRIS_FILE_EXTENSION_REPOSITORY))
    message(FATAL_ERROR
        "Cannot tell which GitHub owner to fetch Iris-Headers and Iris-File-Extension "
        "from: GITHUB_REPOSITORY is unset and this source tree has no GitHub `origin` "
        "(git said: '${_iris_origin}'). Pass -DIRIS_DEPS_OWNER=<owner> "
        "(IrisDigitalPathology for the Org's repositories), or full "
        "-DIRIS_HEADERS_REPOSITORY / -DIRIS_FILE_EXTENSION_REPOSITORY URLs.")
endif()
if (NOT IRIS_HEADERS_REPOSITORY)
    set(IRIS_HEADERS_REPOSITORY "https://github.com/${IRIS_DEPS_OWNER}/Iris-Headers.git")
endif()
if (NOT IRIS_FILE_EXTENSION_REPOSITORY)
    set(IRIS_FILE_EXTENSION_REPOSITORY "https://github.com/${IRIS_DEPS_OWNER}/Iris-File-Extension.git")
endif()
message(STATUS "Iris-Headers <- ${IRIS_HEADERS_REPOSITORY}")
message(STATUS "Iris-File-Extension <- ${IRIS_FILE_EXTENSION_REPOSITORY}")

FetchContent_Declare (
    IrisHeaders
    GIT_REPOSITORY "${IRIS_HEADERS_REPOSITORY}"
    GIT_TAG "origin/main"
    GIT_SHALLOW ON
    FETCHCONTENT_UPDATES_DISCONNECTED ON
)
# Do not export the IFE API.
set(IFE_BUILD_SHARED OFF)
set(IFE_BUILD_STATIC OFF)
# NOTE: The following IFE_Export definition
# is IFE_EXPORT= to blank out import/export
# declarations. This is important as we use
# intermediate objects rather than link to
# the Iris File Extension Library (MSVC)
add_compile_definitions(IFE_EXPORT=)
FetchContent_Declare (
    IrisFileExtension
    GIT_REPOSITORY "${IRIS_FILE_EXTENSION_REPOSITORY}"
    GIT_TAG "origin/main"
    GIT_SHALLOW ON
    FETCHCONTENT_UPDATES_DISCONNECTED ON
    FETCHCONTENT_QUIET ON
)
message(STATUS "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")
FetchContent_MakeAvailable(
    IrisHeaders
    IrisFileExtension
)
message(STATUS "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")
