_Disclaimer: This by no means provide a way of building xhp for production environment. Performance is poor and you should use it for testing only._

### Building XHP 1.4 for PHP 5.5

Requirements:

* PHP 5.5 source
* Visual Studio 2012 (Express version is Okay)
* A Linux machine/VM (Optional)

Instructions:

* Follow [PHP step by step build for Windows](https://wiki.php.net/internals/windows/stepbystepbuild) to compile PHP 5.5. If you are compiling it for FPM, choose non-thread safe in `configure` parameters.

Suppose your PHP source directory is `C:\php-sdk\phpdev\vc11\x86\phpsrc\`, there should be new directories created namely `C:\php-sdk\phpdev\vc11\x86\phpsrc\Release\php-5.5.XX-devel-VC11-x86\lib` and `C:\php-sdk\phpdev\vc11\x86\phpsrc\Release\php-5.5.XX-devel-VC11-x86\include`, housing the library and included files respectively. XX is the minor version of PHP.

* In Visual Studio, create a new Win32 DLL project, set up the project as stated in [this article](http://blog.slickedit.com/2007/09/creating-a-php-5-extension-with-visual-c-2005/).

Some of the instructions are obsolete. In particular, VC++ does not support preprocessor directives such as `ZEND_DEBUG=0` anymore. We need a workaround described in later part of this article.

Add all XHP source and header files to the project. Some files will be missing since they are automatically generated using tools such as `re2c` and `bison`, both of which are missing on the Windows system. You need a Linux machine to generate those files. Alternatively, you can download the generated files from [this fork](https://github.com/tengyifei/xhp/tree/master/xhp).

The generated files are:
```
xhp/fastpath.cpp
xhp/parser.yacc.cpp
xhp/parser.yacc.hpp
xhp/scanner.lex.cpp
xhp/scanner.lex.hpp
```
Add those files to your project as well.

### Fixing problems in compilation

The XHP extension uses GNU rope data structure to manipulate strings quickly. The rope structure is located in the GNU extension to the C++ standard library, but not supported by VC++ and is hard to port. A makeshift replacement would be the standard `string`, which is inferior in terms of running time. To do this we modify `code_rope.hpp` as follows:
```C++
//#include <ext/rope>
//#include <ext/pool_allocator.h>

#include <string>

class code_rope {
  public:
    //typedef __gnu_cxx::rope<char, __gnu_cxx::__pool_alloc<char> > _rope_t;
    typedef std::string _rope_t;
  protected:
    _rope_t str;
    size_t lf; /* how many line breaks this code contains */
    size_t no; /* line number this code starts on */

    .... omitted ....
```

* Next we need to add some necessary preprocessor directives to the beginning of `ext.cpp`:
```C++
// Turn off debugging manually here
#define ZEND_DEBUG 0

#define _ALLOW_KEYWORD_MACROS
#define ZEND_WIN32_FORCE_INLINE

// This is necessary to export the extensions's functions in the final DLL.
#define COMPILE_DL_XHP
```

* The `scanner.lex.cpp` file uses a function named `isatty` which is deprecated in VC++. Replace it with `_isatty`. To use this function we also need to include `stdio.h` and `io.h` such that the beginning of the file looks like this:
```C++
#line 2 "scanner.lex.cpp"

#line 4 "scanner.lex.cpp"

#include <stdio.h>
#include <io.h>

#define  YY_INT_ALIGNED short int

... omitted ...
```

* Finally, XHP uses the structure `module_registry` to look for APC in its init function `xhp`. This structure is located in `php5.lib` but has C linkage and will throw an unresolved symbol error. To fix this, we need to look for the definition of `module_registry` in `zend_modules.h`, and add `extern "C"` in front:
```C++
extern "C" extern ZEND_API HashTable module_registry;
```

Now you should be able to compile the xhp extension.