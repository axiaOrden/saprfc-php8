# Building SAPRFC 1.4.2-php8

This tree is the PHP 8 port of SAPRFC 1.4.1. It targets **PHP 8.3 NTS** and is
also built and tested against **PHP 8.5 NTS**. It links against the **classic
SAP RFC SDK** (`librfccm`), not the NetWeaver RFC SDK 7.50.

## Requirements

- Linux x86_64 (the SDK and PHP must use the same word size)
- PHP 8.3 NTS with development headers
- `phpize`, `php-config`, a C compiler, `make`, `autoconf`
- The classic SAP RFCSDK 6.20/6.40 with:
  - `include/saprfc.h`
  - `include/sapitab.h`
  - `include/rfcsi.h`
  - `lib/librfccm.so` (or `lib64/librfccm.so`, or `librfc.so`/`librfc.a`)

On Arch/CachyOS the PHP 8.3 toolchain is packaged as `php-legacy`:

```bash
command -v php-legacy phpize-legacy php-config-legacy
php-legacy -v
```

Confirm that PHP is NTS:

```bash
php-legacy -i | grep -i "Thread Safety"
# Thread Safety => disabled
```

## Build against the real classic RFCSDK

```bash
cd saprfc-1.4.1
phpize-legacy
./configure \
  --with-php-config="$(command -v php-config-legacy)" \
  --with-saprfc=/opt/rfcsdk
make -j"$(nproc)"
```

`--with-saprfc` may be omitted if the SDK is in one of the default locations
(`/usr/sap/rfcsdk`, `/usr/local/rfcsdk`, `/opt/rfcsdk`). If the SDK headers are
elsewhere:

```bash
./configure --with-php-config="$(command -v php-config-legacy)" \
            --with-saprfc=/path/to/rfcsdk
```

`config.m4` adds an rpath for the selected SDK library directory, so the built
`modules/saprfc.so` normally finds `librfccm.so` without `LD_LIBRARY_PATH`.
If your loader still cannot find it:

```bash
export LD_LIBRARY_PATH=/opt/rfcsdk/lib:$LD_LIBRARY_PATH
```

## Verify that the extension loads

```bash
php-legacy -v
php-legacy -d extension="$PWD/modules/saprfc.so" -m | grep -i saprfc
php-legacy -d extension="$PWD/modules/saprfc.so" \
  -r 'var_dump(function_exists("saprfc_open"));'
```

Expected: `saprfc` in the module list and `bool(true)`.

A fuller smoke test:

```bash
php-legacy -d extension="$PWD/modules/saprfc.so" -r '
  $rfc = @saprfc_open(["ASHOST"=>"127.0.0.1","SYSNR"=>"00",
                       "CLIENT"=>"100","USER"=>"x","PASSWD"=>"x"]);
  var_dump($rfc === false ? saprfc_error() : "connected");
'
```

This only verifies that a connection attempt reaches the SDK; it does not prove
that your ECC system accepts the credentials.

## Build and run the automated tests without SAP

Because the classic RFCSDK is EOL and licensed, this repository includes a
**test-only** mock SDK under `tests/mock_rfcsdk`. It implements a small subset of
the classic API, never opens a network connection, and is rejected for
production use by a configure warning.

```bash
./tests/mock_rfcsdk/build.sh
phpize-legacy
./configure \
  --with-php-config="$(command -v php-config-legacy)" \
  --with-saprfc="$PWD/tests/mock_rfcsdk"
make -j"$(nproc)"
make test TESTS="tests"
```

Note: PHP's generated `make clean` target removes every `*.so` under the source
 tree, including the mock library. Re-run `./tests/mock_rfcsdk/build.sh` after
 `make clean` before rebuilding/testing.

Expected: 8 tests pass. The tests cover:

- extension load, all 36 public functions, and constants
- unusable connection parameters and PHP 8 argument errors
- offline `saprfc_function_define` / interface inspection
- import/export and RAW/CHAR conversion
- table init/append/insert/modify/remove/read/rows
- mock connection attributes, code page, SSO ticket, tRFC TID
- server registration / tRFC install helpers
- connection close invalidating dependent function handles

The mock's `Z_MOCK_ECHO`, `Z_MOCK_FAIL`, and `Z_MOCK_TABLE` functions exist only
in the mock. They are not SAP function modules.

## Build for PHP 8.5

The same sources build with PHP 8.5 using its own toolchain:

```bash
phpize
./configure --with-php-config="$(command -v php-config)" \
            --with-saprfc=/opt/rfcsdk
make -j"$(nproc)"
make test TESTS="tests"
```

PHP 8.5.10 NTS was validated in a clean copy of this tree; all 8 tests passed.

## Installing

`make install` copies `saprfc.so` into the PHP extension directory. Add:

```ini
extension=saprfc.so
```

to the CLI/web SAPRFC SAPI configuration (for Arch's PHP 8.3 package this is
usually `/etc/php-legacy/conf.d/saprfc.ini`). Restart the web SAPI after
installing.

## Troubleshooting

| Symptom | Cause / fix |
| --- | --- |
| `Cannot find the classic SAP RFCSDK (include/saprfc.h)` | Wrong `--with-saprfc` path; the NetWeaver SDK does not contain `saprfc.h`. |
| `Cannot find librfccm.so` | SDK library is in `lib64` or the wrong architecture; install the 64-bit SDK. |
| `undefined symbol: Rfc...` at load time | Extension was built against one SDK and loaded with another; rebuild against the target SDK. |
| `saprfc.so: cannot open shared object file` | Add the SDK library directory to `LD_LIBRARY_PATH` or fix the rpath. |
| Configure warns about `SAPRFC_MOCK_SDK` | You selected the test double; it cannot connect to SAP. Use the real SDK path. |
