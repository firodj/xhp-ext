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

#include "xhp.hpp"
#include "xhp_preprocess.hpp"
#include "fastpath.hpp"
#include <sstream>

using namespace std;

extern int xhpdebug;
#include <iostream>

const char* yytokname(int tok);

XHPResult xhp_preprocess(istream &in, string &out, bool isEval, string &errDescription, uint32_t &errLineno) {

  // Read stream to string
  stringbuf sb;
  in >> noskipws >> &sb;
  string buffer = sb.str();
  return xhp_preprocess(buffer, out, isEval, errDescription, errLineno);
}

XHPResult xhp_preprocess(string &in, string &out, bool isEval, string &errDescription, uint32_t &errLineno) {
  xhp_flags_t flags;
  memset(&flags, 0, sizeof(xhp_flags_t));
  flags.eval = isEval;
  flags.short_tags = true;
  flags.idx_expr = true;
  flags.include_debug = true;
  flags.force_global_namespace = true;
  flags.moderate_parse = false;
  return xhp_preprocess(in, out, errDescription, errLineno, flags);
}

XHPResult xhp_preprocess(std::string &in, std::string &out, std::string &errDescription, uint32_t &errLineno, const xhp_flags_t &flags) {

  // Early bail if the code doesn't contain anything that looks like XHP
  char* buffer = const_cast<char*>(in.c_str());
  if (!xhp_fastpath(buffer, in.length(), flags)) {
    return XHPDidNothing;
  }

  // Create a flex buffer
  in.reserve(in.size() + 1);
  buffer = const_cast<char*>(in.c_str());
  buffer[in.size() + 1] = 0; // need double NULL for scan_buffer

  // Parse the PHP
  void* scanner;
  code_rope new_code;
  yy_extra_type extra;
  extra.idx_expr = flags.idx_expr;
  extra.include_debug = flags.include_debug;
  extra.insert_token = flags.eval ? T_OPEN_TAG_FAKE : 0;
  extra.short_tags = flags.short_tags;
  extra.force_global_namespace = flags.force_global_namespace;

  xhplex_init(&scanner);
  xhpset_extra(&extra, scanner);
  xhp_scan_buffer(buffer, in.size() + 2, scanner);
#ifdef DEBUG
  xhpdebug = 1;
#endif
  xhpparse(scanner, &new_code);
  xhplex_destroy(scanner);

  // Check to see what happened
  if (extra.terminated) {
    errDescription = extra.error;
    errLineno = extra.lineno;
    return XHPErred;
  } else if (extra.used || extra.hh_tags) {
    out = new_code.c_str();
    return XHPRewrote;
  } else {
    return XHPDidNothing;
  }
}

XHPResult xhp_tokenize(istream &in, string &out)
{
  // Read stream to string
  stringbuf sb;
  in >> noskipws >> &sb;
  string buffer = sb.str();
  return xhp_tokenize(buffer, out);
}

XHPResult xhp_tokenize(std::string &in, std::string &out)
{
  // Create a flex buffer
  in.reserve(in.size() + 1);
  char *buffer = const_cast<char*>(in.c_str());
  buffer[in.size() + 1] = 0; // need double NULL for scan_buffer

  // Parse the PHP
  void *lex_state;
  xhp_init_lexical_state(buffer, in.size()+2, &lex_state);
  char *code_str;

  int64_t tok;
  while(tok = xhp_lex(&code_str, lex_state)) {
    stringstream ss;
    if (tok < 255) {
        if (tok >= 20 && tok <= 126) {
            ss << "(" << tok << " " << char(tok) << ")";
        } else {
            ss << "(" << tok << ")";
        }
    } else {
        ss << "[" << yytokname(tok) << "]";
        ss << code_str;
    }
    out += ss.str();
  }

  xhp_destroy_lexical_state(lex_state);

  return XHPDidNothing;
}

const char * xhp_get_token_type_name(int64_t tok)
{
  return yytokname(tok);
}

//
// Internal Struct
struct xhp_lex_state_t
{
public:
  xhp_lex_state_t() {
    scanner = 0;
    xhplex_init(&scanner);
  }
  ~xhp_lex_state_t() {
    xhplex_destroy(scanner);
  }
  void scan_buffer(char *buffer, size_t size) {
    xhpset_extra(&extra, scanner);
    xhp_scan_buffer(buffer, size, scanner);
  }

  void *scanner;
  code_rope new_code;
  yy_extra_type extra;
};

int64_t
xhp_lex(char **code_str, void *lex_state)
{
  xhp_lex_state_t *l = static_cast<xhp_lex_state_t*>(lex_state);
  if (l) {
    int64_t tok = xhplex(&l->new_code, l->scanner);
    if (tok) {
      *code_str = (char*)l->new_code.c_str();
    } else {
      *code_str = 0;
    }
    return tok;
  }
  return 0;
}

void
xhp_init_lexical_state(char *buffer, size_t size, void **lex_state)
{
  xhp_lex_state_t *l = new xhp_lex_state_t();

  l->extra.return_all_tokens = true;
  l->scan_buffer(buffer, size);

  *lex_state = l;
}

void
xhp_destroy_lexical_state(void *lex_state)
{
    xhp_lex_state_t *l = static_cast<xhp_lex_state_t*>(lex_state);
    if (l) {
       delete l;
    }
}
