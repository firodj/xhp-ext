--TEST--
Whitespace 05
--FILE--
<?php //xhp
require 'xhp_x.php';
echo
  <x>
    foo
  </x>;
--EXPECT--
<x> foo </x>
