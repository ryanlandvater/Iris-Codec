# Iris Codec Community Module

Copyright &copy; 2025 Ryan Landvater; MIT License

This repository allows for the reading and writing of Iris File Extension whole slide imaging digital slide files and allows for the decoding of Iris Codec encoded slide tile data (the Iris Codec). This repository allows for extremely fast slide access using a very simple API. This repository may downloaded as pre-compiled binaries in the [releases tab](https://github.com/IrisDigitalPathology/Iris-Codec/releases), or [may be built](README.md#c-and-c-implementations) as a static or dynamically linked C++ library or [python modules](README.md#python). We have additionally provided these python modules in prebuilt releases on the Conda-Forge and PyPi package Python repositories.

> [!NOTE]
> This repository applies higher level abstractions for slide access and statically links compression codec libraries such as turbo-jpeg and AVIF. This repository relies upon the [Iris File Extension (IFE) repository](https://github.com/IrisDigitalPathology/Iris-File-Extension) but is **not** designed to publically expose the IFE (API)serialization. **If you are a scanning device manufacturer or programmer developing a custom encoder/decoder, the [IFE repository](https://github.com/IrisDigitalPathology/Iris-File-Extension) will provide the necessary calls to read, write, and validate**. This repository is still a useful resource in providing examples of how we construct encoders and decoders in a safe, massively parallel manner.

## Installation
### C and C++ Implementations
This library can be built from source using CMake. We will add them to package managers in the near future. 

The following shell commands clone and build the repository. Remember to `-CMAKE_INSTALL_PREFIX` your chosen install directory if not installing system-wide. Additionally, Iris Codec CMake script is designed to look for and dynamically link [turbo-jpeg](https://github.com/libjpeg-turbo/libjpeg-turbo) and [AVIF](https://github.com/AOMediaCodec/libavif) by default; however, some implementations would rather simply build a self-contained statically linked binary without the need to dynamically load libraries. Some architectures, such as iOS require this. If you prefer to avoid dependency headaches, you may instead set `-DIRIS_BUILD_DEPENDENCIES=ON` as CMakeAgs to avoid search and linkage. More info on the dependencies lookup in the [cmake directory](./cmake/). The corresponding python module may also be built from source by setting `-DBUILD_PYTHON=ON` and will be available in library folder upon install.

```sh
git clone --depth 1 https://github.com/IrisDigitalPathology/Iris-Codec.git
# Configure your install directory with -DCMAKE_INSTALL_PREFIX=''
# The following CMake Arguments are the default arguments; you may remove the -DARG_NAME entries below and it will build the same. I have just included them to add clarity to optional configurations.
cmake -B build \
    -DIRIS_BUILD_SHARED=ON \
    -DIRIS_BUILD_STATIC=ON \
    -DIRIS_BUILD_ENCODER=ON \
    -DIRIS_BUILD_DEPENDENCIES=OFF \
    -DBUILD_PYTHON=OFF \
    ./Iris-Codec
cmake --build ./Iris-Codec/build -j$CPU_COUNT
cmake --install ./Iris-Codec/build
```
### Python
Iris Codec is also available as a python module. It is distributed as a submodule under the larger Iris. 
```python
from Iris import Codec as codec

```


## Implementations
