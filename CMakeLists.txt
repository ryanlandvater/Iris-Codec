 # 2025 Copyright Ryan Landvater
# Created 3/3/2025

cmake_minimum_required(VERSION 3.11...3.30)
include(FetchContent)
include(ExternalProject)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(IRIS_EXTENSION_BUILD_ALL ON)

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
FetchContent_MakeAvailable(
    IrisHeaders 
    VulkanHeaders 
    VkMemoryAllocator
)
FetchContent_Populate (
    GLM
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG "2d4c4b4" #"origin/master"
    GIT_SHALLOW ON
    FETCHCONTENT_UPDATES_DISCONNECTED ON
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/glm-src
)
FetchContent_Populate (
    IrisFileExtension
    GIT_REPOSITORY https://github.com/IrisDigitalPathology/Iris-File-Extension.git
    GIT_TAG "origin/main"
    GIT_SHALLOW ON
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/irisfileextension-src
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/irisfileextension-build
)

if(WIN32)
    set(STATIC_LIB_SUFFIX static.lib)
    set (TURBOJPEG_INSTALL_DIR ${CMAKE_BINARY_DIR}/_deps/turbojpeg)
    set (TURBOJPEG_LIB_NAME turbojpeg${STATIC_LIB_SUFFIX})
    set (AVIF_INSTALL_DIR ${CMAKE_BINARY_DIR}/_deps/avif)
    set (AVIF_LIB_NAME avif${STATIC_LIB_SUFFIX})
elseif(UNIX)
    set(STATIC_LIB_SUFFIX .a)
    set (TURBOJPEG_INSTALL_DIR ${CMAKE_BINARY_DIR}/_deps/turbojpeg)
    set (TURBOJPEG_LIB_NAME libturbojpeg${STATIC_LIB_SUFFIX})
    set (AVIF_INSTALL_DIR ${CMAKE_BINARY_DIR}/_deps/avif)
    set (AVIF_LIB_NAME libavif${STATIC_LIB_SUFFIX})
endif()


find_file (TURBOJPEG_LIBRARY ${TURBOJPEG_LIB_NAME} ${TURBOJPEG_INSTALL_DIR}/lib)
if (TURBOJPEG_LIBRARY)
    MESSAGE(STATUS "TURBOJPEG found: ${TURBOJPEG_LIBRARY}")
else ()
    MESSAGE(STATUS "TURBOJPEG not found. Set to clone during build process.")
    set (TURBOJPEG_LIBRARY ${TURBOJPEG_INSTALL_DIR}/lib/${TURBOJPEG_LIB_NAME})
    ExternalProject_Add(
        TurboJpeg
        GIT_REPOSITORY https://github.com/libjpeg-turbo/libjpeg-turbo.git
        GIT_TAG "20ade4d" #"origin/main"
        GIT_SHALLOW ON
        UPDATE_DISCONNECTED ON
        CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX:PATH=${TURBOJPEG_INSTALL_DIR}
            -DENABLE_SHARED:BOOL=OFF
    )
endif()

find_file(AVIF_LIBRARY libavif${STATIC_LIB_SUFFIX} ${AVIF_INSTALL_DIR}/lib)
if (AVIF_LIBRARY)
    MESSAGE(STATUS "AVIF found: ${AVIF_LIBRARY}")
else ()
    MESSAGE(STATUS "AVIF not found. Set to clone during build process.")
    set (AVIF_LIBRARY ${AVIF_INSTALL_DIR}/lib/${AVIF_LIB_NAME})
    ExternalProject_Add(
        Avif
        GIT_REPOSITORY https://github.com/AOMediaCodec/libavif.git
        GIT_TAG "a28899a" #"origin/main"
        GIT_SHALLOW ON
        UPDATE_DISCONNECTED ON
        CMAKE_ARGS 
            -DCMAKE_INSTALL_PREFIX:PATH=${AVIF_INSTALL_DIR}
            -DBUILD_SHARED_LIBS=OFF 
            -DENABLE_TOOLS=OFF
            -DENABLE_EXAMPLES=OFF
            -DAVIF_LIBYUV=LOCAL
            -DAVIF_CODEC_AOM=LOCAL 
    )
endif()
# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()


PROJECT(
    IrisCodec
    LANGUAGES CXX
    VERSION ${VERSION_STRING}
)
set(CODEC_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(CODEC_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(IFE_SOURCE_DIR ${irisfileextension_SOURCE_DIR}/src)
option(BUILD_PYTHON "Build IrisCodec Python modules" OFF)
if (BUILD_PYTHON)
    message(STATUS "Configured to build Python Module")
endif()

add_library(
    IrisCodecLib OBJECT
)
target_sources (
    IrisCodecLib PRIVATE
    ${IFE_SOURCE_DIR}/IrisCodecExtension.cpp
    ${CODEC_SOURCE_DIR}/IrisCodecContext.cpp
    ${CODEC_SOURCE_DIR}/IrisCodecFile.cpp
    ${CODEC_SOURCE_DIR}/IrisCodecCache.cpp
    ${CODEC_SOURCE_DIR}/IrisCodecSlide.cpp
    ${irisheaders_SOURCE_DIR}/src/IrisBuffer.cpp
)
target_include_directories (
    IrisCodecLib PRIVATE
    ${IFE_SOURCE_DIR}
    ${glm_SOURCE_DIR}
    ${irisheaders_SOURCE_DIR}/priv
    ${vkmemoryallocator_SOURCE_DIR}/include
    ${vulkanheaders_SOURCE_DIR}/include
    ${TURBOJPEG_INSTALL_DIR}/include
    ${AVIF_INSTALL_DIR}/include
    # $<BUILD_INTERFACE:${IFE_SOURCE_DIR}>
    # $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries (
    IrisCodecLib
    PUBLIC IrisHeaders
)
add_library(
    IrisCodec SHARED 
    $<TARGET_OBJECTS:IrisCodecLib>
)
target_link_libraries(
    IrisCodec
    PRIVATE ${TURBOJPEG_LIBRARY}
    PRIVATE ${AVIF_LIBRARY}
)
add_library(
    IrisCodecStatic STATIC 
    $<TARGET_OBJECTS:IrisCodecLib>
)
target_link_libraries(
    IrisCodecStatic
    PRIVATE ${TURBOJPEG_LIBRARY}
    PRIVATE ${AVIF_LIBRARY}
)
if(BUILD_PYTHON AND ${PROJECT_IS_TOP_LEVEL})
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

