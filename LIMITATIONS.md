# Remaining limitations

## Validation boundary (important)

This port was built and tested on:

- CachyOS, Linux x86_64, GCC 16.2.1
- PHP 8.3.35 NTS (`php-legacy`), Zend Engine 4.3.35
- PHP 8.5.10 NTS, Zend Engine 4.5.10

No classic SAP RFCSDK installation and no SAP ECC system were available on the
build host. Therefore:

- **Compilation against the real SAP headers was not performed here.** The
  extension was compiled against the test-only mock headers in
  `tests/mock_rfcsdk`, which reproduce the API surface used by SAPRFC 1.4.1.
- **No real SAP connection, function call, Unicode conversation, server
  registration, or tRFC exchange was tested.**
- The mock RFCSDK is not the SAP SDK. Do not ship it or point a production
  build at it. `config.m4` prints a warning when it detects the mock.

Before production use, build against `/opt/rfcsdk` (or the real SDK path) on the
target host and run a controlled smoke test against a non-production client in
ECC. Compile-time and mock-runtime validation cannot prove interoperability
with a specific `librfccm.so` build.

## Classic RFC SDK limits

- The classic RFCSDK (6.20/6.40) is end-of-life. This extension deliberately
  does **not** use the NetWeaver RFC SDK 7.50 API (`sapnwrfc.h`,
  `libsapnwrfc.so`). Pointing `--with-saprfc` at NW RFC 7.50 will fail because
  `saprfc.h` is absent.
- The classic SDK has both non-Unicode and Unicode variants. Unicode support is
  only as good as the installed `librfccm` and the remote ECC release.
- The port preserves 1.4.1's discovery logic:
  - non-Unicode target: `RFC_GET_STRUCTURE_DEFINITION_P`
  - Unicode target: `RFC_GET_UNICODE_STRUCTURE`
  - Unicode detection uses `RFC_SYSTEM_INFO` `RFCSI_EXPORT-RFCSAPRL` and
    `RFCCHARTYP` code pages 4102/4103.
- In the Unicode path, field lengths are taken from `LENGTH_B1` (the
  single-byte length). Fields with multi-byte or UTF-16-only content may be
  truncated or converted incorrectly. Fixing this requires a deliberately
  designed UTF-16 buffer interface, not just a PHP API change.
- `saprfc_set_code_page()` passes the code page through to
  `RfcSetCodePage()`. Correct behavior depends on the SDK and the SAP system's
  code page configuration.
- The classic SDK is not thread-safe in all configurations. Only the NTS build
  was validated. The ZTS code path is present (`ZEND_MODULE_GLOBALS_ACCESSOR`)
  but untested.

## Untested functional areas

- `saprfc_function_discover()`: needs a live SAP system because it calls
  `RFC_SYSTEM_INFO`, `RFC_GET_FUNCTION_INTERFACE_P`, and the structure
  discovery function module.
- `saprfc_server_accept()`, `saprfc_server_dispatch()`,
  `saprfc_trfc_dispatch()`, and the full tRFC callback path: need a registered
  RFC server program and an SAP gateway. The mock only returns
  `RFC_HANDLE_NULL`/`RFC_RETRY`; the callback plumbing is compiled but not
  exercised end-to-end.
- `saprfc_trfc_install()`: the mock accepts registration, but no real tRFC
  LUW was executed. The literal function name `"%%USER_GLOBAL_SERVER"` is
  unchanged from 1.4.1; verify it against your SDK's documentation before
  deploying a tRFC server.
- `saprfc_get_ticket()`: requires SSO configuration in ECC and the SDK.
- `saprfc_attributes()`: the exact set and width of `RFC_ATTRIBUTES` fields
  varies between SDK releases. Fields not present in your headers will be
  empty; the 1.4.1 field list is preserved.
- BCD (`P`) and FLOAT (`F`) conversions: the original `RfcConvertBcdToChar` /
  `RfcConvertCharToBcd` and `sprintf("%f")` paths are preserved, but the mock
  only provides placeholder BCD conversion. Verify numeric formatting against
  real data, especially decimal separators.
- Windows: `config.m4` and the PHPT suite target Unix. The old `.dsp` files are
  untouched and not a supported build path for PHP 8.

## Behavior that may still surprise

- CHAR (`C`) export values are returned with `strlen()`, so embedded NUL bytes
  truncate unless the field is RAW (`X`) or the function was defined with
  `not_trim`/raw-string handling.
- `saprfc_error()` returns the SDK's last-error buffer. It can be stale or
  unrelated to the most recent PHP-level failure; the warning emitted at the
  failure site is usually more specific.
- `saprfc_function_debug_info()` writes HTML directly to output. It is a
  debugging aid, not a structured API.
- Connection and function resources are request-scoped. Long-running CLI
  processes that call `saprfc_trfc_install()` across requests will have the
  PHP callback globals freed at request shutdown even though the SDK's tRFC
  registration may still exist. Install and dispatch within one request, or
  manage the server lifecycle explicitly.
- `saprfc_table_remove()` now returns `true` (documented) rather than the
  implicit `NULL` of 1.4.1. Code that tested `=== null` must be updated; code
  that tested truthiness now sees success.
- Closing a connection invalidates its function handles. Free function modules
  before closing the connection if you need their metadata afterward, or expect
  `-1`/warnings from calls.
- PHP exceptions raised inside user callbacks during server dispatch are
  converted into an RFC failure/abort; the PHP exception itself is not
  propagated back through the SDK boundary.

## Build/tooling limits

- `config.m4` searches `lib` and `lib64` for `librfccm.so`, `librfc.so`, or
  `librfc.a`. If your SDK uses a different library name or layout, adapt the
  `SAPRFC_LIBDIR` / `SAPRFC_LIBNAME` checks.
- The rpath embedded at configure time is absolute. Moving the SDK after
  building requires rebuilding or setting `LD_LIBRARY_PATH`.
- PHP 8.4 was not separately validated, but the code uses APIs available since
  8.3. PHP 9 is not addressed.
- No memory sanitizer/Valgrind run was possible on the build host (Valgrind is
  not installed). The PHPT suite and manual lifecycle scripts were run with
  `USE_ZEND_ALLOC=0` to catch allocator misuse; that is not a substitute for a
  full sanitizer build against the real SDK.
