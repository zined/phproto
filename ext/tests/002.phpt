--TEST--
phproto_info tests
--SKIPIF--
<?php if (! extension_loaded("phproto")) print "skip"; ?>
--FILE--
<?php if (function_exists("phproto_info")) print "this test is stupid.\n"; ?>
--EXPECT--
this test is stupid.
