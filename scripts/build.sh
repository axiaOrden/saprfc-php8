#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

PHP_CONFIG="${PHP_CONFIG:-php-config}"
PHPIZE="${PHPIZE:-}"
SAPRFC_SDK_PATH="${SAPRFC_SDK_PATH:-/opt/rfcsdk}"
SAPRFC_COMPAT_LIB="${SAPRFC_COMPAT_LIB:-}"

usage() {
    echo "Usage: $0 [--php-config PATH] [--phpize PATH] [--sdk PATH]"
}

while (($#)); do
    case "$1" in
        --php-config) PHP_CONFIG="$2"; shift 2 ;;
        --phpize) PHPIZE="$2"; shift 2 ;;
        --sdk) SAPRFC_SDK_PATH="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown option: $1" >&2; usage; exit 1 ;;
    esac
done

PHP_CONFIG="$(command -v "$PHP_CONFIG")" || {
    echo "PHP config not found" >&2
    exit 1
}

if [[ -z "$PHPIZE" ]]; then
    case "$(basename "$PHP_CONFIG")" in
        php-config) PHPIZE=phpize ;;
        php-config-*) PHPIZE="phpize-${PHP_CONFIG##*php-config-}" ;;
        php-config*) PHPIZE="phpize${PHP_CONFIG##*php-config}" ;;
    esac
fi

PHPIZE="$(command -v "$PHPIZE")" || {
    echo "Matching phpize not found; specify --phpize" >&2
    exit 1
}

VERSION="$("$PHP_CONFIG" --version)"
PHP_BINARY="$("$PHP_CONFIG" --php-binary)"
PHP_API="$("$PHPIZE" -v | sed -n 's/^PHP Api Version:[[:space:]]*//p' | head -1)"
RUNTIME_VERSION="$($PHP_BINARY -r 'echo PHP_VERSION;' 2>/dev/null || true)"
RUNTIME_API="$($PHP_BINARY -i 2>/dev/null | sed -n 's/^PHP API => \([0-9][0-9]*\)$/\1/p' | head -1)"
INCLUDE_DIR="$("$PHP_CONFIG" --include-dir)"
CONFIG_API="$(sed -n 's/^#define ZEND_MODULE_API_NO[[:space:]]*\([0-9][0-9]*\).*$/\1/p' "$INCLUDE_DIR/Zend/zend_modules.h" 2>/dev/null | head -1)"
PHP_MINOR="$(printf '%s\n' "$VERSION" | cut -d. -f1,2)"

if [[ ! "$PHP_API" =~ ^[0-9]+$ || ! "$RUNTIME_API" =~ ^[0-9]+$ || ! "$CONFIG_API" =~ ^[0-9]+$ ]]; then
    echo "Unable to determine a numeric PHP API from phpize, PHP, and development headers." >&2
    exit 1
fi

if [[ "$VERSION" != "$RUNTIME_VERSION" || "$PHP_API" != "$RUNTIME_API" || "$PHP_API" != "$CONFIG_API" ]]; then
    echo "PHP toolchain mismatch:" >&2
    echo "  php-config: version $VERSION, API $CONFIG_API" >&2
    echo "  PHP binary: version $RUNTIME_VERSION, API $RUNTIME_API ($PHP_BINARY)" >&2
    echo "  phpize:     API $PHP_API ($PHPIZE)" >&2
    exit 1
fi

if [[ ! -f "$SAPRFC_SDK_PATH/include/saprfc.h" ]]; then
    echo "SAP RFC SDK header not found: $SAPRFC_SDK_PATH/include/saprfc.h" >&2
    exit 1
fi

SAPRFC_LIB_DIR=""
for candidate in "$SAPRFC_SDK_PATH/lib" "$SAPRFC_SDK_PATH/lib64"; do
    if [[ -f "$candidate/librfccm.so" || -f "$candidate/librfc.so" || -f "$candidate/librfc.a" ]]; then
        SAPRFC_LIB_DIR="$candidate"
        break
    fi
done
if [[ -z "$SAPRFC_LIB_DIR" ]]; then
    echo "SAP RFC SDK library (librfccm or librfc) not found under $SAPRFC_SDK_PATH/lib or lib64" >&2
    exit 1
fi

echo "========================================"
echo "SAPRFC PHP Build"
echo "========================================"
echo "PHP version : $VERSION"
echo "PHP API     : $PHP_API"
echo "PHP binary  : $PHP_BINARY"
echo "PHP config  : $PHP_CONFIG"
echo "PHPize      : $PHPIZE"
echo "SAP SDK     : $SAPRFC_SDK_PATH"
echo "========================================"

WORKDIR="$(mktemp -d)"
trap 'rm -rf "$WORKDIR"' EXIT

cp "$ROOT/saprfc.c" \
   "$ROOT/php_saprfc.h" \
   "$ROOT/rfccal.c" \
   "$ROOT/rfccal.h" \
   "$ROOT/config.m4" \
   "$WORKDIR/"

cd "$WORKDIR"

"$PHPIZE"

./configure \
    --with-php-config="$PHP_CONFIG" \
    --with-saprfc="$SAPRFC_SDK_PATH"

make -j"$(nproc)"

OUTPUT="$ROOT/dist/php-$PHP_MINOR/api-$PHP_API"
mkdir -p "$OUTPUT"

cp modules/saprfc.so "$OUTPUT/saprfc.so"

echo
echo "Build output: $OUTPUT/saprfc.so"
echo
echo "Checking dependencies:"

RUNTIME_LIBRARY_PATH="$SAPRFC_LIB_DIR${SAPRFC_COMPAT_LIB:+:$SAPRFC_COMPAT_LIB}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

LD_LIBRARY_PATH="$RUNTIME_LIBRARY_PATH" \
    ldd "$OUTPUT/saprfc.so"

echo
echo "Checking extension loading:"

LD_LIBRARY_PATH="$RUNTIME_LIBRARY_PATH" \
    "$PHP_BINARY" -n \
    -d "extension=$OUTPUT/saprfc.so" \
    --ri saprfc

echo
echo "Build completed successfully."
