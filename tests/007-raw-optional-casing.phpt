--TEST--
saprfc RAW values, not_trim, optional parameters, and case-insensitive names
--SKIPIF--
<?php if (!extension_loaded('saprfc')) die('skip saprfc not loaded'); ?>
--FILE--
<?php
$definition = [
    ['name' => 'IV_RAW', 'type' => 'IMPORT',
     'def' => [['name' => '', 'abap' => 'X', 'len' => 4]]],
    ['name' => 'EV_RAW', 'type' => 'EXPORT',
     'def' => [['name' => '', 'abap' => 'X', 'len' => 4]]],
    ['name' => 'EV_TEXT', 'type' => 'EXPORT',
     'def' => [['name' => '', 'abap' => 'C', 'len' => 6]]],
    ['name' => 'IV_OPT', 'type' => 'IMPORT', 'optional' => true,
     'def' => [['name' => '', 'abap' => 'C', 'len' => 3]]],
];

/* not_trim keeps trailing blanks of CHAR values. */
$fce = saprfc_function_define(0, 'ZRAW', $definition, true);
var_dump(saprfc_import($fce, 'iv_raw', "\x01\x02\x03\x04\x05"));
echo 'raw=', bin2hex(saprfc_server_import($fce, 'IV_RAW')), "\n";

var_dump(saprfc_server_export($fce, 'ev_raw', "\xDE\xAD\xBE\xEF"));
echo 'export=', bin2hex(saprfc_export($fce, 'EV_RAW')), "\n";

var_dump(saprfc_server_export($fce, 'EV_TEXT', 'abc  '));
var_dump(saprfc_export($fce, 'EV_TEXT'));

$optional = null;
foreach (saprfc_function_interface($fce) as $parameter) {
    if ($parameter['name'] === 'IV_OPT') {
        $optional = $parameter['optional'];
    }
}
echo "optional-before=$optional\n";
var_dump(saprfc_optional($fce, 'iv_opt', false));
foreach (saprfc_function_interface($fce) as $parameter) {
    if ($parameter['name'] === 'IV_OPT') {
        $optional = $parameter['optional'];
    }
}
echo "optional-after=$optional\n";

/* Offline function modules have no connection: calls must not crash. */
var_dump(@saprfc_call_and_receive($fce));
var_dump(saprfc_function_free($fce));
?>
--EXPECT--
bool(true)
raw=01020304
bool(true)
export=deadbeef
bool(true)
string(6) "abc   "
optional-before=1
bool(true)
optional-after=0
int(-1)
bool(true)
