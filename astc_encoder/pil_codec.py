import struct

from PIL import Image, ImageFile

from astc_encoder import (
    ASTCConfig,
    ASTCConfigFlags,
    ASTCContext,
    ASTCImage,
    ASTCProfile,
    ASTCSwizzle,
    ASTCType,
)


class ASTCEncoder(ImageFile.PyEncoder):
    _pushes_fd: bool = True
    context: ASTCContext

    def init(
        self,
        profile: int,
        quality: float,
        block_width: int,
        block_height: int,
        block_depth: int = 1,
    ):
        assert block_depth == 1, "Cannot handle 3D textures"
        profile = ASTCProfile(profile)
        config = ASTCConfig(
            ASTCProfile.LDR_SRGB,
            block_width,
            block_height,
            block_depth,
            quality=quality,
        )
        self.context = ASTCContext(config)

    def encode(self, bufsize: int) -> tuple[int, int, bytes]:
        assert self.im is not None, "No image set"

        if self.mode == "RGBA":
            swizzle = ASTCSwizzle.from_str("rgba")
        elif self.mode == "RGB":
            swizzle = ASTCSwizzle.from_str("rgb1")
        else:
            raise ValueError(f"Unsupported mode: {self.mode}")

        rgba_struct = struct.Struct("4B")
        data = b"".join(
            rgba_struct.pack(*self.im.getpixel((x, y)))
            for y in range(self.state.ysize)
            for x in range(self.state.xsize)
        )
        astc_img = ASTCImage(
            ASTCType.U8,
            self.state.xsize,
            self.state.ysize,
            data=data,
        )
        comp = self.context.compress(astc_img, swizzle)
        return len(comp), 0, comp


Image.register_encoder("astc", ASTCEncoder)


class ASTCDecoder(ImageFile.PyDecoder):
    _pull_fd: bool = True
    context: ASTCContext

    def init(self, args):
        assert len(args) in (3, 4), "Invalid number of arguments"
        profile: int = ASTCProfile(args[0])
        block_width: int = args[1]
        block_height: int = args[2]
        block_depth: int = args[3] if len(args) > 3 else 1
        assert block_depth == 1, "Cannot handle 3D textures"

        config = ASTCConfig(
            profile,
            block_width,
            block_height,
            block_depth,
            flags=ASTCConfigFlags.DECOMPRESS_ONLY,
        )
        self.context = ASTCContext(config)

    def decode(self, buffer: bytes | Image.SupportsArrayInterface) -> tuple[int, int]:
        assert self.state.xoff == 0 and self.state.yoff == 0, "Cannot handle offsets"

        config = self.context.config
        assert (
            self.state.xsize % config.block_x == 0
        ), "Invalid width, must be multiple of block width"
        assert (
            self.state.ysize % config.block_y == 0
        ), "Invalid height, must be multiple of block height"

        block_count_x = (self.state.xsize + config.block_x - 1) // config.block_x
        block_count_y = (self.state.ysize + config.block_y - 1) // config.block_y
        expected_size = block_count_x * block_count_y * 16

        if len(buffer) != expected_size:
            buffer = self.fd.read(expected_size)

        if len(buffer) != expected_size:
            raise ValueError("Not enough data")

        astc_img = ASTCImage(
            ASTCType.U8,
            self.state.xsize,
            self.state.ysize,
        )
        if self.mode == "RGBA":
            swizzle = ASTCSwizzle.from_str("rgba")
        elif self.mode == "RGB":
            swizzle = ASTCSwizzle.from_str("rgb1")
        else:
            raise ValueError(f"Unsupported mode: {self.mode}")

        self.context.decompress(buffer, astc_img, swizzle)

        self.set_as_raw(astc_img.data, "RGBA")
        return -1, 0


Image.register_decoder("astc", ASTCDecoder)
