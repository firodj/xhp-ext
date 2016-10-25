--TEST--
Whitespace 07
--FILE--
<?php
require 'xhp_x.php';
echo <x> a{ 'b' }c </x>;
--EXPECT--
<x> abc </x>
