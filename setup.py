from __future__ import annotations
import os
from dataclasses import dataclass, field
from typing import List

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext

ASTC_ENCODER_SOURCES = [
    # cmake_core.cmake - static
    "astcenc_averages_and_directions.cpp",
    "astcenc_block_sizes.cpp",
    "astcenc_color_quantize.cpp",
    "astcenc_color_unquantize.cpp",
    "astcenc_compress_symbolic.cpp",
    "astcenc_compute_variance.cpp",
    "astcenc_decompress_symbolic.cpp",
    "astcenc_diagnostic_trace.cpp",
    "astcenc_entry.cpp",
    "astcenc_find_best_partitioning.cpp",
    "astcenc_ideal_endpoints_and_weights.cpp",
    "astcenc_image.cpp",
    "astcenc_integer_sequence.cpp",
    "astcenc_mathlib.cpp",
    "astcenc_mathlib_softfloat.cpp",
    "astcenc_partition_tables.cpp",
    "astcenc_percentile_tables.cpp",
    "astcenc_pick_best_endpoint_format.cpp",
    "astcenc_quantization.cpp",
    "astcenc_symbolic_physical.cpp",
    "astcenc_weight_align.cpp",
    "astcenc_weight_quant_xfer_tables.cpp",
]


NON_WIN_LINKER_OPTIONS = ["-pthread"]


@dataclass
class BuildConfig:
    ASTCENC_NEON: int = 0
    ASTCENC_SVE: int = 0
    ASTCENC_SSE: int = 0
    ASTCENC_AVX: int = 0
    ASTCENC_POPCNT: int = 0
    ASTCENC_F16C: int = 0
    compile_flags: List[str] = field(default_factory=list)
    msvc_flags: List[str] = field(default_factory=list)
    unix_flags: List[str] = field(default_factory=list)


configs = {
    "neon": BuildConfig(ASTCENC_NEON=1),
    "sve128": BuildConfig(
        ASTCENC_NEON=1,
        ASTCENC_SVE=4,
        compile_flags=["-march=armv8-a+sve", "-msve-vector-bits=128"],
    ),
    "sve256": BuildConfig(
        ASTCENC_NEON=1,
        ASTCENC_SVE=8,
        compile_flags=["-march=armv8-a+sve", "-msve-vector-bits=256"],
    ),
    "sse2": BuildConfig(
        ASTCENC_SSE=20,
        ASTCENC_POPCNT=1,
        msvc_flags=["/arch:SSE2"],
        unix_flags=["-msse2", "-mno-sse4.1", "-Wno-unused-command-line-argument"],
    ),
    "sse41": BuildConfig(
        ASTCENC_SSE=41,
        msvc_flags=["/arch:SSE4.1"],
        unix_flags=["--msse4.1", "-mpopcnt", "-Wno-unused-command-line-argument"],
    ),
    "avx2": BuildConfig(
        ASTCENC_SSE=41,
        ASTCENC_AVX=2,
        ASTCENC_POPCNT=1,
        ASTCENC_F16C=1,
        msvc_flags=["/arch:AVX2"],
        unix_flags=[
            "-mavx2",
            "-mpopcnt",
            "-mf16c",
            "-Wno-unused-command-line-argument",
        ],
    ),
}


with open("README.md", "r") as fh:
    long_description = fh.read()


class CustomBuildExt(build_ext):
    def build_extensions(self):
        extra_compile_args: List[str] = []
        extra_link_args: List[str] = []

        msvc: bool = False
        if self.compiler.compiler_type == "msvc":
            extra_compile_args = [
                "/std:c++14",
                "/wd4324",
                "/EHsc",
                "/W4",
                "/fp:precise",
            ]
            msvc = True
        else:
            extra_compile_args = [
                "-std=c++14",
                "-flto",
                "-ffp-model=precise",
                "-ffp-contract=fast",
            ]
            extra_link_args = ["-flto"]

        for ext in self.extensions:
            ext: ASTCExtension
            ext.extra_compile_args.extend(extra_compile_args)
            ext.extra_link_args.extend(extra_link_args)

            build_config = ext.build_config
            ext.extra_compile_args.extend(build_config.compile_flags)
            if msvc:
                ext.extra_compile_args.extend(build_config.msvc_flags)
            else:
                ext.extra_compile_args.extend(build_config.unix_flags)

        super().build_extensions()


class ASTCExtension(Extension):
    build_config: BuildConfig

    def __init__(self, name: str, build_config: BuildConfig):
        module_name = f"_encoder_{name}"
        super().__init__(
            f"astc_encoder.{module_name}",
            sources=[
                "src/pybind.cpp",
                *[
                    f"src/astc-encoder/Source/{source}"
                    for source in ASTC_ENCODER_SOURCES
                ],
            ],
            include_dirs=["src/astc-encoder/Source"],
            language="c++",
            extra_compile_args=build_config.compile_flags,
            define_macros=[
                ("Py_LIMITED_API", "0x03070000"),
                ("MODULE_NAME", '"module_name"'),
                ("INIT_FUNC_NAME", f"PyInit_{module_name}"),
                # arm
                ("ASTCENC_NEON", str(build_config.ASTCENC_NEON)),
                ("ASTCENC_SVE", str(build_config.ASTCENC_SVE)),
                # x86
                ("ASTCENC_SSE", str(build_config.ASTCENC_SSE)),
                ("ASTCENC_AVX", str(build_config.ASTCENC_AVX)),
                ("ASTCENC_POPCNT", str(build_config.ASTCENC_POPCNT)),
                ("ASTCENC_F16C", str(build_config.ASTCENC_F16C)),
            ],
        )

        if os.name != "nt":
            self.extra_compile_args.append("-pthread")
            self.extra_link_args.append("-pthread")

        self.build_config = build_config


def get_extensions():
    from platform import machine

    from archspec.cpu import host

    local_host = host()
    local_machine = machine()

    extensions = []

    cibuildwheel = os.environ.get("CIBUILDWHEEL", None)

    if local_machine == "aarch64":
        # archspec doesn't detect the relevant features for arm
        # so we assume neon is available,
        # as it's unlikely for a device using the lib to not have it
        extensions.append(ASTCExtension("neon", configs["neon"]))

    elif local_host.family.name == "x86_64":
        if cibuildwheel:
            extensions.append(ASTCExtension("avx2", configs["avx2"]))
            extensions.append(ASTCExtension("sse41", configs["sse41"]))
            extensions.append(ASTCExtension("sse2", configs["sse2"]))
        elif "avx2" in local_host.features:
            extensions.append(ASTCExtension("avx2", configs["avx2"]))
        elif "sse4_1" in local_host.features:
            extensions.append(ASTCExtension("sse41", configs["sse41"]))
        elif "sse2" in local_host.features:
            extensions.append(ASTCExtension("sse2", configs["sse2"]))

    if not extensions or cibuildwheel:
        extensions.append(ASTCExtension("none", BuildConfig()))

    return extensions


setup(
    name="astc_encoder_py",
    description="python wrapper for astc-encoder",
    author="K0lb3",
    version="0.1.0",
    packages=["astc_encoder"],
    package_data={
        "astc_encoder": ["__init__.py", "__init__.pyi", "py.typed", "enum.py"]
    },
    keywords=["astc", "texture", "python-c"],
    classifiers=[
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
        "Intended Audience :: Developers",
        "Programming Language :: Python",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Programming Language :: Python :: 3.13",
        "Topic :: Multimedia :: Graphics",
    ],
    url="https://github.com/K0lb3/astc-encoder-py",
    download_url="https://github.com/K0lb3/astc-encoder-py/tarball/master",
    long_description=long_description,
    long_description_content_type="text/markdown",
    ext_modules=get_extensions(),
    cmdclass={"build_ext": CustomBuildExt},
)
