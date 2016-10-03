--TEST--
XHP Classes within hh-tag
--FILE--
<?hh
class :x:etc {
attribute array a = [];
attribute string s = "bakwan";
}
abstract class :x:foo {}
abstract class :x:bar extends :x:foo {
public function own(): string {}
public function now(): :x:etc {
$x = null;
return $x ? <a/> : <b/>;
}
}
echo "pass";
--EXPECT--
pass
