--TEST--
Logical Operator Whitespace
--FILE--
<?php
echo true xor false;
if (0) echo <a />;
--EXPECT--
1
