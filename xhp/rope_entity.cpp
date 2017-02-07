#include <iostream>
#include <algorithm>
#include <string>
#include <functional>
#include <map>

using namespace std;
#include "code_rope.hpp"

map<string, string> ENTITIES = {
  {"quot", "\""},
  {"amp", "&"},
  {"apos", "'"},
  {"lt", "<"},
  {"gt", ">"},
  {"nbsp", " "},
  {"copy", "\u00a9"},
  {"reg", "\u00ae"},
  {"trade", "\u2122"},
  {"hyphen", "\u2010"},
  {"dash", "\u2010"},
  {"ndash", "\u2013"},
  {"mdash", "\u2014"},
};

#ifdef USE_SGI_ROPE
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
#endif

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
#ifdef USE_SGI_ROPE
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
        map<string, string>::iterator it = ENTITIES.find(name);

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
#else
  string s(str.c_str());

  const char *current = s.c_str();
  const char *end = current + s.size();
  size_t pos = 0;
  while (current != end) {
    if (*current == '&') {
      enum {EN_NAME, EN_HEX, EN_DEC} ent = EN_NAME;
      const char *fix = current;

      if (++current == end) break;

      if (*current == '#') {
        ent = EN_DEC;
        if (++current == end) break;
        if (*current == 'x') {
          ent = EN_HEX;
          if (++current == end) break;
        }
      }

      const char *last = current;
      size_t len = 0;
      for(;last != end; ++last, ++len) {
        if (*last == ';') break;

        if (ent <= EN_DEC) {
          if (*last >= '0' && *last <= '9') continue;
        }
        if (ent <= EN_HEX) {
          if (*last >= 'a' && *last <= 'f') continue;
          if (*last >= 'A' && *last <= 'F') continue;
        }
        if (*last >= 'g' && *last <= 'z') continue;
        if (*last >= 'G' && *last <= 'Z') continue;
        break;
      }

      if (last == end) break;
      if (*last == ';') {
        string name(current, len);
        string entity;

        if (ent == EN_DEC) {
          char buf[5];
          if (utf8ize(atoi(name.c_str()), buf)) {
            entity = buf;
            len += 3; // skip: &#;
          }
        } else if (ent == EN_HEX) {
          char buf[5], *_end;
          if (utf8ize(strtol(name.c_str(), &_end, 16), buf)) {
            entity = buf;
            len += 4; // skip: &#x;
          }
        } else {
          map<string, string>::iterator it = ENTITIES.find(name);
          if (it != ENTITIES.end()) {
            entity = it->second;
            len += 2; // skip: &;
          }
        }

        if (!entity.empty()) {
          //cout << name << " -< [" << entity << "] " << entity.length() << endl;
          str.replace(pos, len, entity.c_str());
          current = last;
        }
      } else {
        current = fix;
      }
    }

    current++; pos++;
  }
#endif
}
