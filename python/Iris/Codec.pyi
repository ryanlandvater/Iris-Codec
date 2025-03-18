"""
This is the Codec submodule that allows access to Iris Encoded Slide Files
"""
import Iris.Iris.Core
from __future__ import annotations
import numpy
import typing
__all__ = ['Context', 'Encoding', 'Slide', 'SlideInfo', 'get_codec_version', 'open_slide', 'validate_slide_path']
class Context:
    """
    Iris Codec Context helper class used for access to GPU encoding and decoding methods. Use of a context is most beneficial when encoding or decoding multiple files as it can optimize and orchistrate an encoding queue.
    """
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
class Encoding:
    """
    Members:
    
      ENCODING_UNDEFINED
    
      ENCODING_JPEG
    
      ENCODING_AVIF
    
      ENCODING_IRIS
    
      ENCODING_DEFAULT
    """
    ENCODING_AVIF: typing.ClassVar[Encoding]  # value = <Encoding.ENCODING_AVIF: 3>
    ENCODING_DEFAULT: typing.ClassVar[Encoding]  # value = <Encoding.ENCODING_JPEG: 2>
    ENCODING_IRIS: typing.ClassVar[Encoding]  # value = <Encoding.ENCODING_IRIS: 1>
    ENCODING_JPEG: typing.ClassVar[Encoding]  # value = <Encoding.ENCODING_JPEG: 2>
    ENCODING_UNDEFINED: typing.ClassVar[Encoding]  # value = <Encoding.ENCODING_UNDEFINED: 0>
    __members__: typing.ClassVar[dict[str, Encoding]]  # value = {'ENCODING_UNDEFINED': <Encoding.ENCODING_UNDEFINED: 0>, 'ENCODING_JPEG': <Encoding.ENCODING_JPEG: 2>, 'ENCODING_AVIF': <Encoding.ENCODING_AVIF: 3>, 'ENCODING_IRIS': <Encoding.ENCODING_IRIS: 1>, 'ENCODING_DEFAULT': <Encoding.ENCODING_JPEG: 2>}
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
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
class Slide:
    """
    Iris Codec Slide (distinct from Iris Core Slide, loader class) is a light-weight wrapper around a whole slide image file used to access slide information and raw image data
    """
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def get_info(self) -> tuple[Iris.Iris.Core.Result, SlideInfo]:
        ...
    def read_slide_tile(self, layer_index: int = 0, tile_index: int = 0) -> numpy.ndarray[numpy.uint8]:
        ...
    def read_slide_tile_channels(self, layer_index: int = 0, tile_index: int = 0) -> numpy.ndarray[numpy.uint8]:
        """
        Returns slide tile pixel data in the form of a [4,256,256] numpy array with each major
        """
class SlideInfo:
    """
    Basic slide information that includes the version of Iris Codec used to encode the slide file, the slide extent, both in lowest resolution pixels and layers comprising number of 256 pixel tiles
    """
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    @property
    def extent(self) -> Iris.Iris.Core.Extent:
        ...
    @property
    def version(self) -> Iris.Iris.Core.Version:
        ...
def get_codec_version() -> Iris.Iris.Core.Version:
    """
    Get version of the Iris Codec module specifically, which is distinct from the overall Iris version (Iris.Core.get_version())
    """
def open_slide(file_path: str, codec_context: Context = None, write_access: bool = False) -> Slide:
    """
    Method to open a whole slide image file at the given file path with optional Iris Codec Context for efficiency and GPU optimizations as well as a write_access boolean flag request mutable access. Returns a NULL object on failure. For more information about failure, use validate_slide_path and examine the result.message
    """
def validate_slide_path(file_path: str) -> Iris.Iris.Core.Result:
    """
    Method to validate a whole slide image file to assess if the file path can produce a valid Iris Codec Slide. Returns a Result with possible erorr message on failure
    """
