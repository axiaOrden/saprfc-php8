--TEST--
saprfc argument validation raises PHP 8 exceptions instead of crashing
--SKIPIF--
<?php if (!extension_loaded('saprfc')) die('skip saprfc not loaded'); ?>
--FILE--
<?php
try {
    saprfc_close('not-a-resource');
} catch (TypeError $e) {
    echo "close TypeError\n";
}

try {
    saprfc_import(1, 'NAME');
} catch (ArgumentCountError $e) {
    echo "import ArgumentCountError\n";
}

$definition = [['name' => 'IV', 'type' => 'IMPORT',
                'def' => [['name' => '', 'abap' => 'C', 'len' => 3]]]];
$fce = saprfc_function_define(0, 'ZARGS', $definition);
var_dump(saprfc_import($fce, 'IV', 'abc'));
var_dump(saprfc_function_free($fce));
var_dump(@saprfc_import($fce, 'IV', 'abc'));
var_dump(@saprfc_close($fce));
?>
--EXPECT--
close TypeError
import ArgumentCountError
bool(true)
bool(true)
bool(false)
bool(false)
