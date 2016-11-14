--TEST--
xhp_token_get_all should return array
--FILE--
<?php //xhp
$tokens = xhp_token_get_all(<<<'code'
<?hh

echo "Cool!";
code
);

echo 'TOKENS:'. count($tokens) . PHP_EOL;
foreach($tokens as $tok) {
  if (is_array($tok)) {
  echo xhp_token_name($tok[0]).' '.$tok[2];
  } else {
  var_export($tok);
  }
  echo PHP_EOL;
}
--EXPECT--
TOKENS:6
T_OPEN_TAG 2
T_WHITESPACE 3
T_ECHO 3
T_WHITESPACE 3
T_CONSTANT_ENCAPSED_STRING 3
';'
