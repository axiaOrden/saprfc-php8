--TEST--
saprfc call/import/export roundtrip against the mock RFCSDK
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
echo is_resource($rfc) ? "connected\n" : "not-connected\n";

$definition = [
    ['name' => 'ECHO_IN', 'type' => 'IMPORT',
     'def' => [['name' => '', 'abap' => 'C', 'len' => 10]]],
    ['name' => 'ECHO_OUT', 'type' => 'EXPORT',
     'def' => [['name' => '', 'abap' => 'C', 'len' => 10]]],
];
$fce = saprfc_function_define($rfc, 'Z_MOCK_ECHO', $definition);

var_dump(saprfc_import($fce, 'ECHO_IN', 'hello'));
var_dump(saprfc_call_and_receive($fce));
var_dump(saprfc_export($fce, 'ECHO_OUT'));
var_dump(saprfc_exception($fce));

$attributes = saprfc_attributes($rfc);
echo $attributes['sysid'], '/', $attributes['client'], '/',
     $attributes['partner_codepage'], "\n";
var_dump(saprfc_set_code_page($rfc, '4103'));
var_dump(saprfc_trfc_tid($rfc));
var_dump(saprfc_get_ticket($rfc));

var_dump(saprfc_function_free($fce));
var_dump(saprfc_close($rfc));
?>
--EXPECT--
connected
bool(true)
int(0)
string(5) "hello"
bool(false)
MOCK/100/4103
bool(true)
string(23) "MOCKTID0123456789ABCDEF"
string(15) "MOCK-SSO-TICKET"
bool(true)
bool(true)
