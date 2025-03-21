from Iris.Encoder.Encoder import Encoder
from Iris.Encoder.Encoder import EncoderProgress
from Iris.Encoder.Encoder import EncoderStatus
from Iris.Encoder.Encoder import create_encoder
from Iris.Encoder.Encoder import dispatch_encoder
from Iris.Encoder.Encoder import get_encoder_dst_path
from Iris.Encoder.Encoder import get_encoder_progress
from Iris.Encoder.Encoder import get_encoder_src
from Iris.Encoder.Encoder import interrupt_encoder
from Iris.Encoder.Encoder import reset_encoder
from Iris.Encoder.Encoder import set_encoder_dst_path
from Iris.Encoder.Encoder import set_encoder_src
from Iris import Iris
from Iris.Iris import Codec
import __future__
from __future__ import annotations
import os as os
import platform as platform
import sys as sys
import time as time
__all__: list = ['encode_slide_file']
def encode_slide_file(source: str, outdir: str = '', desired_encoding: Iris.Iris.Codec.Encoding = ..., desired_byte_format: Iris.Iris.Format = ..., strip_metadata: bool = False) -> Iris.Iris.Result:
    ...
absolute_import: __future__._Feature  # value = _Feature((2, 5, 0, 'alpha', 1), (3, 0, 0, 'alpha', 0), 262144)
