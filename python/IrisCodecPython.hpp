/**
 * @file IrisCodecPython.hpp
 * @author Ryan Landvater
 * @brief  Iris Codec API Python Bindings.
 * @version 2025.1.0
 * @date 2025-01-14
 *
 * @copyright Copyright (c) 2025
 * Created by Ryan Landvater on 1/14/2025
 *
 */
#include "IrisCodecPriv.hpp"
//#include "IrisCodecExtensionPython.hpp"
namespace IrisCodec {
Version _get_codec_version ()
{
    return Version {
        CODEC_MAJOR_VERSION,
        CODEC_MINOR_VERSION,
        CODEC_BUILD_NUMBER
    };
}
Result _validate_slide  (const std::string& file_path)
{
    return validate_slide ( SlideOpenInfo {
        .filePath       = file_path,
        .context        = nullptr,
        .writeAccess    = false
    });
}
Slide _open_slide (const std::string& file_path, const Context& context = nullptr, const bool write_access = false)
{
    Slide slide = open_slide ( SlideOpenInfo {
        .filePath       = file_path,
        .context        = context,
        .writeAccess    = write_access
    });
    return slide;
}
std::tuple<Result, SlideInfo> _get_info (const Slide& slide)
{
    SlideInfo info;
    return std::tuple<Result,SlideInfo>
        (get_slide_info(slide, info),info);
}
struct SlideTile {
    Buffer          data;
};
py::array_t<uint8_t> _read_slide_tile (const Slide& __sl, const unsigned __li, const unsigned __ti)
{
    auto shape  = std::vector<size_t>{TILE_PIX_LENGTH,TILE_PIX_LENGTH,4};
    auto array  = py::array_t<uint8_t>(shape);
    auto buffer = Iris::Wrap_weak_buffer_fom_data(array.data(0), array.size());
    buffer = read_slide_tile( SlideTileReadInfo {
        .slide      = __sl,
        .layerIndex = __li,
        .tileIndex  = __ti,
        .optionalDestination = buffer
    });
    if (!buffer)
        return py::array_t<uint8_t>();
    return array;
}
py::array_t<uint8_t> _read_slide_tile_channels (const Slide& __sl, const unsigned __li, const unsigned __ti)
{
    auto buffer = read_slide_tile( SlideTileReadInfo {
        .slide      = __sl,
        .layerIndex = __li,
        .tileIndex  = __ti,
    });
    if (!buffer) return py::array_t<uint8_t>();
    std::vector<size_t> array_shape {3,TILE_PIX_LENGTH,TILE_PIX_LENGTH};
    auto array = py::array_t<uint8_t>(array_shape);
    auto ptr_0 = const_cast<BYTE*>(array.data(0));
    auto ptr_1 = const_cast<BYTE*>(array.data(1));
    auto ptr_2 = const_cast<BYTE*>(array.data(2));
    auto src   = static_cast<uint32_t*>(buffer->data());
    // TODO: SIMD INSTRUCTIONS
    for (int p_i = 0; p_i < TILE_PIX_AREA; ++p_i) {
        uint32_t val    = src[p_i];
        ptr_0[p_i]      = (val>> 0) & 0xFF;
        ptr_1[p_i]      = (val>> 8) & 0xFF;
        ptr_2[p_i]      = (val>>16) & 0xFF;
    }
    return array;
}
}


static inline void DEFINE_IRIS_CODEC_SUBMODULE (pybind11::module_& base)
{
    using namespace Iris;
    using namespace IrisCodec;
    using namespace pybind11::literals;
    namespace Codec = IrisCodec;
    
    auto m = base.def_submodule("Codec", "This is the Codec submodule that allows access to Iris Encoded Slide Files");
    
    // CLASS DEFINITIONS
    py::enum_<Encoding>                                     (m, "Encoding")
        .value("TILE_ENCODING_UNDEFINED",   TILE_ENCODING_UNDEFINED)
        .value("TILE_ENCODING_JPEG",        TILE_ENCODING_JPEG)
        .value("TILE_ENCODING_AVIF",        TILE_ENCODING_AVIF)
        .value("TILE_ENCODING_IRIS",        TILE_ENCODING_IRIS)
        .value("TILE_ENCODING_DEFAULT",     ENCODING_DEFAULT);
    
    py::class_<Metadata>                                    (m, "Metadata")
        .def_readonly("codec_version",      &Metadata::codec)
        .def_readonly("attributes",         &Metadata::attributes)
        .def_readonly("associated_images",  &Metadata::associatedImages)
        .def_readonly("annotations",        &Metadata::annotations)
        .def_readonly("annotation_groups",  &Metadata::annotationGroups)
        .def_readonly("microns_per_pixel",  &Metadata::micronsPerPixexl)
        .def_readonly("magnification",      &Metadata::magnification)
        .doc()="Slide metadata containing information about the Iris File Extension slide file";
    
    py::class_<SlideInfo>                                   (m, "SlideInfo")
        .def_readonly("extent",             &SlideInfo::extent)
        .def_readonly("encoding",           &SlideInfo::encoding)
        .doc() = "Basic slide information that includes the version of Iris Codec used to encode the slide file, the slide extent, both in lowest resolution pixels and layers comprising number of 256 pixel tiles";
    
    py::class_<Codec::__INTERNAL__Context,  Codec::Context> (m, "Context")
        .doc() = "Iris Codec Context helper class used for access to GPU encoding and decoding methods. Use of a context is most beneficial when encoding or decoding multiple files as it can optimize and orchistrate an encoding queue.";
    
    py::class_<Codec::__INTERNAL__Slide, Codec::Slide>      (m, "Slide")
        .def("get_info", &_get_info)
        .def("read_slide_tile", &_read_slide_tile,
             py::arg("layer_index") = 0,
             py::arg("tile_index") = 0)
        .def("read_slide_tile_channels", &_read_slide_tile_channels,
             "Returns slide tile pixel data in the form of a [4,256,256] numpy array with each major",
             py::arg("layer_index") = 0,
             py::arg("tile_index") = 0)
        .doc() = "Iris Codec Slide (distinct from Iris Core Slide, loader class) is a light-weight wrapper around a whole slide image file used to access slide information and raw image data";
    
    // FUNCTION DEFINITIONS
    m.def("get_codec_version", &Codec::_get_codec_version,
          "Get version of the Iris Codec module specifically, which is distinct from the overall Iris version (Iris.Core.get_version())");
    
    m.def("validate_slide_path", &Codec::_validate_slide,
          "Method to validate a whole slide image file to assess if the file path can produce a valid Iris Codec Slide. Returns a Result with possible erorr message on failure",
          py::arg("file_path"));
    
    m.def("open_slide",&_open_slide,
          "Method to open a whole slide image file at the given file path with optional Iris Codec Context for efficiency and GPU optimizations as well as a write_access boolean flag request mutable access. Returns a NULL object on failure. For more information about failure, use validate_slide_path and examine the result.message",
          py::arg("file_path"),
          py::arg("codec_context")  = Codec::Context(),
          py::arg("write_access")   = false);
    
//    m.def("encode_slide_file",)
}
