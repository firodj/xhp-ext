--TEST--
Whitespace 02
--FILE--
<?php
require 'xhp_x.php';
echo <x> {'a'}<x /></x>;
--EXPECT--
<x>a<x></x></x>
