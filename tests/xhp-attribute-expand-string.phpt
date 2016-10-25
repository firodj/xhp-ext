--TEST--
XHP Attribute expanded inside string
--FILE--
<?php
require 'xhp_x.php';

class :base extends :x {
  public function info() {
    return "a: {$this->:a}, b: {$this->:b}";
  }
  public function getAttribute($name) {
    return $this->attrs[$name];
  }
}

$base = <base a="2" b="3" />;
echo $base->info();
--EXPECT--
a: 2, b: 3
