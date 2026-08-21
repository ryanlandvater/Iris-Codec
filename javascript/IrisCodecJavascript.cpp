/**
 * @file IrisCodecJavascript.cpp
 * @author Ryan Landvater (ryanlandvater [at] gmail [dot] com)
 * @brief Iris-codec implementation for JavaScript bindings that
 * allows remote HTTP range request streaming of Iris File Extension
 * encoded whole slide image files. This provides required logic to
 * abstract the file and the Emscripten Bindings to allow JS access.
 * @version 2025.2.0
 * @date 2025-08-09
 *
 * @copyright Copyright Iris Developers (c) 2025
 *
 */
#include "IrisFileExtension.hpp"

#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <string>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <vector>
#include <sstream>
#include <map>
#include <set>

// Direct JavaScript fetch function using EM_JS
EM_JS(emscripten::EM_VAL, fetch_tile_data, (const char* url_ptr, const char* range_header_ptr, const char* mime_type_ptr), {
  const url = UTF8ToString(url_ptr);
  const rangeHeader = UTF8ToString(range_header_ptr);
  const mimeType = UTF8ToString(mime_type_ptr);
  
  // Create a Promise that fetches the data
  const promise = fetch(url, {
    headers: {'Range': rangeHeader}
  })
  .then(response => {
    if (!response.ok) {
      throw new Error(`Fetch failed: ${response.status} ${response.statusText}`);
    }
    return response.arrayBuffer();
  })
  .then(buffer => {
    const uint8Array = new Uint8Array(buffer);
    
    // Create the Blob with the correct MIME type
    const blob = new Blob([uint8Array], { type: mimeType });
    return blob;
  })
  .catch(error => {
    console.error("Fetch failed:", error);
    throw error;
  });
  
  return Emval.toHandle(promise);
});

// HEAD request to get file size
EM_ASYNC_JS(size_t, get_file_size_async, (const char* url), {
    const urlString = UTF8ToString(url);
    
    try {
        const response = await fetch(urlString, { method: 'HEAD' });
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const contentLength = response.headers.get('content-length');
        if (!contentLength) {
            throw new Error('Content-Length header not found');
        }
        
        const fileSize = parseInt(contentLength, 10);
        return fileSize;
        
    } catch (error) {
        console.error('[JS] Error getting file size:', error);
        return 0; // Return 0 to indicate error
    }
});

// Range read test to confirm server supports range requests
EM_ASYNC_JS(bool, confirm_range_support, (const char* url, int header_size), {
    const urlString = UTF8ToString(url);
    const headerSize = header_size;
    
    try {
        // Create an AbortController for timeout
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), 1000); // 1 second timeout
        
        const response = await fetch(urlString, {
            method: 'GET',
            headers: {
                'Range': `bytes=0-${headerSize - 1}`
            },
            signal: controller.signal
        });
        
        clearTimeout(timeoutId);
        
        // Check if server supports range requests (should return 206 Partial Content)
        if (response.status !== 206) {
            console.error('[JS] Server does not support range requests, status:', response.status);
            return false;
        }
        
        // Get the response data
        const arrayBuffer = await response.arrayBuffer();
        
        // Validate the arrayBuffer
        if (!arrayBuffer || !(arrayBuffer instanceof ArrayBuffer)) {
            console.error(`[JS] Invalid response: arrayBuffer is not a valid ArrayBuffer`);
            return false;
        }
        
        const actualSize = arrayBuffer.byteLength;

        // Confirm we got exactly the number of bytes we requested
        if (actualSize !== headerSize) {
            console.error(`[JS] Expected ${headerSize} bytes, got ${actualSize} bytes`);
            return false;
        }
        
        return true;
        
    } catch (error) {
        if (error.name === 'AbortError') {
            console.error('[JS] Range request timed out after 1 second');
        } else {
            console.error('[JS] Error confirming range support:', error);
        }
        return false;
    }
});
// Convenience Serialization functions
// MARK: - SERIALIZATION HELPER FUNCTIONS
namespace Iris {
inline std::string SERIALIZE_FORMAT (const Iris::Format &format)
{
    switch (format) {
        case FORMAT_UNDEFINED:  return "\"FORMAT_UNDEFINED\"";
        case FORMAT_B8G8R8:     return "\"FORMAT_B8G8R8\"";
        case FORMAT_R8G8B8:     return "\"FORMAT_R8G8B8\"";
        case FORMAT_B8G8R8A8:   return "\"FORMAT_B8G8R8A8\"";
        case FORMAT_R8G8B8A8:   return "\"FORMAT_R8G8B8A8\"";
    }
    return "\"UNDEFINED FORMAT\"";
}
inline std::string SERIALIZE_ENCODING (const IrisCodec::Encoding &encoding)
{
    switch (encoding) {
        case IrisCodec::TILE_ENCODING_UNDEFINED:    return "\"ENCODING_UNDEFINED\"";
        case IrisCodec::TILE_ENCODING_IRIS:         return "\"image/iris\"";
        case IrisCodec::TILE_ENCODING_JPEG:         return "\"image/jpeg\"";
        case IrisCodec::TILE_ENCODING_AVIF:         return "\"image/avif\"";
    }
    return "\"UNDEFINED ENCODING\"";
}
inline void SERIALIZE_LAYER_EXTENT (const LayerExtents &extent, std::stringstream& stream)
{
    stream << "[";
    for (auto&& layer : extent) {
        stream  << "{"
                << "\"x_tiles\": " << layer.xTiles << ","
                << "\"y_tiles\": " << layer.yTiles << ","
                << "\"scale\": " << layer.scale
                << "},";
    }
    stream.seekp(-1,stream.cur) << "]";
}
inline void SERALIZE_SLIDE_EXTENT (const Extent &extent, std::stringstream& stream)
{
    stream  << "{"
            << "\"width\": " << extent.width << ","
            << "\"height\": " << extent.height << ","
            << "\"layers\": ";
    
    SERIALIZE_LAYER_EXTENT(extent.layers, stream);
    stream << "},";
}
} // END IRIS namespace

// MARK: - IRIS CODEC JavaScript SLIDE WRAPPER
namespace IrisCodec {
using namespace emscripten;
class __INTERNAL__Slide {
    const std::string               _url;
    const Abstraction::File         _abstraction;
public:
    explicit __INTERNAL__Slide      (const std::string& url,
                                    const Abstraction::File& __a) :
    _url                            (url),
    _abstraction                    (__a) {}
    __INTERNAL__Slide               (const __INTERNAL__Slide&) = delete;
    __INTERNAL__Slide operator =    (const __INTERNAL__Slide&) = delete;
   ~__INTERNAL__Slide               (){}
    // Return codec version used to encode the slide
    Version get_slide_codec_version () const {
        return _abstraction.metadata.codec;
    }
    SlideInfo get_slide_info () const {
        return SlideInfo {
            .format         = _abstraction.tileTable.format,
            .encoding       = _abstraction.tileTable.encoding,
            .extent         = _abstraction.tileTable.extent,
            .metadata       = _abstraction.metadata,
        };
    }
    std::string get_metadata_JSON () const
    {
        using namespace Iris;
        SlideInfo info {
            .format         = _abstraction.tileTable.format,
            .encoding       = _abstraction.tileTable.encoding,
            .extent         = _abstraction.tileTable.extent,
            .metadata       = _abstraction.metadata,
        };
        std::stringstream stream;
        stream << "{";
        stream << "\"type\": \"slide_metadata\",";
        if (info.format)
            stream  <<"\"format\": " << SERIALIZE_FORMAT (info.format) << ",";
        if (info.encoding)
            stream  <<"\"encoding\": " << SERIALIZE_ENCODING(info.encoding) << ",";
        
        stream <<"\"extent\": ";
        SERALIZE_SLIDE_EXTENT(info.extent,stream);
        
        stream.seekp(-1,stream.cur) << "}";
        return stream.str();
    }
    emscripten::val get_slide_tile (uint32_t layer, uint32_t tile_indx) {
        try {
            // Pull the extent and check that the layer is within bounds
            auto& layers = _abstraction.tileTable.layers;
            if (layer >= layers.size()) throw std::runtime_error
                ("Requested layer("+ std::to_string(layer) +") is out of bounds");
            
            // Pull the layer extent and check that the tile is within bounds
            auto& tiles = layers[layer];
            if (tile_indx >= tiles.size()) throw std::runtime_error
                ("Requested tile("+ std::to_string(tile_indx) +") is out of layer "+std::to_string(layer)+" bounds");
            
            // Get the entry information (offset and size)
            auto& entry = tiles[tile_indx];
            
            // Get the encoding type as a string - make sure it's null-terminated
            std::string mime_str;
            switch (_abstraction.tileTable.encoding) {
                case IrisCodec::TILE_ENCODING_IRIS:  mime_str = "image/iris"; break;
                case IrisCodec::TILE_ENCODING_JPEG:  mime_str = "image/jpeg"; break;
                case IrisCodec::TILE_ENCODING_AVIF:  mime_str = "image/avif"; break;
                default:                             mime_str = "application/octet-stream"; break;
            }
            std::string range_header = "bytes="
                + std::to_string(entry.offset) + "-"
                + std::to_string(entry.offset+entry.size-1);
            
            // Use direct EM_JS fetch function
            emscripten::EM_VAL promise_handle = fetch_tile_data(
                _url.c_str(),
                range_header.c_str(),
                mime_str.c_str()
            );
            
            // Convert EM_VAL to emscripten::val
            emscripten::val promise = emscripten::val::take_ownership(promise_handle);
            
            return promise;
            
        } catch (const std::exception& e) {
            std::cerr << "[C++] Exception in get_slide_tile: " << e.what() << std::endl;
            throw;
        } catch (...) {
            std::cerr << "[C++] Unknown exception in get_slide_tile" << std::endl;
            throw;
        }
    }
};
/// Fetch the structural prefix of a hosted slide and shallow-validate it.
///
/// The generated runtime is bytes-only: validate_file_structure and
/// abstract_file_structure take a flat buffer, and a hosted file is never
/// fully resident. This fetches just the five blocks the abstraction requires
/// (FILE_HEADER, TILE_TABLE, LAYER_EXTENTS, TILE_OFFSETS, METADATA) through
/// IFE's own windowed fetch -- Window::remote + fetch_http_range, the same
/// ranged transport v1 used, with the caching and bounds logic tested
/// natively on IFE's side. The encoder lays every optional block after
/// metadata, so their handles fall outside the prefix and the abstraction
/// skips them; the Slide needs the tile table and metadata only.
///
/// On success `out` holds the prefix and `out_version` the file's declared
/// version; returns false when a range could not be fetched.
bool fetch_structural_prefix(const std::string& url, size_t file_size,
                             std::vector<IFE::BYTE>& out, uint32_t& out_version) {
    IFE::Window window = IFE::Window::remote(
        static_cast<IFE::Size>(file_size), IFE::fetch_http_range,
        const_cast<char*>(url.c_str()));

    // Bootstrap the version the way the runtime does: every gate open for one
    // block, then rebuild at the version the file declares.
    const IFE::BYTE* header_page = window.map(0, Serialization::FILE_HEADER::header_size);
    if (!header_page) return false;
    const Serialization::FILE_HEADER bootstrap{
        header_page, 0, static_cast<IFE::Size>(file_size), UINT32_MAX};
    out_version = (static_cast<uint32_t>(bootstrap.extension_major()) << 16)
                | bootstrap.extension_minor();

    const IFE::BYTE* table_page = window.map(
        bootstrap.tile_table_offset().__offset, Serialization::TILE_TABLE::header_size);
    if (!table_page) return false;
    const Serialization::TILE_TABLE table{
        table_page, 0, static_cast<IFE::Size>(file_size), out_version};

    const IFE::Offset extents_off  = table.layer_extents_offset().__offset;
    const IFE::Offset offsets_off  = table.tile_offsets_offset().__offset;
    const IFE::Offset metadata_off = bootstrap.metadata_offset().__offset;

    // The counts size the two array blocks; each block's header page is
    // enough to read its count.
    const IFE::BYTE* extents_page = window.map(extents_off, Serialization::LAYER_EXTENTS::header_size);
    const IFE::BYTE* offsets_page = window.map(offsets_off, Serialization::TILE_OFFSETS::header_size);
    if (!extents_page || !offsets_page) return false;
    const Serialization::LAYER_EXTENTS extents{
        extents_page, 0, static_cast<IFE::Size>(file_size), out_version};
    const Serialization::TILE_OFFSETS offsets{
        offsets_page, 0, static_cast<IFE::Size>(file_size), out_version};

    const IFE::Size prefix_end = std::max({
        metadata_off + Serialization::METADATA::header_size,
        extents_off + Serialization::LAYER_EXTENTS::header_size
                    + static_cast<IFE::Size>(extents.count())
                        * Serialization::LAYER_EXTENTS::LAYER_EXTENT::entry_size,
        offsets_off + Serialization::TILE_OFFSETS::header_size
                    + static_cast<IFE::Size>(offsets.count())
                        * Serialization::TILE_OFFSETS::TILE_OFFSET::entry_size,
    });

    // One contiguous page covering the whole structural region; copied out
    // because the Window owns the page and dies with this function.
    const IFE::BYTE* prefix = window.map(0, prefix_end);
    if (!prefix) return false;
    out.assign(prefix, prefix + prefix_end);
    return true;
}

emscripten::val _validateFileStructure(const std::string& url) {
    size_t file_size = get_file_size_async(url.c_str());
    if (file_size < Serialization::FILE_HEADER::header_size)
    {
        return emscripten::val
        (Result(Iris::IRIS_VALIDATION_FAILURE,
        "The hosted file is not an Iris slide file."));
    }
    if (!confirm_range_support(url.c_str(), Serialization::FILE_HEADER::header_size))
    {
        return emscripten::val
        (Result(Iris::IRIS_FAILURE,
        "The server hosting the slide file does not support Ranged Reads."));
    }

    std::vector<IFE::BYTE> prefix;
    uint32_t version = 0;
    if (!fetch_structural_prefix(url, file_size, prefix, version))
        return emscripten::val
        (Result(Iris::IRIS_FAILURE,
        "Failed to fetch the hosted file's structural blocks from the remote endpoint."));

    // Shallow structural validation over the prefix: the five blocks the
    // abstraction requires, bounded by the full file size so tile data
    // offsets -- which point past the prefix -- still validate. The tiles
    // themselves are fetched on demand and validated at decode time.
    const Serialization::FILE_HEADER header{
        prefix.data(), 0, static_cast<IFE::Size>(file_size), version};
    if (!header.validate())
        return emscripten::val(Result(Iris::IRIS_VALIDATION_FAILURE,
        "The hosted file's header failed validation."));
    const auto table = header.tile_table_offset();
    if (!table.validate())
        return emscripten::val(Result(Iris::IRIS_VALIDATION_FAILURE,
        "The hosted file's tile table failed validation."));
    const auto extents = table.layer_extents_offset();
    if (!extents.validate())
        return emscripten::val(Result(Iris::IRIS_VALIDATION_FAILURE,
        "The hosted file's layer extents failed validation."));
    const auto offsets = table.tile_offsets_offset();
    if (!offsets.validate())
        return emscripten::val(Result(Iris::IRIS_VALIDATION_FAILURE,
        "The hosted file's tile offsets failed validation."));
    const auto metadata = header.metadata_offset();
    if (!metadata.validate())
        return emscripten::val(Result(Iris::IRIS_VALIDATION_FAILURE,
        "The hosted file's metadata block failed validation."));

    return emscripten::val(Result(Iris::IRIS_SUCCESS, ""));
}
emscripten::val _openSlide(const std::string& url) {
    size_t file_size = get_file_size_async(url.c_str());
    if (file_size < Serialization::FILE_HEADER::header_size)
        return emscripten::val();
    if (!confirm_range_support(url.c_str(), Serialization::FILE_HEADER::header_size))
        return emscripten::val();

    std::vector<IFE::BYTE> prefix;
    uint32_t version = 0;
    if (!fetch_structural_prefix(url, file_size, prefix, version))
        throw std::runtime_error
        ("Failed to fetch the hosted file's structural blocks from remote endpoint ("+url+")");
    auto file = abstract_file_structure({prefix.data(), prefix.size()});
    return emscripten::val(std::make_shared<__INTERNAL__Slide>(url,file));
}
} // END IRIS_CODEC


// MARK: - EMSCRIPTEN BINDINGS
// BEGIN BINDINGS
EMSCRIPTEN_BINDINGS(iris_codec) {
    using namespace emscripten;
    class_<std::set<std::string>>("StringSet")
        .function("has", +[](std::set<std::string>& m, std::string key){
            return m.find(key) == m.end() ? false : true;
        })
        .function("keys", +[](std::set<std::string>& m) {
            val arr = val::array();
            unsigned idx = 0;
            for (auto& p : m) arr.set(idx++, p);
            return arr;
        });
    class_<std::set<uint32_t>>("Uint32Set")
        .function("has", +[](std::set<uint32_t>& m, uint32_t key){
            return m.find(key) == m.end() ? false : true;
        })
        .function("keys", +[](std::set<uint32_t>& m) {
            val arr = val::array();
            unsigned idx = 0;
            for (auto& p : m) arr.set(idx++, p);
            return arr;
        });

    
    using namespace IrisCodec;
    emscripten::value_object<Iris::Version>("Version")
        .field("major", &Iris::Version::major)
        .field("minor", &Iris::Version::minor)
        .field("build", &Iris::Version::build);

    enum_<Iris::ResultFlag>("ResultFlag")
        .value("IRIS_SUCCESS", Iris::ResultFlag::IRIS_SUCCESS)
        .value("IRIS_FAILURE", Iris::ResultFlag::IRIS_FAILURE)
        .value("IRIS_UNINITIALIZED", Iris::ResultFlag::IRIS_UNINITIALIZED)
        .value("IRIS_VALIDATION_FAILURE", Iris::ResultFlag::IRIS_VALIDATION_FAILURE)
        .value("IRIS_WARNING", Iris::ResultFlag::IRIS_WARNING)
        .value("IRIS_WARNING_VALIDATION", Iris::ResultFlag::IRIS_WARNING_VALIDATION)
        .value("RESULT_MAX_ENUM", Iris::ResultFlag::RESULT_MAX_ENUM);

    value_object<Iris::Result>("Result")
        .field("flag", &Iris::Result::flag)
        .field("message", &Iris::Result::message);

    enum_<Iris::Format>("Format")
        .value("FORMAT_UNDEFINED", Iris::FORMAT_UNDEFINED)
        .value("FORMAT_B8G8R8", Iris::FORMAT_B8G8R8)
        .value("FORMAT_R8G8B8", Iris::FORMAT_R8G8B8)
        .value("FORMAT_B8G8R8A8", Iris::FORMAT_B8G8R8A8)
        .value("FORMAT_R8G8B8A8", Iris::FORMAT_R8G8B8A8);
    enum_<IrisCodec::Encoding>("Encoding")
        .value("TILE_ENCODING_UNDEFINED", IrisCodec::TILE_ENCODING_UNDEFINED)
        .value("TILE_ENCODING_IRIS", IrisCodec::TILE_ENCODING_IRIS)
        .value("TILE_ENCODING_JPEG", IrisCodec::TILE_ENCODING_JPEG)
        .value("TILE_ENCODING_AVIF", IrisCodec::TILE_ENCODING_AVIF);

    value_object<LayerExtent>("LayerExtent")
        .field("xTiles", &LayerExtent::xTiles)
        .field("yTiles", &LayerExtent::yTiles)
        .field("scale", &LayerExtent::scale)
        .field("downsample", &LayerExtent::downsample);
    register_vector<LayerExtent>("LayerExtents");

    value_object<Iris::Extent>("Extent")
        .field("width", &Extent::width)
        .field("height", &Extent::height)
        .field("layers", &Extent::layers);
    
    enum_<MetadataType>("MetadataType")
        .value("METADATA_UNDEFINED", METADATA_UNDEFINED)
        .value("METADATA_I2S", METADATA_I2S)
        .value("METADATA_DICOM", METADATA_DICOM)
        .value("METADATA_FREE_TEXT", METADATA_FREE_TEXT);

    class_<Attributes>("Attributes")
        .function("has", +[](Attributes& m, std::string key){
            return m.find(key) == m.end() ? false : true;
        })
        .function("get",+[](Attributes& m, std::string key) -> std::string {
            auto it = m.find(key);
            return it == m.end() ? std::string{} :
            std::string(reinterpret_cast<const char* const>
                (it->second.c_str()), it->second.size());
        })
        .function("keys", +[](Attributes& m) {
            val arr = val::array();
            unsigned idx = 0;
            for (auto& p : m) arr.set(idx++, p.first);
            return arr;
        })
        .property("type", &Attributes::type)
        .property("version", &Attributes::version);
        
    value_object<Metadata>("Metadata")
        .field("codec", &Metadata::codec)
        .field("attributes", &Metadata::attributes)
        .field("associatedImages", &Metadata::associatedImages)
        .field("ICC_profile", &Metadata::ICC_profile)
        .field("annotations", &Metadata::annotations)
        .field("annotationGroups", &Metadata::annotationGroups)
        .field("micronsPerPixel", &Metadata::micronsPerPixel)
        .field("magnification", &Metadata::magnification);

    value_object<SlideInfo>("SlideInfo")
        .field("format", &SlideInfo::format)
        .field("encoding", &SlideInfo::encoding)
        .field("extent", &SlideInfo::extent)
        .field("metadata", &SlideInfo::metadata);
        
    class_<IrisCodec::__INTERNAL__Slide>("Slide")
        .smart_ptr<IrisCodec::Slide>("Slide")
        .function("getSlideCodecVersion", &IrisCodec::__INTERNAL__Slide::get_slide_codec_version)
        .function("getSlideInfo",  &IrisCodec::__INTERNAL__Slide::get_slide_info)
        .function("getMetadataJSON",  &IrisCodec::__INTERNAL__Slide::get_metadata_JSON)
        .function("getSlideTile", &IrisCodec::__INTERNAL__Slide::get_slide_tile);
    
    function("validateFileStructure", &_validateFileStructure);
    function("openIrisSlide", &_openSlide);
}
