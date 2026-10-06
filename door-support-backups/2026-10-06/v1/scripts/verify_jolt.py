#!/usr/bin/env python3
"""Check official dependency bytes, run HelloWorld, and record local evidence."""
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tarfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build-deps/jolt-release-sse2"
SOURCE = ROOT / "vendor/JoltPhysics-5.6.0"
ARCHIVE = ROOT / "vendor/downloads/JoltPhysics-v5.6.0.tar.gz"
WHEEL = ROOT / "tools/downloads/cmake-3.31.6-py3-none-manylinux_2_17_x86_64.manylinux2014_x86_64.whl"
CMAKE = ROOT / "tools/cmake-3.31.6/cmake/data/bin/cmake"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise SystemExit(message)


require(digest(ARCHIVE) == "6e069ee0172478cc78182047aac87e5310ba14a67a53348ae14cc37801fd3f8e", "Jolt archive checksum mismatch")
require(digest(WHEEL) == "1c8b05df0602365da91ee6a3336fe57525b137706c4ab5675498f662ae1dbcec", "CMake wheel checksum mismatch")
verified_files = 0
with tarfile.open(ARCHIVE, "r:gz") as archive:
    for member in archive.getmembers():
        if member.isfile():
            local_file = ROOT / "vendor" / member.name
            require(local_file.is_file(), f"Missing upstream file: {member.name}")
            require(local_file.read_bytes() == archive.extractfile(member).read(), f"Modified upstream file: {member.name}")
            verified_files += 1
with zipfile.ZipFile(WHEEL) as wheel:
    require(CMAKE.read_bytes() == wheel.read("cmake/data/bin/cmake"), "Installed CMake binary differs from pinned wheel")

result = subprocess.run([str(BUILD / "HelloWorld")], capture_output=True, text=True, timeout=60, check=False)
(BUILD / "helloworld.log").write_text(result.stdout + result.stderr)
require(result.returncode == 0, f"Official HelloWorld failed: {result.returncode}")
require("A contact was added" in result.stdout, "No contact event in official HelloWorld")
require("A body went to sleep" in result.stdout, "Dynamic body did not sleep")
steps = re.findall(r"^Step (\d+): Position = \(([^)]+)\), Velocity = \(([^)]+)\)$", result.stdout, re.MULTILINE)
require(len(steps) > 1, "Expected multiple physics simulation steps")

dependencies = subprocess.run(["ldd", str(BUILD / "HelloWorld")], capture_output=True, text=True, check=True).stdout
require(not re.search(r"vulkan|libGL\.|libEGL\.|libX11\.|libSDL|libglfw", dependencies, re.IGNORECASE), "Unexpected graphics dependency")
(BUILD / "helloworld-ldd.txt").write_text(dependencies)
objects = subprocess.run(["ar", "t", str(BUILD / "libJolt.a")], capture_output=True, text=True, check=True).stdout.splitlines()
require(len(objects) > 0, "Jolt static library is empty")
flags = (BUILD / "CMakeFiles/HelloWorld.dir/flags.make").read_text()
for define in ("-DJPH_ENABLE_ASSERTS", "-DJPH_OBJECT_LAYER_BITS=16", "-DNDEBUG"):
    require(define in flags, f"Missing ABI definition: {define}")
cmake_version = subprocess.run([str(CMAKE), "--version"], capture_output=True, text=True, check=True).stdout.splitlines()[0]
compiler = subprocess.run(["/usr/bin/g++", "--version"], capture_output=True, text=True, check=True).stdout.splitlines()[0]
manifest = {
    "verified_at_utc": datetime.now(timezone.utc).isoformat(),
    "jolt_version": "5.6.0",
    "jolt_commit": "e77f175595e64cb44218cc9d9d56fc365ad0e36a",
    "jolt_archive_sha256": digest(ARCHIVE),
    "unmodified_upstream_files_verified": verified_files,
    "cmake_version": cmake_version,
    "cmake_wheel_sha256": digest(WHEEL),
    "installed_cmake_binary_matches_wheel": True,
    "compiler": compiler,
    "build_type": "Release",
    "precision": "single",
    "assertions": True,
    "instruction_set": "SSE2",
    "abi_defines": ["JPH_ENABLE_ASSERTS", "JPH_OBJECT_LAYER_BITS=16", "NDEBUG"],
    "consumer_compile_options": ["-std=c++17", "-O3", "-fno-rtti", "-fno-exceptions", "-ffp-contract=off", "-msse2", "-mfpmath=sse", "-pthread"],
    "include_root": "vendor/JoltPhysics-5.6.0",
    "static_library": "build-deps/jolt-release-sse2/libJolt.a",
    "static_library_sha256": digest(BUILD / "libJolt.a"),
    "static_library_object_count": len(objects),
    "hello_world_exit_code": result.returncode,
    "hello_world_contact_added": True,
    "hello_world_body_slept": True,
    "hello_world_step_count": len(steps),
    "hello_world_final_reported_step": int(steps[-1][0]),
    "hello_world_final_reported_position": [float(v) for v in steps[-1][1].split(",")],
    "hello_world_final_reported_velocity": [float(v) for v in steps[-1][2].split(",")],
    "hello_world_log_sha256": digest(BUILD / "helloworld.log"),
    "no_graphics_shared_dependencies": True,
    "build_accommodation": "-Wno-error=array-bounds; upstream SoftBodySharedSettings diagnostics remain visible. No upstream source edits.",
    "scope": "Official CPU rigid-body dependency smoke only; does not validate separate door fixtures or engine integration."
}
(ROOT / "build-deps/dependency-verification.json").write_text(json.dumps(manifest, indent=2) + "\n")
print(json.dumps(manifest, indent=2))
