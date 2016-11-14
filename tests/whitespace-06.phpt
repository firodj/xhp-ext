--TEST--
Whitespace 06
--FILE--
<?php //xhp
require 'xhp_x.php';
echo <x> a { 'b' } c </x>;
--EXPECT--
<x> a b c </x>
