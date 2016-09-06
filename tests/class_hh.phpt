--TEST--
XHP Classes within hh-tag
--FILE--
<?hh
class :x:etc {}
abstract class :x:foo {}
abstract class :x:bar extends :x:foo {
public function own(): string {}
public function now(): :x:etc {
$x = null;
return $x ?? 0;
}
}
echo "pass";
--EXPECT--
pass
