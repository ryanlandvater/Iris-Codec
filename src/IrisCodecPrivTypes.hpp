//
//  IrisCodecPrivTypes.hpp
//  Iris
//
//  Created by Ryan Landvater on 1/10/24.
//

#ifndef IrisCodecPrivTypes_h
#define IrisCodecPrivTypes_h
namespace IrisCodec {
using File = std::shared_ptr<struct __INTERNAL__File>;

/// Create a new file for writing
File    create_file         (const struct FileCreateInfo&);

/// Open a file for read or read-write access.
File    open_file           (const struct FileOpenInfo&);

/// Create a new file-system temporary file for temporary archiving of slide data to disk.
File    create_cache_file   (const struct CacheCreateInfo&);

/// Resize a file
Result  resize_file         (const File&, const struct FileResizeInfo&);

struct FileCreateInfo {
    std::string     filePath;
    size_t          initial_size = 5E6;
};
struct FileOpenInfo {
    std::string     filePath;
    bool            writeAccess = false;
};
struct FileResizeInfo {
    size_t          size;
    bool            pageAlign           = false;
};
struct CompressTileInfo {
    Buffer          pixelArray          = NULL;
    Buffer          destinationOptional = NULL;
    Format          format              = Iris::FORMAT_UNDEFINED;
    Encoding        encoding            = TILE_ENCODING_UNDEFINED;
    Quality         quality             = QUALITY_DEFAULT;
    Subsampling     subsampling         = SUBSAMPLE_DEFAULT;
};
struct DecompressTileInfo {
    Buffer          compressed          = NULL;
    Buffer          optionalDestination = NULL;
    Format          desiredFormat       = Iris::FORMAT_UNDEFINED;
    Encoding        encoding            = TILE_ENCODING_UNDEFINED;
};

// MARK: - FILE ACCESS DATA STRUCTURES
using FileLock = std::shared_ptr<class __INTERNAL__FileLock>;

// MARK: - ENCODER STRUCTURES
enum __tileStatus {
    TILE_PENDING,
    TILE_ENCODING,
    TILE_COMPLETE,
};
struct TileTracker {
    std::atomic<__tileStatus>   status;
    TileTracker() : status(TILE_PENDING) {}
};
extern "C" {
typedef struct tiff         TIFF;
typedef struct _openslide   openslide_t;
}
struct EncoderSource {
    enum {
        ENCODER_SRC_UNDEFINED   = 0,
        ENCODER_SRC_IRISSLIDE,
        ENCODER_SRC_OPENSLIDE,
        ENCODER_SRC_APERIO,
    }               sourceType  = ENCODER_SRC_UNDEFINED;
    Extent          extent;
    Slide           irisSlide   = NULL;
    openslide_t*    openslide   = NULL;
    TIFF*           svs         = NULL;
};
struct EncoderTracker {
    using Layer                 = std::vector<TileTracker>;
    using Layers                = std::vector<Layer>;
    using Counter               = Iris::atomic_uint32;
    std::string     dst_path;
    Layers          layers;
    Counter         completed;
    uint32_t        total;
    Mutex           error_msg_mutex;
    std::string     error_msg;
    EncoderTracker  ():
    completed       (0),
    total           (0){}
};
} // END IRIS CODEC NAMESPACE
#endif /* IrisCodecPrivTypes_h */
