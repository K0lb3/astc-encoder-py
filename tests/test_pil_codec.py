import os
from PIL import Image

import imagehash

import astc_encoder
import astc_encoder.pil_codec

TEST_DIR = os.path.dirname(os.path.realpath(__file__))
IMG_RGB = Image.open(os.path.join(TEST_DIR, "RGB.png")).convert("RGB")
IMG_RGBA = Image.open(os.path.join(TEST_DIR, "RGBA.png"))


def compress_image_via_astc(img: Image.Image) -> bytes:
    config = astc_encoder.ASTCConfig(astc_encoder.ASTCProfile.LDR, 4, 4, 1, 100)
    context = astc_encoder.ASTCContext(config)
    astc_image = astc_encoder.ASTCImage(
        astc_encoder.ASTCType.U8,
        img.width,
        img.height,
        data=img.tobytes("raw", "RGBA"),
    )

    if img.mode == "RGB":
        swizzle = astc_encoder.ASTCSwizzle.from_str("rgb1")
    elif img.mode == "RGBA":
        swizzle = astc_encoder.ASTCSwizzle.from_str("rgba")
    else:
        raise ValueError(f"Unsupported mode: {img.mode}")

    return context.compress(astc_image, swizzle)


DATA_RGB = compress_image_via_astc(IMG_RGB)
DATA_RGBA = compress_image_via_astc(IMG_RGBA)


def test_decoder():
    for img, comp_lib in [(IMG_RGB, DATA_RGB), (IMG_RGBA, DATA_RGBA)]:
        img_re = Image.frombuffer(
            "RGBA",
            (img.width, img.height),
            comp_lib,
            "astc",
            (astc_encoder.ASTCProfile.LDR, 4, 4),
        ).convert(img.mode)
        assert _compare_images(img, img_re), "Decompression mismatch"


def test_encoder():
    for img, comp_lib in [(IMG_RGB, DATA_RGB), (IMG_RGBA, DATA_RGBA)]:
        comp_pil = img.tobytes("astc", (astc_encoder.ASTCProfile.LDR, 100, 4, 4))
        assert comp_pil == comp_lib, "Compression mismatch"


def _compare_images(im1: Image.Image, im2: Image.Image) -> bool:
    # lossy compression, so some leeway is allowed
    return abs(imagehash.average_hash(im1) - imagehash.average_hash(im2)) <= 1


if __name__ == "__main__":
    for name in dir():
        if name.startswith("test_"):
            globals()[name]()
    print("All tests passed")
