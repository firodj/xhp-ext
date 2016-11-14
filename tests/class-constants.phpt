--TEST--
XHP Class Attributes
--FILE--
<?php //xhp
class :foo {
  public static $bar;
  const etc = 1;
}
(:foo::$bar = 2);
echo :foo::etc;
echo :foo::$bar;
--EXPECT--
12
