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

#include <iostream>
#include <algorithm>
#include <string.h>

using namespace std;
#include "code_rope.hpp"

code_rope::code_rope(const _rope_t str, const size_t no /* = 0 */, const size_t lf /* = 0 */) : str(str), lf(lf), no(no) {}

code_rope::code_rope(const code_rope& str, const size_t no /* = 0 */, const size_t lf /* = 0 */) : str(str.str), lf(lf), no(no) {
  if (str.lf || str.no) {
    if (!no && !lf) {
      this->lf = str.lf;
      this->no = str.no;
    } else {
      throw new std::exception();
    }
  } else {
    this->no = no;
    this->lf = lf;
  }
}

const char* code_rope::c_str() const {
    return this->str.c_str();
}

void code_rope::prepend(const char* pre_str) {
  this->str.insert(0, pre_str);
}

void code_rope::strip_lines() {
  lf = no = 0;
}

size_t code_rope::lineno() const {
  return no;
}

code_rope code_rope::operator+(const code_rope& right) const {
  size_t diff;
  size_t no, lf;
  _rope_t glue;
  if (this->no && right.no) {
    no = this->no;
    if (right.no > this->no + this->lf) {
      diff = right.no - this->no - this->lf;
      lf = this->lf + right.lf + diff;
      glue = _rope_t(diff, '\n');
    } else {
      no = this->no;
      lf = this->lf + right.lf;
    }
  } else if (right.no) {
    no = right.no;
    lf = this->lf + right.lf;
  } else {
    no = this->no;
    lf = this->lf + right.lf;
  }

  code_rope res(this->str, no, lf);
  if (!glue.empty()) res.str += glue;
  res.str += right.str;
  return res;
}

code_rope code_rope::operator+(const char* right) const {
  code_rope res(this->str, this->no, this->lf);
  res.str += right;
  return res;
}

code_rope& code_rope::operator=(const char* str) {
  this->str = str;
  this->no = this->lf = 0;
  return *this;
}

code_rope operator+(const char* left, const code_rope& right) {
  code_rope ret(code_rope::_rope_t(left), right.no, right.lf);
  ret.str += right.str;
  return ret;
}

bool code_rope::operator==(const char* right)
{
  return str == _rope_t(right);
}

void code_rope::replaceAll(const char *find, const char *rep) {
  size_t j,i=this->str.length();
  string s(str.c_str());
  string f(find);
  _rope_t r(rep);
  while ((j = s.rfind(find, i)) != std::string::npos) {
    this->str.replace(j, f.length(), r);
    i=j-1;
  }
}

void code_rope::xhpLabel(bool global_ns /* = true */) {
  replaceAll(":", "__");
  replaceAll("-", "_");

  prepend("xhp_");
  if (global_ns) prepend("\\");
}

bool code_rope::htmlTrim() {
  _rope_t::iterator p0 = str.mutable_begin(), p00 = p0;
  _rope_t::iterator p1 = str.mutable_end();

  /* is empty */
  if (p0 == p1) return false;

  /* find position to trim left */
  while (isspace(*p0) && p0 != p1) ++p0;

  /* howlee siht, nothing found! */
  if (p0 == p1) {
    str.clear();
    return false;
  }

  /* TRIM! left */
  if (p0 != p00) {
    *--p0 = ' ';
    str.erase(p00, p0);
  }

  _rope_t::reverse_iterator q0 = str.mutable_rbegin(), q00 = q0;
  _rope_t::reverse_iterator q1 = str.mutable_rend();

  /* find position to trim right */
  while (isspace(*q0) && q0 != q1) ++q0;

  /* TRIM! right */
  if (q0 != q00) {
    *--q0 = ' ';
    str.erase(q0.base(), q00.base()+1);
  }

  /* find multiple sequences of spaces in the middle */
  for (q0 = str.mutable_rbegin()+1; q0 != str.mutable_rend(); ++q0) {
    if (isspace(*q0)) {
      q1 = q0+1;
      while ((q1 != str.mutable_rend()) && isspace(*q1)) ++q1;

      *--q1 = ' ';
      q0 = _rope_t::reverse_iterator( str.erase(q1.base(), q0.base()) );
    }
  }

  return true;
}

void code_rope::squote_escape()
{
  _rope_t::iterator current = str.mutable_begin(), fix;
  while (current != str.mutable_end()) {
    if (*current == '\'') {
      str.replace(current, current+1, "\\'");
      current += 2;
    } else {
      current++;
    }
  }
}
