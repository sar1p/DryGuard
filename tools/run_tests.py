"""Compile and run firmware core, gateway, and storage C++ without ESP32 hardware."""

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
    source = root / "firmware" / "dryguard" / "src"
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
    result = subprocess.run([str(executable)], cwd=root, check=False)
    if result.returncode:
        return result.returncode

    gateway_source = build / "gateway" / "src"
    copies = (
        "iot/BlynkGateway.cpp", "iot/BlynkGateway.h",
        "core/Types.h", "config/HardwareConfig.h",
    )
    for relative in copies:
        target = gateway_source / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source / relative, target)
    (gateway_source / "Secrets.h").write_text(
        '#pragma once\n#define WIFI_SSID "gateway-host-test"\n'
        '#define WIFI_PASSWORD "gateway-host-test-password"\n'
        '#define BLYNK_AUTH_TOKEN "gateway-host-test-token"\n',
        encoding="utf-8",
    )

    gateway_executable = build / ("gateway_tests.exe" if os.name == "nt" else "gateway_tests")
    gateway_command = [
        compiler, "-std=c++11", "-Wall", "-Wextra", "-Werror", "-pedantic", "-O2",
        "-I", str(source), "-I", str(root / "tests" / "fakes"),
        str(root / "tests" / "test_blynk_gateway.cpp"),
        str(gateway_source / "iot" / "BlynkGateway.cpp"),
        str(source / "core" / "Controller.cpp"), "-o", str(gateway_executable),
    ]
    subprocess.run(gateway_command, cwd=root, check=True)
    gateway_result = subprocess.run([str(gateway_executable)], cwd=root, check=False)
    if gateway_result.returncode:
        return gateway_result.returncode

    storage_executable = build / ("state_store_tests.exe" if os.name == "nt" else "state_store_tests")
    storage_command = [
        compiler, "-std=c++11", "-Wall", "-Wextra", "-Werror", "-pedantic", "-O2",
        "-I", str(source), "-I", str(root / "tests" / "fakes"),
        str(root / "tests" / "test_state_store.cpp"),
        str(source / "storage" / "StateStore.cpp"),
        str(source / "core" / "Controller.cpp"),
        str(source / "core" / "StateCodec.cpp"), "-o", str(storage_executable),
    ]
    subprocess.run(storage_command, cwd=root, check=True)
    return subprocess.run([str(storage_executable)], cwd=root, check=False).returncode


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, subprocess.CalledProcessError) as error:
        print(f"Test build failed: {error}", file=sys.stderr)
        raise SystemExit(1)
