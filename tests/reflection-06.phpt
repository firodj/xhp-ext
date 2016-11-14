--TEST--
Reflection API 06
--FILE--
<?php //xhp
class foo {
  /** a */ public /** b */ $bar /** c */;
}
$foo = new ReflectionProperty('foo', 'bar');
echo $foo->getDocComment();
exit;
<a />;
--EXPECT--
/** c */
