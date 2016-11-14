--TEST--
Constant in Array
--FILE--
<?php //xhp
class :foo {
  const bar = 'pass';
}
$foo = array('etc' => :foo::bar);
echo $foo['etc'];
--EXPECT--
pass
