# SAPRFC for PHP 8 v1.4.2 — draft release notes

This release ports SAPRFC 1.4.1 to Zend Engine 4 while preserving its classic
procedural API and its dependency on the legacy SAP RFC SDK 6.40.

## Highlights

- Builds on PHP 8 with arginfo and current Zend resource handling.
- Verified on PHP 8.3.35 NTS (API 20230831) and PHP 8.5.10 NTS (API 20250925)
  on CachyOS / Arch Linux.
- Reported SAP ECC validation covers `RFC_SYSTEM_INFO`, `RFC_READ_TABLE`, and
  the Sales District Viewer in `examples/sales-district/`.
- Eight offline PHPT tests cover extension loading, argument errors, resource
  lifetime, raw/character conversion, tables, and mock client/server helpers.
- Build, diagnostic, and installation tooling supports Arch's lack of
  `php-config --phpapi` by reading the numeric API from the selected PHP
  development headers and checking it against the PHP runtime and `phpize`.
- Fixes include safer buffer handling, case-insensitive names, functional
  optional parameters, and invalidation of dependent handles after close.

## Operational requirements

The proprietary, end-of-life classic SAP RFC SDK is not included. Many SDK
6.40 Linux builds also require legacy `libstdc++.so.5`. Operators must source
both through authorized channels, match the host architecture, and configure
the dynamic linker. This project does not use the NetWeaver RFC SDK 7.50.

## Compatibility notes

All 36 extension functions remain exported. PHP 8 argument errors now use
normal `TypeError`, `ArgumentCountError`, or `ValueError` behavior. Several
unsafe or accidentally broken PHP 4/5-era behaviors were corrected; review
`COMPATIBILITY.md` before upgrading.

Only NTS Linux x86_64 is verified. PHP 8.4, ZTS, Windows, PHP 9, full server and
tRFC flows, SSO, and several data-conversion paths remain unverified. See
`LIMITATIONS.md` for the complete boundary.

## Security

Credentials and SDK binaries are not part of the release. The Sales District
Viewer now hides SDK/exception detail by default and logs it server-side; do
not enable `APP_DEBUG` in production. Historical examples are reference code,
not hardened web applications. Use least-privilege SAP accounts, especially
for `RFC_READ_TABLE`.

## Before publishing

This file is a draft. Complete `RELEASE_CHECKLIST.md`, attach the actual build
and test results, and obtain maintainer approval before creating a tag or
release.
