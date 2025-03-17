//
//  IrisCodecSlide.hpp
//  Iris
//
//  Created by Ryan Landvater on 1/9/24.
//

#ifndef IrisCodecSlide_hpp
#define IrisCodecSlide_hpp
namespace IrisCodec {
class __INTERNAL__Slide {
    const Context                       _context;
    const File                          _file;
    const Abstraction::File             _abstraction;
public:
    explicit __INTERNAL__Slide          (const Context&, const File&);
    __INTERNAL__Slide                   (const __INTERNAL__Slide&) = delete;
    __INTERNAL__Slide operator =        (const __INTERNAL__Slide&) = delete;
   ~__INTERNAL__Slide                   ();
    
    Version     get_slide_codec_version () const;
    SlideInfo   get_slide_info          () const;
    Buffer      read_slide_tile         (const SlideTileReadInfo&) const;
    
    Result      write_slide_annotation  (const IrisCodec::Annotation&);
private:
    void        refresh_file_structure  ();
};
} // END IRIS CODEC NAMESPACE
#endif /* IrisCodecSlide_hpp */
