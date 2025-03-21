# 2025 Copyright Ryan Landvater
# Created 3/3/2025

cmake_minimum_required(VERSION 3.27)
include(FetchContent)
include(ExternalProject)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(IRIS_BUILD_SHARED "Build IrisCodec Shared Library" ON)
option(IRIS_BUILD_STATIC "Build IrisCodec Static Library" ON)
option(IRIS_BUILD_ENCODER "Build the IrisCodec Encoder executable" ON)
option(IRIS_BUILD_PYTHON "Build IrisCodec Python modules" OFF)
option(IRIS_BUILD_DEPENDENCIES "Build all dependencies and statically link into self-contained binary" OFF)

if (IRIS_BUILD_DEPENDENCIES)
    message(STATUS "Iris Codec is configured to build all dependencies and statically link into a single self-contained binary.")
endif()
if (IRIS_BUILD_PYTHON)
    message(STATUS "Iris Codec is configured to build Python Module")
endif()


function(get_codec_version)
    set(codec_priv_header "${CMAKE_CURRENT_SOURCE_DIR}/src/IrisCodecPriv.hpp")
    if (NOT EXISTS ${codec_priv_header})
        message(FATAL_ERROR "Could NOT locate Iris Codec Private Header (IrisCodecPriv.hpp)")
    endif()

    file(READ ${codec_priv_header} header)

    if (header MATCHES "#define CODEC_MAJOR_VERSION[ ]*([0-9]+)")
        set(MAJOR_VERSION "${CMAKE_MATCH_1}")
    else () 
        message(FATAL_ERROR "Couldn't parse the Iris Codec major version")
    endif()

    if (header MATCHES "#define CODEC_MINOR_VERSION[ ]*([0-9]+)")
        set(MINOR_VERSION "${CMAKE_MATCH_1}")
    else () 
        message(FATAL_ERROR "Couldn't parse the Iris Codec minor version")
    endif()

    if (header MATCHES "#define CODEC_BUILD_NUMBER[ ]*([0-9]+)")
        set(BUILD_NUMBER "${CMAKE_MATCH_1}")
    else () 
        message(FATAL_ERROR "Couldn't parse the Iris Codec build iteration")
    endif()

    set(VERSION_STRING "${MAJOR_VERSION}.${MINOR_VERSION}.${BUILD_NUMBER}" PARENT_SCOPE)
endfunction()
get_codec_version()
message(STATUS "BUILDING IRIS CODEC COMMUNITY MODULE VERSION v" ${VERSION_STRING})

PROJECT(
    IrisCodec
    LANGUAGES CXX
    VERSION ${VERSION_STRING}
)

FetchContent_Declare (
    IrisHeaders
    GIT_REPOSITORY https://github.com/IrisDigitalPathology/Iris-Headers.git
    GIT_TAG "origin/main"
    GIT_SHALLOW ON
    FETCHCONTENT_UPDATES_DISCONNECTED ON
)
FetchContent_Declare (
    VulkanHeaders
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG "cacef30"    #"origin/main"
    GIT_SHALLOW ON
    FIND_PACKAGE_ARGS NAMES Vulkan
    FETCHCONTENT_UPDATES_DISCONNECTED ON
)
FetchContent_Declare (
    VkMemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG "c788c52" #"origin/master"
    GIT_SHALLOW ON
    FETCHCONTENT_UPDATES_DISCONNECTED ON
)
FetchContent_Populate (
    GLM
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG "2d4c4b4" #"origin/master"
    GIT_SHALLOW ON
    FETCHCONTENT_UPDATES_DISCONNECTED ON
    FETCHCONTENT_QUIET ON
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/glm-src
    SUBBUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/glm-subbuild
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/glm-build
)
FetchContent_Populate (
    IrisFileExtension
    GIT_REPOSITORY https://github.com/IrisDigitalPathology/Iris-File-Extension.git
    GIT_TAG "origin/main"
    GIT_SHALLOW ON
    FETCHCONTENT_UPDATES_DISCONNECTED ON
    FETCHCONTENT_QUIET ON
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/irisfileextension-src
    SUBBUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/irisfileextension-subbuild
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/irisfileextension-build
)
FetchContent_MakeAvailable(
    IrisHeaders 
    VulkanHeaders 
    VkMemoryAllocator
)


include(./cmake/turbo-jpeg.cmake)
include(./cmake/avif.cmake)

set(CODEC_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(CODEC_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(IFE_SOURCE_DIR ${irisfileextension_SOURCE_DIR}/src)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Iris Codec Universal Object Build
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
set (
    IrisCodecSources
    ${IFE_SOURCE_DIR}/IrisCodecExtension.cpp
    ${CODEC_SOURCE_DIR}/IrisCodecContext.cpp
    ${CODEC_SOURCE_DIR}/IrisCodecFile.cpp
    ${CODEC_SOURCE_DIR}/IrisCodecCache.cpp
    ${CODEC_SOURCE_DIR}/IrisCodecSlide.cpp
    ${irisheaders_SOURCE_DIR}/src/IrisBuffer.cpp
)
set (
    IrisCodecDependencies
    ${TURBOJPEG_LIBRARY}
    ${AVIF_LIBRARY}
)
# if (IRIS_BUILD_ENCODER)
#     message(STATUS "BUILDING IRIS CODEC ENCODER: Encoding from vendor files requires Openslide")
#     find_library (OPENSLIDE_LIB openslide REQUIRED)
#     message(STATUS "Found OPENSLIDE_LIB: ${OPENSLIDE_LIB}")
#     set(IrisCodecSources ${IrisCodecSources} ${CODEC_SOURCE_DIR}/IrisCodecEncoder.cpp)
# endif(IRIS_BUILD_ENCODER)

add_library(
    IrisCodecLib OBJECT
    ${IrisCodecSources}
)
target_include_directories (
    IrisCodecLib PRIVATE
    ${IFE_SOURCE_DIR}
    ${glm_SOURCE_DIR}
    ${irisheaders_SOURCE_DIR}/priv
    ${vkmemoryallocator_SOURCE_DIR}/include
    ${vulkanheaders_SOURCE_DIR}/include
)
target_link_libraries (
    IrisCodecLib
    PUBLIC IrisHeaders
)
if (TURBOJPEG_EXTERNAL_PROJECT_ADD) 
    add_dependencies(IrisCodecLib TurboJpeg)
endif()
if (AVIF_EXTERNAL_PROJECT_ADD) 
    add_dependencies(IrisCodecLib Avif)
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Iris Codec Shared Lib Build
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

if (IRIS_BUILD_SHARED)
    add_library(
        IrisCodec SHARED 
        $<TARGET_OBJECTS:IrisCodecLib>
    )
    target_link_libraries(
        IrisCodec
        PRIVATE ${TURBOJPEG_LIBRARY}
        PRIVATE ${AVIF_LIBRARY}
    )
endif(IRIS_BUILD_SHARED)

if (IRIS_BUILD_STATIC)
    add_library(
        IrisCodecStatic STATIC 
        $<TARGET_OBJECTS:IrisCodecLib>
    )
    target_link_libraries(
        IrisCodecStatic
        PRIVATE ${TURBOJPEG_LIBRARY}
        PRIVATE ${AVIF_LIBRARY}
    )
endif(IRIS_BUILD_STATIC)

if(IRIS_BUILD_ENCODER)
    # message(STATUS "Encoder requires Openslide")
    # find_library (OPENSLIDE_LIB openslide REQUIRED)
    # message(STATUS "Found OPENSLIDE_LIB: ${OPENSLIDE_LIB}")
    # add_library (
    #     EncoderObject OBJECT
    #     $<TARGET_OBJECTS:IrisCodecLib>
    #     ${CODEC_SOURCE_DIR}/IrisCodecEncoder.cpp
    # )
    # target_include_directories (
    #     EncoderObject PRIVATE
    #     ${irisheaders_SOURCE_DIR}/include
    #     ${irisheaders_SOURCE_DIR}/priv
    # )
    # add_executable (
    #     Encoder
    #     $<TARGET_OBJECTS:IrisCodecLib>
    #     $<TARGET_OBJECTS:EncoderObject>
    #     ${CODEC_SOURCE_DIR}/EncoderMain.cpp
    # )
    # target_link_libraries (
    #     Encoder
    #     PRIVATE IrisHeaders
    #     PRIVATE ${TURBOJPEG_LIBRARY}
    #     PRIVATE ${AVIF_LIBRARY}
    #     PRIVATE ${OPENSLIDE_LIB}
    # )
endif()

if(IRIS_BUILD_PYTHON)
    set(PYBIND11_FINDPYTHON ON)
    FetchContent_Declare (
        Pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG "a2e59f0" #"origin/main"
        GIT_SHALLOW ON
        FETCHCONTENT_QUIET ON
    )
    FetchContent_MakeAvailable (
        Pybind11
    )
    pybind11_add_module(
        IrisCodecPython
        ${PROJECT_SOURCE_DIR}/python/IrisCodecPython.cpp
    )
    target_include_directories (
        IrisCodecPython PRIVATE
        ${IFE_SOURCE_DIR}
        ${CODEC_SOURCE_DIR}
        ${irisheaders_SOURCE_DIR}/priv
    )
    target_link_libraries (
        IrisCodecPython
        PRIVATE IrisHeaders
        PRIVATE $<TARGET_OBJECTS:IrisCodecLib>
        PRIVATE ${TURBOJPEG_LIBRARY}
        PRIVATE ${AVIF_LIBRARY}
    )
    set_property(
        TARGET IrisCodecPython
        PROPERTY OUTPUT_NAME Iris
    )
    install(
        TARGETS IrisCodecPython
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/python/Iris
    )
    # pybind11_add_module(
    #     IrisEncoderPython
    #     ${PROJECT_SOURCE_DIR}/python/IrisCodecPython.cpp
    # )
    install(
        DIRECTORY ${PROJECT_SOURCE_DIR}/python/Iris 
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/python
    )
        
endif()
if (${PROJECT_IS_TOP_LEVEL})
    install(
        TARGETS IrisHeaders
        EXPORT IrisHeadersConfig
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Iris
    )
    install(
        EXPORT IrisHeadersConfig
        FILE IrisHeadersConfig.cmake
        NAMESPACE "Iris::"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Iris
    )
    install(
        TARGETS IrisCodecLib IrisCodec IrisCodecStatic 
        EXPORT IrisCodecConfig
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Iris
    )
    install(
        EXPORT IrisCodecConfig
        FILE IrisCodecConfig.cmake
        NAMESPACE "IrisCodec::"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Iris
    )
endif(${PROJECT_IS_TOP_LEVEL})

#Debug print variables
# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()
# CMAKE_SHARED_LIBRARY_PREFIX
# CMAKE_SHARED_LIBRARY_SUFFIX
# CMAKE_STATIC_LIBRARY_PREFIX
# CMAKE_STATIC_LIBRARY_SUFFIX
