"""Check the optional credential header and enabled-network build using dummy data."""

from pathlib import Path
import subprocess
import sys


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    secrets = root / "firmware" / "jemuran_otomatis" / "src" / "Secrets.h"
    dummy = (
        '#pragma once\n'
        '// Build validation only; these are not real credentials.\n'
        '#define BLYNK_TEMPLATE_ID "TMPL_BUILD_TEST"\n'
        '#define BLYNK_TEMPLATE_NAME "DryGuard CI"\n'
        '#define BLYNK_AUTH_TOKEN "00000000000000000000000000000000"\n'
        '#define WIFI_SSID "BUILD_TEST_NETWORK"\n'
        '#define WIFI_PASSWORD "BUILD_TEST_PASSWORD"\n'
    ).encode()
    try:
        with secrets.open("xb") as file:
            file.write(dummy)
    except FileExistsError:
        print("Secrets.h already exists; existing credentials were left unchanged.", file=sys.stderr)
        return 2
    try:
        return subprocess.run(
            [sys.executable, "-m", "platformio", "run", "-e", "esp32dev"],
            cwd=root, check=False,
        ).returncode
    finally:
        # Do not remove a file someone edited while the build was running.
        if secrets.exists() and secrets.read_bytes() == dummy:
            secrets.unlink()


if __name__ == "__main__":
    raise SystemExit(main())
