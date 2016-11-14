--TEST--
PHP5.4 (new thing)->call()
--SKIPIF--
<?php 
if (version_compare(PHP_VERSION, '5.4', '<')) exit("Skip This test is for PHP 5.5 only.");
?>
--FILE--
<?php //xhp
class simple {
  public function call() {
    echo 'pass';
  }
}

(new simple)->call();
--EXPECT--
pass
