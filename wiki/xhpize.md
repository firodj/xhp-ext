xhpize
======

If you read [How It Works](How-It-Works.md) you’ll know that XHP is actually just a metaprogramming layer that sits on top of PHP, much like how the [C preprocessor] interacts with C. In some situations it may be impossible to install the XHP extension, but that doesn’t mean you can’t still use XHP! Included with XHP is a standalone utility called `xhpize`. `xhpize` can translate PHP with XHP syntax into plain PHP that any server can understand. That way if you’re in a situation where you can’t install extensions to your web server, you can use XHP by building a preprocess stage which converts your XHP source files into regular PHP.

Important: `xhpize` is a non-reversible transformation. It is not recommended that you edit code that has been xhpized. Therefore, you should be careful not to run `xhpize` on your working copy of code.

Building
========

To build `xhpize`, make sure you have `bison`, `flex`, `gcc`, and `re2c` (see [Building XHP](Building-XHP-on-Linux.md)) and then enter the [xhp] directory in your source files. From here just run `make xhpize` and it will build and place the binary in the current directory.

Usage
=====

```sh
xhpize [file]
xhpize < file
xhpize -i [-d] [file1] [file2]
xhpize -v
```

`xhpize` will take a file and will write to stdout an xhpized copy of the file. If no command line arguments are supplied it will expect PHP source code from stdin. The following flags are available to modify the behavior of xhpize:

`-i`

Run in in-place mode. Instead of outputting the xhpized source to stdout it will overwrite the supplied file with an xhpized version. This can be used to xhpize many files at once. For instance if you wanted to recursively xhpize all PHP files in a directory you could use this command: `find . -name '*.php' -type f -print0 | xargs -0 xhpize -i`.

`-d`

Run in dry-run mode. This must be used in addition to `-i`. This will prevent xhpize from writing to any files, but it will still parse and generate PHP code. This is useful for testing for build scripts before actually running them, as it will tell you which files would have been xhpized.

`-v`

Show `xhpize` version.

  [C preprocessor]: http://en.wikipedia.org/wiki/C_preprocessor
  [xhp]: https://github.com/facebook/XHP/tree/master/xhp/
