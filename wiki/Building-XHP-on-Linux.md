Building XHP on Linux
=====================

Building XHP requires:

-   [PHP] 5.2.x or 5.3.x
-   [gcc] 4.0 or higher
-   [flex] 2.5.35 or higher
-   [Bison] 2.3 or higher
-   [re2c] 0.13.5 or higher

You may have luck with different versions of these packages, but these are the versions that have been tested to work.

If PHP was configured correctly, and you meet the system requirements (Linux) you should be able to get XHP running just like you would most PHP extensions:

```sh
phpize
./configure
make
make install
```

Then add `extension=xhp.so` to your `php.ini` and you should be good to go. An easy way to test if XHP is working is by running `php -r 'echo "XHP!\n"; exit; <a />;'`. If you get a syntax error then XHP is not working.

See Also
========

[Configuration](Configuration.md)

  [PHP]: http://php.net/
  [gcc]: http://gcc.gnu.org/
  [flex]: http://flex.sourceforge.net/
  [Bison]: http://www.gnu.org/software/bison/
  [re2c]: http://re2c.org/
