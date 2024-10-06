__version__ = "0.1.3"

from archspec.cpu import host

from .enum import (
    ASTCConfigFlags,
    ASTCProfile,
    ASTCQualityPreset,
    ASTCSwizzleComponentSelector,
    ASTCType,
)

local_host = host()

try:
    # pypi wheels won't have these for now
    if local_host.family.name == "aarch64":
        # archspec doesn't detect the relevant features for arm
        # so we assume neon is available,
        # as it's unlikely for a device using the lib to not have it
        from ._encoder_neon import *
    elif local_host.family.name == "x86_64":
        if "avx2" in local_host.features:
            from ._encoder_avx2 import *
        elif "sse4_1" in local_host.features:
            from ._encoder_sse41 import *
        elif "sse2" in local_host.features:
            from ._encoder_sse2 import *
except ImportError:
    pass

if "ASTCConfig" not in locals():
    from ._encoder_none import *


__all__ = [
    "ASTCConfig",
    "ASTCContext",
    "ASTCImage",
    "ASTCSwizzle",
    "ASTCConfigFlags",
    "ASTCProfile",
    "ASTCQualityPreset",
    "ASTCSwizzleComponentSelector",
    "ASTCType",
]
