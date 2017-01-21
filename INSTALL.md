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

Build:
```
$ phpize
$ ./configure
$ make
```

Make sure the `modules/xhp.so` linked with the `libxhp.dylib`:
```
$ otool -L modules/xhp.so
$ ln -s $(pwd)/xhp/libxhp.dylib /usr/local/lib/libxhp.dylib
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
