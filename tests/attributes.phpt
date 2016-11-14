--TEST--
XHP attributes Declaration
--FILE--
<?php //xhp
class :foo {
  attribute
    string foo;
}
class :bar {
  attribute
    :foo,
    :foo,
    string bar;
}
echo "pass";
--EXPECT--
pass
