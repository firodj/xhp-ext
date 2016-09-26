--TEST--
xhp_token_name return the token name
--FILE--
<?php
echo xhp_token_name(260);
--EXPECT--
T_EVAL
