--TEST--
saprfc extension loads and exports the documented API
--SKIPIF--
<?php if (!extension_loaded('saprfc')) die('skip saprfc not loaded'); ?>
--FILE--
<?php
$functions = [
    'saprfc_open', 'saprfc_function_discover', 'saprfc_function_define',
    'saprfc_function_interface', 'saprfc_function_debug_info',
    'saprfc_optional', 'saprfc_import', 'saprfc_export',
    'saprfc_table_init', 'saprfc_table_append', 'saprfc_table_insert',
    'saprfc_table_modify', 'saprfc_table_remove', 'saprfc_table_read',
    'saprfc_table_rows', 'saprfc_call_and_receive', 'saprfc_error',
    'saprfc_function_free', 'saprfc_close', 'saprfc_set_code_page',
    'saprfc_attributes', 'saprfc_server_accept', 'saprfc_server_import',
    'saprfc_server_export', 'saprfc_server_dispatch', 'saprfc_trfc_install',
    'saprfc_trfc_dispatch', 'saprfc_trfc_call', 'saprfc_trfc_tid',
    'saprfc_set_trace', 'saprfc_server_register_check',
    'saprfc_server_register_cancel', 'saprfc_function_name',
    'saprfc_exception', 'saprfc_allow_start_program', 'saprfc_get_ticket',
];

$missing = 0;
foreach ($functions as $function) {
    if (!function_exists($function)) {
        echo "missing $function\n";
        $missing++;
    }
}
echo count($functions) - $missing, " functions available\n";
var_dump(SAPRFC_OK, SAPRFC_FAILURE, SAPRFC_EXCEPTION, SAPRFC_RETRY, SAPRFC_INVALID_HANDLE);
echo is_string(saprfc_error()) ? "error-string\n" : "no-error-string\n";
?>
--EXPECT--
36 functions available
int(0)
int(1)
int(2)
int(7)
int(18)
error-string
