if(WIN32)
    set(STATIC_LIB_SUFFIX .lib)
    set (TURBOJPEG_INSTALL_DIR ${CMAKE_BINARY_DIR}/_deps/turbojpeg)
    set (TURBOJPEG_LIB_NAME turbojpeg-static${STATIC_LIB_SUFFIX})
elseif(UNIX)
    set(STATIC_LIB_SUFFIX .a)
    set (TURBOJPEG_INSTALL_DIR ${CMAKE_BINARY_DIR}/_deps/turbojpeg)
    set (TURBOJPEG_LIB_NAME libturbojpeg${STATIC_LIB_SUFFIX})
endif()

# Start by trying to find turbojpeg locally
if (NOT IRIS_BUILD_DEPENDENCIES)
    find_library(TURBOJPEG_LIBRARY turbojpeg)
    find_path(TURBOJPEG_INCLUDE turbojpeg.h)
endif()

#if that does not work, check to see if 
if (NOT TURBOJPEG_LIBRARY OR NOT TURBOJPEG_INCLUDE)
    find_file (TURBOJPEG_LIBRARY ${TURBOJPEG_LIB_NAME} ${TURBOJPEG_INSTALL_DIR}/lib)
    find_path (TURBOJPEG_INCLUDE turbojpeg.h HINTS ${TURBOJPEG_INSTALL_DIR})
endif()
if (TURBOJPEG_LIBRARY AND TURBOJPEG_INCLUDE)
    MESSAGE(STATUS "TURBOJPEG found from previous build attempt: ${TURBOJPEG_LIBRARY}")
else ()
    MESSAGE(STATUS "TURBOJPEG not found. Set to clone during build process.")
    set(TURBOJPEG_EXTERNAL_PROJECT_ADD ON)
    set(TURBOJPEG_LIBRARY ${TURBOJPEG_INSTALL_DIR}/lib/${TURBOJPEG_LIB_NAME})
    set(TURBOJPEG_INCLUDE ${TURBOJPEG_INSTALL_DIR}/include/)
    ExternalProject_Add(
        TurboJpeg
        GIT_REPOSITORY https://github.com/libjpeg-turbo/libjpeg-turbo.git
        GIT_TAG "20ade4d" #"origin/main"
        GIT_SHALLOW ON
        UPDATE_DISCONNECTED ON
        CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX:PATH=${TURBOJPEG_INSTALL_DIR}
            -DENABLE_SHARED:BOOL=OFF
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        FIND_PACKAGE_ARGS NAMES libjpeg-turbo
        BUILD_BYPRODUCTS ${TURBOJPEG_LIBRARY}
    )
endif()
include_directories(
    ${TURBOJPEG_INCLUDE}
)

