import os
from typing import Tuple

import imagehash
from PIL import Image

import astc_encoder

TEST_DIR = os.path.dirname(os.path.realpath(__file__))
IMG_RGB = Image.open(os.path.join(TEST_DIR, "RGB.png"))
IMG_RGBA = Image.open(os.path.join(TEST_DIR, "RGBA.png"))


def _run_test_config(
    img: Image.Image, swizzle: astc_encoder.ASTCSwizzle, block_size: Tuple[int, int]
):
    raw = img.tobytes()
    astc_image = astc_encoder.ASTCImage(
        astc_encoder.ASTCType.U8, img.width, img.height, data=raw
    )

    config = astc_encoder.ASTCConfig(
        astc_encoder.ASTCProfile.LDR_SRGB, block_size[0], block_size[1]
    )

    context = astc_encoder.ASTCContext(config, threads=0)

    for i in range(2):
        comp = context.compress(astc_image, swizzle)

    astc_image_new = astc_encoder.ASTCImage(
        astc_encoder.ASTCType.U8, img.width, img.height
    )

    decomp_new = context.decompress(comp, astc_image_new, swizzle)
    decomp_re = context.decompress(comp, astc_image, swizzle)
    assert decomp_new.data == decomp_re.data

    img_new = Image.frombytes(
        "RGBA", (img.width, img.height), decomp_new.data, "raw", "RGBA"
    ).convert(img.mode)
    # img_re = Image.frombytes(
    #     "RGBA", (img.width, img.height), decomp_re.data, "raw", "RGBA"
    # ).convert(img.mode)
    assert compare_images(img, img_new)


def compare_images(im1: Image.Image, im2: Image.Image) -> bool:
    # lossy compression, so some leeway is allowed
    return abs(imagehash.average_hash(im1) - imagehash.average_hash(im2)) <= 1


def test_4x4():
    _run_test_config(
        IMG_RGBA,
        astc_encoder.ASTCSwizzle(
            astc_encoder.ASTCSwizzleComponentSelector.R,
            astc_encoder.ASTCSwizzleComponentSelector.G,
            astc_encoder.ASTCSwizzleComponentSelector.B,
            astc_encoder.ASTCSwizzleComponentSelector.ZERO,
        ),
        (4, 4),
    )
    _run_test_config(IMG_RGBA, astc_encoder.ASTCSwizzle(), (4, 4))


def test_invalid_block_sizes():
    for block_size in [(3, 3), (7, 7), (13, 13), (2, 2, 2), (7, 7, 7)]:
        try:
            astc_encoder.ASTCConfig(astc_encoder.ASTCProfile.LDR_SRGB, *block_size)
            raise AssertionError("Expected ASTCError")
        except astc_encoder.ASTCError:
            pass


def test_swizzle():
    # check default values
    swizzle = astc_encoder.ASTCSwizzle()
    assert swizzle.r == astc_encoder.ASTCSwizzleComponentSelector.R
    assert swizzle.g == astc_encoder.ASTCSwizzleComponentSelector.G
    assert swizzle.b == astc_encoder.ASTCSwizzleComponentSelector.B
    assert swizzle.a == astc_encoder.ASTCSwizzleComponentSelector.A
    # check from_string
    swizzle = astc_encoder.ASTCSwizzle.from_str("10ZB")
    assert swizzle.r == astc_encoder.ASTCSwizzleComponentSelector.ONE
    assert swizzle.g == astc_encoder.ASTCSwizzleComponentSelector.ZERO
    assert swizzle.b == astc_encoder.ASTCSwizzleComponentSelector.Z
    assert swizzle.a == astc_encoder.ASTCSwizzleComponentSelector.B
    swizzle = astc_encoder.ASTCSwizzle.from_str("razg")
    assert swizzle.r == astc_encoder.ASTCSwizzleComponentSelector.R
    assert swizzle.g == astc_encoder.ASTCSwizzleComponentSelector.A
    assert swizzle.b == astc_encoder.ASTCSwizzleComponentSelector.Z
    assert swizzle.a == astc_encoder.ASTCSwizzleComponentSelector.G


if __name__ == "__main__":
    for name in dir():
        if name.startswith("test_"):
            globals()[name]()
