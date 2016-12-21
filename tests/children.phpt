--TEST--
XHP Children Content
--FILE--
<?php //xhp
require 'xhp_x.php';
echo <x>Se&lt;l'ama"t Ma'l"am &copy;</x>;
--EXPECT--
<x>Se&lt;l'ama&quot;t Ma'l&quot;am ©</x>
