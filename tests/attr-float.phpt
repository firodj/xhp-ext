--TEST--
Float attribute parsing
--FILE--
<?php //xhp
class :foo {
  attribute
    float b;
}
echo "pass";
--EXPECT--
pass
