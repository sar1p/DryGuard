"""Compile and run the firmware's actual core C++ code without ESP32 hardware."""

from pathlib import Path
import os
import shutil
import subprocess
import sys


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    compiler = os.environ.get("CXX") or shutil.which("g++") or shutil.which("clang++")
    if not compiler:
        print("C++ compiler missing. Install GCC/Clang or run the GitHub Actions tests.", file=sys.stderr)
        return 2
    source = root / "firmware" / "jemuran_otomatis" / "src"
    build = root / "build" / "native"
    build.mkdir(parents=True, exist_ok=True)
    executable = build / ("controller_tests.exe" if os.name == "nt" else "controller_tests")
    command = [
        compiler, "-std=c++11", "-Wall", "-Wextra", "-Werror", "-pedantic", "-O2",
        "-I", str(source), str(root / "tests" / "test_controller.cpp"),
        str(source / "core" / "Controller.cpp"),
        str(source / "core" / "SensorPolicy.cpp"),
        str(source / "core" / "StateCodec.cpp"), "-o", str(executable),
    ]
    subprocess.run(command, cwd=root, check=True)
    return subprocess.run([str(executable)], cwd=root, check=False).returncode


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, subprocess.CalledProcessError) as error:
        print(f"Test build failed: {error}", file=sys.stderr)
        raise SystemExit(1)
