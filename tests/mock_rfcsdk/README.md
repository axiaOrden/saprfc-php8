# Mock classic RFCSDK (test double)

This directory contains a **test-only** implementation of the subset of the
classic SAP RFC SDK API that SAPRFC uses. It is not the SAP RFCSDK, is not
derived from SAP binaries, and cannot connect to SAP.

## Why it exists

The original extension links against the proprietary, end-of-life classic
RFCSDK (`librfccm`), which is not available in this build environment. Without
a library and headers, neither compilation nor any runtime test of the PHP
extension would be possible. The mock provides:

- `include/saprfc.h`, `include/sapitab.h`, `include/rfcsi.h`
- `lib/librfccm.so`
- enough behavior for the PHPT suite to exercise the PHP layer

`config.m4` detects `SAPRFC_MOCK_SDK` in `saprfc.h` and prints a prominent
warning. Never use this directory for a production build.

## Mock behavior

- `RfcOpenEx()` succeeds only when the connection string contains `MOCK=1`.
  Every other connection string fails with a mock error, without network I/O.
- `Z_MOCK_ECHO`: copies import `ECHO_IN` to export `ECHO_OUT`.
- `Z_MOCK_FAIL`: returns `RFC_FAILURE` and sets `RfcLastError()`.
- `Z_MOCK_TABLE`: appends two rows to table `T_DATA`.
- All other function modules succeed and fill elementary exports with `MOCK`.
- `RfcAccept()` / `RfcAcceptExt()` return `RFC_HANDLE_NULL`; the server and
  gateway paths cannot be exercised with this mock.
- Internal tables (`ItCreate`, `ItAppLine`, `ItGetLine`, `ItPutLine`, ...) are
  implemented in memory.
- Attributes, code page, SSO ticket, transaction ID, trace, registration, and
  tRFC installation calls return deterministic mock values.

## Build

```bash
./tests/mock_rfcsdk/build.sh
```

This creates `tests/mock_rfcsdk/lib/librfccm.so` with the system C compiler.
Then configure SAPRFC with `--with-saprfc="$PWD/tests/mock_rfcsdk"`.

PHP's generated `make clean` removes all `*.so` files under the source tree, so
run `build.sh` again after `make clean`.

## Fidelity caveats

The headers are a faithful-enough declaration set for the calls SAPRFC 1.4.1
makes, but they are not a copy of the SAP headers. Struct sizes, error-info
layouts, and some function signatures may differ from a particular RFCSDK
release. The mock proves PHP-level correctness; it does not prove ABI
compatibility with SAP's library.
