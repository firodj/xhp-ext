--TEST--
xhp_rename_function
--FILE--
<?php

function a() {
echo "a".PHP_EOL;
}

function b() {
echo "b".PHP_EOL;
}

xhp_rename_function("c", "a");
xhp_rename_function("b", "a");

a();

xhp_rename_function("a", "old_a");
//xhp_rename_function("old_a", "a");
//a();

--EXPECTF--
Warning: xhp_rename_function() failed: original 'c' does not exist!%S

Warning: xhp_rename_function() failed: new 'a' already exist!%S
a
