#!/usr/bin/env bash
set -uo pipefail

PHP_BIN=php
PHP_CONFIG=php-config
SDK="${SAPRFC_SDK_PATH:-}"
FAILURES=0
WARNINGS=0

while (($#)); do
    case "$1" in
        --php)
            PHP_BIN="${2:?Missing PHP executable}"
            shift 2 ;;
        --php-config)
            PHP_CONFIG="${2:?Missing php-config executable}"
            shift 2 ;;
        --sdk)
            SDK="${2:?Missing SDK directory}"
            shift 2 ;;
        -h|--help)
            echo "Usage: $0 [--php php] [--php-config php-config] [--sdk /path/to/rfcsdk]"
            exit 0 ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 2 ;;
    esac
done

ok()   { printf '[OK]   %s\n' "$*"; }
warn() { printf '[WARN] %s\n' "$*"; WARNINGS=$((WARNINGS+1)); }
fail() { printf '[FAIL] %s\n' "$*"; FAILURES=$((FAILURES+1)); }

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "SAPRFC PHP 8 - Environment Doctor"
echo "Repository: $ROOT"
echo

if ! command -v "$PHP_BIN" >/dev/null 2>&1; then
    fail "PHP executable not found: $PHP_BIN"
    exit 1
fi

if ! command -v "$PHP_CONFIG" >/dev/null 2>&1; then
    fail "php-config not found: $PHP_CONFIG"
    exit 1
fi

PHP_VERSION="$("$PHP_BIN" -r 'echo PHP_VERSION;' 2>/dev/null)" || PHP_VERSION=""
PHP_API="$("$PHP_BIN" -i | sed -n '/^PHP API => [0-9][0-9]*$/s/^PHP API => //p' | head -n 1 2>/dev/null)" || PHP_API=""
EXT_DIR="$("$PHP_CONFIG" --extension-dir 2>/dev/null)" || EXT_DIR=""
CONFIG_VERSION="$("$PHP_CONFIG" --version 2>/dev/null)" || CONFIG_VERSION=""
CONFIG_BINARY="$("$PHP_CONFIG" --php-binary 2>/dev/null)" || CONFIG_BINARY=""
INCLUDE_DIR="$("$PHP_CONFIG" --include-dir 2>/dev/null)" || INCLUDE_DIR=""
CONFIG_API="$(sed -n 's/^#define ZEND_MODULE_API_NO[[:space:]]*\([0-9][0-9]*\).*$/\1/p' "$INCLUDE_DIR/Zend/zend_modules.h" 2>/dev/null | head -n 1)"

echo "PHP executable: $PHP_BIN"
echo "PHP version:    $PHP_VERSION"
echo "PHP API:        $PHP_API"
echo "Extension dir:  $EXT_DIR"
echo "php-config:     $CONFIG_VERSION / API ${CONFIG_API:-unknown}"
echo

if [[ "$PHP_VERSION" == 8.3.* || "$PHP_VERSION" == 8.5.* ]]; then
    ok "PHP version has been integration-tested"
elif [[ "$PHP_VERSION" == 8.* ]]; then
    warn "PHP $PHP_VERSION has not been integration-tested"
else
    fail "PHP 8 is required"
fi

if [[ "$PHP_API" =~ ^[0-9]+$ && "$CONFIG_API" =~ ^[0-9]+$ && -n "$EXT_DIR" ]]; then
    ok "Numeric PHP API detected without php-config --phpapi"
else
    fail "Could not determine numeric APIs from PHP and the development headers"
fi

if [[ "$PHP_VERSION" == "$CONFIG_VERSION" && "$PHP_API" == "$CONFIG_API" ]]; then
    ok "PHP executable and development tools match"
else
    fail "PHP/php-config mismatch (runtime $PHP_VERSION API ${PHP_API:-unknown}; development $CONFIG_VERSION API ${CONFIG_API:-unknown})"
fi

if [[ -n "$CONFIG_BINARY" && "$(readlink -f "$(command -v "$PHP_BIN")")" != "$(readlink -f "$CONFIG_BINARY" 2>/dev/null || printf '%s' "$CONFIG_BINARY")" ]]; then
    fail "php-config names a different PHP binary: $CONFIG_BINARY"
fi

if "$PHP_BIN" -m 2>/dev/null | grep -iq '^saprfc$'; then
    ok "SAPRFC extension is loaded"
    "$PHP_BIN" --ri saprfc 2>/dev/null | grep -E \
        'SAPRFC support|Version =>|RFC Library =>' || true
else
    fail "SAPRFC extension is not loaded"
fi

if [[ -n "$EXT_DIR" && -f "$EXT_DIR/saprfc.so" ]]; then
    ok "Installed extension exists: $EXT_DIR/saprfc.so"

    if command -v ldd >/dev/null 2>&1; then
        MISSING="$(ldd "$EXT_DIR/saprfc.so" 2>/dev/null |
            grep 'not found' || true)"

        if [[ -n "$MISSING" ]]; then
            fail "Unresolved shared libraries:"
            printf '%s\n' "$MISSING"
        else
            ok "No unresolved shared libraries reported by ldd"
        fi
    fi
else
    warn "No saprfc.so found in the selected extension directory"
fi

if [[ -n "$SDK" ]]; then
    if [[ -f "$SDK/include/saprfc.h" ]]; then
        ok "SAP RFC SDK header found"
    else
        fail "Missing SDK header: $SDK/include/saprfc.h"
    fi

    if [[ -f "$SDK/lib/librfccm.so" || -f "$SDK/lib64/librfccm.so" ||
          -f "$SDK/lib/librfc.so" || -f "$SDK/lib64/librfc.so" ||
          -f "$SDK/lib/librfc.a" || -f "$SDK/lib64/librfc.a" ]]; then
        ok "SAP RFC SDK library found"
    else
        fail "Missing SDK library (librfccm or librfc under lib/lib64)"
    fi
else
    warn "SDK path not supplied; skipping SDK build checks"
fi

for envfile in .env .sap.env; do
    if [[ -f "$ROOT/$envfile" ]]; then
        ok "$envfile exists"
    else
        warn "$envfile is missing"
    fi

    if git -C "$ROOT" check-ignore -q "$envfile"; then
        ok "$envfile is Git-ignored"
    else
        fail "$envfile is NOT Git-ignored"
    fi
done

echo
echo "Results: $FAILURES failure(s), $WARNINGS warning(s)"

if ((FAILURES > 0)); then
    exit 1
fi

exit 0
