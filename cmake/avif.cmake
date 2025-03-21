if(WIN32)
    set(STATIC_LIB_SUFFIX .lib)
    set (AVIF_INSTALL_DIR ${CMAKE_BINARY_DIR}/_deps/avif)
    set (AVIF_LIB_NAME avif${STATIC_LIB_SUFFIX})
elseif(UNIX)
    set(STATIC_LIB_SUFFIX .a)
    set (AVIF_INSTALL_DIR ${CMAKE_BINARY_DIR}/_deps/avif)
    set (AVIF_LIB_NAME libavif${STATIC_LIB_SUFFIX})
endif()

# Start by trying to find turbojpeg locally
if (NOT IRIS_BUILD_DEPENDENCIES)
    find_library(AVIF_LIBRARY avif)
    find_path(AVIF_INCLUDE avif/avif.h)
endif()

#if that does not work, check to see if it was made in a previous build
if (NOT AVIF_LIBRARY OR NOT AVIF_INCLUDE)
    find_file (AVIF_LIBRARY ${TURBOJPEG_LIB_NAME} ${TURBOJPEG_INSTALL_DIR}/lib)
    find_path (AVIF_INCLUDE turbojpeg.h HINTS ${TURBOJPEG_INSTALL_DIR})
endif()

if (AVIF_LIBRARY AND AVIF_INCLUDE)
    MESSAGE(STATUS "AVIF found from previous build attempt: ${AVIF_LIBRARY}")
else ()
    MESSAGE(STATUS "AVIF not found ${AVIF_INSTALL_DIR}/lib/${AVIF_LIB_NAME}. Set to clone during build process.")
    set(AVIF_EXTERNAL_PROJECT_ADD ON)
    set(AVIF_LIBRARY ${AVIF_INSTALL_DIR}/lib/${AVIF_LIB_NAME})
    set(AVIF_INCLUDE ${AVIF_INSTALL_DIR}/include/)
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
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        FIND_PACKAGE_ARGS NAMES libavif
        BUILD_BYPRODUCTS ${AVIF_LIBRARY}
    )
    
endif()
include_directories(
    ${AVIF_INCLUDE}
)