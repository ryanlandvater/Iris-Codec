//
//  IrisCodecSlide.cpp
//  Iris
//
//  Created by Ryan Landvater on 1/9/24.
//
#include <assert.h>
#include "IrisCodecPriv.hpp"

namespace IrisCodec {
Iris::Result is_iris_codec_file(const std::string &file_path) noexcept
{
    try {
        const auto result = Iris::File::Parser::open(file_path).is_iris_codec_file();
        if (result != IRIS_SUCCESS) throw std::runtime_error(result.message);
        return IRIS_SUCCESS;
    } catch (const std::exception&e) {
        return Iris::Result (
            IRIS_FAILURE, 
            "Iris File Extension test failed ("+
            file_path + "): " +
            e.what() + "\n"
        );
    }
}
Iris::Result validate_slide (const struct SlideOpenInfo &info) noexcept
{
    try {
        return Iris::File::Parser::open(info.filePath).validate_file_structure();
    } catch (const std::exception &e) {
        return Iris::Result (
            IRIS_FAILURE, 
            "Iris File Extension slide (" + 
            info.filePath + ") failed validation: " +
            e.what () + "\n"
        );
    }
}
Slide open_slide (const struct SlideOpenInfo &info) noexcept
{
    try {
        // Create a context if not provided
        Context context = info.context;
        if (context == nullptr) {
            ContextCreateInfo context_info {};
            context = std::make_shared<__INTERNAL__Context>(context_info);
        }
        if (context == nullptr) 
            throw std::runtime_error("No valid context");
        
        // The Parser maps the file read-only and owns the mapping for as long
        // as the slide holds it.
        return std::make_shared<__INTERNAL__Slide>(context, Iris::File::Parser::open(info.filePath));
        
    } catch (const std::exception &e) {
        std::cerr   << "Failed to open the slide "
                    << info.filePath << ": "
                    << e.what() << "\n";
        return nullptr;
    }
}
Iris::Result get_slide_info(const Slide &slide, SlideInfo& info) noexcept
{
    try {
        if (!slide)
            throw std::runtime_error("no valid slide object");
        
        info = slide->get_slide_info();
        
        return IRIS_SUCCESS;
    } catch (const std::exception& e) {
        return  {
            IRIS_FAILURE,
            std::string("Failed to read slide info: ") + e.what()
        };
    }   return IRIS_FAILURE;
}
Buffer read_slide_tile(const SlideTileReadInfo &info) noexcept
{
    try {
        // Ensure the slide object is valid
        if (info.slide == NULL)
            throw std::runtime_error("No valid codec slide object");
        
        // Read the slide tile
        auto result = info.slide->read_slide_tile(info);
        return result;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to read the slide tile"
                    << "[layer " << info.layerIndex
                    << ", tile " << info.tileIndex
                    << "]: " << e.what() << "\n";
        return NULL;
    }   return NULL;
}
Result get_associated_image_info(const Slide &slide, AssociatedImageInfo &info) noexcept
{
    try {
        if (!slide) throw std::runtime_error
            ("No valid codec slide object");
        if (!info.imageLabel.size()) throw std::runtime_error
            ("No image label provided within AssociatedImageInfo struct");
        
        info = slide->get_assoc_image_info(info.imageLabel);
        return IRIS_SUCCESS;
        
    } catch (const std::exception& e) {
        return {
            IRIS_FAILURE,
            std::string("Failed to get associated image info: ") +
            e.what()
        };
    }   return IRIS_FAILURE;
}
Buffer read_associated_image(const AssociatedImageReadInfo &info) noexcept
{
    try {
        if (!info.slide) throw std::runtime_error
            ("No valid slide object");
        if (!info.imageLabel.size()) throw std::runtime_error
            ("No image label provided within AssociatedImageReadInfo struct");
        
        return info.slide->read_assc_image(info);
        
    } catch (const std::exception& error) {
        std::cerr   << "Failed to read the associated image labeled \""
                    << info.imageLabel <<  "\": " << error.what();
        return NULL;
    }   return NULL;
}
Iris::Result annotate_slide(const Annotation &annotation) noexcept
{
    try {
        if (annotation.slide == NULL)
            throw std::runtime_error("No valid codec slide object");
        
        assert(false && "IrisCodec::annotate_slide() in irisCodecSlide.cpp not yet fully written");
//        return annotation.slide->write_slide_annotation(annotation);
    } catch (std::runtime_error& e) {
        return {
            IRIS_FAILURE,
            std::string("Failed to annotate the codec slide: ") +
            e.what()
        };
    }   return IRIS_FAILURE;
}
Iris::Result get_slide_annotations(const Slide &slide, Annotations &annotations) noexcept
{
    try {
        if (slide == NULL)
            throw std::runtime_error("No valid codec slide object");
        
        assert(false && "IrisCodec::get_slide_annotations() in irisCodecSlide.cpp not yet fully written");
        annotations.clear();
        
        return IRIS_SUCCESS;
    } catch (std::runtime_error& e) {
        return {
            IRIS_FAILURE,
            std::string("Failed to retrieve slide annotation objects: ") +
            e.what()
        };
    }   return IRIS_FAILURE;
}
__INTERNAL__Slide::__INTERNAL__Slide    (const Context& cxt, const Iris::File::Parser& parser) :
_context                                (cxt),
_parser                                 (parser)
{
    // Lift now, so a structurally damaged file is refused when it is opened
    // rather than on its first read.
    _parser.abstraction();
}
Version __INTERNAL__Slide::get_slide_codec_version() const
{
    return _parser.abstraction().metadata.codec;
}
SlideInfo __INTERNAL__Slide::get_slide_info() const
{
    const auto& file = _parser.abstraction();
    return SlideInfo {
        .format         = file.tileTable.format,
        .encoding       = file.tileTable.encoding,
        .extent         = file.tileTable.extent,
        .metadata       = file.metadata,
    };
}
const Iris::File::Parser& __INTERNAL__Slide::get_slide_parser() const
{
    return _parser;
}
Buffer __INTERNAL__Slide::get_slide_tile_entry(uint32_t layer, uint32_t tile_indx) const
{
    const auto stream = _parser.tile(layer, tile_indx);
    return Iris::Copy_strong_buffer_from_data(stream.data(), stream.size());
}
Buffer __INTERNAL__Slide::read_slide_tile(const SlideTileReadInfo &info) const
{
    const auto stream = _parser.tile(info.layerIndex, info.tileIndex);
    if (stream.empty()) throw std::runtime_error
        ("no tile at layer " + std::to_string(info.layerIndex) +
         ", tile " + std::to_string(info.tileIndex) + " (NULL_TILE)");
    Buffer src      = Iris::Wrap_weak_buffer_fom_data (stream.data(), stream.size());
    
    // Initialize the write destination
    Buffer dst_buffer   = nullptr;
    size_t dst_size     = 0;
    switch (info.desiredFormat) {
        case FORMAT_UNDEFINED: throw std::runtime_error
            ("desired format in SLideTileReadInfo is undefined");
        case Iris::FORMAT_B8G8R8:
        case Iris::FORMAT_R8G8B8:
            dst_size = TILE_PIX_AREA * 3;
            break;
        case Iris::FORMAT_B8G8R8A8:
        case Iris::FORMAT_R8G8B8A8:
            dst_size = TILE_PIX_AREA * 4;
            break;
    } if (!dst_size) throw std::runtime_error
        ("invalid desired slide format in SlideTileReadInfo");
    
    // Check to see if there is a destination provided to write into, and if that
    // destination buffer is sufficiently large to hold the unpacked data.
    if (info.optionalDestination && info.optionalDestination->capacity() >= dst_size)
            dst_buffer = info.optionalDestination;
    else    dst_buffer = Iris::Create_strong_buffer(dst_size);
    
    // Return the decompressed file structure
    dst_buffer = _context->decompress_tile({
        .compressed             = src,
        .optionalDestination    = dst_buffer,
        .desiredFormat          = info.desiredFormat,
        .encoding               = _parser.abstraction().tileTable.encoding,
    });
    if (!dst_buffer) throw std::runtime_error
        ("Failed to decompress slide tile");
    
    return dst_buffer;
}
AssociatedImageInfo __INTERNAL__Slide::get_assoc_image_info (const std::string &image_label) const
{
    const auto& images   = _parser.abstraction().images;
    const auto image_itr = images.find(image_label);
    if (image_itr == images.cend())
        throw std::runtime_error("get_assoc_image_info failed as there is no image with label \""+
                                 image_label + "\" within the slide file.");
    
    return image_itr->second.info;
}
Buffer __INTERNAL__Slide::get_assoc_image (const std::string &image_label) const
{
    const auto stream = _parser.image(image_label);
    return Copy_strong_buffer_from_data(stream.data(), stream.size());
}
Buffer __INTERNAL__Slide::read_assc_image (const AssociatedImageReadInfo &info) const
{
    const auto  image   = get_assoc_image_info(info.imageLabel);
    const auto  stream  = _parser.image(info.imageLabel);
    Buffer src          = Iris::Wrap_weak_buffer_fom_data (stream.data(), stream.size());
    
    // Initialize the write destination
    Buffer dst_buffer   = nullptr;
    size_t image_pixels = image.width * image.height;
    size_t dst_size     = 0;
    switch (info.desiredFormat) {
        case FORMAT_UNDEFINED: throw std::runtime_error
            ("desired format in SLideTileReadInfo is undefined");
        case Iris::FORMAT_B8G8R8:
        case Iris::FORMAT_R8G8B8:
            dst_size = image_pixels * 3; // 3 bytes per pixel format
            break;
        case Iris::FORMAT_B8G8R8A8:
        case Iris::FORMAT_R8G8B8A8:
            dst_size = image_pixels * 4; // 4 bytes per pixel format
            break;
    } if (!dst_size) throw std::runtime_error
        ("invalid desired slide format in SlideTileReadInfo");
    
    // Check to see if there is a destination provided to write into, and if that
    // destination buffer is sufficiently large to hold the unpacked data.
    if (info.optionalDestination && info.optionalDestination->capacity() >= dst_size)
            dst_buffer = info.optionalDestination;
    else    dst_buffer = Iris::Create_strong_buffer(dst_size);
    
    // Return the decompressed file structure
    dst_buffer = _context->decompress_image(DecompressImageInfo {
        .compressed             = src,
        .optionalDestination    = dst_buffer,
        .width                  = image.width,
        .height                 = image.height,
        .sourceFormat           = image.sourceFormat,
        .desiredFormat          = info.desiredFormat,
        .encoding               = image.encoding,
    });
    if (!dst_buffer) throw std::runtime_error
        ("Failed to decompress slide tile");
    
    return dst_buffer;
}
Iris::Result __INTERNAL__Slide::write_slide_annotation (const IrisCodec::Annotation &annotation)
{
    return IRIS_FAILURE;
}
} // END IRIS CODEC NAMESPACE
