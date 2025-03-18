from .Iris import Core
from .Iris import Codec
__all__ = ['Core','Codec']
version = Core.get_version()
__version__=f'{version.major}.{version.minor}.{version.build}'