--TEST--
Whitespace 01
--FILE--
<?php
require 'xhp_x.php';
echo <x>
<x>
</x>.
</x>;
--EXPECT--
<x><x></x>. </x>
