# v1.4.2 release checklist

Complete this checklist on `develop`. Do not tag, merge, or publish until every
required item is verified by a maintainer.

## Source and security

- [ ] Review the complete diff from the intended previous release.
- [ ] Confirm the version and release date in `saprfc.c`.
- [ ] Confirm no credentials, `.env` files, internal hosts, SAP traces, SDK
      files, compiled objects, or generated build files are tracked.
- [ ] Confirm the proprietary SDK and `libstdc++.so.5` are not distributed.
- [ ] Review C changes for resource lifetime and buffer-boundary regressions.
- [ ] Review all web examples; keep `APP_DEBUG=false` and SAP tracing disabled.

## Build and test matrix

- [ ] PHP 8.3.35 NTS / API 20230831: doctor, clean build, 8 PHPT tests.
- [ ] PHP 8.5.10 NTS / API 20250925: doctor, clean build, 8 PHPT tests.
- [ ] Build against the licensed classic RFC SDK 6.40 on the target host.
- [ ] `ldd saprfc.so` resolves `librfccm.so`, `libstdc++.so.5`, and all other
      dependencies from approved locations.
- [ ] `php -n -d extension=/absolute/path/saprfc.so --ri saprfc` succeeds.
- [ ] `scripts/install.sh --dry-run` selects the expected API directory and
      destination for each PHP version.

## Authorized SAP smoke test

- [ ] Use a non-production client and a least-privilege test account.
- [ ] `RFC_SYSTEM_INFO` succeeds and returned identifiers are not captured in
      public logs or release artifacts.
- [ ] `RFC_READ_TABLE` succeeds for the approved test table/query.
- [ ] Sales District Viewer renders escaped data and shows no SDK detail with
      `APP_DEBUG=false`.
- [ ] Connection failures produce a generic HTTP response and a protected log.
- [ ] Confirm SAP trace files were not created, or securely remove them.

## Documentation and publication

- [ ] Recheck README compatibility claims against recorded test output.
- [ ] Review `COMPATIBILITY.md`, `LIMITATIONS.md`, and release notes.
- [ ] Confirm upgrade/behavior changes are acceptable to API consumers.
- [ ] Obtain maintainer approval on `develop`.
- [ ] Merge/tag/push only as a separate, explicitly authorized action.
