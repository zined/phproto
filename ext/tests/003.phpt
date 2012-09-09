--TEST--
phproto_pack tests
--SKIPIF--
<?php if (! extension_loaded("phproto")) print "skip"; ?>
--FILE--
<?php if (function_exists("phproto_pack")) print "this test is stupid.\n"; ?>
--EXPECT--
this test is stupid.
