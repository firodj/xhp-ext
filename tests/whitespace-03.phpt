--TEST--
Whitespace 03
--FILE--
<?php
require 'xhp_x.php';
echo <x> {'a'} </x>;
--EXPECT--
<x>a</x>
