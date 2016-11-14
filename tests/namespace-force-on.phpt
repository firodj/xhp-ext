--TEST--
Force Global Namespace ON
--INI--
xhp.force_global_namespace = 1
--FILE--
<?php //xhp
namespace Foo;
require 'namespace-lib.php';
<x />;
--EXPECT--
--
