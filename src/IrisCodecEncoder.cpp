//
//  IrisCodecEncoder.cpp
//  Iris
//
//  Created by Ryan Landvater on 8/2/22.
//
#include <cmath>
#include <cstring>
#include <filesystem>
#include <random>
#include <vector>
#include "IrisCodecPriv.hpp"
#include "IFE_Builder.hpp"   // the block tier: builder->claim / fill / append

// TODO: Make max pending memory a runtime configurable
constexpr size_t MAX_MEMORY_PRESSURE = 2E9; // 2 GB

namespace IrisCodec {
using Iris::File::Builder;

inline void CHECK_ENCODER (const Encoder& encoder) {
    if (!encoder)               throw std::runtime_error ("No valid encoder provided");
}
inline void CHECK_MUTABLE (const Encoder& encoder) {
    CHECK_ENCODER(encoder);
    switch (encoder->get_status()) {
        case ENCODER_INACTIVE:  return;
        case ENCODER_ACTIVE:    throw std::runtime_error("Encoder currently active and thus immutable");
        case ENCODER_ERROR:     throw std::runtime_error("Encoder failed in error and must be reset first.");
        case ENCODER_SHUTDOWN:  throw std::runtime_error("Encoder undergoing destruction and thus immutable");
    }
}
inline std::string TO_STRING (EncoderStatus status)
{
    switch (status) {
        case ENCODER_INACTIVE:  return "ENCODER_INACTIVE";
        case ENCODER_ACTIVE:    return "ENCODER_ACTIVE";
        case ENCODER_ERROR:     return "ENCODER_ERROR";
        case ENCODER_SHUTDOWN:  return "ENCODER_SHUTDOWN";
    }   return "CORRUPT ENCODER STATUS IDENTIFIED";
}
Encoder create_encoder(EncodeSlideInfo &info) noexcept
{
    try {
        // Check the context status; If no context, create one.
        Context& context = info.context;
        if (context == nullptr) {
            ContextCreateInfo context_info {nullptr};
            context = std::make_shared<__INTERNAL__Context>(context_info);
        } if (context == nullptr)
            throw std::runtime_error("No valid context created or available");
        
        // Set the destination file directory to the source directory
        // if it does not exist
        std::filesystem::path source_file_path = info.srcFilePath;
        auto source_name = source_file_path.filename();
        auto source_dir  = source_file_path.parent_path();
        
        if (std::filesystem::exists(source_file_path) == false)
            throw std::runtime_error("source slide file "+info.srcFilePath+" does not exist");
        
        if (info.dstFilePath.size() == 0 ||
            std::filesystem::is_directory(info.dstFilePath) == false)
            info.dstFilePath = source_dir.string();
        
        // Return the newly constructed encoder object
        return std::make_shared<__INTERNAL__Encoder>(info);
        
    } catch (std::runtime_error&e) {
        std::cerr   << "Failed to create a valid slide encoder: "
        << e.what() << "\n";
        return NULL;
    }   return NULL;
}
Result reset_encoder(Encoder &encoder) noexcept
{
    try {
        CHECK_ENCODER(encoder);
        return encoder->reset_encoder();
    } catch (std::runtime_error&e) {
        return {
            IRIS_FAILURE,
            e.what()
        };
    }
}
Result dispatch_encoder (const Encoder& encoder) noexcept
{
    try {
        CHECK_ENCODER(encoder);
        return encoder->dispatch_encoder();
    } catch (std::runtime_error& e) {
        encoder->reset_encoder();
        return Iris::Result {
            IRIS_FAILURE,
            e.what()
        };
    } return IRIS_FAILURE;
}
Result interrupt_encoder(const Encoder & encoder) noexcept
{
    try {
        CHECK_ENCODER(encoder);
        return encoder->interrupt_encoder();
    } catch (std::runtime_error&e) {
        return Iris::Result {
            IRIS_FAILURE,
            e.what()
        };
    } return IRIS_FAILURE;
}
Result get_encoder_progress (const Encoder &encoder, EncoderProgress &progress) noexcept
{
    try {
        CHECK_ENCODER(encoder);
        return encoder->get_encoder_progress(progress);
    } catch (std::runtime_error&e) {
        return Result {
            IRIS_FAILURE,
            e.what()
        };
    } return IRIS_FAILURE;
}
Result get_encoder_src(const Encoder &encoder, std::string &src_string) noexcept
{
    try {
        CHECK_ENCODER(encoder);
        src_string = encoder->get_src_path();
        return IRIS_SUCCESS;
    } catch (std::runtime_error&e) {
        return Result {
            IRIS_FAILURE,
            e.what()
        };
    } return IRIS_FAILURE;
}
Result get_encoder_dst_path(const Encoder &encoder, std::string &dst_string) noexcept
{
    try {
        CHECK_ENCODER(encoder);
        dst_string = encoder->get_dst_path();
        return IRIS_SUCCESS;
    } catch (std::runtime_error&e) {
        return Result {
            IRIS_FAILURE,
            e.what()
        };
    } return IRIS_FAILURE;
}
Result set_encoder_src(const Encoder &encoder, const std::string &source_path) noexcept
{
    try {
        CHECK_ENCODER(encoder);
        CHECK_MUTABLE(encoder);
        encoder->set_src_path(source_path);
        return IRIS_SUCCESS;
    } catch (std::runtime_error&e) {
        return {
            IRIS_FAILURE,
            e.what()
        };
    }
}
Result set_encoder_dst_path(const Encoder &encoder, const std::string &dst_path) noexcept
{
    try {
        CHECK_ENCODER(encoder);
        CHECK_MUTABLE(encoder);
        encoder->set_dst_path(dst_path);
        return IRIS_SUCCESS;
    } catch (std::runtime_error&e) {
        return {
            IRIS_FAILURE,
            e.what()
        };
    }
}
__INTERNAL__Encoder::__INTERNAL__Encoder    (const EncodeSlideInfo& __i) :
_concurrency                                (__i.concurrency),
_derive                                     (__i.derivation),
_context                                    (__i.context),
_srcPath                                    (__i.srcFilePath),
_dstPath                                    (__i.dstFilePath),
_anonymize                                  (__i.anonymize),
_encoding                                   (__i.desiredEncoding),
_derivation                                 (_derive?*__i.derivation:EncoderDerivation()),
_status                                     (ENCODER_INACTIVE)
{
    
}
__INTERNAL__Encoder::~__INTERNAL__Encoder   ()
{
    while (_status == ENCODER_ACTIVE) {
        _status.wait(ENCODER_ACTIVE);
        auto STATUS = ENCODER_INACTIVE;
        _status.compare_exchange_strong(STATUS, ENCODER_SHUTDOWN);
        switch (STATUS) {
            case ENCODER_INACTIVE:
                _status = ENCODER_SHUTDOWN;
            case ENCODER_SHUTDOWN:
            case ENCODER_ERROR:
                break;
            case ENCODER_ACTIVE:
                continue;
        }
    } for (auto& thread : _threads)
        if (thread.joinable()) thread.join();
}
// MARK: Getters
EncoderStatus __INTERNAL__Encoder::get_status() const
{
    return _status;
}
Context __INTERNAL__Encoder::get_context() const
{
    return _context;
}
std::string __INTERNAL__Encoder::get_src_path() const
{
    return _srcPath;
}
std::string __INTERNAL__Encoder::get_dst_path() const
{
    return _dstPath;
}
Encoding __INTERNAL__Encoder::get_encoding() const {
    return _encoding;
}
Result __INTERNAL__Encoder::get_encoder_progress (EncoderProgress &progress) const
{
    progress.dstFilePath    = _dstPath;
    progress.status         = _status.load();
    switch (progress.status) {
        case ENCODER_ACTIVE:
            if (_tracker.total == 0)
                return {IRIS_FAILURE,"Tracker returned 0 total tiles in slide"};
            progress.progress       = static_cast<float>(_tracker.completed.load())
                                    / static_cast<float>(_tracker.total);
            progress.dstFilePath    = _tracker.dst_path;
            progress.errorMsg       = _tracker.error_msg;
            return IRIS_SUCCESS;
            
        case ENCODER_ERROR: {
            MutexLock __ (const_cast<Mutex&>(_tracker.error_msg_mutex));
            progress.errorMsg       = _tracker.error_msg;
        }
        case ENCODER_INACTIVE:
        case ENCODER_SHUTDOWN:
            return IRIS_SUCCESS;
    }   return IRIS_FAILURE;
}
// MARK: Setters
void __INTERNAL__Encoder::set_src_path(const std::string &source)
{
    switch (_status) {
        case ENCODER_INACTIVE:break;
        default:
            throw std::runtime_error("Encoder is currently active; cannot change source path");
    }
    _srcPath = source;
}
void __INTERNAL__Encoder::set_dst_path(const std::string &destination)
{
    switch (_status) {
        case ENCODER_INACTIVE:break;
        default:
            throw std::runtime_error("Encoder is currently active; cannot change destination path");
    }
    _dstPath = destination;
}
void __INTERNAL__Encoder::set_encoding(Encoding desired_encoding)
{
    switch (_status) {
        case ENCODER_INACTIVE:break;
        default:
            std::cerr << "Encoder is currently active; cannot change encoding\n";
            return;
    }
    _encoding = desired_encoding;
}
Result __INTERNAL__Encoder::reset_encoder()
{
    switch (_status) {
        case ENCODER_INACTIVE:
        case ENCODER_ACTIVE:
            _status.store(ENCODER_ERROR);
        case ENCODER_ERROR:
            _status.notify_all();
            for (auto& thread : _threads)
                if (thread.joinable()) thread.join();
            break;
            
        case ENCODER_SHUTDOWN:  return {
            IRIS_FAILURE,
            "Cannot reset an encoder in SHUTDOWN"
        };
    }
    
    MutexLock __ (_tracker.error_msg_mutex);
    _tracker.dst_path.clear();
    _tracker.error_msg.clear();
    _tracker.completed = 0;
    _status.store(ENCODER_INACTIVE);
    
    return IRIS_SUCCESS;
}

// MARK: - OPENSLIDE METHODS
#if IRIS_INCLUDE_OPENSLIDE
#include <openslide/openslide.h>
inline Extent READ_EXTENT_OPENSLIDE (openslide_t* openslide)
{
    Extent          extent;
    
    int64_t L0_width, L0_height;
    openslide_get_level0_dimensions(openslide,&L0_width,&L0_height);

    auto n_levels = openslide_get_level_count(openslide);
    int64_t width, height;
    openslide_get_level_dimensions(openslide, n_levels-1, &width, &height);
    extent.width        = U32_CAST(width);
    extent.height       = U32_CAST(height);
    extent.layers       = LayerExtents(n_levels);
    auto extent_IT     = extent.layers.begin();
    auto f_level        = U32_CAST(0);
    auto r_level        = U32_CAST(extent.layers.size() - 1);
    for (; extent_IT  != extent.layers.end(); f_level++, r_level--, extent_IT++) {
        auto& _SL_      = *extent_IT;
        openslide_get_level_dimensions(openslide, r_level, &width, &height);
        _SL_.xTiles      = U32_CAST(std::ceil(F32_CAST(width)/F32_CAST(TILE_PIX_LENGTH)));
        _SL_.yTiles      = U32_CAST(std::ceil(F32_CAST(height)/F32_CAST(TILE_PIX_LENGTH)));
        _SL_.scale       =  width > height ?
        F32_CAST(width)/F32_CAST(extent.width) :
        F32_CAST(height)/F32_CAST(extent.height);
    }
    for (extent_IT = extent.layers.begin(); extent_IT != extent.layers.end(); extent_IT++)
        extent_IT->downsample = extent.layers.back().scale / extent_IT->scale;
    
    return extent;
}
inline Buffer READ_OPENSLIDE_TILE (const EncoderSource src, LayerIndex __LI, TileIndex __TI)
{
    const auto os       = src.openslide;
    if (os == NULL)                                     return NULL;
    const auto extent   = src.extent;
    if (__LI    >= extent.layers.size())                return NULL;
    auto& __LE   = extent.layers[__LI];
    if (__TI    >= __LE.xTiles * __LE.yTiles)           return NULL;
    auto buffer         = Create_strong_buffer  (TILE_PIX_BYTES_RGBA);
    auto& level_extent  = extent.layers[__LI];
    auto openSlideLevel = static_cast<uint32_t> ((extent.layers.size()-1)-__LI);
    auto x_tile_index   = static_cast<float>    (__TI % level_extent.xTiles);
    auto y_tile_index   = static_cast<float>    (__TI / level_extent.xTiles);
    openslide_read_region(os, static_cast<uint32_t*>(buffer->append(TILE_PIX_BYTES_RGBA)),
                          static_cast<int64_t>  (std::round(x_tile_index * TILE_PIX_LENGTH * level_extent.downsample)),
                          static_cast<int64_t>  (std::round(y_tile_index * TILE_PIX_LENGTH * level_extent.downsample)),
                          openSlideLevel,
                          TILE_PIX_LENGTH, TILE_PIX_LENGTH);
    return buffer;
}
enum OpenSlideProperties {
    NOT_OPENSLIDE,
    UNUSED,
    MPP,
    OBJECTIVE_POWER,
    
};
inline OpenSlideProperties PARSE_OPENSLIDE_PROPERTY (const char* const key_chars) {
    if (strstr(key_chars, "openslide") == NULL) return NOT_OPENSLIDE;
    if (strcmp(key_chars, OPENSLIDE_PROPERTY_NAME_MPP_X) == 0) return MPP;
    if (strcmp(key_chars, OPENSLIDE_PROPERTY_NAME_OBJECTIVE_POWER) == 0) return OBJECTIVE_POWER;
    return UNUSED;
}
inline Metadata READ_OPENSLIDE_METADATA (const EncoderSource src, const Extent& extent, bool anonymize) {
    openslide_t* os = src.openslide;
    Metadata metadata;
    
    metadata.codec = get_codec_version();
    // Insert the attributes
    metadata.attributes.type = METADATA_FREE_TEXT;
    const char* const* attributes = openslide_get_property_names(os);
    for (int index = 0; attributes[index] != NULL; ++index) {
        switch (PARSE_OPENSLIDE_PROPERTY(attributes[index])) {
            // Unused openslie parameter, probably duplicated elsewhere
            case UNUSED: continue;
            
            // Break and encode as is. It's likely a vendor feature
            case NOT_OPENSLIDE: break;
                
            // MpPN (Normalized MPP to layer scale)
            // We multiply because this value is currently norm
            // relative to the highest resolution layer (ex 1/64x)
            // This will allow for direct comp with on-screen pixels
            // Because viewers work in relative zoom.
            // We want this 1:1 lowest res layer (layer scale = 1)
            case MPP: {
                metadata.micronsPerPixel = round(atof
                (openslide_get_property_value(os, attributes[index])) *
                 extent.layers.front().downsample * 1000.f)/1000.f;
                // You will note it's rounded to the 1000ths
                continue;
            }
                
            // Normalize the magnification coefficient ratio
            // to the layer scale. This allows for multiplying
            // with relative zoom for direct comp with on-screen pixels
            case OBJECTIVE_POWER:
                metadata.magnification = round(atof
                (openslide_get_property_value(os, attributes[index])) /
                 extent.layers.front().downsample * 1000.f)/1000.f;
                // You will note it's rounded to the 1000ths
                continue;
        }
        if (auto attribute = openslide_get_property_value(os, attributes[index])) {
            auto __key = std::string(attributes[index]);
            auto __str = std::string(attribute);
            metadata.attributes[__key] = std::u8string(__str.begin(),__str.end());
        }
    }
    
    // Insert the associated image labels
    const char* const* image_labels = openslide_get_associated_image_names(os);
    for (int label = 0; image_labels[label] != NULL; ++label)
        metadata.associatedImages.insert(std::string(image_labels[label]));
    
    // Insert the ICC profile (if present)
    int64_t icc_bytes = openslide_get_icc_profile_size(os);
    if (icc_bytes > 0) {
        metadata.ICC_profile.resize(icc_bytes);
        openslide_read_icc_profile(os, metadata.ICC_profile.data());
    }
    
    return metadata;
}
inline AssociatedImageInfo READ_OPENSLIDE_ASSOCIATED_IMAGE_INFO (openslide_t* os, const std::string& label)
{
    int64_t width, height;
    openslide_get_associated_image_dimensions(os, label.c_str(), &width, &height);
    if (width > UINT32_MAX) throw std::runtime_error
        ("openslide associated image width is greater than 32-bit max value");
    if (height > UINT32_MAX) throw std::runtime_error
        ("openslide associated image height is greater than 32-bit max value");
    return AssociatedImageInfo {
        .imageLabel     = label,
        .width          = static_cast<uint32_t>(width),
        .height         = static_cast<uint32_t>(height),
        .encoding       = IMAGE_ENCODING_DEFAULT,
        .sourceFormat   = Iris::FORMAT_B8G8R8A8, // Openslide is alway ARGB (big-endian)
        .orientation    = ORIENTATION_0
    };
}
inline Buffer READ_OPENSLIDE_ASSOCIATED_IMAGE (openslide_t* os, const AssociatedImageInfo& info)
{
    size_t image_size = info.width * info.height * sizeof(uint32_t);
    Buffer dst = Create_strong_buffer(image_size);
    openslide_read_associated_image(os, info.imageLabel.c_str(), static_cast<uint32_t*>(dst->data()));
    dst->set_size(image_size);
    return dst;
}
#endif
// MARK: - DICOM SPECIFIC METHODS
using DcmFile = std::shared_ptr<struct __INTERNAL__DcmFile>;
DcmFile  open_dicom_file             (const std::filesystem::path&);
uint32_t get_dicom_number_of_levels  (DcmFile dicom);
uint32_t get_dicom_number_of_frames  (DcmFile dicom, unsigned level);
uint32_t get_dicom_layer_tile_width  (DcmFile dicom, unsigned level);
uint32_t get_dicom_layer_tile_height (DcmFile dicom, unsigned level);
uint32_t get_dicom_layer_width       (DcmFile dicom, unsigned level);
uint32_t get_dicom_layer_height      (DcmFile dicom, unsigned level);
Encoding get_dicom_encoding          (DcmFile dicom);
Buffer   get_dicom_frame_buffer      (DcmFile dicom, unsigned levelIndex, unsigned frame);
Metadata get_dicom_metadata          (DcmFile dicom, bool anonymize);
inline Extent READ_EXTENT_DICOM (DcmFile dicom_file)
{
    Extent          extent;

    // Get the total dimensions of the image
    auto n_levels   = get_dicom_number_of_levels (dicom_file);
    extent.width    = get_dicom_layer_width(dicom_file, 0);
    extent.height   = get_dicom_layer_height(dicom_file, 0);
    extent.layers   = LayerExtents(n_levels);
    auto extent_IT  = extent.layers.begin();
    auto f_level    = U32_CAST(0);
    auto r_level    = U32_CAST(extent.layers.size() - 1);
    for (; extent_IT  != extent.layers.end();
         f_level++, r_level--, extent_IT++) {
        auto& __e   = *extent_IT;
        auto width  = get_dicom_layer_width(dicom_file, f_level);
        auto height = get_dicom_layer_height(dicom_file, f_level);
        __e.xTiles  = (width/TILE_PIX_LENGTH) + (width%TILE_PIX_LENGTH?1:0);
        __e.yTiles  = (height/TILE_PIX_LENGTH) + (height%TILE_PIX_LENGTH?1:0);
        __e.scale   =  width > height ?
        round(F32_CAST(width)/F32_CAST(extent.width)*100.f)/100.f :
        round(F32_CAST(height)/F32_CAST(extent.height)*100.f)/100.f;
        assert(__e.xTiles*__e.yTiles == get_dicom_number_of_frames(dicom_file, f_level));
    }
    for (extent_IT = extent.layers.begin(); extent_IT != extent.layers.end(); extent_IT++)
        extent_IT->downsample = extent.layers.back().scale / extent_IT->scale;
    
    return extent;
}
inline Buffer GET_DICOM_TILE (const EncoderSource src, LayerIndex __LI, TileIndex __TI)
{
    const auto dicom = src.dicomFile;
    if (dicom == NULL)                      return NULL;
    const auto extent = src.extent;
    if (__LI >= extent.layers.size())       return NULL;
    auto& __LE   = extent.layers[__LI];
    if (__TI >= __LE.xTiles * __LE.yTiles)  return NULL;
    
    return get_dicom_frame_buffer(dicom, __LI, __TI+1);
}
inline Metadata READ_DICOM_METADATA (const EncoderSource src, const Extent& extent, bool anonymize) {
    auto dicom = src.dicomFile;
    
    // Read the raw metadata from the DICOM image
    Metadata metadata = get_dicom_metadata (dicom, anonymize);
    
    // Inject the current codec version
    // NOTE: This is NOT the Iris File Extension version.
    // That's written in under the hood.
    metadata.codec = get_codec_version();
    
    // Normalize the magnification and Microns per pixel
    // Relative to the first layer. This is more considerate for viewers
    metadata.magnification /= extent.layers.front().downsample;
    metadata.micronsPerPixel /= extent.layers.front().downsample;
    
    return metadata;
}
// MARK: - APERIO SPECIFIC METHODS

// MARK: - FILE ENCODING METHODS
inline EncoderSource OPEN_SOURCE (const std::string& path_, const Context context = NULL)
{
    std::filesystem::path path (path_);
    if (!std::filesystem::exists(path)) throw std::runtime_error
        ("File system failed to identify source file " + path.string());
    
    if (is_iris_codec_file(path.string())) {
        EncoderSource source;
        source.sourceType   = EncoderSource::ENCODER_SRC_IRISSLIDE;
        source.irisSlide    = open_slide(SlideOpenInfo {
            .filePath       = path_,
            .context        = context,
            .writeAccess    = false,
        });
        if (!source.irisSlide) throw std::runtime_error
            ("No valid Iris slide returned from IrisCodec::open_slide");
        
        source.extent       = source.irisSlide->get_slide_info().extent;
        source.format       = source.irisSlide->get_slide_info().format;
        source.encoding     = source.irisSlide->get_slide_info().encoding;
            
        return source;
    }
    
    // If the path ends in a DICOM Extension, try DICOM
    if (path.extension() == ".dcm") try {
        if (auto handle = open_dicom_file(path)) {
            EncoderSource source;
            source.sourceType   = EncoderSource::ENCODER_SRC_DICOM;
            source.dicomFile    = handle;
            source.extent       = READ_EXTENT_DICOM(handle);
            source.encoding     = get_dicom_encoding(handle);
            
            return source;
        }
    } catch (std::runtime_error &e) {
        std::cout   << "[WARNING] Failed to establish DICOM source \'"
                    << path_ << "\' as an encoder source: "
                    << e.what() << ". Reattempting using OpenSlide.\n";
        goto TRY_OPENSLIDE;
    }
    
    if (path.extension() == ".svs") try {
        // SVS WILL GO HERE
    } catch (std::runtime_error &e) {
        std::cout   << "[WARNING] Failed to establish Aperio SVS source \'"
                    << path_ << "\' as an encoder source: "
                    << e.what() << ". Reattempting using OpenSlide.\n";
        goto TRY_OPENSLIDE;
    }
    TRY_OPENSLIDE:
    #if IRIS_INCLUDE_OPENSLIDE
    if (openslide_detect_vendor(path_.c_str())) {
        EncoderSource source;
        source.sourceType   = EncoderSource::ENCODER_SRC_OPENSLIDE;
        source.openslide    = openslide_open(path_.c_str());
        
        if (!source.openslide) throw std::runtime_error
            ("No valid openslide handle returned from openslide_open");

        source.extent       = READ_EXTENT_OPENSLIDE(source.openslide);
        source.format       = FORMAT_B8G8R8A8; // OpenSlide always reads ARGB
        
        return source;
    }
    throw std::runtime_error("Provided source file path was not recognized by any available decoders.");
    #else
    throw std::runtime_error("Provided source file path was not recognized by any available decoders. You may need an encoder built with OpenSlide enabled.");
    #endif
   
}
inline Buffer GET_SOURCE_TILE (const EncoderSource& src, LayerIndex layer, TileIndex tile)
{
    switch (src.sourceType) {
        case EncoderSource::ENCODER_SRC_UNDEFINED: throw std::runtime_error("Cannot read source tile; undefined source");
        case EncoderSource::ENCODER_SRC_IRISSLIDE:
            return src.irisSlide->get_slide_tile_entry(layer, tile);
        case EncoderSource::ENCODER_SRC_OPENSLIDE:
            #if IRIS_INCLUDE_OPENSLIDE
            return NULL;
            #else
            throw std::runtime_error("Openslide linkage was NOT compiled into this binary. Request a new version of Iris Codec with OpenSlide support if you would like to decode slide scanning vendor slide files only accessable to OpenSlide.");
            #endif
        case EncoderSource::ENCODER_SRC_DICOM:
            return GET_DICOM_TILE(src, layer, tile);
        case EncoderSource::ENCODER_SRC_APERIO:
            throw std::runtime_error("APERIO TIFF reads not yet built; Use openslide for the moment");
    }
    return NULL;
}
inline Buffer READ_SOURCE_TILE (const Context& ctx, const EncoderSource& src, LayerIndex layer, TileIndex tile)
{
    switch (src.sourceType) {
        case EncoderSource::ENCODER_SRC_UNDEFINED: throw std::runtime_error("Cannot read source tile; undefined source");
        case EncoderSource::ENCODER_SRC_IRISSLIDE:
            return IrisCodec::read_slide_tile (SlideTileReadInfo{
                .slide          = src.irisSlide,
                .layerIndex     = layer,
                .tileIndex      = tile,
                .desiredFormat  = FORMAT_R8G8B8A8});
        case EncoderSource::ENCODER_SRC_OPENSLIDE:
            #if IRIS_INCLUDE_OPENSLIDE
            return READ_OPENSLIDE_TILE (src, layer, tile);
            #else
            throw std::runtime_error("Openslide linkage was NOT compiled into this binary. Request a new version of Iris Codec with OpenSlide support if you would like to decode slide scanning vendor slide files only accessable to OpenSlide.");
            #endif
            
        case EncoderSource::ENCODER_SRC_DICOM: {
            return ctx->decompress_tile({
                .compressed     = GET_DICOM_TILE(src, layer, tile),
                .desiredFormat  = FORMAT_R8G8B8A8,
                .encoding       = get_dicom_encoding(src.dicomFile)
            });
        }
        case EncoderSource::ENCODER_SRC_APERIO:
            throw std::runtime_error("APERIO TIFF reads not yet built; Use openslide for the moment");
    }
    return NULL;
}
inline static void ENCODE_SOURCE_PYRAMID (const Context ctx,
                                          const EncoderSource& src,
                                          const Builder builder,
                                          const Encoding encoding,
                                          EncoderTracker* _tracker,
                                          AtomicEncoderStatus* _status)
{
    auto& extent    = src.extent;
    auto& tracker   = *_tracker;
    auto& status    = *_status;

    // Allocate a layer and tile index counter.
    uint32_t __LI           = 0;
    uint32_t __TI           = 0;
    uint32_t __LAYERS       = U32_CAST(extent.layers.size());

    try { for (__LI = 0; __LI < __LAYERS; ++__LI) {
        auto& layer_tracker = tracker.layers[__LI];
        for (__TI = 0; __TI < layer_tracker.size(); ++__TI) {
            // System check step: Only continue if the encoder is active
            if (status != ENCODER_ACTIVE) return;
            
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            //  CAPTURE TILE STEP
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            auto& tile  = layer_tracker[__TI];
            auto STATUS = TILE_FREE;
            if (tile.status.compare_exchange_strong(STATUS, TILE_READING)==false)
                continue;
            
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            //  READ TILE STEP
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            auto bytes           = GET_SOURCE_TILE (src, __LI, __TI);
            if  (bytes == NULL) {
                auto pixel_array = READ_SOURCE_TILE (ctx, src, __LI, __TI);
                if (!pixel_array) throw std::runtime_error
                    ("Failed to read slide image data");
                bytes           = ctx->compress_tile({
                    .pixelArray = pixel_array,
                    .format     = src.format,
                    .encoding   = encoding
                });
            }
            if (!bytes) throw std::runtime_error("Failed to compress slide image data");

            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            //  WRITE TO FILE STEP
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            //  An Iris source's tiles pass through as they are: an empty stream
            //  is its NULL_TILE, still "no tile at this position", and a
            //  Z-stacked stream keeps the plane count its frame records.
            if (bytes->size() == 0) builder.append_null_tile(__LI, __TI);
            else builder.append_tile(__LI, __TI, static_cast<const BYTE*>(bytes->data()), bytes->size(),
                                     src.irisSlide ? src.irisSlide->get_slide_parser().tile_planes(__LI, __TI) : 0);

            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            //  RELEASE TILE STEP
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            tile.status.store(TILE_COMPLETE);
            tracker.completed++;
        }
    }
    } catch (const std::exception&e) {   // the Builder refuses with logic_error kinds too
        status.store(ENCODER_ERROR);
        MutexLock __ (tracker.error_msg_mutex);
        tracker.error_msg += std::string("Slide tile encoding failed: ") +
                             e.what() + "\n";
        status.notify_all();
        return;
    }
    
}
inline static void ENCODE_DERIVE_PYRAMID (const Context ctx,
                                          const EncoderSource& src,
                                          EncoderTracker* _tracker,
                                          AtomicEncoderStatus* _status,
                                          const std::function <void(uint32_t layer_index,
                                                                    uint32_t y_index,
                                                                    uint32_t x_index)>
                                          & ENQUEUE_TILE)
{
    const auto& src_extent   = src.extent;
    const auto& layer_extent = src_extent.layers.back();
    auto& layer_tracker      = _tracker->layers.back();
    try {
        uint32_t src_l = U32_CAST(src_extent.layers.size()-1);
        uint32_t dst_l = U32_CAST(_tracker->layers.size()-1);
        for (uint32_t y = 0; y < layer_extent.yTiles; ++y) {
            for (uint32_t x = 0; x < layer_extent.xTiles; ++x) {
                if (_status->load() != ENCODER_ACTIVE) return;
                
                uint32_t __TI = y * layer_extent.xTiles + x;
                //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
                //  CAPTURE TILE STEP
                //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
                auto& tile  = layer_tracker[__TI];
                auto STATUS = TILE_FREE;
                if (tile.status.compare_exchange_strong(STATUS, TILE_READING)==false)
                    continue;
                //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
                //  READ TILE STEP
                //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
                tile.stream     = GET_SOURCE_TILE (src, src_l, __TI);
                if (tile.stream) {
                    // Decode into the tile table's format: derived tiles are
                    // downsampled and recompressed as that format.
                    tile.pixels = ctx->decompress_tile({
                        .compressed     = tile.stream,
                        .desiredFormat  = src.format,
                        .encoding       = src.encoding,
                    });
                }
                if (tile.pixels == NULL)
                    tile.pixels = READ_SOURCE_TILE(ctx, src, src_l, __TI);
                if (!tile.pixels) throw std::runtime_error("Failed to read slide image data");
                //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
                //  PROPOGATE TILE ENCODING STEP
                //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
                tile.status.store(TILE_PENDING);
                ENQUEUE_TILE (dst_l,y,x);
            }
        }
    } catch (const std::exception&e) {   // an Iris source's Parser reports out_of_range
        _status->store(ENCODER_ERROR);
        MutexLock __ (_tracker->error_msg_mutex);
        _tracker->error_msg += std::string("Slide tile encoding failed: ") +
                             e.what() + "\n";
        _status->notify_all();
        return;
    }
}
inline Metadata READ_METADATA (const EncoderSource& source, const Extent& extent, bool anonymize) {
    switch (source.sourceType) {
        case EncoderSource::ENCODER_SRC_UNDEFINED:
            throw std::runtime_error
            ("READ_METADATA failed due to ENCODER_SRC_UNDEFINED source type");
        case EncoderSource::ENCODER_SRC_IRISSLIDE: {
            auto metadata = source.irisSlide->get_slide_info().metadata;
            if (anonymize)  {
                metadata.attributes.clear();
                metadata.associatedImages.clear();
            } return metadata;
        }
        case EncoderSource::ENCODER_SRC_OPENSLIDE:
            return READ_OPENSLIDE_METADATA(source,extent,anonymize);
        case EncoderSource::ENCODER_SRC_DICOM:
            return READ_DICOM_METADATA(source,extent,anonymize);
        case EncoderSource::ENCODER_SRC_APERIO:
            //TODO: APERIO READ METADATA
            throw std::runtime_error
            ("READ_METADATA failed as APERIO TIFF reads not yet built; Use openslide for the moment");
    } throw std::runtime_error
    ("READ_METADATA due to invalid source type value ("+std::to_string(source.sourceType)+")");
}
inline void APPEND_ASSOCIATED_IMAGES (const Context& ctx,
                                      const Builder& builder,
                                      const EncoderSource& source,
                                      const Metadata& metadata)
{
    // One IMAGE_BYTES block per label, in label order. An image that cannot be
    // read is reported and skipped; it does not fail the slide.
    for (auto& label : metadata.associatedImages) try {
        AssociatedImageInfo info;
        Buffer bytes;
        switch (source.sourceType) {
            case EncoderSource::ENCODER_SRC_UNDEFINED:
                throw std::runtime_error("undefined source type");
            case EncoderSource::ENCODER_SRC_IRISSLIDE:
                info    = source.irisSlide->get_assoc_image_info(label);
                bytes   = source.irisSlide->get_assoc_image(label);
                break;
            case EncoderSource::ENCODER_SRC_OPENSLIDE:
                info    = READ_OPENSLIDE_ASSOCIATED_IMAGE_INFO(source.openslide, label);
                bytes   = ctx->compress_image(CompressImageInfo{
                    .pixelArray = READ_OPENSLIDE_ASSOCIATED_IMAGE(source.openslide, info),
                    .width      = info.width,
                    .height     = info.height,
                    .format     = info.sourceFormat,
                    .encoding   = info.encoding,
                    .quality    = QUALITY_DEFAULT
                });
                break;
            case EncoderSource::ENCODER_SRC_DICOM:
                throw std::runtime_error("DICOM associated images are not yet read");
            case EncoderSource::ENCODER_SRC_APERIO:
                throw std::runtime_error("APERIO TIFF reads not yet built; Use openslide for the moment");
        }
        if (!bytes) throw std::runtime_error("no compressed image stream");
        builder.append_image(info, static_cast<const BYTE*>(bytes->data()), bytes->size());
    } catch (const std::exception &error) {
        std::cout   << "Failed to store associated image labeled \""
                    << label << "\": " << error.what() << "\n";
    }
}
inline Iris::File::BuilderFinalizeInfo WRITE_SLIDE_STRUCTURE (const Context& ctx,
                                                              const Iris::File::BuilderTileTableInfo& grid,
                                                              const Builder& builder,
                                                              const EncoderSource& source,
                                                              const Metadata& metadata)
{
    using namespace Iris::File::Serialization;
    // The layout after the tiles puts blocks that never change ahead of those an
    // edit replaces: TILE TABLE | METADATA | ICC | IMAGES | ATTRIBUTES, with
    // annotations to follow. The grid is the caller's (it set it on the
    // Builder); the Builder reports where it placed the tiles.
    const auto tiles  = builder.tile_offsets();
    const auto planes = grid.planes.empty()
        ? std::vector<uint16_t>(grid.extent.layers.size(), 0) : grid.planes;
    std::vector<LayerExtentEntry> extents;
    extents.reserve(grid.extent.layers.size());
    for (std::size_t l = 0; l < grid.extent.layers.size(); ++l)
        extents.push_back({.X_TILES  = grid.extent.layers[l].xTiles,
                           .Y_TILES  = grid.extent.layers[l].yTiles,
                           .SCALE    = grid.extent.layers[l].scale,
                           .Z_PLANES = planes[l]});
    const Offset offsets_at = builder->append(TileOffsetsCreateInfo{.entries = tiles});
    const Offset extents_at = builder->append(LayerExtentsCreateInfo{.entries = extents});
    const Offset table_at   = builder->append(TileTableCreateInfo{
        .ENCODING             = static_cast<Iris::File::constants::TileEncodings>(grid.encoding),
        .FORMAT               = static_cast<Iris::File::constants::PixelFormats>(grid.format),
        .TILE_OFFSETS_OFFSET  = offsets_at,
        .LAYER_EXTENTS_OFFSET = extents_at,
        .X_EXTENT             = grid.extent.width,
        .Y_EXTENT             = grid.extent.height,
        .TILE_LENGTH          = grid.tileLength});
    const Offset metadata_at    = builder->claim(METADATA::header_size);
    const Offset icc_at         = metadata.ICC_profile.empty() ? NULL_OFFSET :
        builder->append(IccProfileCreateInfo{
            .bytes = reinterpret_cast<const BYTE*>(metadata.ICC_profile.data()),
            .count = metadata.ICC_profile.size()});
    APPEND_ASSOCIATED_IMAGES (ctx, builder, source, metadata);
    const auto   images         = builder.image_entries();
    const Offset images_at      = images.empty() ? NULL_OFFSET
        : builder->append(ImagesCreateInfo{.entries = images});
    std::vector<AttributeSizeEntry> pairs;
    pairs.reserve(metadata.attributes.size());
    for (const auto& [key, value] : metadata.attributes)
        pairs.push_back({.key   = key,
                         .value = std::string(reinterpret_cast<const char*>(value.data()),
                                              value.size())});
    Offset attributes_at        = NULL_OFFSET;
    if (!metadata.attributes.empty()) {
        const Offset sizes_at = builder->append(AttributeSizesCreateInfo{.entries = pairs});
        const Offset bytes_at = builder->append(AttributeBytesCreateInfo{.entries = pairs});
        attributes_at = builder->append(AttributesCreateInfo{
            .FORMAT       = static_cast<Iris::File::constants::MetadataFormats>(metadata.attributes.type),
            .VERSION      = metadata.attributes.version,
            .SIZES_OFFSET = sizes_at,
            .BYTES_OFFSET = bytes_at});
    }
    // TODO: annotations. Neither the Builder nor this encoder writes them yet.
    builder->fill(metadata_at, MetadataCreateInfo{
        .CODEC_MAJOR        = U16_CAST(metadata.codec.major),
        .CODEC_MINOR        = U16_CAST(metadata.codec.minor),
        .CODEC_BUILD        = U16_CAST(metadata.codec.build),
        .ATTRIBUTES_OFFSET  = attributes_at,
        .IMAGES_OFFSET      = images_at,
        .ICC_COLOR_OFFSET   = icc_at,
        .MICRONS_PIXEL      = metadata.micronsPerPixel,
        .MAGNIFICATION      = metadata.magnification,
        // An Iris source's Z-stack spacing passes through with its planes.
        .MICRONS_PLANE      = source.irisSlide ?
            source.irisSlide->get_slide_parser().abstraction().micronsPerPlane : 0.f,
    });
    return {.tileTable = table_at, .metadata = metadata_at};
}
inline void RESET_TRACKER (EncoderTracker &_tracker, const std::string &dst_path, const Extent &extent) {
    _tracker.dst_path   = dst_path;
    _tracker.completed  = 0;
    _tracker.total      = 0;
    _tracker.layers     = EncoderTracker::Layers(extent.layers.size());
    for (auto __li = 0; __li < _tracker.layers.size(); ++__li) {
        auto& __le              = extent.layers[__li];
        auto  n_tiles           = __le.xTiles*__le.yTiles;
        _tracker.layers[__li]   = EncoderTracker::Layer(n_tiles);
        _tracker.total         += n_tiles;
    }
}
/// The focal planes per layer an Iris source passes through: a Z-stacked
/// layer's streams each hold several planes, which the output layer must
/// declare. Single-plane layers stay 0, as this encoder has always written them;
/// derived layers and every other source are single-plane.
inline std::vector<uint16_t> SOURCE_PLANES (const EncoderSource& source, bool derive)
{
    if (derive || !source.irisSlide) return {};
    auto planes = source.irisSlide->get_slide_parser().abstraction().tileTable.planes;
    for (auto& layer_planes : planes) if (layer_planes <= 1) layer_planes = 0;
    return planes;
}
/// Removes the encoder's scratch file at scope exit: an encode that fails
/// leaves nothing behind, and one that succeeds has already moved it away.
struct ScratchFile {
    std::filesystem::path path;
    ~ScratchFile() {
        // Nothing to report: a missing file is the success path.
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
    }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~ TILE DERIVATION ~~~~~~~~~~~~~~~~~~~~~~~~ //
Iris::Extent GENERATE_DERIVED_EXTENT (const EncoderDerivation &_derivation,
                                      const EncoderSource &source);
void ENCODE_DERIVED_TILE (const DerivationInfo& info,
                          AtomicEncoderStatus* _status,
                          uint32_t l, uint32_t y, uint32_t x);

// ~~~~~~~~~~~~~~~~~~~~~~~ END TILE DERIVATION ~~~~~~~~~~~~~~~~~~~~~~ //

Result __INTERNAL__Encoder::dispatch_encoder()
{
    ENCODING_START:
    auto STATUS = ENCODER_INACTIVE;
    if (_status.compare_exchange_strong(STATUS, ENCODER_ACTIVE) == false)
    switch (STATUS) {
    case ENCODER_INACTIVE: goto ENCODING_START;
    case ENCODER_ACTIVE: throw std::runtime_error
            ("[ERROR] The encoder is currently active. An encoder instance must complete before reuse. Exiting");
    case ENCODER_ERROR: throw std::runtime_error
            ("[ERROR] The encoder encountered an error in previous encoding. Please reset.");
    case ENCODER_SHUTDOWN: throw std::runtime_error
            ("[ERROR] Encoder is being shutdown. Cannot start encoding.");
    }

    // Attempt to open the source slide file
    auto source = OPEN_SOURCE (_srcPath);

    // Validate encoding
    switch (_encoding) {
        case TILE_ENCODING_JPEG: break;
        case TILE_ENCODING_AVIF: break;
        case TILE_ENCODING_IRIS: throw std::runtime_error
            ("[ERROR] Encoding using the Iris Codec is not available for community use.");
        default: throw std::runtime_error
            ("[ERROR] Encoder does not have a valid Iris::Encoding format set");
    }
    if (source.format == FORMAT_UNDEFINED)
        source.format = FORMAT_R8G8B8A8;

    // Get the source file's name
    std::filesystem::path source_file_path = _srcPath;
    auto source_name = source_file_path.stem();
    auto source_dir  = source_file_path.parent_path();

    // Format the output file path
    if (_dstPath.length() == 0)
        _dstPath = source_dir.make_preferred().string();
    else _dstPath = std::filesystem::path(_dstPath).make_preferred().string();
    if (std::filesystem::is_directory(_dstPath) == false) throw std::runtime_error
        ("[ERROR] Invalid encoder destination directory path "+_dstPath);
    if (_dstPath.back() != std::filesystem::path::preferred_separator)
        _dstPath += std::filesystem::path::preferred_separator;
    std::filesystem::path dst_file_path = _dstPath + source_name.string() + ".iris";

    // If the output file already exists, inform that it will be overwritten
    if (std::filesystem::exists(dst_file_path))
        std::cout       << "[WARNING] Destination file " << dst_file_path
                        << " already exists. Overwriting...\n";

    // The pyramid of the output slide: derived lower-resolution layers, or the
    // source's own.
    const Iris::File::BuilderTileTableInfo table {
        .encoding   = _encoding,
        .format     = source.format,
        .extent     = _derive ? GENERATE_DERIVED_EXTENT (_derivation, source) : source.extent,
        .planes     = SOURCE_PLANES (source, _derive),
    };

    // We do not write directly to the output file path. The slide is written
    // to a scratch file in the temp directory and moved into place once
    // encoding succeeds, so a failed encode leaves no artifact behind. Naming
    // and moving files is the encoder's job; the Builder writes where it is told.
    std::random_device entropy;
    const auto scratch_path = std::filesystem::temp_directory_path() /
        ("IrisCodecCache_" + std::to_string(uint64_t{entropy()} << 32 | entropy()));
    RESET_TRACKER (_tracker, scratch_path.string(), table.extent);
    auto builder = Builder::create({
        // A tile stream never outgrows its raw RGBA pixels, and the default
        // reservation covers everything else. Sparse: only written pages cost
        // disk, and the file is truncated to its size when sealed.
        .capacity       = Iris::File::BuilderCreateInfo{}.capacity +
                          Size{_tracker.total} * TILE_PIX_BYTES_RGBA,
        .filepath       = scratch_path,
        // Every layer is single-plane, where tile frames are optional; this
        // encoder has never written them.
        .tile_frames    = false,
    });
    builder.set_tile_table(table);

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // BEGIN OUR ASYNCHRONOUS STEPS; This thread will return immediately
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Begin to dispatch threads
    // First create an asynchronous callback pool for derived layer
    // encoding methods. These execute stochastically when tiles are ready.
    //
    // Then we begin dispatching our core encoding threads:
    // __INTERNAL__Encoder::dispatch_encoder() is an ASYNCHRONOUS method
    // there will be a separate main thread that waits upon the encoding
    // threads. This is threads[0]. We will move the remainder of the
    // method to this separate thread...
    switch (source.sourceType) {
        // LibDICOM is not thread safe. Only read from 1 thread...
        case EncoderSource::ENCODER_SRC_DICOM:
            _threads    = Threads(2);
            break;
        default:
            _threads    = Threads(_concurrency+1);
            break;
    }
    _threads[0] = std::thread {[this, builder, source, table, scratch_path, dst_file_path]() mutable {

        // ~~~ We are now on the separate asynchronous main thread ~~~

        // Declared in this order so the Builder's mapping is released before
        // the scratch file is removed: Windows refuses to delete a mapped file.
        const ScratchFile scratch {scratch_path};
        const Builder     writer = std::move(builder);

        // Create the downsample information struct
        // WARNING: THIS MUST PERSIST UNTIL ALL ASYNC THREADS ARE COMPLETE
        const auto queue = _derive?Iris::Async::createThreadPool(_concurrency):NULL;
        const DerivationInfo downsample_info {
            .context    = _context,
            .queue      = queue,
            .strategy   = _derivation,
            .builder    = writer,
            .table      = table,
            .tracker    = _tracker,
        };

        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // DISPATCH THE TILE ENCODING THREADS AND WAIT UPON THEIR COMPLETION
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        for (auto thread_idx = 1; thread_idx < _threads.size(); ++thread_idx)
            if (!_derive) /* Just copy source */ _threads[thread_idx] =
                std::thread {&ENCODE_SOURCE_PYRAMID,
                    _context, source, writer,       // Compressor, source and dst
                    _encoding, &_tracker,           // Tile encoding and tracker
                    &_status                        // Encoder status
                };
            else /* Spool up async tile derivation */ _threads[thread_idx] =
                std::thread {&ENCODE_DERIVE_PYRAMID,
                    _context, source,               // Compressor and source
                    &_tracker, &_status,            // Tile and Encoder statuses

                    // This lambda function starts the propagation of encoding
                    // the slide pyramid by enqueueing downsampling / writing
                    [this,downsample_info](uint32_t l, uint32_t y, uint32_t x){
                        auto& queue = downsample_info.queue;
                        // If the queue has more than 2GB pending...
                        // Slow the reads down to let the sampler keep up.
                        while (queue->pending_tasks() > MAX_MEMORY_PRESSURE/TILE_PIX_BYTES_RGBA)
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        downsample_info.queue->issue_task
                        (std::bind(ENCODE_DERIVED_TILE ,downsample_info,
                                   &_status, l, y, x));
                    }
                };
        for (auto thread_idx = 1; thread_idx < _threads.size(); ++thread_idx)
            if (_threads[thread_idx].joinable()) _threads[thread_idx].join();
        // Await asynchronous thread pool if encoding tasks were delegated
        if (queue) queue->wait_until_complete();
        // It is NOW safe to destroy the downsample_info struct
        // ~~~~~~~~~~~~~~~~~~~~~ END TILE ENCODING ~~~~~~~~~~~~~~~~~~~~~~~~~

        // If any thread has inactivated the encoder, exit; the scratch file
        // goes with this scope.
        if (_status != ENCODER_ACTIVE) { _status.notify_all(); return; }

        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // SLIDE STRUCTURE, SEAL, AND MOVE INTO PLACE
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        try {
            const Metadata metadata = READ_METADATA (source, table.extent, _anonymize);
            writer.finalize(WRITE_SLIDE_STRUCTURE (_context, table, writer, source, metadata));
            // A rename cannot cross volumes (the temp directory is often on
            // another one); copy instead. The scratch file goes at scope exit.
            std::error_code across_volumes;
            std::filesystem::rename(scratch_path, dst_file_path, across_volumes);
            if (across_volumes) std::filesystem::copy_file
                (scratch_path, dst_file_path, std::filesystem::copy_options::overwrite_existing);
        } catch (const std::exception& e) {   // the Builder refuses with logic_error kinds too
            _status.store(ENCODER_ERROR);
            MutexLock __ (_tracker.error_msg_mutex);
            _tracker.error_msg += std::string("Slide encoding failed: ") +
                                  e.what() + "\n";
            _status.notify_all();
            return;
        }

        // Encoding is complete. Notify any waiting threads
        auto STATUS = ENCODER_ACTIVE;
        if (_status.compare_exchange_strong(STATUS, ENCODER_INACTIVE) == false) {
            std::cerr   << "Codec Error -- Encoder exited with status "
                        << TO_STRING(_status) << "\n";
        } _status.notify_all();
    }};

    // We have successfully dispatched the encoding method and may return.
    // All other steps will continue on the _threads[0] thread.
    return IRIS_SUCCESS;
}
Result __INTERNAL__Encoder::interrupt_encoder()
{
    switch (_status) {
        case ENCODER_ACTIVE: {
            _status.store(ENCODER_ERROR);
            MutexLock __ (_tracker.error_msg_mutex);
            _tracker.error_msg += "Encoder manually interrupted\n";
            _status.notify_all();
        } return IRIS_SUCCESS;
            
        case ENCODER_ERROR:
            _status.notify_all();
            return {
                IRIS_FAILURE,
                "Encoder already possesses the ENCODER_ERROR status."
            };
            
        case ENCODER_INACTIVE:
        case ENCODER_SHUTDOWN:
            return IRIS_SUCCESS;
    }   return IRIS_FAILURE;
}
} // END IRIS CODEC NAMESPACE
