--TEST--
Stack Balance Fail
--FILE--
<?php //xhp
class xhp_x__y {}
class xhp_tag {
 const CONSTANT = 0;
}
$a = <x:y attr={:tag::CONSTANT} />;
function f() {}
echo 'pass';
--EXPECT--
pass
