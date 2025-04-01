"""
This is the Codec submodule that allows access to Iris Encoded Slide Files
"""
import Iris.Iris
from __future__ import annotations
import numpy
import typing
__all__ = ['Context', 'Encoding', 'Metadata', 'Slide', 'SlideInfo', 'get_codec_version', 'open_slide', 'validate_slide_path']
class Context:
    """
    Iris Codec Context helper class used for access to GPU encoding and decoding methods. Use of a context is most beneficial when encoding or decoding multiple files as it can optimize and orchistrate an encoding queue.
    """
class Encoding:
    """
    Members:
    
      TILE_ENCODING_UNDEFINED
    
      TILE_ENCODING_JPEG
    
      TILE_ENCODING_AVIF
    
      TILE_ENCODING_IRIS
    
      TILE_ENCODING_DEFAULT
    """
    TILE_ENCODING_AVIF: typing.ClassVar[Encoding]  # value = <Encoding.TILE_ENCODING_AVIF: 3>
    TILE_ENCODING_DEFAULT: typing.ClassVar[Encoding]  # value = <Encoding.TILE_ENCODING_JPEG: 2>
    TILE_ENCODING_IRIS: typing.ClassVar[Encoding]  # value = <Encoding.TILE_ENCODING_IRIS: 1>
    TILE_ENCODING_JPEG: typing.ClassVar[Encoding]  # value = <Encoding.TILE_ENCODING_JPEG: 2>
    TILE_ENCODING_UNDEFINED: typing.ClassVar[Encoding]  # value = <Encoding.TILE_ENCODING_UNDEFINED: 0>
    __members__: typing.ClassVar[dict[str, Encoding]]  # value = {'TILE_ENCODING_UNDEFINED': <Encoding.TILE_ENCODING_UNDEFINED: 0>, 'TILE_ENCODING_JPEG': <Encoding.TILE_ENCODING_JPEG: 2>, 'TILE_ENCODING_AVIF': <Encoding.TILE_ENCODING_AVIF: 3>, 'TILE_ENCODING_IRIS': <Encoding.TILE_ENCODING_IRIS: 1>, 'TILE_ENCODING_DEFAULT': <Encoding.TILE_ENCODING_JPEG: 2>}
    def __eq__(self, other: typing.Any) -> bool:
        ...
    def __getstate__(self) -> int:
        ...
    def __hash__(self) -> int:
        ...
    def __index__(self) -> int:
        ...
    def __init__(self, value: int) -> None:
        ...
    def __int__(self) -> int:
        ...
    def __ne__(self, other: typing.Any) -> bool:
        ...
    def __repr__(self) -> str:
        ...
    def __setstate__(self, state: int) -> None:
        ...
    def __str__(self) -> str:
        ...
    @property
    def name(self) -> str:
        ...
    @property
    def value(self) -> int:
        ...
class Metadata:
    """
    Slide metadata containing information about the Iris File Extension slide file
    """
    @property
    def annotation_groups(self) -> set[str]:
        ...
    @property
    def annotations(self) -> set[int]:
        ...
    @property
    def associated_images(self) -> set[str]:
        ...
    @property
    def attributes(self) -> ...:
        ...
    @property
    def codec_version(self) -> Iris.Iris.Version:
        ...
    @property
    def magnification(self) -> float:
        ...
    @property
    def microns_per_pixel(self) -> float:
        ...
class Slide:
    """
    Iris Codec Slide (distinct from Iris Core Slide, loader class) is a light-weight wrapper around a whole slide image file used to access slide information and raw image data
    """
    def get_info(self) -> tuple[Iris.Iris.Result, SlideInfo]:
        ...
    def read_slide_tile(self, layer_index: int = 0, tile_index: int = 0) -> numpy.ndarray[numpy.uint8]:
        ...
    def read_slide_tile(self, layer_index: int = 0, x_index: int = 0, y_index: int = 0) -> numpy.ndarray[numpy.uint8]:
        ...
    def read_slide_tile_channels(self, layer_index: int = 0, tile_index: int = 0) -> numpy.ndarray[numpy.uint8]:
        """
        Returns slide tile pixel data in the form of a [4,256,256] numpy array with each major
        """
class SlideInfo:
    """
    Basic slide information that includes the version of Iris Codec used to encode the slide file, the slide extent, both in lowest resolution pixels and layers comprising number of 256 pixel tiles
    """
    @property
    def encoding(self) -> Encoding:
        ...
    @property
    def extent(self) -> Iris.Iris.Extent:
        ...
def get_codec_version() -> Iris.Iris.Version:
    """
    Get version of the Iris Codec module specifically, which is distinct from the overall Iris version (Iris.Core.get_version())
    """
def open_slide(file_path: str, codec_context: Context = None, write_access: bool = False) -> Slide:
    """
    Method to open a whole slide image file at the given file path with optional Iris Codec Context for efficiency and GPU optimizations as well as a write_access boolean flag request mutable access. Returns a NULL object on failure. For more information about failure, use validate_slide_path and examine the result.message
    """
def validate_slide_path(file_path: str) -> Iris.Iris.Result:
    """
    Method to validate a whole slide image file to assess if the file path can produce a valid Iris Codec Slide. Returns a Result with possible erorr message on failure
    """
