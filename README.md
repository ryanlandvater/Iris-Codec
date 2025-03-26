# Iris Codec Community Module

Copyright &copy; 2025 Iris Developers; MIT Software License

The Iris Codec Community module is a part of the Iris Digital Pathology project. This module allows for the reading and writing of Iris whole slide image (WSI) digital slide files (*.iris*) and allows for the decoding of Iris Codec-type compressed tile image data. This repository was designed to allow for extremely fast slide access using what we consider a very simple API as we want to simplify access to these files for you. It may downloaded as pre-compiled binaries in the [releases tab](https://github.com/IrisDigitalPathology/Iris-Codec/releases), or [may be built](README.md#c-and-c-implementations) as a static or dynamically linked C++ library or [python modules](README.md#python). We have additionally provided these python modules in prebuilt releases on the Anaconda [Conda-Forge](https://conda-forge.org) Python package manager.

> [!NOTE]
> **If you are a scanning device manufacturer or programmer developing a custom encoder/decoder, the [Iris File Extension (IFE) repository](https://github.com/IrisDigitalPathology/Iris-File-Extension) will provide the necessary calls to read, write, and validate slide files in accordance with the Iris File Extension Specification.** The current repository (Iris Codec Module) applies higher level abstractions for slide access and incorporates image codecs for image compression. The IFE repository does not. It is limited to (de)serialization and validation. The Iris Codec Module incorporates the [IFE repository](https://github.com/IrisDigitalPathology/Iris-File-Extension) as a dependency, so if you use the IFE respository instead, the Iris Codec module source files may be a helpful guide in how we choose to read and write to Iris files using the IFE's API.

> [!WARNING]
>  The Iris Codec module is still in active development. We do not anticipate altering the established API functions in the [header files](https://github.com/IrisDigitalPathology/Iris-Headers) but as we add in new features, some elements of the API may change slightly. Please check in regularly if you intend to update your dynamically linked libraries to ensure no API breaking changes have been merged. 

This module has reliatively limited dependencies. As our encoder builds shift away from using OpenSlide, we will add additional library dependencies for reading vendor files. 
- [Iris File Extension](https://github.com/IrisDigitalPathology/Iris-File-Extension)
- [Libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo)
- [Libavif](https://github.com/AOMediaCodec/libavif)
- [OpenSlide](https://github.com/openslide/openslide) (*optional*, encoder-only) <p>*Note: We may remove this dependency in future releases of the encoder. At the present, this limits Windows encoder builds to x86_64 only. You must disable openslide for ARM based windows encoder builds.*

*If you are a software engineer looking to help with Iris, we are always looking for additional passionate engineers to help in developing the Iris Project.*

## Installation
### Building From Source
This library can be built from source using CMake. We will add this module to package managers in the near future. 

The following shell commands clone and build the repository. Remember to `-DCMAKE_INSTALL_PREFIX` your chosen install directory if not installing system-wide. Additionally, Iris Codec CMake script is designed to look for and dynamically link [turbo-jpeg](https://github.com/libjpeg-turbo/libjpeg-turbo) and [AVIF](https://github.com/AOMediaCodec/libavif) by default; however, some implementations would rather simply build a self-contained statically linked binary without the need to dynamically load libraries. **In some instances where reliablity is key, this may be the most secure option.** Some architectures, such as iOS require this. To enable static dependency linkage, instead set `-DIRIS_BUILD_DEPENDENCIES=ON`. More info on the dependencies lookup and **cross compiling** Iris Codec in the [cmake directory](./cmake/).

```sh
git clone --depth 1 https://github.com/IrisDigitalPathology/Iris-Codec.git
# Configure your install directory with -DCMAKE_INSTALL_PREFIX=''
# The following CMake Arguments are the default arguments; you may remove the -DARG_NAME entries below and it will build the same. I have just included them to add clarity to optional configurations.
cmake -B build \
    -D IRIS_BUILD_SHARED=ON \
    -D IRIS_BUILD_STATIC=ON \
    -D IRIS_BUILD_ENCODER=ON \
    -D IRIS_BUILD_DEPENDENCIES=OFF \
    -D IRIS_BUILD_PYTHON=OFF \
    -D IRIS_USE_OPENSLIDE=ON \
    ./Iris-Codec
cmake --build ./Iris-Codec/build --config Release -j$CPU_COUNT
cmake --install ./Iris-Codec/build
```
### Python
Iris Codec is also available as a Python module. It is available as a conda package on the Conda Forge channel. The corresponding python module may also be built from source by setting `-DIRIS_BUILD_PYTHON=ON` in the above CMake command if you would rather build the module rather than install it via Anaconda. 
```shell
conda install - c conda-forge Iris-Codec 
```



## Implementations
### C++
```cpp
#import <Iris/IrisCodec.hpp>

```
### Python
```python
from Iris import Codec as codec

```