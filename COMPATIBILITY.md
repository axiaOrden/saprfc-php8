# PHP 8 porting notes and compatibility

Original: SAPRFC 1.4.1 (2005), PHP 4/5 Zend API, classic SAP RFCSDK 6.20/6.40.
Ported: SAPRFC 1.4.2-php8, Zend Engine 4.x (PHP 8.3 / 8.4 / 8.5), same SDK.

## Public PHP API is preserved

All 36 functions exported by 1.4.1 are still exported under the same names,
including the `saprfc_trfc_dispatch` alias:

```
saprfc_open                    saprfc_table_modify
saprfc_function_discover       saprfc_table_remove
saprfc_function_define         saprfc_table_read
saprfc_function_interface      saprfc_table_rows
saprfc_function_debug_info     saprfc_call_and_receive
saprfc_optional                saprfc_error
saprfc_import                  saprfc_exception
saprfc_export                  saprfc_function_free
saprfc_table_init              saprfc_close
saprfc_table_append            saprfc_set_code_page
saprfc_table_insert            saprfc_attributes
saprfc_server_accept           saprfc_trfc_install
saprfc_server_import           saprfc_trfc_dispatch
saprfc_server_export           saprfc_trfc_call
saprfc_server_dispatch         saprfc_trfc_tid
saprfc_set_trace               saprfc_server_register_check
saprfc_server_register_cancel  saprfc_function_name
saprfc_allow_start_program     saprfc_get_ticket
```

Connections and function modules are still PHP **resources**, so existing
`is_resource()` checks and resource-passing code keep working.

## Zend API mapping

| PHP 4/5 API (removed or changed) | PHP 8.3 replacement |
| --- | --- |
| `function_entry` / `PHP_FE(name, NULL)` | `zend_function_entry`, arginfo for every function, `PHP_FE_END` |
| `zend_get_parameters_ex`, `ZEND_NUM_ARGS`, `WRONG_PARAM_COUNT` | `ZEND_PARSE_PARAMETERS_START` with `Z_PARAM_*`; failures throw `TypeError` / `ArgumentCountError` / `ValueError` |
| `zend_rsrc_list_entry` destructor | `zend_resource` destructor |
| `zend_list_find`, `Z_RESVAL_P` | `Z_RES_P`, resource type check, `zend_register_resource` |
| `zend_list_delete(int)` | `zend_list_close(zend_resource*)` |
| `MAKE_STD_ZVAL`, `FREE_ZVAL`, `zval_dtor` | stack `zval`, `ZVAL_*`, `zval_ptr_dtor` |
| `call_user_function_ex(..., TSRMLS_CC)` | `call_user_function(...)` |
| `zend_hash_find(ht, key, len, ...)` | `zend_hash_str_find(ht, key, len)` |
| `zend_hash_index_find`, `zend_hash_num_elements` | same names, now returning `zval *` / `uint32_t` |
| `zend_hash_get_current_key` pointer loops | `ZEND_HASH_FOREACH_STR_KEY_VAL` etc. |
| `zend_hash_next_index_insert`, `zend_hash_update` | `add_next_index_zval`, `add_assoc_zval` |
| `add_assoc_string(z, k, v, 1)` | `add_assoc_string(z, k, v)` (always copies) |
| `RETURN_STRING(s, 1)` / `RETVAL_STRINGL(s, l, 1)` | `RETURN_STRING(s)` / `RETVAL_STRINGL(s, l)` |
| `TSRMLS_DC`, `TSRMLS_CC`, `TSRMLS_FETCH` | no-op / removed in Zend Engine 4 |
| `ZEND_INIT_MODULE_GLOBALS` + old `ts_resource` macro | `ZEND_INIT_MODULE_GLOBALS` + `ZEND_MODULE_GLOBALS_ACCESSOR` |
| `convert_to_*_ex` | `Z_PARAM_*` coercion, `zval_get_string`, `zval_get_long`, `zend_is_true` |
| `array_init()` return check | `array_init()` is a statement macro in PHP 8 |

Other changes:

- Every function has arginfo, required by PHP 8.
- Module version is `1.4.2-php8`; `phpinfo()` also reports the Zend Engine 4.x
  target and the SDK library version.
- `%` characters in SDK error messages are no longer interpreted as format
  specifiers (`php_error("%s", msg)`).
- `saprfc_open` builds the SDK connection string with `smart_str`; the wire
  format (`KEY=VALUE ` separated by spaces) is unchanged.

## Intentional behavior changes and bug fixes

These are deliberate; each one either prevents a crash/overflow or fixes a
documented-but-broken path. Review them before deploying to code that depended
on the old accidental behavior.

1. **`saprfc_optional()` now works.** 1.4.1 had a missing semicolon in
   `convert_to_boolean_ex(value)` and could not compile as shipped on modern
   compilers. It is rewritten with argument parsing.
2. **`saprfc_function_define()` honors `optional`.** 1.4.1 looked up the key
   `"type"` with the length of `"optional"`, so the flag was always false.
3. **Names are case-insensitive.** Interface, parameter, and field names are
   uppercased and compared with `strcasecmp` inside CAL. 1.4.1 uppercased only
   some entry points, so lowercase `saprfc_import()` names could fail.
4. **`saprfc_table_remove()` returns `true` on success.** The 1.4.1 code fell
   through without setting a return value (implicit `NULL`), contradicting its
   documented `bool` return type.
5. **RAW (`TYPX`) values are clamped to the field length.** Both the PHP layer
   and `cal_conv_string_to_rfc()` now copy at most `def->len` bytes. 1.4.1 could
   read or write past the end of a short/long PHP string.
6. **Closing a connection invalidates dependent function handles.** A later
   `saprfc_call_and_receive()` / `saprfc_trfc_call()` returns `-1` with a warning
   instead of passing a freed SDK handle. 1.4.1 could use the closed handle.
7. **Calling a function module defined offline (`saprfc_function_define(0, ...)`)
   returns `-1` with a warning** instead of passing `RFC_HANDLE_NULL` to the SDK.
8. **`saprfc_allow_start_program()` with no argument no longer touches an
   uninitialized zval.** 1.4.1 called `convert_to_string_ex()` on an
   uninitialized pointer in that path.
9. **Malformed interface descriptions are skipped with a warning** instead of
   using uninitialized `itype` / `iofs` values.
10. **`saprfc_function_debug_info()` parses its second argument as a bool.**
    1.4.1 treated mere presence of the argument as true, so
    `debug_info($fce, false)` printed only values.
11. **`saprfc_set_trace()` parses the level as bool** but keeps the 1.4.1
    behavior of accepting an invalid/absent connection and tracing
    `RFC_HANDLE_NULL`.
12. **Non-array `saprfc_open()` and non-resource handle arguments throw PHP 8
    `TypeError`.** 1.4.1 emitted warnings and often returned `NULL`; PHP 8
    engines expect argument errors to throw. `saprfc_open([])` still returns
    `false` plus a warning when the SDK rejects the connection string.
13. **Interface names/fields are stored uppercase**, matching the values that
    SAP returns and that `saprfc_function_interface()` has always exposed.
14. **`saprfc_server_accept()` keeps converted argument strings alive for the
    duration of `RfcAccept()`**, avoiding use-after-free with non-string array
    elements.

## Return-value compatibility summary

| Function | Success | Failure |
| --- | --- | --- |
| `saprfc_open` | connection resource | `false` + warning |
| `saprfc_function_discover` / `_define` | function resource | `false` + warning |
| `saprfc_function_interface` | array | `false` + warning |
| `saprfc_import` / `_export` | `true` / value | `false` + warning |
| `saprfc_table_*` | `true` (or value/row count) | `false` / `-1` + warning |
| `saprfc_call_and_receive` / `saprfc_trfc_call` | RFC return code (`0` = `SAPRFC_OK`) | RFC code; warning on error |
| `saprfc_error` | string (may describe the last SDK error) | never `false` |
| `saprfc_exception` | exception string | `false` |
| `saprfc_close` / `saprfc_function_free` | `true` | `false` + warning |
| `saprfc_set_code_page` / `_optional` | `true` | `false` + warning |
| `saprfc_attributes` | array | `false` + warning |
| `saprfc_server_register_*` | array | integer RFC code |
| `saprfc_allow_start_program` / `_get_ticket` | `true` / string | `false` + warning |

RFC codes themselves are unchanged and are exposed through the `SAPRFC_*`
constants.

## Compatibility with existing PHP code

Typical 1.4.1 code continues to work unchanged:

```php
$rfc = saprfc_open($conn);
$fce = saprfc_function_discover($rfc, 'RFC_READ_TABLE');
saprfc_import($fce, 'QUERY_TABLE', 'MARA');
saprfc_table_init($fce, 'DATA');
$rc = saprfc_call_and_receive($fce);
$rows = saprfc_table_rows($fce, 'DATA');
for ($i = 1; $i <= $rows; $i++) {
    $row = saprfc_table_read($fce, 'DATA', $i);
}
saprfc_function_free($fce);
saprfc_close($rfc);
```

The main code-review items are: passing non-array connection parameters,
relying on `saprfc_table_remove()` returning `NULL`, relying on the broken
`optional` flag being ignored, and calling function handles after closing their
connection. All of those now behave safely and/or as documented.
