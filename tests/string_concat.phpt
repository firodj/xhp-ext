--TEST--
String concat with number
--FILE--
<?php // xhp
$x = 'a' . ':' . 1;
echo $x;

$x = 1.5 . ':' . 'b';
echo $x;
--EXPECT--
a:11.5:b
