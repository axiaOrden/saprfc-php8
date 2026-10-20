#!/usr/bin/env bash
set -euo pipefail

PHP_BIN=php
PHP_CONFIG=php-config
DRY_RUN=0

while (($#)); do
    case "$1" in
        --php)
            PHP_BIN="${2:?Missing PHP executable}"
            shift 2 ;;
        --php-config)
            PHP_CONFIG="${2:?Missing php-config executable}"
            shift 2 ;;
        --dry-run)
            DRY_RUN=1
            shift ;;
        -h|--help)
            echo "Usage: $0 [--php php] [--php-config php-config] [--dry-run]"
            exit 0 ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 2 ;;
    esac
done

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

for command in "$PHP_BIN" "$PHP_CONFIG" install; do
    if ! command -v "$command" >/dev/null 2>&1; then
        echo "Missing command: $command" >&2
        exit 1
    fi
done

FULL_VERSION="$("$PHP_BIN" -r 'echo PHP_VERSION;')"
VERSION="$("$PHP_BIN" -r 'echo PHP_MAJOR_VERSION.".".PHP_MINOR_VERSION;')"
RUNTIME_API="$("$PHP_BIN" -i |
    sed -n '/^PHP API => [0-9][0-9]*$/s/^PHP API => //p' | head -n 1)"
CONFIG_VERSION="$("$PHP_CONFIG" --version)"
CONFIG_BINARY="$("$PHP_CONFIG" --php-binary)"
INCLUDE_DIR="$("$PHP_CONFIG" --include-dir)"
CONFIG_API="$(sed -n 's/^#define ZEND_MODULE_API_NO[[:space:]]*\([0-9][0-9]*\).*$/\1/p' "$INCLUDE_DIR/Zend/zend_modules.h" 2>/dev/null | head -n 1)"
EXT_DIR="$("$PHP_CONFIG" --extension-dir)"

if [[ ! "$RUNTIME_API" =~ ^[0-9]+$ || ! "$CONFIG_API" =~ ^[0-9]+$ ]]; then
    echo "Unable to determine a numeric PHP API from PHP and development headers." >&2
    exit 1
fi

if [[ "$FULL_VERSION" != "$CONFIG_VERSION" || "$RUNTIME_API" != "$CONFIG_API" ]]; then
    echo "PHP executable and php-config API mismatch." >&2
    echo "Runtime: $FULL_VERSION / API $RUNTIME_API" >&2
    echo "Development: $CONFIG_VERSION / API $CONFIG_API ($CONFIG_BINARY)" >&2
    exit 1
fi

if [[ "$(readlink -f "$(command -v "$PHP_BIN")")" != "$(readlink -f "$CONFIG_BINARY")" ]]; then
    echo "PHP executable and php-config select different PHP binaries." >&2
    echo "Runtime: $(command -v "$PHP_BIN")" >&2
    echo "php-config: $CONFIG_BINARY" >&2
    exit 1
fi

SOURCE="$ROOT/dist/php-$VERSION/api-$CONFIG_API/saprfc.so"
TARGET="$EXT_DIR/saprfc.so"

if [[ ! -f "$SOURCE" ]]; then
    echo "Compiled extension not found:" >&2
    echo "  $SOURCE" >&2
    echo "Build this PHP version first using scripts/build.sh." >&2
    exit 1
fi

echo "PHP version: $VERSION"
echo "PHP API:     $CONFIG_API"
echo "Source:      $SOURCE"
echo "Destination: $TARGET"

if command -v ldd >/dev/null 2>&1; then
    MISSING="$(ldd "$SOURCE" | grep 'not found' || true)"
    if [[ -n "$MISSING" ]]; then
        echo "Unresolved dependencies:" >&2
        echo "$MISSING" >&2
        exit 1
    fi
fi

if ((DRY_RUN)); then
    echo "Dry run: no files modified."
    exit 0
fi

if [[ -f "$TARGET" ]]; then
    BACKUP="${TARGET}.bak.$(date +%Y%m%d%H%M%S)"
    echo "Backing up existing extension to $BACKUP"
    sudo cp -p "$TARGET" "$BACKUP"
fi

sudo install -m 755 "$SOURCE" "$TARGET"

echo
echo "Extension installed successfully."
echo "Enable it in the appropriate PHP configuration:"
echo "  extension=saprfc.so"
echo
echo "Then verify with:"
echo "  $PHP_BIN --ri saprfc"
