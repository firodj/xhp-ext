--TEST--
Whitespace 04
--FILE--
<?php
require 'xhp_x.php';
echo <x> <x /> {'a'} </x>;
--EXPECT--
<x><x></x>a</x>
