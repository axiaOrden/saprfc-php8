--TEST--
saprfc_open rejects unusable connection strings without crashing
--SKIPIF--
<?php if (!extension_loaded('saprfc')) die('skip saprfc not loaded'); ?>
--FILE--
<?php
var_dump(@saprfc_open([]));
try {
    saprfc_open('not-an-array');
} catch (TypeError $e) {
    echo "TypeError\n";
}
?>
--EXPECT--
bool(false)
TypeError
