# Headless Jolt dependency provenance

This laboratory uses the **official Jolt Physics implementation**, with no replacement physics solver. Dependencies are local to this laboratory. No system package installation is required.

## Jolt Physics

- Upstream: https://github.com/jrouwe/JoltPhysics
- Release: **v5.6.0**
- Release page: https://github.com/jrouwe/JoltPhysics/releases/tag/v5.6.0
- Commit referenced by that release: **e77f175595e64cb44218cc9d9d56fc365ad0e36a**
- Download: https://github.com/jrouwe/JoltPhysics/archive/refs/tags/v5.6.0.tar.gz
- Downloaded archive: `vendor/downloads/JoltPhysics-v5.6.0.tar.gz`
- Archive SHA-256: `6e069ee0172478cc78182047aac87e5310ba14a67a53348ae14cc37801fd3f8e`
- Unmodified source: `vendor/JoltPhysics-5.6.0/`
- License: MIT, copyright 2021 Jorrit Rouwe. Full license is preserved at `vendor/JoltPhysics-5.6.0/LICENSE`.
- The archive hash was measured on the official HTTPS download on 2026-10-06 and pinned in the setup script. It is a reproducibility pin, not a separately verified upstream signature.

Build choices were checked against this release's `Build/README.md`, `Build/CMakeLists.txt`, and `Jolt/Jolt.cmake`. The archive includes upstream graphical samples and other optional source files, but those targets and all compute backends are disabled and are not built or run.

## Local CMake distribution

- CMake: **3.31.6**, Linux x86_64 binary distribution packaged by scikit-build.
- Kitware lists this PyPI distribution as an alternative binary release: https://cmake.org/download/
- Package metadata: https://pypi.org/pypi/cmake/3.31.6/json
- Packaging project: https://github.com/scikit-build/cmake-python-distributions
- Wheel: `cmake-3.31.6-py3-none-manylinux_2_17_x86_64.manylinux2014_x86_64.whl`
- Exact download: https://files.pythonhosted.org/packages/59/e8/096984b89133681533650b9078c5ed1c5c9b534e869b5487f22d4de1935c/cmake-3.31.6-py3-none-manylinux_2_17_x86_64.manylinux2014_x86_64.whl
- Wheel SHA-256: `1c8b05df0602365da91ee6a3336fe57525b137706c4ab5675498f662ae1dbcec`
- The digest was obtained from PyPI's version-specific metadata and verified against the downloaded wheel before installation.
- Local executable: `tools/cmake-3.31.6/cmake/data/bin/cmake`
- Package metadata snapshot: `tools/downloads/cmake-3.31.6-pypi.json`
- CMake is BSD-3-Clause; the Python packaging distribution also includes Apache-2.0 terms. Full bundled notices are preserved in `tools/cmake-3.31.6/cmake-3.31.6.dist-info/licenses/` and `tools/cmake-3.31.6/cmake/data/doc/cmake-3.31/`.

The wheel was installed with `python3 -m pip install --no-index --no-deps --target tools/cmake-3.31.6 <wheel>`. This uses an already downloaded and verified wheel; it does not change the system installation or resolve any dependencies.

## Reproduction

From this laboratory's root, run:

```sh
JOBS=4 bash scripts/setup_jolt.sh
```

The script pins both archives, validates SHA-256 before use, installs CMake only below `tools/`, configures the official upstream build, builds only `Jolt` and the official `HelloWorld`, and runs `HelloWorld` with a 60-second limit. `scripts/verify_jolt.py` checks every upstream source file against the archive, checks the installed CMake binary against the wheel, and records actual simulation and linkage results. Cached artifacts avoid repeat downloads. It requires Linux x86_64, `/usr/bin/g++`, GNU Make, Python 3 with pip, curl, tar, coreutils, binutils and approximately a few hundred MB of workspace storage. Source and tool directories are not rewritten if already present; preserve them unmodified, or use a fresh laboratory directory to reproduce from the pinned archives.

### Configuration and consumer linkage

- Build: GNU g++ **14.2.0**, Unix Makefiles, **Release**, C++17, static library.
- Compiler accommodation: `-Wno-error=array-bounds`. GCC 14.2 emits two array-bounds diagnostics from upstream `SoftBodySharedSettings::Optimize`; the first build stopped because upstream enables `-Werror`. The specific warning category is kept visible but is not fatal. All other warnings-as-errors remain enabled, and no upstream source was changed. The involved axis helpers return 0–2; equal min/max axes are separated before computing the remaining axis. The original failed build is recorded in `build-initial-werror.log`. This is a build accommodation, not a claim of a complete soft-body correctness audit.
- Precision: single precision, `DOUBLE_PRECISION=OFF`.
- Assertions: ON despite Release; consumers must also define `JPH_ENABLE_ASSERTS`.
- Object-layer width: 16 bits.
- Minimum selected SIMD: SSE2. SSE4.1/4.2, AVX/AVX2/AVX512, LZCNT/TZCNT, F16C and FMA are OFF.
- RTTI, C++ exceptions, LTO, debug symbols, object streams, debug renderer and profiler: OFF.
- DirectX 12, Vulkan, Metal and CPU compute fallback: OFF. Ordinary Jolt CPU rigid-body simulation remains enabled.
- Samples, viewer, performance tests and upstream unit tests: OFF.
- Official HelloWorld: ON.

Include root: `vendor/JoltPhysics-5.6.0`

Static library: `build-deps/jolt-release-sse2/libJolt.a`

Consumers must use matching public definitions:

```text
-DJPH_ENABLE_ASSERTS -DJPH_OBJECT_LAYER_BITS=16 -DNDEBUG
```

Matching compilation/linkage options:

```text
-std=c++17 -O3 -fno-rtti -fno-exceptions -ffp-contract=off -msse2 -mfpmath=sse -pthread
```

Include `Jolt/Jolt.h` before other Jolt headers. Link `libJolt.a` after the consumer's object files and include `-pthread` at link time. Generated canonical options and complete compiler commands are in `build-deps/jolt-release-sse2/CMakeFiles/HelloWorld.dir/flags.make`, `CMakeFiles/HelloWorld.dir/link.txt`, and `compile_commands.json`.

### Verification artifacts

- `build-deps/jolt-release-sse2/configure.log`
- `build-deps/jolt-release-sse2/build.log`
- `build-deps/jolt-release-sse2/helloworld.log`
- `build-deps/jolt-release-sse2/helloworld-ldd.txt`
- `build-deps/dependency-verification.json` (written after compile/link/run verification)

The dependency smoke test establishes that the official Jolt static library compiles, links and executes a simple CPU rigid-body contact simulation. It does not establish correctness of the separate door-support fixture or integration with a graphics engine.
