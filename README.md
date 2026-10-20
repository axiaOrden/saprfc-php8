# SAPRFC for PHP 8

SAPRFC 1.4.2 is a PHP 8 port of the classic SAPRFC extension. It preserves the
procedural SAPRFC API and links to the legacy SAP RFC SDK 6.40 (`librfccm`), not
the NetWeaver RFC SDK.

> **Legacy and proprietary dependency:** SAP owns and licenses the classic RFC
> SDK. It is not included here and must not be redistributed with this project.
> The SDK is end-of-life; assess its support and security implications before
> production use.

## Verified compatibility

The following environments have been reported as verified for v1.4.2:

| Component | Verified value |
| --- | --- |
| PHP | 8.3.35 NTS, API 20230831 |
| PHP | 8.5.10 NTS, API 20250925 |
| Operating system | CachyOS / Arch Linux, x86_64 |
| SAP | ECC using `RFC_SYSTEM_INFO` and `RFC_READ_TABLE` |
| Example | `examples/sales-district/` |

PHP 8.4 uses the same supported source APIs but was not separately verified.
ZTS, Windows, PHP 9, the NetWeaver RFC SDK, and other operating systems are not
claimed as tested. See [COMPATIBILITY.md](COMPATIBILITY.md) and
[LIMITATIONS.md](LIMITATIONS.md).

## Requirements

- PHP 8 with the matching CLI, `php-config`, `phpize`, and development headers
- C compiler, `make`, and Autoconf
- proprietary classic SAP RFC SDK 6.40 headers and `librfccm.so`
- on current Linux distributions, the legacy C++ runtime `libstdc++.so.5`
  required by many SDK 6.40 builds

The PHP binary, development headers, and `phpize` must report the same numeric
PHP API. On Arch, `php-config --phpapi` is not available; the supplied scripts
read `ZEND_MODULE_API_NO` from the selected development headers instead.

## Configure the SDK and dynamic linker

Install the licensed SDK outside this repository, for example:

```text
/opt/rfcsdk/include/saprfc.h
/opt/rfcsdk/include/sapitab.h
/opt/rfcsdk/include/rfcsi.h
/opt/rfcsdk/lib/librfccm.so
```

Obtain `libstdc++.so.5` from a trusted package or compatibility bundle suitable
for the host architecture. Do not copy random libraries from the internet and
do not commit either dependency. Prefer a system linker configuration:

```text
# /etc/ld.so.conf.d/saprfc.conf
/opt/rfcsdk/lib
/opt/saprfc-compat/lib
```

Run `sudo ldconfig`, then check `ldd /path/to/saprfc.so`. For a temporary test,
set `SAPRFC_COMPAT_LIB=/opt/saprfc-compat/lib`; `scripts/build.sh` uses that
directory while checking and loading the extension. Avoid global
`LD_LIBRARY_PATH` settings in service accounts when a scoped linker
configuration or service setting is possible.

## Build

First diagnose a selected PHP toolchain:

```bash
scripts/doctor.sh --php php --php-config php-config --sdk /opt/rfcsdk
```

On Arch/CachyOS with `php-legacy`:

```bash
scripts/doctor.sh \
  --php php-legacy \
  --php-config php-config-legacy \
  --sdk /opt/rfcsdk

scripts/build.sh \
  --php-config php-config-legacy \
  --phpize phpize-legacy \
  --sdk /opt/rfcsdk
```

The build output is stored under `dist/php-MAJOR.MINOR/api-NUMBER/saprfc.so`.
For the manual `phpize` workflow and mock-SDK tests, see
[BUILDING.md](BUILDING.md).

## Install and enable

Review the target and dependency check before changing the system:

```bash
scripts/install.sh --php php-legacy --php-config php-config-legacy --dry-run
scripts/install.sh --php php-legacy --php-config php-config-legacy
```

Enable the module in the configuration for the same PHP SAPI:

```ini
extension=saprfc.so
```

Restart PHP-FPM or the web server if applicable, then verify:

```bash
php-legacy --ri saprfc
```

## Connection example

Keep credentials in the process environment or a protected, Git-ignored
`.sap.env`; never hard-code or commit them.

```php
<?php
$connection = saprfc_open([
    'ASHOST' => getenv('SAP_ASHOST'),
    'SYSNR'  => getenv('SAP_SYSNR') ?: '00',
    'CLIENT' => getenv('SAP_CLIENT') ?: '100',
    'USER'   => getenv('SAP_USER'),
    'PASSWD' => getenv('SAP_PASS'),
    'LANG'   => getenv('SAP_LANG') ?: 'EN',
]);

if ($connection === false) {
    throw new RuntimeException('SAP connection failed; inspect the protected server log.');
}

$function = saprfc_function_discover($connection, 'RFC_SYSTEM_INFO');
$rc = saprfc_call_and_receive($function);
if ($rc !== SAPRFC_OK) {
    throw new RuntimeException('RFC_SYSTEM_INFO failed.');
}

$result = saprfc_export($function, 'RFCSI_EXPORT');
saprfc_function_free($function);
saprfc_close($connection);
```

The SDK error buffer can contain host, system, user, or network details. Do not
send `saprfc_error()` or PHP warnings directly to an HTTP response.

## Environment files

Copy the examples only for local development:

```bash
cp .env.example .env
cp .sap.env.example .sap.env
chmod 600 .env .sap.env
```

`config/bootstrap.php` is a deliberately small loader. It supports blank lines,
full-line `#` comments, one `KEY=VALUE` assignment per line, and removal of one
matching pair of quotes. It does **not** implement shell syntax, interpolation,
escaping, multiline values, `export`, inline comments, or variable expansion.
Existing process variables win. Values containing `#`, `=`, quotes, backslashes,
or newlines should be supplied by the service environment or tested carefully.
The loader is not a secret manager and does not change file permissions.

## Testing

The repository includes a test-only mock of the SDK. It is not SAP software and
cannot establish an SAP connection:

```bash
./tests/mock_rfcsdk/build.sh
phpize
./configure --with-php-config="$(command -v php-config)" \
  --with-saprfc="$PWD/tests/mock_rfcsdk"
make -j"$(nproc)"
make test TESTS=tests
```

There are eight PHPT tests. Passing them validates the extension's offline PHP
API and mock interactions, not production interoperability. Live tests require
an authorized non-production SAP client and least-privilege account.

## Sales District Viewer

`examples/sales-district/` demonstrates a fixed `RFC_READ_TABLE` request for
`T171T`. Run it only behind normal authentication and network controls. The
example escapes output and hides exception/SDK detail unless `APP_DEBUG=true`.
Keep `APP_DEBUG=false` in production, disable SDK tracing, restrict the SAP user
to the minimum required authorization, and never expose this example directly
to an untrusted network. `RFC_READ_TABLE` can expose business data when the SAP
account is over-privileged.

The historical root-level and `sapclasses/examples/` scripts are retained for
API reference. They are not hardened web applications and should not be
deployed unchanged.

## Troubleshooting

| Symptom | Action |
| --- | --- |
| PHP toolchain mismatch | Select a matching `php`, `php-config`, and `phpize`; run `scripts/doctor.sh`. |
| `saprfc.h` missing | Point `--sdk`/`--with-saprfc` to the classic SDK, not NW RFC 7.50. |
| `librfccm.so` not found | Configure the linker path and run `ldconfig`, or use a scoped `LD_LIBRARY_PATH`. |
| `libstdc++.so.5` not found | Install the architecture-matching legacy compatibility library from a trusted source. |
| undefined SDK symbol | Ensure build-time and runtime `librfccm.so` are the same SDK build. |
| module API error | Rebuild for the exact numeric PHP API used by the target SAPI. |
| CLI works but web does not | Check the web SAPI's INI scan directory, service environment, linker path, and restart status. |

Run `ldd dist/php-*/api-*/saprfc.so` and `php --ini` when investigating loader
or configuration problems. More detail is in [BUILDING.md](BUILDING.md).

## Release material

- [Release checklist](RELEASE_CHECKLIST.md)
- [Draft v1.4.2 release notes](RELEASE_NOTES_1.4.2.md)
- [License](LICENSE)
