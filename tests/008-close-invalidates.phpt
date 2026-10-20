--TEST--
closing an RFC connection invalidates dependent function resources
--SKIPIF--
<?php
if (!extension_loaded('saprfc')) die('skip saprfc not loaded');
$rfc = @saprfc_open(['MOCK' => '1']);
if (!is_resource($rfc)) die('skip mock RFCSDK not available');
saprfc_close($rfc);
?>
--FILE--
<?php
$rfc = saprfc_open(['MOCK' => '1']);
$definition = [['name' => 'ECHO_IN', 'type' => 'IMPORT',
                'def' => [['name' => '', 'abap' => 'C', 'len' => 10]]]];
$fce = saprfc_function_define($rfc, 'Z_MOCK_ECHO', $definition);

var_dump(saprfc_close($rfc));
var_dump(@saprfc_call_and_receive($fce));
var_dump(@saprfc_close($rfc));
var_dump(saprfc_function_name($fce));
var_dump(saprfc_function_free($fce));
?>
--EXPECT--
bool(true)
int(-1)
bool(false)
string(11) "Z_MOCK_ECHO"
bool(true)
