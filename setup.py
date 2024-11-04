from __future__ import annotations

import os
from dataclasses import dataclass, field
from typing import List

from archspec.cpu import host
from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext
from wheel.bdist_wheel import bdist_wheel

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

ASTC_ENCODER_HEADERS = [
    # sdist on github runners fails to include these via MANIFEST.in
    "astcenc_diagnostic_trace.h",
    "astcenc_internal_entry.h",
    "astcenc_internal.h",
    "astcenc_mathlib.h",
    "astcenc_vecmathlib_avx2_8.h",
    "astcenc_vecmathlib_common_4.h",
    "astcenc_vecmathlib_neon_4.h",
    "astcenc_vecmathlib_none_4.h",
    "astcenc_vecmathlib_sse_4.h",
    "astcenc_vecmathlib_sve8.h",
    "astcenc_vecmathlib.h",
    "astcenc.h",
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
        msvc_flags=["/arch:SSE2"],
        unix_flags=["-msse2", "-mno-sse4.1"],
    ),
    "sse41": BuildConfig(
        ASTCENC_SSE=41,
        ASTCENC_POPCNT=1,
        msvc_flags=["/arch:SSE4.1"],
        unix_flags=["-msse4.1", "-mpopcnt"],
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
        ],
    ),
}


class CustomBuildExt(build_ext):
    ENV_DEBUG_INFO = "DEBUG_INFO"

    def build_extensions(self):
        extra_compile_args: List[str] = []
        extra_link_args: List[str] = []

        # check for cibuildwheel
        cibuildwheel = os.environ.get("CIBUILDWHEEL", False)

        msvc: bool = False
        if self.compiler.compiler_type == "msvc":
            extra_compile_args = [
                "/std:c++14",
                "/wd4324",
                "/EHsc",
                "/W4",
                "/fp:precise",
            ]
            # not in the astc-encoder CMakeLists.txt
            # even tho it should improve performance
            extra_link_args = ["/LTCG:incremental"]
            # Generate debug info if specified (MSVC)
            if os.environ.get(self.ENV_DEBUG_INFO, False):
                extra_compile_args.append("/Od")
                extra_compile_args.append("/Zi")
                extra_link_args.append("/debug")
            msvc = True
        else:
            extra_compile_args = [
                "-std=c++14",
                "-flto",
                # "-ffp-model=precise", - clang only
                "-ffp-contract=fast",
            ]
            extra_link_args = ["-flto"]

            # Generate debug info if specified (GCC/Clang/Mingw/etc)
            if os.environ.get(self.ENV_DEBUG_INFO, False):
                extra_compile_args.append("-O0")
                extra_compile_args.append("-ggdb")

            if not cibuildwheel:
                # do some native optimizations for the current machine
                # can't be used for generic builds
                if "-arm" in self.plat_name or "-aarch64" in self.plat_name:
                    native_arg = "-mcpu=native"
                else:
                    native_arg = "-march=native"
                extra_compile_args.append(native_arg)

        local_host = host()

        if self.plat_name.endswith(("amd64", "x86_64")):
            if cibuildwheel:
                self.extensions.extend(
                    [
                        ASTCExtension("sse2", configs["sse2"]),
                        ASTCExtension("sse41", configs["sse41"]),
                        ASTCExtension("avx2", configs["avx2"]),
                    ]
                )
            elif "avx2" in local_host.features:
                self.extensions.append(ASTCExtension("avx2", configs["avx2"]))
            elif "sse4_1" in local_host.features:
                self.extensions.append(ASTCExtension("sse41", configs["sse41"]))
            elif "sse2" in local_host.features:
                self.extensions.append(ASTCExtension("sse2", configs["sse2"]))
        elif self.plat_name.endswith(("arm64", "aarch64")):
            # TODO: somehow detect neon, sve128, sve256
            # atm assume neon is always available
            self.extensions.append(ASTCExtension("neon", configs["neon"]))
        elif self.plat_name.endswith("armv7l"):
            # TODO: detect neon
            pass

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
    _needs_stub: bool = False

    def __init__(self, name: str, build_config: BuildConfig):
        module_name = f"_encoder_{name}"
        super().__init__(
            f"astc_encoder.{module_name}",
            sources=[
                "src/pybind.cpp",
                "src/astcenc_error_metrics.cpp",
                *[
                    f"src/astc-encoder/Source/{source}"
                    for source in ASTC_ENCODER_SOURCES
                ],
            ],
            depends=[
                "src/astcenc_error_metrics.hpp",
                *[
                    f"src/astc-encoder/Source/{header}"
                    for header in ASTC_ENCODER_HEADERS
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
            py_limited_api=True,
        )

        if os.name != "nt":
            self.extra_compile_args.append("-pthread")
            self.extra_link_args.append("-pthread")

        self.build_config = build_config


class bdist_wheel_abi3(bdist_wheel):
    def get_tag(self):
        python, abi, plat = super().get_tag()

        if python.startswith("cp"):
            # on CPython, our wheels are abi3 and compatible back to 3.6
            return "cp37", "abi3", plat

        return python, abi, plat


setup(
    name="astc_encoder_py",
    packages=["astc_encoder"],
    package_data={"astc_encoder": ["*.py", "*.pyi", "py.typed"]},
    ext_modules=[ASTCExtension("none", BuildConfig())],
    cmdclass={"build_ext": CustomBuildExt, "bdist_wheel": bdist_wheel_abi3},
)
