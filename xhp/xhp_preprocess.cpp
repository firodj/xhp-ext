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
  flags.idx_expr = false;
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
  yyscan_t scanner;
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
  yyscan_t scanner = 0;
  code_rope new_code;
  yy_extra_type extra;
  size_t size = in.size()+2;

  xhplex_init(&scanner);
  extra.return_all_tokens = true;

  xhpset_extra(&extra, scanner);
  xhp_scan_buffer(buffer, size, scanner);

  char *code_str;
  size_t lineno, oldlineno=0;

  int tok;
  while (tok = xhplex(&new_code, scanner)) {
    stringstream ss;

    size_t lineno = extra.lineno;
    code_str = (char*)new_code.c_str();

    if (oldlineno != lineno) {
        oldlineno = lineno;
        ss << endl << lineno << ": ";
    }

    if (tok >= 256) {
        ss << "[" << tok << ":" << yytokname(tok) << "]";
        ss << code_str;
    } else {
        if (tok >= 20 && tok <= 126) {
            ss << "(" << char(tok) << ")";
        } else {
            ss << tok;
        }
    }
    out += ss.str();
  }

  xhplex_destroy(scanner);

  return XHPDidNothing;
}

const char * xhp_get_token_type_name(int tok)
{
  return yytokname(tok);
}
