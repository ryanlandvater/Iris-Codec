# Iris Codec Community Module

This repository allows for the reading and writing of Iris File Extension whole slide imaging digital slide files and allows for the decoding of Iris Codec encoded slide tile data. This repository allows for extremely fast slide access using a very simple API. This repository may downloaded as pre-compiled binaries in the releases tab, or may be built as a static or dynamically linked C++ library or python modules. We have additionally provided these python modules in prebuilt releases on the Conda-Forge and PyPi package Python repositories.

> [!NOTE]
> This repository applies higher level abstractions for slide access and statically links compression codec libraries such as turbo-jpeg and AVIF. This repository relies upon the [Iris File Extension (IFE) repository](https://github.com/IrisDigitalPathology/Iris-File-Extension) but is **not** designed to publically expose the IFE (API)serialization. **If you are a scanning device manufacturer or programmer developing a custom encoder/decoder, the [IFE repository](https://github.com/IrisDigitalPathology/Iris-File-Extension) will provide the necessary calls to read, write, and validate**. This repository is still a useful resource in providing examples of how we construct encoders and decoders in a safe massively parallel manner.

## Installation

## Implementations

https://github.com/ryanlandvater/Iris-Codec/archive/refs/tags/2025.0.0-alpha1.tar.gz
