//
//  IrisCodecPriv.hpp
//  Iris
//
//  Created by Ryan Landvater on 1/9/24.
//

#ifndef IrisCodecPriv_h
#define IrisCodecPriv_h
#ifndef CODEC_MAJOR_VERSION
#define CODEC_MAJOR_VERSION 2025
#endif
#ifndef CODEC_MINOR_VERSION
#define CODEC_MINOR_VERSION 3
#endif
#ifndef CODEC_BUILD_NUMBER
#define CODEC_BUILD_NUMBER  2
#endif
#ifndef FLT16_MIN
#define _Float16            float // use 32-bit if no 16
#endif
#ifndef U16_CAST
#define U16_CAST(X)         static_cast<uint16_t>(X)
#endif
#ifndef U32_CAST
#define U32_CAST(X)         static_cast<uint32_t>(X)
#endif
#ifndef F16_CAST
#define F16_CAST(X)         static_cast<_Float16>(X)
#endif
#ifndef F32_CAST
#define F32_CAST(X)         static_cast<float>(X)
#endif
#ifndef F64_CAST
#define F64_CAST(X)         static_cast<double>(X)
#endif
#ifndef IRIS_INCLUDE_OPENSLIDE
#define IRIS_INCLUDE_OPENSLIDE 1
#endif
#include <iostream>
#include <assert.h>
#include "IrisCore.hpp"
#include "IrisCodecCore.hpp"
#include "IrisBuffer.hpp"
#include "IrisQueue.hpp"
#include "IrisAsync.hpp"
#include "IrisFileExtension.hpp"
// The Iris File Extension moved its public surface out of namespace IrisCodec:
// its entry points and abstraction types are namespace Iris::File now, and the
// core they build on is Iris::. This codec grew up with those names in scope —
// IrisCodecTypes.hpp aliases most of the core, and this names the two the move
// left dangling plus the abstraction namespace. Scoped here, ahead of the codec
// headers that spell them, rather than reopening a namespace everywhere.
namespace IrisCodec {
namespace Abstraction = ::Iris::File::Abstraction;  // Abstraction::File, ::TileTable
namespace Async       = ::Iris::Async;              // Async::ThreadPool
using ::Iris::atomic_uint64;                        // the encoder's running offset
}  // namespace IrisCodec
#include "IrisCodecPrivTypes.hpp"
#include "IrisCodecFile.hpp"
#include "IrisCodecContext.hpp"
#include "IrisCodecSlide.hpp"
#include "IrisCodecCache.hpp"
#include "IrisCodecEncoder.hpp"

#endif /* IrisCodecPriv_h */
