//
//  IrisCodecFile.hpp
//  Iris
//
//  Created by Ryan Landvater on 1/10/24.
//

#ifndef IrisCodecFile_hpp
#define IrisCodecFile_hpp
namespace IrisCodec {
using namespace Iris;
class __INTERNAL__File {
public:
    const std::string               path;
    FILE*                           handle;
    size_t                          size;
#if _WIN32
    HANDLE                          map = INVALID_HANDLE_VALUE;
#endif
    BYTE*                           ptr;
    SharedMutex                     resize; //TODO: REPLACE THIS WITH FILE LOCK
    bool                            writeAccess;
    
    explicit __INTERNAL__File       (const FileOpenInfo&);
    explicit __INTERNAL__File       (const FileCreateInfo&);
    explicit __INTERNAL__File       (const CacheCreateInfo&);
    __INTERNAL__File                (const __INTERNAL__File&) = delete;
    __INTERNAL__File operator =     (const __INTERNAL__File&) = delete;
   ~__INTERNAL__File                ();
    std::string get_path            ();
    BYTE*       get_ptr             ();
};
}
#endif /* IrisCodecFile_hpp */
