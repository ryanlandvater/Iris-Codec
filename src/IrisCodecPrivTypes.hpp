//
//  IrisCodecPrivTypes.hpp
//  Iris
//
//  Created by Ryan Landvater on 1/10/24.
//

#ifndef IrisCodecPrivTypes_h
#define IrisCodecPrivTypes_h
extern "C" {
typedef struct _openslide       openslide_t;
typedef struct tiff             TIFF;
}
namespace IrisCodec {
using       Tile            = std::shared_ptr<class __INTERNAL__Tile>;

struct CompressTileInfo {
    Buffer          pixelArray          = NULL;
//    Buffer          destinationOptional = NULL;
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
struct CompressImageInfo {
    Buffer          pixelArray          = NULL;
//    Buffer          destinationOptional = NULL;
    uint32_t        width               = 0;
    uint32_t        height              = 0;
    Format          format              = Iris::FORMAT_UNDEFINED;
    ImageEncoding   encoding            = IMAGE_ENCODING_UNDEFINED;
    Quality         quality             = QUALITY_DEFAULT;
    Subsampling     subsampling         = SUBSAMPLE_DEFAULT;
};
struct DecompressImageInfo {
    Buffer          compressed          = NULL;
    Buffer          optionalDestination = NULL;
    uint32_t        width               = 0;
    uint32_t        height              = 0;
    Format          sourceFormat        = Iris::FORMAT_UNDEFINED;
    Format          desiredFormat       = Iris::FORMAT_UNDEFINED;
    ImageEncoding   encoding            = IMAGE_ENCODING_UNDEFINED;
};
// MARK: - ENCODER STRUCTURES
enum __tileStatus {
    TILE_FREE,
    TILE_INITIALIZING,
    TILE_READING,
    TILE_PENDING,
    TILE_ENCODING,
    TILE_COMPLETE,
};
using Subtile                   = uint16_t;
using SubtileTracker            = std::atomic<Subtile>;
#define SUBTILES_COMPLETE       UINT16_MAX
//#define SUBTILES_COMPLETE       UINT16_MAX;
static_assert(std::is_same<Subtile, uint16_t>::value,
"If you change/expand subtile flag, remember to update the \
SUBTILESCMPLT to the max value of the new type");
using DcmFile = std::shared_ptr<struct __INTERNAL__DcmFile>;
struct TileTracker {
    std::atomic<__tileStatus>   status;
    SubtileTracker              subtile;
    Iris::Buffer                pixels  = NULL;
    Iris::Buffer                stream  = NULL;
    TileTracker() :
    status  (TILE_FREE),
    subtile (0){}
};
struct EncoderSource {
    enum {
        ENCODER_SRC_UNDEFINED   = 0,
        ENCODER_SRC_IRISSLIDE,
        ENCODER_SRC_OPENSLIDE,
        ENCODER_SRC_DICOM,
        ENCODER_SRC_APERIO,
    }               sourceType  = ENCODER_SRC_UNDEFINED;
    Format          format      = Iris::FORMAT_UNDEFINED;
    Encoding        encoding    = TILE_ENCODING_UNDEFINED;
    Extent          extent;
    Slide           irisSlide   = NULL;
    DcmFile         dicomFile   = NULL;
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
struct DerivationInfo {
    using Queue                 = Iris::Async::ThreadPool;
    using Strategy              = EncoderDerivation;
    using Tracker               = EncoderTracker;
    using Table                 = Iris::File::BuilderTileTableInfo;
    const Context&              context;
    const Queue&                queue;
    const Strategy&             strategy;
    const Iris::File::Builder&  builder;
    const Table&                table;
    Tracker&                    tracker;
};
} // END IRIS CODEC NAMESPACE
#endif /* IrisCodecPrivTypes_h */
