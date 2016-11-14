--TEST--
Logical Operator Whitespace
--FILE--
<?php //xhp
echo true xor false;
if (0) echo <a />;
--EXPECT--
1
