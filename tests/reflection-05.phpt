--TEST--
Reflection API 05 (PHP 7.0)
--FILE--
<?php
class foo {
  /** a */ public /** b */ $bar /** c */ = /** d */ 1 /** e */;
}
$foo = new ReflectionProperty('foo', 'bar');
echo $foo->getDocComment();
exit;
<a />;
--EXPECT--
/** e */
