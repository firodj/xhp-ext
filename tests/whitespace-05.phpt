--TEST--
Whitespace 05
--FILE--
<?php
require 'xhp_x.php';
echo
  <x>
    foo
  </x>;
--EXPECT--
<x> foo </x>
