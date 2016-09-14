--TEST--
Config moderate parse, but the php using xhp syntax
--INI--
xhp.moderate_parse = On
--FILE--
<?php
class :x:etc {}
--EXPECTF--
Parse error: syntax error, unexpected ':', %s
