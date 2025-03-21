#/bin/bash

cmake -B modules_build \
    -DIRIS_BUILD_SHARED=OFF \
    -DIRIS_BUILD_STATIC=OFF \
    -DIRIS_BUILD_ENCODER=ON \
    -DIRIS_BUILD_DEPENDENCIES=ON \
    -DBUILD_PYTHON=ON \
    -DCMAKE_INSTALL_PREFIX='./modules_install'
    ../
cmake --build ./modules_build -j$CPU_COUNT
cmake --install ./modules_build
