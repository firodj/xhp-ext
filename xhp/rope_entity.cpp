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

static bool utf8ize(uint32_t v, char* buf /* [5] */) {
  if (v <= 0x7f) { // 0xxxxxxx
    buf[0] = v;
    buf[1] = 0;
  } else if (v <= 0x7ff) { // 110yyyxx 10xxxxxx
    buf[0] = 0xc0 | (v >> 6);
    buf[1] = 0x80 | (v & 0x3f);
    buf[2] = 0;
  } else if (v <= 0xffff) { // 1110yyyy 10yyyyxx 10xxxxxx
    buf[0] = 0xe0 | (v >> 12);
    buf[1] = 0x80 | ((v >> 6) & 0x3f);
    buf[2] = 0x80 | (v & 0x3f);
    buf[3] = 0;
  } else if (v <= 0x1fffff) { // 11110zzz 10zzyyyy 10yyyyxx 10xxxxxx
    buf[0] = 0xf0 | (v >> 18);
    buf[1] = 0x80 | ((v >> 12) & 0x3f);
    buf[2] = 0x80 | ((v >> 6) & 0x3f);
    buf[3] = 0x80 | (v & 0x3f);
    buf[4] = 0;
  } else {
    return false;
  }
  return true;
}

void code_rope::xhpDecode() {
  _rope_t::iterator current = str.mutable_begin(), fix;

  while ((current = find_if(current, str.mutable_end(), is_start_entity)) != str.mutable_end()) {
    fix = current;
    function<bool(int)> stop_by = is_named_entity;

    if (++current == str.mutable_end()) break;

    if (*current == '#') {
      stop_by = is_dec_entity;
      if (++current == str.mutable_end()) break;

      if (*current == 'x') {
        stop_by = is_hex_entity;
        if (++current == str.mutable_end()) break;
      }
    }

    _rope_t::iterator last = find_if(current, str.mutable_end(), stop_by);
    if (last == str.mutable_end()) break;

    if (*last == ';') {
      string name = str.substr(current, last++).c_str();
      string entity;

      if (stop_by.target<IsNamedEntity>()) {
        map<string,string>::iterator it = ENTITIES.find(name);

        if (it != ENTITIES.end()) {
          entity = it->second;

        }
      } else if (stop_by.target<IsDecEntity>()) {
        char buf[5];
        if (utf8ize(atoi(name.c_str()), buf)) {
          entity = buf;
        }
      } else if (stop_by.target<IsHexEntity>()) {
        char buf[5], *_end;
        if (utf8ize(strtol(name.c_str(), &_end, 16), buf)) {
          entity = buf;
        }
      }

      if (!entity.empty()) {
          str.replace(fix, last, entity.c_str());
          current = fix + entity.length();
      }
    } else
    current = last;
  }

}
