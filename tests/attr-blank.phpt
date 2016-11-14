--TEST--
Blank attribute
--FILE--
<?php //xhp
class xhp_a {}
$foo = <a b="" />;
echo "pass";
--EXPECT--
pass
