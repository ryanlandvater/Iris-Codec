//
//  IrisCodecEncoder.cpp
//  Iris
//
//  Created by Ryan Landvater on 8/2/22.
//
#include <cmath>
#include <filesystem>
#include "IrisCodecPriv.hpp"

#if IRIS_INCLUDE_OPENSLIDE
#include <openslide/openslide.h>
#endif

namespace IrisCodec {
#if IRIS_INCLUDE_OPENSLIDE
inline Extent SET_SLIDE_EXTENT_OPENSLIDE (openslide_t* openslide)
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
#endif
inline Buffer READ_SOURCE_TILE (const EncoderSource& src, LayerIndex layer, TileIndex tile)
{
    switch (src.sourceType) {
            
        case EncoderSource::ENCODER_SRC_UNDEFINED: throw std::runtime_error("Cannot read source tile; undefined source");
        case EncoderSource::ENCODER_SRC_IRISSLIDE:
            return IrisCodec::read_slide_tile (SlideTileReadInfo{
                .slide          = src.irisSlide,
                .layerIndex     = layer,
                .tileIndex      = tile,
                .desiredFormat  = src.format
        });
        case EncoderSource::ENCODER_SRC_OPENSLIDE:
            #if IRIS_INCLUDE_OPENSLIDE
            return READ_OPENSLIDE_TILE (src, layer, tile);
            #else
            throw std::runtime_error("Openslide linkage was NOT compiled into this binary. Request a new version of Iris Codec with OpenSlide support if you would like to decode slide scanning vendor slide files only accessable to OpenSlide.");
            #endif
            
        case EncoderSource::ENCODER_SRC_APERIO:
            throw std::runtime_error("APERIO TIFF reads not yet built; Use openslide for the moment");
    } return Buffer();
}
inline EncoderSource OPEN_SOURCE (std::string& path, const Context context = NULL)
{
    if (!std::filesystem::exists(path)) throw std::runtime_error
        ("File system failed to identify source file " + path);
    
    if (is_iris_codec_file(path)) {
        EncoderSource source;
        source.sourceType   = EncoderSource::ENCODER_SRC_IRISSLIDE;
        source.irisSlide    = open_slide(SlideOpenInfo {
            .filePath       = path,
            .context        = context,
            .writeAccess    = false,
        });
        if (!source.irisSlide) throw std::runtime_error
            ("No valid Iris slide returned from IrisCodec::open_slide");
        
        source.extent       = source.irisSlide->get_slide_info().extent;
        source.format       = source.irisSlide->get_slide_info().format;
            
        return source;
    }
    #if IRIS_INCLUDE_OPENSLIDE
    if (openslide_detect_vendor(path.c_str())) {
        EncoderSource source;
        source.sourceType   = EncoderSource::ENCODER_SRC_OPENSLIDE;
        source.openslide    = openslide_open(path.c_str());
        
        if (!source.openslide) throw std::runtime_error
            ("No valid openslide handle returned from openslide_open");

        source.extent       = SET_SLIDE_EXTENT_OPENSLIDE(source.openslide);
        source.format       = FORMAT_B8G8R8A8; // OpenSlide always reads as such
        
        return source;
    }
    throw std::runtime_error("Provided source file path was not recognized by any available decoders.");
    #else
    throw std::runtime_error("Provided source file path was not recognized by any available decoders. You may need an encoder built with OpenSlide enabled.");
    #endif
   
}
inline std::string to_string (EncoderStatus status)
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
_context                                    (__i.context),
_srcPath                                    (__i.srcFilePath),
_dstPath                                    (__i.dstFilePath),
_encoding                                   (__i.desiredEncoding),
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
// MARK: - FILE ENCODING METHODS
inline BYTE* FILE_CHECK_EXPAND (const File& file, size_t required_size)
{
    if (required_size > file->size) {
        auto result     = resize_file(file, {
            .size       = required_size,
            .pageAlign  = false,
        });
        if (result != IRIS_SUCCESS)
            throw std::runtime_error
            ("Failed to resize slide file "+file->path+": " + result.message);
    } return file->ptr;
}
inline static void ENCODE_SLIDE_TILES (const Context ctx,
                                       const EncoderSource& src,
                                       const File file,
                                       EncoderTracker* _tracker,
                                       Abstraction::TileTable* _table,
                                       atomic_uint64* _offset,
                                       AtomicEncoderStatus* _status)
{
    auto& extent    = src.extent;
    auto& tracker   = *_tracker;
    auto& table     = *_table;
    auto& offset    = *_offset;
    auto& status    = *_status;
    
    // Allocate a layer and tile index counter.
    uint32_t __LI           = 0;
    uint32_t __TI           = 0;
    uint32_t __LAYERS       = U32_CAST(extent.layers.size());
    
    try { for (__LI = 0; __LI < __LAYERS; ++__LI) {
        auto& layer_tracker = tracker.layers[__LI];
        auto& layer_table   = table.layers[__LI];
        for (__TI = 0; __TI < layer_tracker.size(); ++__TI) {
            // System check step: Only continue if the encoder is active
            if (status != ENCODER_ACTIVE) return;
            
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            //  CAPTURE TILE STEP
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            auto& tile  = layer_tracker[__TI];
            auto STATUS = TILE_PENDING;
            if (tile.status.compare_exchange_strong(STATUS, TILE_ENCODING)==false)
                continue;
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            //  READ TILE STEP
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            auto pixel_array = READ_SOURCE_TILE(src, __LI, __TI);
            if (!pixel_array) throw std::runtime_error("Failed to read slide image data");
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            //  COMPRESS PIXEL ARRAY STEP
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            CompressTileInfo compress_tile_info {
                .pixelArray = pixel_array,
                .format     = src.format,
                .encoding   = table.encoding
            };
            auto bytes      = ctx->compress_tile(compress_tile_info);
            if (!bytes) throw std::runtime_error("Failed to compress slide image data");
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            //  WRITE TO FILE STEP
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            auto& entry     = layer_table[__TI];
            entry.size      = U32_CAST(bytes->size());
            entry.offset    = offset.fetch_add(entry.size);
            ReadLock shared_write_lock (file->resize);
            if (entry.offset + entry.size > file->size) {
                shared_write_lock.unlock();
                WriteLock resize_lock (file->resize);
                // Expand the file by 500 MB per expansion
                // We will shrink it back down to size at the end.
                auto result = resize_file(file, FileResizeInfo {
                    .size = file->size + (size_t)5E8,
                });
                if (result != IRIS_SUCCESS)
                    throw std::runtime_error("Failed to resize growing tile blocks");
                resize_lock.unlock();
                shared_write_lock.lock();
            }
            auto dst = file->ptr + entry.offset;
            memcpy(dst, bytes->data(), entry.size);
            shared_write_lock.unlock();
            // TODO: Should we file synchronize (POSIX msyc) here? Seems like unecessary overhead
            
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            //  RELEASE TILE STEP
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            tile.status.store(TILE_COMPLETE);
            tracker.completed++;
        }
    }
    } catch (std::runtime_error&e) {
        status.store(ENCODER_ERROR);
        MutexLock __ (tracker.error_msg_mutex);
        tracker.error_msg += std::string("Slide tile encoding failed: ") +
                             e.what() + "\n";
        status.notify_all();
        return;
    }
    
}
inline void VALIDATE_TILE_WRITES (const EncoderTracker& tracker, const Abstraction::TileTable& table)
{
    if (tracker.layers.size() != table.layers.size())
        throw std::runtime_error("Tile encoder tracker and tile ptr map mismatch. File corruption.");
    for (auto layer_idx = 0; layer_idx < tracker.layers.size(); ++layer_idx) {
        auto&& tracker_layer = tracker.layers[layer_idx];
        auto&& table_layer   = table.layers[layer_idx];
        if (tracker_layer.size() != table_layer.size())
            throw std::runtime_error
            ("Tile encoder tracker and tile ptr map mismatch for layer " +
             std::to_string(layer_idx) + ". File corruption.\n" +
             "Place a breakpoint in"+__FILE__+"at line " +std::to_string(__LINE__));
        for (auto tile_idx = 0; tile_idx < tracker_layer.size(); ++tile_idx) {
            if (tracker_layer[tile_idx].status != TILE_COMPLETE)
                throw std::runtime_error
                ("Layer" + std::to_string(layer_idx) + ", tile " + std::to_string(tile_idx) +
                 "was marked as incompletely encoded. Encoding incomplete.\n" +
                 "Place a breakpoint in"+__FILE__+"at line " +std::to_string(__LINE__));
            auto&& tile = table_layer[tile_idx];
            if (tile.offset == NULL_OFFSET || tile.size == 0)
                throw std::runtime_error
                ("Layer" + std::to_string(layer_idx) + ", tile " + std::to_string(tile_idx) +
                 "contained invalid size or mapped file ptr identified in the table. File corruption.\n" +
                 "Place a breakpoint in"+__FILE__+"at line " +std::to_string(__LINE__));
        }
    }
}
inline Offset STORE_TILE_TABLE (const File& file, const Abstraction::TileTable& table, atomic_uint64& offset)
{
    // Perform checks here
    if (table.layers.size() != table.extent.layers.size())
        throw std::runtime_error("Failure in tile ptr encoding; table does not match slide extent.");
    
    uint32_t    n_layers    = U32_CAST(table.layers.size());
    auto        __base      = file->ptr;
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // SLIDE TILE ARRAY SERIALIZATION
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    auto    tiles_size      = Serialization::SIZE_TILE_OFFSETS(table.layers);
    Offset  tiles_offset    = offset.fetch_add(tiles_size);
    __base                  = FILE_CHECK_EXPAND(file, offset); // Always check bounds before writing
    Serialization::STORE_TILE_OFFSETS (__base, tiles_offset, table.layers);
    
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // SLIDE TILE EXTENT SERIALIZATION
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Write Iris::Extent to file
    // This must be written backwards as an array of layer extents
    auto   l_extents_size   = Serialization::SIZE_EXTENTS(table.extent.layers);
    Offset l_extents_offset = offset.fetch_add(l_extents_size);
    __base                  = FILE_CHECK_EXPAND(file, offset);
    Serialization::STORE_EXTENTS (__base, l_extents_offset, table.extent.layers);
    
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // WRITE THE TILE TABLE HEADER
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //
    auto    ttable_size     = Serialization::TILE_TABLE::TABLE_HEADER_SIZE;
    Offset  ttable_offset   = offset.fetch_add(ttable_size);
    __base                  = FILE_CHECK_EXPAND(file, offset);
    Serialization::TileTableCreateInfo tile_table_create_info {
        .tileTableOffset    = ttable_offset,
        .encoding           = table.encoding,
        .format             = table.format,
        .tilesOffset        = tiles_offset,
        .layerExtentsOffset = l_extents_offset,
        .layers             = n_layers,
        .widthPixels        = table.extent.width,
        .heightPixels       = table.extent.height,
    };
    Serialization::STORE_TILE_TABLE (__base, tile_table_create_info);
    
    // Return the Tile Table Offset
    return ttable_offset;
}
inline Offset STORE_METADATA (const File& file, const Metadata& metadata, atomic_uint64& offset)
{
    auto    metadata_size   = Serialization::METADATA::HEADER_SIZE;
    Offset  metadata_offset = offset.fetch_add(metadata_size);
    auto    __base          = FILE_CHECK_EXPAND(file, offset);
    
    // WRITE METADATA HERE
    Serialization::MetadataCreateInfo create_info {
        .metadataOffset     = metadata_offset,
//        .codecVersion   =
        .micronsPerPixel    = metadata.micronsPerPixexl
    };
    Serialization::STORE_METADATA(__base, create_info);
    
    return metadata_offset;
}
inline void STORE_FILE_HEADER (const File& file, const Serialization::HeaderCreateInfo& create_info)
{
    if (file->size < create_info.fileSize) throw std::runtime_error
        ("File failed size check. Attempting to write header for truncated file.");
    Serialization::STORE_FILE_HEADER(file->ptr, create_info);
    resize_file(file, FileResizeInfo {
        .size       = create_info.fileSize,
        .pageAlign  = false
    });
}
Result __INTERNAL__Encoder::dispatch_encoder()
{
    ENCODING_START:
    auto STATUS = ENCODER_INACTIVE;
    if (_status.compare_exchange_strong(STATUS, ENCODER_ACTIVE) == false)
    switch (STATUS) {
    case ENCODER_INACTIVE: goto ENCODING_START;
    case ENCODER_ACTIVE: throw std::runtime_error
            ("The encoder is currently active. An encoder instance must complete before reuse.");
    case ENCODER_ERROR: throw std::runtime_error
            ("The encoder encountered an error in previous encoding. Please reset.");
    case ENCODER_SHUTDOWN: throw std::runtime_error
            ("Encoder is being shutdown. Cannot start encoding");
    }
    
    // Attempt to open the source slide file
    auto source = OPEN_SOURCE (_srcPath);
    
    
    // Validate encoding
    switch (_encoding) {
        case TILE_ENCODING_JPEG: break;
        case TILE_ENCODING_AVIF: break;
        case TILE_ENCODING_IRIS: throw std::runtime_error
            ("IRIS CODEC ENCODING SUPPORT IS NOT AVAILABLE FOR COMMUNITY USE.");
        default: throw std::runtime_error
            ("Encoder does not have a valid Iris::Encoding format set");
    }
    if (source.format == FORMAT_UNDEFINED)
        source.format = FORMAT_R8G8B8A8;
    
    // Create the output file
    std::filesystem::path source_file_path = _srcPath;
    auto source_name = source_file_path.stem();
    auto source_dir  = source_file_path.parent_path();
    std::filesystem::path dst_dir;
    std::filesystem::path dst_file_path;
    if (_dstPath.length() == 0)
        _dstPath = source_dir.string();
    if (std::filesystem::is_directory(_dstPath) == false)
        throw std::runtime_error("Invalid encoder destination directory path "+_dstPath);
    #if _WIN32
    dst_file_path = _dstPath + "\\" + source_name.string() + ".iris";
    #else
    dst_file_path = _dstPath + "/" + source_name.string() + ".iris";
    #endif
    
    if (std::filesystem::exists(dst_file_path)) {
        std::cout       << "Destination file " << dst_file_path
                        << " already exists. Overwriting...\n";
    }
        
    FileCreateInfo file_info {
        .filePath       = dst_file_path.string()
    };
    auto file = create_file (file_info);
    if (file == nullptr)
        throw std::runtime_error("Could not create a destination slide file");
    
    // Reset the tracker
    auto& extent        = source.extent;
    _tracker.dst_path   = dst_file_path.string();
    _tracker.completed  = 0;
    _tracker.total      = 0;
    _tracker.layers     = EncoderTracker::Layers(extent.layers.size());
    for (auto __li = 0; __li < _tracker.layers.size(); ++__li) {
        auto& __le              = extent.layers[__li];
        auto  n_tiles           = __le.xTiles*__le.yTiles;
        _tracker.layers[__li]   = EncoderTracker::Layer(n_tiles);
        _tracker.total         += n_tiles;
    }
    
    // Begin to dispatch threads;
    // __INTERNAL__Encoder::dispatch_encoder() is an ASYNCHRONOUS method
    // there will be a separate main thread that waits upon the encoding
    // threads. This is threads[0]. We will move the remainder of the
    // metho to this separate thread...
    _threads = Threads(std::thread::hardware_concurrency()+1);
    _threads[0] = std::thread {[this, file, source, dst_file_path](){
        
        // ~~~ We are now on the separate assynchronous main thread ~~~
        
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // CREATE TILE TABLE STEP
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // Create the tracker to synchronize threads and monitor progress
        using TileTable     = Abstraction::TileTable;
        
        TileTable tile_table;
        auto& tracker       = _tracker;
        auto extent         = source.extent;
        auto n_layers       = extent.layers.size();
        
        tile_table.encoding = _encoding;
        tile_table.format   = source.format;
        tile_table.layers   = TileTable::Layers(n_layers);
        tile_table.extent   = extent;
        for (auto __li = 0; __li < n_layers; ++__li) {
            auto& __le      = extent.layers[__li];
            auto  n_tiles   = __le.xTiles*__le.yTiles;
            tile_table.layers[__li] = TileTable::Layer(n_tiles);
        }
        
        // Create the file byte offset tracker and reserve space for the footer
        atomic_uint64 offset = Serialization::FILE_HEADER::HEADER_SIZE;
        
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // DISPATCH THE TILE ENCODING THREADS AND WAIT UPON THEIR COMPLETION
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        for (auto thread_idx = 1; thread_idx < _threads.size(); ++thread_idx)
            _threads[thread_idx] = std::thread {
            &ENCODE_SLIDE_TILES,
                _context, source, file,         // Compressor, source and dst
                &tracker, &tile_table, &offset, // File structure trackers
                &_status                        // Encoder status
            };
        for (auto thread_idx = 1; thread_idx < _threads.size(); ++thread_idx)
            if (_threads[thread_idx].joinable()) _threads[thread_idx].join();
        if (_status!= ENCODER_ACTIVE) { _status.notify_all(); return;}
        // ~~~~~~~~~~~~~~~~~~~~~ END TILE ENCODING ~~~~~~~~~~~~~~~~~~~~~~~~~
        
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // READ THE METADATA FROM THE SOURCE FILE
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Metadata metadata;
        
        try {
            WriteLock exclusive_lock (file->resize);
            // Check the tiles to ensure they were properly written to file
            VALIDATE_TILE_WRITES (tracker, tile_table);
            // Write the tile table and return the offset
            Offset tile_table_offset = STORE_TILE_TABLE (file, tile_table, offset);
            // Write the metadata and return the metadata block offset
            Offset metadata_offset  = STORE_METADATA(file, metadata, offset);
            STORE_FILE_HEADER (file, {
                .fileSize           = offset.load(),
                .revision           = 0,
                .tileTableOffset    = tile_table_offset,
                .metadataOffset     = metadata_offset
            });
        } catch (std::runtime_error& e) {
            _status.store(ENCODER_ERROR);
            MutexLock __ (_tracker.error_msg_mutex);
            _tracker.error_msg += std::string("File structure encoding failed: ") +
                                  e.what() + "\n";
            _status.notify_all();
            return;
        }
        
        // Encoding is complete. Notify any waiting threads
        auto STATUS = ENCODER_ACTIVE;
        if (_status.compare_exchange_strong(STATUS, ENCODER_INACTIVE) == false) {
            std::cerr   << "Codec Error -- Encoder exited with status "
                        << to_string(_status) << "\n";
        } _status.notify_all();
    }};
    
    // We have successfully dispatched the encoding method and may return.
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
