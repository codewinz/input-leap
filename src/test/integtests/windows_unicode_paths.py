"""Run against a Windows build: python windows_unicode_paths.py build/bin.

No input sharing is started: help exits during argument parsing, and the server
configuration intentionally fails validation before a screen is opened.
"""

import pathlib
import shutil
import subprocess
import sys
import tempfile


def run(executable, *args):
    result = subprocess.run(
        [str(executable), *map(str, args)], capture_output=True, timeout=15,
        creationflags=subprocess.CREATE_NO_WINDOW,
    )
    return result.returncode, (result.stdout + result.stderr).decode("utf-8", errors="replace")


def main():
    binaries = pathlib.Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="input-leap-unicode-") as temporary:
        root = pathlib.Path(temporary) / "\uc5b4\ub2c8\uc2a4\ud2b8\ube44\uc804 test \U0001f431"
        root.mkdir()
        profile = root / "AppData" / "Local" / "InputLeap"
        profile.mkdir(parents=True)
        for name in ("input-leapc.exe", "input-leaps.exe"):
            executable = root / name
            shutil.copy2(binaries / name, executable)
            code, output = run(executable, "--profile-dir", profile, "--help")
            # The existing applications return kExitArgs (3) for --help.
            assert code == 3 and "Usage:" in output, (name, code, output)
            print(f"PASS: {name} Unicode executable and profile paths")

        config = root / "\uc124\uc815.conf"
        config.write_text("section: invalid-unicode-test-section\nend\n", encoding="utf-8")
        log = root / "\uc2e4\ud589.log"
        code, output = run(
            root / "input-leaps.exe", "--no-daemon", "--no-tray", "--debug", "DEBUG",
            "--profile-dir", profile, "--log", log, "--config", config,
        )
        assert code != 0 and "cannot read configuration" in output, (code, output)
        assert "invalid-unicode-test-section" in output, output
        assert "cannot read configuration" in log.read_text(encoding="utf-8"), output
        print("PASS: Unicode server configuration and log paths")


if __name__ == "__main__":
    main()
