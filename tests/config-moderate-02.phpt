--TEST--
Config moderate parse, enable with '// xhp'
--INI--
xhp.moderate_parse = On
--FILE--
<?php // xhp
class :x:etc {}
echo ini_get('xhp.moderate_parse');
--EXPECT--
1
