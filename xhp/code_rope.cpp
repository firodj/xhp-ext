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
#include <functional>
#include <map>

using namespace std;
#include "code_rope.hpp"

map<string, string> ENTITIES = {
  {"quot", "\""},
  {"apos", "'"},
  {"gt", ">"},
  {"amp", "&"},
  {"lt", "<"}
};

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
  if (0 && this->no > 1) {
    return NULL;
    // lolololololol
    // this code is clowntown -- returns dealloced memory
    _rope_t whitespace(this->no - 1, '\n');
    whitespace += this->str;
    return whitespace.c_str();
  } else {
    return this->str.c_str();
  }
}

void code_rope::prepend(const char* pre_str) {
  this->str.insert(0, pre_str);
}

const char code_rope::back() const {
  return this->str.empty() ? 0 : this->str.back();
}

void code_rope::pop_back() {
  this->str.pop_back();
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

  if (p0 == p1) return false;

  while (isspace(*p0) && p0 != p1) ++p0;

  if (p0 == p1) {
    str.clear();
    return false;
  }
  if (p0 != p00) {
    str.erase(p00, --p0);
  }

  _rope_t::reverse_iterator q0 = str.mutable_rbegin(), q00 = q0;
  _rope_t::reverse_iterator q1 = str.mutable_rend();

  while (isspace(*q0) && q0 != q1) ++q0;
  if (q0 != q00) {
    str.erase((q0-1).base(), q00.base()+1);
  }

  q0 = str.mutable_rbegin() + 1;

  for (; q0 != str.mutable_rend(); ++q0) {
    if (isspace(*q0)) {
      q1 = q0+1;
      while (isspace(*q1)) ++q1;
      q0 = _rope_t::reverse_iterator( str.erase(q1.base()+1, q0.base()) );
    }
  }

  return true;
}

struct IsStartEntity
{
  bool operator()(int ch) { return ch == '&'; }
} is_start_entity;

struct IsNamedEntity
{
  bool operator()(int ch) {
    std::string::size_type n;
    if (ch >= '0' && ch <= '9') return false;
    if (ch >= 'a' && ch <= 'z') return false;
    if (ch >= 'A' && ch <= 'Z') return false;
    return true;
  }
} is_named_entity;

struct IsHexEntity
{
  bool operator()(int ch) {
    std::string::size_type n;
    if (ch >= '0' && ch <= '9') return false;
    if (ch >= 'a' && ch <= 'f') return false;
    if (ch >= 'A' && ch <= 'F') return false;
    return true;
  }
} is_hex_entity;

struct IsDecEntity
{
  bool operator()(int ch) {
    std::string::size_type n;
    if (ch >= '0' && ch <= '9') return false;
    return true;
  }
} is_dec_entity;

void code_rope::xhpDecode() {
  _rope_t::iterator current = str.mutable_begin(), fix;

  while ((current = find_if(current, str.mutable_end(), is_start_entity)) != str.mutable_end()) {
    fix = current;
    function<bool(int)> stop_by = is_named_entity;

    if (++current == str.mutable_end()) break;

    if (*current == '#') {
      stop_by = is_dec_entity;
      if (++current == str.mutable_end()) break;

      if (*current == 'x') stop_by = is_hex_entity;
      if (++current == str.mutable_end()) break;
    }

    _rope_t::iterator last = find_if(current, str.mutable_end(), stop_by);
    if (last == str.mutable_end()) break;

    if (*last == ';') {
      // TODO: check stop_by.target to which entity type ?
      //
      string name = str.substr(current, last).c_str();
      map<string,string>::iterator it = ENTITIES.find(name);

      if (it != ENTITIES.end()) {
        string entity = it->second;
        str.replace(fix, last+1, entity.c_str());
        cout << name << endl;

        current = fix + entity.length();

        cout << "next(" << *current << ")" << endl;
      }
    }

    current = last;
  }

}
