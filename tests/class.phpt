--TEST--
XHP Classes
--FILE--
<?php //xhp
class :x:etc {}
abstract class :x:foo {}
abstract class :x:bar extends :x:foo {}
echo "pass";
--EXPECT--
pass
