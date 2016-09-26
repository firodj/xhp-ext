--TEST--
xhp_token_get_all should return array
--FILE--
<?php
$tokens = xhp_token_get_all(<<<'code'
<?hh

echo "Cool!";
code
);

var_dump($tokens);
--EXPECT--
array(0) {
}
