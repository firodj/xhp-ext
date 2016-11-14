--TEST--
Callable attribute parsing
--FILE--
<?php //xhp
class :foo {
  attribute
    callable b;
}
echo "pass";
--EXPECT--
pass
