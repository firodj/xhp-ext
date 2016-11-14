--TEST--
XHP Function Call Param
--FILE--
<?php //xhp
function id($i) { return $i; }
class xhp_thing {
  public function exist() {}
}
id(<thing />)->exist();
echo 'pass';
--EXPECT--
pass
