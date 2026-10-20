--TEST--
saprfc server/tRFC helper functions against the mock RFCSDK
--SKIPIF--
<?php
if (!extension_loaded('saprfc')) die('skip saprfc not loaded');
$rfc = @saprfc_open(['MOCK' => '1']);
if (!is_resource($rfc)) die('skip mock RFCSDK not available');
saprfc_close($rfc);
?>
--FILE--
<?php
$check = saprfc_server_register_check('TPID', 'gwhost', 'gwservice');
ksort($check);
var_dump($check);

$cancel = saprfc_server_register_cancel('TPID', 'gwhost', 'gwservice');
ksort($cancel);
var_dump($cancel);

var_dump(saprfc_allow_start_program());
var_dump(saprfc_allow_start_program('prog1,prog2'));
var_dump(saprfc_trfc_install('check', 'commit', 'rollback', 'confirm', 'dispatch'));

$rfc = saprfc_open(['MOCK' => '1']);
saprfc_set_trace($rfc, true);
var_dump(saprfc_set_code_page($rfc, '4103'));
var_dump(strlen(saprfc_get_ticket($rfc)));
saprfc_close($rfc);
?>
--EXPECT--
array(4) {
  ["nbusy"]=>
  int(0)
  ["ninit"]=>
  int(0)
  ["nready"]=>
  int(1)
  ["ntotal"]=>
  int(1)
}
array(2) {
  ["ncancel"]=>
  int(1)
  ["ntotal"]=>
  int(1)
}
bool(true)
bool(true)
bool(true)
bool(true)
int(15)
