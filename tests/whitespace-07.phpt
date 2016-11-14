--TEST--
Whitespace 07
--FILE--
<?php //xhp
require 'xhp_x.php';
echo <x> a{ 'b' }c </x>;
--EXPECT--
<x> abc </x>
