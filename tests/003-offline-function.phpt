--TEST--
saprfc_function_define/import/table API works without a SAP connection
--SKIPIF--
<?php if (!extension_loaded('saprfc')) die('skip saprfc not loaded'); ?>
--FILE--
<?php
$definition = [
    ['name' => 'IV_TEXT', 'type' => 'IMPORT',
     'def' => [['name' => '', 'abap' => 'C', 'len' => 10]]],
    ['name' => 'EV_TEXT', 'type' => 'EXPORT',
     'def' => [['name' => '', 'abap' => 'C', 'len' => 10]]],
    ['name' => 'T_DATA', 'type' => 'TABLE',
     'def' => [
         ['name' => 'FIELD1', 'abap' => 'C', 'len' => 5],
         ['name' => 'FIELD2', 'abap' => 'N', 'len' => 3],
     ]],
];

$fce = saprfc_function_define(0, 'ZTEST', $definition);
echo is_resource($fce) ? "resource\n" : "not-resource\n";
echo saprfc_function_name($fce), "\n";

$interface = saprfc_function_interface($fce);
echo count($interface), " parameters\n";
foreach ($interface as $parameter) {
    echo $parameter['name'], '/', $parameter['type'], '/',
         $parameter['optional'], '/', count($parameter['def']), "\n";
}

var_dump(saprfc_import($fce, 'iv_text', 'hello'));
var_dump(saprfc_table_init($fce, 't_data'));
var_dump(saprfc_table_append($fce, 't_data', ['field1' => 'ab', 'field2' => 7]));
var_dump(saprfc_table_rows($fce, 't_data'));
var_dump(saprfc_table_read($fce, 't_data', 1));
var_dump(saprfc_table_insert($fce, 't_data',
         ['field1' => 'cd', 'field2' => 8], 1));
var_dump(saprfc_table_read($fce, 't_data', 1));
var_dump(saprfc_table_modify($fce, 't_data',
         ['field1' => 'xy', 'field2' => 9], 2));
var_dump(saprfc_table_read($fce, 't_data', 2));
var_dump(saprfc_table_remove($fce, 't_data', 1));
var_dump(saprfc_table_rows($fce, 't_data'));
var_dump(saprfc_function_free($fce));
var_dump(@saprfc_function_name($fce));
?>
--EXPECTF--
resource
ZTEST
3 parameters
IV_TEXT/IMPORT/0/1
EV_TEXT/EXPORT/0/1
T_DATA/TABLE/0/2
bool(true)
bool(true)
bool(true)
int(1)
array(2) {
  ["FIELD1"]=>
  string(2) "ab"
  ["FIELD2"]=>
  string(1) "7"
}
bool(true)
array(2) {
  ["FIELD1"]=>
  string(2) "cd"
  ["FIELD2"]=>
  string(1) "8"
}
bool(true)
array(2) {
  ["FIELD1"]=>
  string(2) "xy"
  ["FIELD2"]=>
  string(1) "9"
}
bool(true)
int(1)
bool(true)
bool(false)
