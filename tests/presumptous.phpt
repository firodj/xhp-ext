--TEST--
Presumptous Closing Tags
--FILE--
<?php //xhp
class xhp_a {}
$foo = <a><a><a>hi</a></></a>;
echo "pass";
--EXPECT--
pass
