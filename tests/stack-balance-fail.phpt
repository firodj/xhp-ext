--TEST--
Stack Balance Fail
--FILE--
<?php //xhp
class xhp_x__y {}
$a = <x:y attr={:tag::CONSTANT} />;
function f() {}
echo 'pass';
--EXPECT--
pass
