# Windows Unicode path support

Goal: start the client/server from a Korean Windows user profile without changing
the account name or the system code page, and preserve Unicode settings, log and
certificate paths.

Implementation:
- Keep IPC command text in UTF-8; convert to UTF-16 at Windows process creation.
- Store string settings in the Windows registry as UTF-16 so a service restart
  also preserves the command and log paths. Existing ASCII settings remain valid;
  previously corrupted non-ASCII commands are refreshed when the GUI applies them.
- Read the original UTF-16 command line in Windows executables and convert each
  argument to UTF-8 before the existing argument parser runs.
- Explicitly construct filesystem paths from UTF-8 for configuration, logs and
  PEM operations; verify the existing TLS loader with Unicode paths.
- Preserve existing command-line options, profile layout and TLS verification.

Validation:
- Reproduce startup failure with Korean, space and supplementary Unicode paths.
- Cover command-line quoting, PEM creation/fingerprints/loading and log rotation.
- Run the existing tests and a Release build, then rebuild the Windows installer.
- A second laptop installation remains a separate acceptance check.

## Validation results (2026-10-01)

The original Release client and server both reproduced the reported FATAL error
when given `--profile-dir` containing Korean text. The original PEM generation and
log writing tests also threw the same conversion exception. ANSI registry storage
did not preserve a UTF-8 command containing Korean text and an emoji.

After the change:
- All 7 Windows Unicode unit tests pass, including registry persistence, command
  quoting, certificate creation/fingerprints/loading, logging and log rotation.
- `ctest --test-dir build -R windows_unicode_paths --output-on-failure` passes.
  This test is registered when Python 3 is available. It can also be run as
  `python src/test/integtests/windows_unicode_paths.py build/bin`.
- The same executable smoke test passes against `build/input-leap-install`.
- The full Release build and Inno Setup package generation succeed. All four
  staged application executables match the build outputs by SHA-256.
- The broad suites have 53 existing failures: one Windows daemon-option test,
  one Korean input-mode key test, and 51 Qt key/hotkey serialization tests. A
  separate build of the unchanged HEAD reproduces the identical failure set.
  GUI tests require Qt DLLs on PATH and were run with `QT_QPA_PLATFORM=offscreen`.

Installer: `out/InputLeap_3.0.3_windows_qt6_unicode-fix.exe`.
After upgrading, apply the settings from the GUI to refresh any previously
corrupted saved service command. The other laptop and an actual privileged
service launch have not been tested in this environment.
