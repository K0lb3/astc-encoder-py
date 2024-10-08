# astc-encoder-py

``astc-encoder-py`` is a  Python binding of [astc-encoder](https://github.com/ARM-software/astc-encoder).
It can compress images into astc textures and decompress astc textures into images.

**Requires Python 3.7+**

## docs

TODO, check the [pyi](./astc_encoder/__init__.pyi) for now.

## examples

### loading astc data as PIL.Image

```py
# import following once to register the codec
import astc_encoder.pil_codec

# pass the relevant data to the decoder via the decoder args as below
profile: int = 1
block_width: int = 4
block_height: int = 4
# the target mode should always be RGBA, even for RGB, as the decompression always returns RGBA
Image.frombytes("RGBA", (512, 512), comp, "astc", (profile, block_width, block_height)).show()
```

### compressing and decompressing using astc_encoder
```py
from PIL import Image

from astc_encoder import (
 ASTCConfig,
 ASTCContext,
 ASTCImage,
 ASTCProfile,
 ASTCSwizzle,
 ASTCType,
)

# create config like astcenc_config_init,
# color profile, block dimensions, quality, flags
# and then allow editing by hand afterward
# optional args:
#   block depth = 1, quality = 60, flags = 0
config = ASTCConfig(ASTCProfile.LDR_SRGB, 4, 4)

# create a context from the config
# as the context creation is expensive,
# this solution allows re-using the context
context = ASTCContext(config)

# create a new image for testing
img = Image.new("RGBA", (512, 512), (255, 0, 0, 255))

# put it's data into an ASTCImage for handling 
image = ASTCImage(ASTCType.U8, *img.size, data=img.tobytes())

# create a RGBA swizzle
swizzle = ASTCSwizzle.from_str("RGBA")

# compress the image with the given context
comp = context.compress(image, swizzle)

# create a destination image with the correct arguments
image_dec = ASTCImage(ASTCType.U8, *img.size)

# decompress the data into the image
# the result has always 4 channels
context.decompress(comp, image_dec, swizzle)

# load the decompressed image into PIL
img = Image.frombytes("RGBA", img.size, image_dec.data)
```

## TODO
- [x] figuring out segfault for re-using ASTCImage
- [x] creating ASTCSwizzle from strings instead of from ints
- [ ] creating ASTCImage directly from PIL.Image
- [x] ~~export ASTCImage directly to PIL.Image~~ via PIL.ImageDecoder
- [ ] SVE support for arm
- [x] tests
- [ ] docs page
