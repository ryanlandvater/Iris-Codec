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
conda install -c conda-forge Iris-Codec 
```



## Implementations
### C++
Iris is natively a C++ program and the majority of features will first be supported in C++ followed by the other language bindings as we find time to write the bindings. 

Begin by importing the [Iris Codec Core header](https://github.com/IrisDigitalPathology/Iris-Headers/blob/main/include/IrisCodecCore.hpp); it contains references to the [Iris Codec specific type definitions](https://github.com/IrisDigitalPathology/Iris-Headers/blob/main/include/IrisCodecTypes.hpp) as well as the general [Iris Core type definitions](https://github.com/IrisDigitalPathology/Iris-Headers/blob/main/include/IrisTypes.hpp). You may chose to perform your own file system validations and recovery routines. Iris will, however catch all of these as the main API methods are declared `noexcept`. Should an runtime error occur, it will be reported in the form of an `IrisResult` message, as seen in the `IrisResult validate_slide (const SlideOpenInfo&) noexcept;` call below. Successful loading of a slide file will return a valid `IrisCodec::Slide` object; failure will return a `nullptr`. 
```cpp
// Import the Iris Codec header
// This import includes the types header automatically
#import <filesystem>
#import <Iris/IrisCodecCore.hpp>
int main(int argc, char const *argv[])
{
    using namespace IrisCodec;
    std::filesystem::path file_path = "path/to/slide_file.iris";

    // You can check the file system to see if the slide exists
    // If you choose not to, that's fine too. Iris will tell you.
    if (!std::filesystem::exists(file_path)) {
        printf(file_path.string() + " file does not exist\n");
        return EXIT_FAILURE;
    }

    // You can quickly check if the header starts with Iris
    // file extension signatures. If not, that's fine too.
    // Iris will catch it during validation.
    if (!is_iris_codec_file(file_path.string())) {
        printf(file_path.string() + " is not a valid Iris slide file\n");
        return EXIT_FAILURE;
    }

    // Create an open slide info struct. Ignore the other
    // parameters at the moment; they will default.
    SlideOpenInfo open_info {
        .filePath = file_path.string();
    };
    // Perform a deep validation of the slide file structure
    // This will navigate the internal offset-chain and
    // check for violations of the IFE standard.
    IrisResult result = validate_slide (open_info);
    if (result != IRIS_SUCCESS) {
        printf (result.message);
        return EXIT_FAILURE;
    }
    
    // Finally create the slide object.
    // Most Iris objects are shared_ptrs,
    // so Iris will handle the memory clean-up
    auto slide = open_slide (open_info);
    if (slide) return EXIT_SUCCESS;
    else return EXIT_FAILURE;
}
```

Once opened, the slide `IrisCodec::SlideInfo` structure can be loaded using the `Result get_slide_info (const Slide&, SlideInfo&) noexcept` call and used as an initialized structure containing all the information needed to navigate the slide file and read elements.
```cpp
// Read the slide information
SlideInfo info;
IrisResult result = get_slide_info (slide, info);
if (result != IRIS_SUCCESS) {
    printf (result.message);
    return EXIT_FAILURE;
}

// Slide tile read info provides a simple mechanism
// for reading slide data.
struct SlideTileReadInfo read_info {
    .slide                  = slide,
    .layer                  = 0,
    .optionalDestination    = NULL, /*wrapper can go here*/
    .desiredFormat          = Iris::FORMAT_R8G8B8A8,
};
// Iterate
for (auto& layer : info.extent.layers) {
    for (int y_index = 0; y_index < layer.yTiles; ++y_index) {
        for (int x_index = 0; x_index < layer.xTiles; ++x_index) {
            // Read the tile slide tile
            auto rgba = read_slide_tile (read_info);
            // Do something with the tile pixel values
            // Do not worry about clean up; the slide
            // pixel values are in a Iris::Buffer shared_ptr
        }
    }
    read_info.layer++;
}
if (optional_buffer) free (optional_buffer);
```
Decompressed slide data can be optionally read into preallocated memory. If the optional destination buffer is insufficiently sized, Iris will instead allocate a new buffer and return that new buffer with the pixel data. The `Iris::Buffer` should weakly reference the underlying memory as strongly referenced `Iris::Buffer` objects free underlying memory on deletion.
```cpp
char* some_GPU_upload_buffer;
size_t tile_byte_offset;
char* destination = some_GPU_upload_buffer + tile_byte_offest;
size_t tile_bytes = 256*256*4;
Iris::Buffer wrapper = Wrap_weak_buffer_fom_data (destination, tile_bytes);
struct SlideTileReadInfo read_info {
    .slide                  = slide,
    .optionalDestination    = NULL, /*wrapper can go here*/
    .desiredFormat          = Iris::FORMAT_R8G8B8A8,
};
Buffer result = read_slide_tile (read_info);
if (weak_wrapper != result) {
    printf ("Insufficient sized buffer provided");
}
```


### Python
```python
#Import the Iris Codec Module
from Iris import Codec
slide_path = 'path/to/slide_file.iris'

# Perform a deep validation of the slide file structure
# This will navigate the internal offset-chain and
# check for violations of the IFE standard.
result = Codec.validate_slide_path(slide_path)
if (result.success() == False):
    raise Exception(f'Invalid slide file path: {result.message()}')
print(f"Slide file '{slide_path}' successfully passed validation")

# Open a slide file
slide = Codec.open_slide(slide_path)

# The following conditional will return True in this instance
# as the slide has already passed validation;
# We simply include it as an example of how to check the slide
if (not slide):
    raise Exception(f'Invalid slide file path: {result.message()}')

# Get the slide abstraction
result, info = slide.get_info()
if (result.success() == False):
    raise Exception(f'Failed to read slide information: {result.message()}')

# Print the slide extent to the console
extent = info.extent
print(f"Slide file {extent.width} px by {extent.height}px with an encoding of {info.encoding}. The layer extents are as follows:")
print(f'There are {len(extent.layers)} layers comprising the following dimensions:')
for i, layer in enumerate(extent.layers):
    print(f' Layer {i}: {layer.x_tiles} x-tiles, {layer.y_tiles} y-tiles, {layer.scale:0.0f}x scale')

# Generate a quick view of a slide tile in the middle of the slide using matplotlib imshow function
import matplotlib.pyplot as plt
layer_index = 0
x_index = int(extent.layers[layer_index].x_tiles/2)
y_index = int(extent.layers[layer_index].y_tiles/2)
tile_index = extent.layers[layer_index].x_tiles * y_index + x_index
fig = plt.figure()
plt.imshow(slide.read_slide_tile(layer_index,tile_index), interpolation='none')
plt.show()

```
