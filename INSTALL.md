# Mac

Requirements:
```
$ brew install php70
$ brew install gcc5
$ brew install flex  # flex 2.6.x
$ brew install bison # bison 3.0.x
$ brew install re2c
$ brew link gcc5
$ brew link flex
$ brew link bison
```

Install build tool:
```
$ phpize --clean
$ phpize
$ ./configure
```

Patch:

If you get error regarding `__ZNSt8ios_base4InitD1Ev` it is because the `libtool`
using `gcc` instead `g++` compiler in which it doesn't link to `libstdc++`.
Change the command produced in `Makefile` from `--mode=link $(CC)` into `--mode=link $(CXX)`
on the rule `./xhp.la` (bottom).

Build:
```
$ make
```

Make sure the `modules/xhp.so` linked with `libstdc++`:
```
$ otool -L modules/xhp.so
```

Test:
```
$ make test
```

Install:
```
$ make install
$ echo "extension=xhp.so" > /usr/local/etc/php/7.0/conf.d/ext-xhp.ini
$ brew services restart php70
```

