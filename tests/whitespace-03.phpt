--TEST--
Whitespace 03
--FILE--
<?php //xhp
require 'xhp_x.php';
echo <x> {'a'} </x>;
--EXPECT--
<x>a</x>
