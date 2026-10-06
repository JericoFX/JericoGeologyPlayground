#!/usr/bin/env bash
# Reproducible Linux x86_64, headless Jolt dependency build. No system install.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
JOLT_VERSION=5.6.0
JOLT_SHA256=6e069ee0172478cc78182047aac87e5310ba14a67a53348ae14cc37801fd3f8e
CMAKE_VERSION=3.31.6
CMAKE_WHEEL=cmake-3.31.6-py3-none-manylinux_2_17_x86_64.manylinux2014_x86_64.whl
CMAKE_SHA256=1c8b05df0602365da91ee6a3336fe57525b137706c4ab5675498f662ae1dbcec
CMAKE_URL="https://files.pythonhosted.org/packages/59/e8/096984b89133681533650b9078c5ed1c5c9b534e869b5487f22d4de1935c/$CMAKE_WHEEL"
JOLT_ARCHIVE="vendor/downloads/JoltPhysics-v$JOLT_VERSION.tar.gz"
CMAKE_ARCHIVE="tools/downloads/$CMAKE_WHEEL"
CMAKE="$ROOT/tools/cmake-$CMAKE_VERSION/cmake/data/bin/cmake"
BUILD="$ROOT/build-deps/jolt-release-sse2"
JOBS="${JOBS:-4}"

[[ "$(uname -s)" == Linux && "$(uname -m)" == x86_64 ]] || {
  echo "This pinned toolchain supports Linux x86_64 only." >&2; exit 1;
}
mkdir -p vendor/downloads tools/downloads "$BUILD"
fetch() {
  local url="$1" destination="$2" expected_hash="$3"
  if [[ ! -f "$destination" ]]; then
    curl --fail --location --max-time 180 --output "$destination.partial" "$url"
    printf '%s  %s\n' "$expected_hash" "$destination.partial" | sha256sum --check -
    mv "$destination.partial" "$destination"
  fi
  printf '%s  %s\n' "$expected_hash" "$destination" | sha256sum --check -
}
fetch "https://github.com/jrouwe/JoltPhysics/archive/refs/tags/v$JOLT_VERSION.tar.gz" "$JOLT_ARCHIVE" "$JOLT_SHA256"
fetch "$CMAKE_URL" "$CMAKE_ARCHIVE" "$CMAKE_SHA256"
if [[ ! -d "vendor/JoltPhysics-$JOLT_VERSION" ]]; then
  tar -xf "$JOLT_ARCHIVE" -C vendor
fi
if [[ ! -x "$CMAKE" ]]; then
  python3 -m pip install --disable-pip-version-check --no-cache-dir --no-index --no-deps \
    --target "tools/cmake-$CMAKE_VERSION" "$CMAKE_ARCHIVE"
fi
"$CMAKE" --version
"$CMAKE" -S "vendor/JoltPhysics-$JOLT_VERSION/Build" -B "$BUILD" -G 'Unix Makefiles' \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
  -DCMAKE_CXX_FLAGS=-Wno-error=array-bounds \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DJPH_BUILD_SHARED_LIBS=OFF -DINTERPROCEDURAL_OPTIMIZATION=OFF \
  -DDOUBLE_PRECISION=OFF -DUSE_ASSERTS=ON -DGENERATE_DEBUG_SYMBOLS=OFF \
  -DCPP_EXCEPTIONS_ENABLED=OFF -DCPP_RTTI_ENABLED=OFF \
  -DFLOATING_POINT_EXCEPTIONS_ENABLED=OFF -DCROSS_PLATFORM_DETERMINISTIC=OFF \
  -DOBJECT_LAYER_BITS=16 -DENABLE_OBJECT_STREAM=OFF -DENABLE_INSTALL=OFF \
  -DDEBUG_RENDERER_IN_DEBUG_AND_RELEASE=OFF -DDEBUG_RENDERER_IN_DISTRIBUTION=OFF \
  -DPROFILER_IN_DEBUG_AND_RELEASE=OFF -DPROFILER_IN_DISTRIBUTION=OFF \
  -DJPH_USE_DX12=OFF -DJPH_USE_VK=OFF -DJPH_USE_MTL=OFF -DJPH_USE_CPU_COMPUTE=OFF \
  -DUSE_SSE4_1=OFF -DUSE_SSE4_2=OFF -DUSE_AVX=OFF -DUSE_AVX2=OFF \
  -DUSE_AVX512=OFF -DUSE_LZCNT=OFF -DUSE_TZCNT=OFF -DUSE_F16C=OFF -DUSE_FMADD=OFF \
  -DTARGET_UNIT_TESTS=OFF -DTARGET_HELLO_WORLD=ON -DTARGET_PERFORMANCE_TEST=OFF \
  -DTARGET_SAMPLES=OFF -DTARGET_VIEWER=OFF \
  2>&1 | tee "$BUILD/configure.log"
"$CMAKE" --build "$BUILD" --target Jolt HelloWorld --parallel "$JOBS" \
  2>&1 | tee "$BUILD/build.log"
python3 "$ROOT/scripts/verify_jolt.py"
echo "Official Jolt HelloWorld exited 0 and the dynamic body went to sleep."
tail -n 6 "$BUILD/helloworld.log"
