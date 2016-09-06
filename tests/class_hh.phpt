--TEST--
XHP Classes within hh-tag
--FILE--
<?hh
class :x:etc {}
abstract class :x:foo {}
abstract class :x:bar extends :x:foo {
public function Down(): string {}
}
echo "pass";
--EXPECT--
pass
