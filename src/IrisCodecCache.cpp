//
//  IrisCodecCache.cpp
//  Iris
//
//  Created by Ryan Landvater on 4/14/24.
//

#include "IrisCodecPriv.hpp"

namespace IrisCodec {
Cache   create_cache (const CacheCreateInfo& info) noexcept
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
        
        auto file = create_cache_file(info);
        if (file == nullptr)
            throw std::runtime_error("no valid file opened.");
        
        // Create the slide object
        Cache   cache = std::make_shared<__INTERNAL__Cache>(context,file);
        if (cache == nullptr) throw std::runtime_error("Failed to create cache object");
        
        // Return the slide
        return cache;
        
    } catch (std::runtime_error &e) {
        std::cerr   << "Failed to create a slide cache"
                    << e.what() << "\n";
        return nullptr;
    }
}
__INTERNAL__Cache::__INTERNAL__Cache (const Context& c, const File& f) :
__INTERNAL__Slide(c,f) {
    
}
} // END IRISCODEC

