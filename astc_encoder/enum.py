from enum import IntEnum, IntFlag


class ASTCProfile(IntEnum):
    """The ASTC profile.

    Parameters
    ----------
    LDR_SRGB : int = 0
        The LDR sRGB color profile.
    LDR : int = 1
        The LDR linear color profile.
    HDR_RGB_LDR_A : int = 2
        The HDR RGB with LDR alpha color profile.
    HDR : int = 3
        The HDR RGB color profile.
    """

    LDR_SRGB = 0
    LDR = 1
    HDR_RGB_LDR_A = 2
    HDR = 3


class ASTCQualityPreset(IntEnum):
    """The ASTC quality preset.

    Parameters
    ----------
    FASTEST : int = 0
        The fastest, lowest quality, search preset.
    FAST : int = 10
        The fast search preset.
    MEDIUM : int = 60
        The medium quality search preset.
    THOROUGH : int = 98
        The thorough quality search preset.
    VERYTHOROUGH : int = 99
        The very thorough quality search preset.
    EXHAUSTIVE : int = 100
        The exhaustive, highest quality, search preset.
    """

    FASTEST = 0
    FAST = 10
    MEDIUM = 60
    THOROUGH = 98
    VERYTHOROUGH = 99
    EXHAUSTIVE = 100


class ASTCConfigFlags(IntFlag):
    MAP_NORMAL = 1 << 0
    USE_DECODE_UNORM8 = 1 << 1
    USE_APLHA_WEIGHT = 1 << 2
    USE_PERCEPTUAL = 1 << 3
    DECOMPRESS_ONLY = 1 << 4
    SELF_DECOMPRESS_ONLY = 1 << 5
    MAP_RGBM = 1 << 6


class ASTCType(IntEnum):
    """A texel component data format.

    Parameters
    ----------
    U8 : int = 0
        Unorm 8-bit data per component.
    F16 : int = 1
        16-bit float per component.
    F32 : int = 2
        32-bit float per component.
    """

    U8 = 0
    F16 = 1
    F32 = 2


class ASTCSwizzleComponentSelector(IntEnum):
    """A codec component swizzle selector.

    Parameters
    ----------
    R : int = 0
        Select the red component.
    G : int = 1
        Select the green component.
    B : int = 2
        Select the blue component.
    A : int = 3
        Select the alpha component.
    ZERO : int = 4
        Use a constant zero component.
    ONE : int = 5
        Use a constant one component.
    Z : int = 6
        Use a reconstructed normal vector Z component.
    """

    R = 0
    G = 1
    B = 2
    A = 3
    ZERO = 4
    ONE = 5
    Z = 6


__all__ = (
    "ASTCProfile",
    "ASTCQualityPreset",
    "ASTCConfigFlags",
    "ASTCType",
    "ASTCSwizzleComponentSelector",
)
