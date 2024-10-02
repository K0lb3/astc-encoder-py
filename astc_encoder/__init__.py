__version__ = "0.1.0"

from platform import machine

from archspec.cpu import host

from .enum import (
    ASTCConfigFlags,
    ASTCProfile,
    ASTCQualityPreset,
    ASTCSwizzleComponentSelector,
    ASTCType,
)

local_host = host()
local_machine = machine()
encoder = None

if local_machine == "aarch64":
    # archspec doesn't detect the relevant features for arm
    # so we assume neon is available,
    # as it's unlikely for a device using the lib to not have it
    from . import _encoder_neon as encoder
elif local_host.family.name == "x86_64":
    if "avx2" in local_host.features:
        from . import _encoder_avx2 as encoder
    elif "sse4_1" in local_host.features:
        from . import _encoder_sse41 as encoder
    elif "sse2" in local_host.features:
        from . import _encoder_sse2 as encoder

if encoder is None:
    from . import _encoder_none as encoder

ASTCConfig = encoder.ASTCConfig
ASTCContext = encoder.ASTCContext
ASTCImage = encoder.ASTCImage
ASTCSwizzle = encoder.ASTCSwizzle

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
