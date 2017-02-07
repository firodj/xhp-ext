/*
  +----------------------------------------------------------------------+
  | XHP                                                                  |
  +----------------------------------------------------------------------+
  | Copyright (c) 2009 - 2014 Facebook, Inc. (http://www.facebook.com)   |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE.PHP, and is    |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
*/

#ifdef USE_SGI_ROPE
#include <ext/rope>
#include <ext/pool_allocator.h>
#else
#include "rope.hpp"
#endif

class code_rope {
  public:
#ifdef USE_SGI_ROPE
    typedef __gnu_cxx::rope<char, __gnu_cxx::__pool_alloc<char> > _rope_t;
    //typedef __gnu_cxx::crope _rope_t;
#else
    typedef jrope _rope_t;
#endif

  protected:
    _rope_t str;
    size_t lf; /* how many line breaks this code contains */
    size_t no; /* line number this code starts on */

  public:
    code_rope(const _rope_t = "", const size_t = 0, const size_t = 0);
    code_rope(const code_rope&, const size_t = 0, const size_t = 0);
    const char* c_str();
    void prepend(const char* str);
    void strip_lines();
    size_t lineno() const;
    code_rope operator+(const code_rope& right) const;
    code_rope operator+(const char*) const;
    code_rope& operator=(const char*);
    bool operator==(const char*);

    void replaceAll(const char *find, const char *rep);
    void xhpLabel(bool global_ns = true);
    bool htmlTrim();
    void xhpDecode();
    void squote_escape();

  friend code_rope operator+(const char*, const code_rope&);
};
code_rope operator+(const char*, const code_rope&);
