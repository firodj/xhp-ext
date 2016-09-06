--TEST--
XHP Classes with Property
--FILE--
<?php
class :x:a {
protected $morphClass = __FILE__ . 'SomeStr';
}
echo "pass";
--EXPECT--
pass
