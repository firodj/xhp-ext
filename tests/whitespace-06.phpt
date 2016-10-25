--TEST--
Whitespace 06
--FILE--
<?php
require 'xhp_x.php';
echo <x> a { 'b' } c </x>;
--EXPECT--
<x> a b c </x>
