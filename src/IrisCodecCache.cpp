//
//  IrisCodecCache.cpp
//  Iris
//
//  Created by Ryan Landvater on 4/14/24.
//

#include "IrisCodecPriv.hpp"

namespace IrisCodec {
Cache create_cache (const CacheCreateInfo&) noexcept
{
    // A cache is meant as scratch space for slide data in the Iris layout —
    // tiles streamed from a remote slide, or an encoder's intermediates. It was
    // never implemented: nothing could store or read an entry, and the slide
    // built over the empty scratch file could not be opened, so this always
    // failed. It still does, without creating a file. Its home when it is
    // built is an anonymous Iris::File::Builder arena (no filepath): RAM-backed,
    // sparse, never on disk.
    std::cerr   << "Failed to create a slide cache: slide caches are not implemented\n";
    return nullptr;
}
} // END IRISCODEC
