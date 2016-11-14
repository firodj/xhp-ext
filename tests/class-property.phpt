--TEST--
XHP Classes with Property
--FILE--
<?php //xhp
class :x:a {
protected $morphClass = __FILE__ . 'SomeStr';
public function getOne(): string
{
}
}
echo "pass";
--EXPECT--
pass
