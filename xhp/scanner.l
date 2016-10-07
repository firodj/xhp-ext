/*
  +----------------------------------------------------------------------+
  | XHP                                                                  |
  +----------------------------------------------------------------------+
  | Copyright (c) 1998-2014 Zend Technologies Ltd. (http://www.zend.com) |
  | Copyright (c) 2009-2014 Facebook, Inc. (http://www.facebook.com)     |
  +----------------------------------------------------------------------+
  | This source file is subject to version 2.00 of the Zend license,     |
  | that is bundled with this package in the file LICENSE.ZEND, and is   |
  | available through the world-wide-web at the following url:           |
  | http://www.zend.com/license/2_00.txt.                                |
  | If you did not receive a copy of the Zend license and are unable to  |
  | obtain it through the world-wide-web, please send a note to          |
  | license@zend.com so we can mail you a copy immediately.              |
  +----------------------------------------------------------------------+
*/

%{ /* -*- mode: c++ -*- */
#include "xhp.hpp"
#include <string.h>

// A PHP file is made of multiple languages, each with its own lexing rules.
// The starting mode or state is the HTML state, then comes the PHP state, 
// and with XHP there is even an XHP state. Certain tokens indicate a 
// transition from one state to another and the macros below are used to 
// manage this state.
#define push_state(s) xhp_new_push_state(s, yyg)
#define pop_state() xhp_new_pop_state(yyg)
#define set_state(s) xhp_set_state(s, yyg)

// This is used to help contextual lexing. For instance 'attribute' is
// considered a XHP keyword after a '{' of a class definition, 
// but a regular identifier in other contexts.
#define last_token() yyextra->last_token

#define YY_USER_ACTION \
  if (yyextra->terminated) \
    return 0; \
  if (!yyg->yy_more_len) \
    yyextra->first_lineno = yyextra->lineno;

#define tok_consume() \
  *yylval = code_rope(yytext, yyextra->first_lineno, yyextra->lineno - yyextra->first_lineno)

#define tok_return(t) \
  if (shouldReturnToken(t, yyextra->return_all_tokens)) \
    return yy_token(t, yyg)

#define tok(t) \
  *yylval = code_rope(yytext, yyextra->first_lineno, yyextra->lineno - yyextra->first_lineno); \
  if (shouldReturnToken(t, yyextra->return_all_tokens)) \
    return yy_token(t, yyg)

#define YY_USER_INIT \
  if (yyextra->insert_token) { \
    yyg->yy_init = 0; \
    int ft = yyextra->insert_token; \
    yyextra->insert_token = 0; \
    return yy_token(ft, yyg); \
  }

using namespace std;

const char* yytokname(int tok);
static int yy_token(int tok, struct yyguts_t* yyg);
static void yy_scan_newlines(const char* text, struct yyguts_t* yyg);

/**
 * "Next token types" tell us how to interpret the next characters in the
 * input stream based on the previous token for the purpose of recognizing
 * XHP tags, XHP class names, XHP category names, type lists, and lambda
 * expressions.
 *   XhpTag:
 *     "<"[a-zA-Z_\x7f-\xff] will be treated as the start of an XHP tag
 *   XhpTagMaybe:
 *     "<"[a-zA-Z_\x7f-\xff] will be treated as possibly being the start of an
 *     XHP tag; we will scan ahead looking at subsequent characters to figure
 *     out if "<" is definitely the start of an XHP tag
 *   XhpClassName:
 *     ":"{XHPLABEL} will be treated as an XHP class name
 *   XhpCategoryName:
 *     "%"{XHPLABEL} will be treated as an XHP category name
 */
namespace NextTokenType {
  static const int Normal = 0x1;
  static const int XhpTag = 0x2;
  static const int XhpTagMaybe = 0x4;
  static const int XhpClassName = 0x8;
  static const int XhpCategoryName = 0x10;
}

static int getNextTokenType(int t) {
  switch (t) {
    case '=': case '.': case '+': case '-': case '*': case '/': case '%':
    case '!': case '~': case '&': case '^': case '<': case '>': case '?':
    case ':': case '[': case '{': case ';': case '@': case -1:
    case T_LOGICAL_OR:
    case T_LOGICAL_XOR:
    case T_LOGICAL_AND:
    case T_SL:
    case T_SR:
    case T_BOOLEAN_OR:
    case T_BOOLEAN_AND:
    case T_IS_EQUAL:
    case T_IS_NOT_EQUAL:
    case T_IS_IDENTICAL:
    case T_IS_NOT_IDENTICAL:
    case T_IS_SMALLER_OR_EQUAL:
    case T_IS_GREATER_OR_EQUAL:
    case T_SPACESHIP:
    case T_PLUS_EQUAL:
    case T_MINUS_EQUAL:
    case T_MUL_EQUAL:
    case T_DIV_EQUAL:
    case T_CONCAT_EQUAL:
    case T_MOD_EQUAL:
    case T_AND_EQUAL:
    case T_OR_EQUAL:
    case T_XOR_EQUAL:
    case T_SL_EQUAL:
    case T_SR_EQUAL:
    case T_ECHO:
    case T_PRINT:
    case T_CLONE:
    case T_EXIT:
    case T_RETURN:
    case T_YIELD:
    case T_YIELD_FROM:
    case T_NEW:
    case T_INSTANCEOF:
    case T_DOUBLE_ARROW:
    case T_NS_SEPARATOR:
    case T_INLINE_HTML:
    case T_INT_CAST:
    case T_DOUBLE_CAST:
    case T_STRING_CAST:
    case T_ARRAY_CAST:
    case T_OBJECT_CAST:
    case T_BOOL_CAST:
    case T_UNSET_CAST:
    case T_UNRESOLVED_LT:
    case T_AS:
      return NextTokenType::XhpTag |
             NextTokenType::XhpClassName;
    case ',': case '(': case '|': case T_UNRESOLVED_OP:
      return NextTokenType::XhpTag |
             NextTokenType::XhpClassName |
             NextTokenType::XhpCategoryName;
    case '}':
    case T_OPEN_TAG:
      return NextTokenType::XhpTagMaybe |
             NextTokenType::XhpClassName;
    case T_INC:
    case T_DEC:
      return NextTokenType::XhpTagMaybe;
    case T_EXTENDS:
    case T_CLASS:
    case T_PRIVATE:
    case T_PROTECTED:
    case T_PUBLIC:
    case T_STATIC:
      return NextTokenType::XhpClassName;
    case T_OBJECT_OPERATOR:
    case T_STRING:
    case T_XHP_CHILDREN:
    case T_XHP_REQUIRED:
    case T_XHP_ENUM:
    case T_ARRAY:
    case T_XHP_ATTRIBUTE:
      return NextTokenType::XhpClassName;
    case T_XHP_CATEGORY:
      return NextTokenType::XhpCategoryName;
    default:
      return NextTokenType::Normal;
  }
}

static bool shouldReturnToken(int t, bool force)
{
    if (!force) {
        switch (t) {
          case T_DOC_COMMENT:
          case T_COMMENT:
          case T_WHITESPACE:
            return false;
        }
    }
    return true;
}

%}

 /* The different lexing states. Note that the transitions are done either
  * in the lex actions, or in a generic manner in yy_token(). */
%x ST_PHP
%x ST_PHP_COMMENT
%x ST_PHP_EOL_COMMENT
%x ST_PHP_DOC_COMMENT
%x ST_PHP_HEREDOC_START
%x ST_PHP_HEREDOC_NSTART
%x ST_PHP_HEREDOC_NEWLINE
%x ST_PHP_HEREDOC_DATA
%x ST_PHP_NO_RESERVED_WORDS
%x ST_PHP_NO_RESERVED_WORDS_PERSIST
%x ST_XHP_IN_TAG
%x ST_XHP_CHILD
%x ST_XHP_ATTR_TYPE_DECL
%x ST_XHP_CHILDREN_DECL
%x ST_XHP_COMMENT
%x ST_XHP_END_SINGLETON_TAG
%x ST_LT_CHECK
%x ST_LOOKING_FOR_COLON
%x ST_XHP_END_CLOSE_TAG

%option prefix="xhp"
%option reentrant
 /* PHP allows IF or if */
%option case-insensitive
%option noyywrap nodefault
%option stack
%option bison-bridge
%option 8bit

 /* I think an interactive scanner is required because of the bison state 
  * pushing we do. I'm putting an explicit interactive declaration here in case
  * someone tries adding -CF or whatever to the make flags. */
%option interactive

 /* to get a Flex debug trace, uncomment %option debug, and uncomment
  *
  * %option debug
  */

 /* Definitions (Substitutions) */

LNUM [0-9]+
DNUM ([0-9]*"."[0-9]+)|([0-9]+"."[0-9]*)
EXPONENT_DNUM (({LNUM}|{DNUM})[eE][+-]?{LNUM})
HNUM "0x"[0-9a-fA-F]+
BNUM "0b"[01]+
LABEL [a-zA-Z_\x7f-\xff][a-zA-Z0-9_\x7f-\xff]*
ANY_CHAR (.|[\n])
WHITESPACE [ \n\r\t]+
TOKENS [;:,.\[\])|^&+\-*/=%!~$<>?@]
TABS_AND_SPACES [ \t]*
NEWLINE ("\r\n"|"\n"|"\r")
XHPLABEL {LABEL}([:-]{LABEL})*
COMMENT_REGEX ("/*"([^\*]|("*"[^/]))*"*/"|("//"|"#")[^\r\n]*{NEWLINE})
WHITESPACE_AND_COMMENTS ([ \n\r\t]|({COMMENT_REGEX}))+

%%

<ST_XHP_ATTR_TYPE_DECL>{
  "bool"     tok(T_XHP_BOOLEAN);
  "int"      tok(T_XHP_NUMBER);
  "float"    tok(T_XHP_FLOAT);
  "callable" tok(T_XHP_CALLABLE);
  "var"      tok(T_VAR);
  "mixed"    tok(T_XHP_MIXED);
  "array"    tok(T_ARRAY);
  "string"   tok(T_XHP_STRING);
  "("        tok('(');
  ":"        tok(T_XHP_COLON);
  ">>" {
    yyless(1);
    tok(T_TYPELIST_GT);
  }
  ">" tok(T_TYPELIST_GT);
  "<" tok(T_TYPELIST_LT);
  ";" { pop_state(); tok(';'); }
  "enum"     tok(T_XHP_ENUM);
  "@required"  tok(T_XHP_REQUIRED);
}

<ST_XHP_CHILDREN_DECL>{
  any tok(T_XHP_ANY);
  pcdata tok(T_XHP_PCDATA);
  empty tok(T_EMPTY);
  {LABEL} tok(T_STRING);
  ";" {
    pop_state();
    tok(';');
  }
}

 /* Open / close PHP + inline HTML */
<INITIAL>{
  "<?php"([ \t]|{NEWLINE}) {
    yy_scan_newlines(yytext + 5, yyg);
    // the state transition will be done in yy_token()
    tok(T_OPEN_TAG);
  }
  "<?hh"([ \t]|{NEWLINE}) {
    yy_scan_newlines(yytext + 4, yyg);
    // the state transition will be done in yy_token()
    yyextra->hh_tags = true;
    tok(T_OPEN_TAG);
  }
  "<?" {
    if (yyextra->short_tags) {
      tok(T_OPEN_TAG);
    } else {
      tok(T_INLINE_HTML);
    }
  }
  "<?=" {
    if (yyextra->short_tags) {
      tok(T_OPEN_TAG_WITH_ECHO);
    } else {
      tok(T_INLINE_HTML);
    }
  }
  "<"|[^<]* {
    yy_scan_newlines(yytext, yyg);
    tok(T_INLINE_HTML);
  }
}
<ST_PHP,ST_PHP_NO_RESERVED_WORDS,ST_PHP_NO_RESERVED_WORDS_PERSIST>{
  "?>"{NEWLINE}? {
    yy_scan_newlines(yytext + 2, yyg);
    tok(T_CLOSE_TAG);
  }
}

 /* Comments and whitespace */
<ST_PHP,ST_PHP_NO_RESERVED_WORDS,ST_PHP_NO_RESERVED_WORDS_PERSIST,ST_XHP_CHILDREN_DECL,ST_XHP_ATTR_TYPE_DECL,ST_XHP_IN_TAG>{
  "#"|"//" {
    push_state(ST_PHP_EOL_COMMENT);
    yymore();
  }
  "/**"{WHITESPACE} {
    yy_scan_newlines(yytext + 3, yyg);
    push_state(ST_PHP_DOC_COMMENT);
    yymore();
  }
  "/*" {
    push_state(ST_PHP_COMMENT);
    yymore();
  }
  {WHITESPACE}+ { yy_scan_newlines(yytext, yyg); tok(T_WHITESPACE); }
}
<ST_PHP_EOL_COMMENT>{
  {NEWLINE} {
    ++yyextra->lineno;
    pop_state();
    tok(T_COMMENT);
  }
  [^\r\n?]+ yymore();
  "?>" {
    yyless(yyleng - 2);
    pop_state();
    tok(T_COMMENT);
  }
  . yymore();
}
<ST_PHP_DOC_COMMENT,ST_PHP_COMMENT>{
  {NEWLINE} {
    ++yyextra->lineno;
    yymore();
  }
  [^*\r\n]+|"*" yymore();
}
<ST_PHP_DOC_COMMENT>"*/" {
  //yyextra->doc_block = code_rope(yytext, yyextra->first_lineno, yyextra->lineno - yyextra->first_lineno);
  //yyextra->has_doc_block = true;
  pop_state();
  tok(T_DOC_COMMENT);
}
<ST_PHP_COMMENT>"*/" {
  pop_state();
  tok(T_COMMENT);
}

 /* Reserved words */
<ST_PHP>{
  include tok(T_INCLUDE);
  include_once tok(T_INCLUDE_ONCE);
  eval tok(T_EVAL);
  require tok(T_REQUIRE);
  require_once tok(T_REQUIRE_ONCE);
  or tok(T_LOGICAL_OR);
  xor tok(T_LOGICAL_XOR);
  and tok(T_LOGICAL_AND);
  print tok(T_PRINT);
  instanceof tok(T_INSTANCEOF);
  new tok(T_NEW);
  clone tok(T_CLONE);
  exit tok(T_EXIT);
  if tok(T_IF);
  elseif tok(T_ELSEIF);
  else tok(T_ELSE);
  endif tok(T_ENDIF);
  echo tok(T_ECHO);
  do tok(T_DO);
  while tok(T_WHILE);
  endwhile tok(T_ENDWHILE);
  for tok(T_FOR);
  endfor tok(T_ENDFOR);
  foreach tok(T_FOREACH);
  endforeach tok(T_ENDFOREACH);
  declare tok(T_DECLARE);
  enddeclare tok(T_ENDDECLARE);
  as tok(T_AS);
  switch tok(T_SWITCH);
  endswitch tok(T_ENDSWITCH);
  case tok(T_CASE);
  default tok(T_DEFAULT);
  break tok(T_BREAK);
  continue tok(T_CONTINUE);
  goto tok(T_GOTO);
  function tok(T_FUNCTION);
  const tok(T_CONST);
  return tok(T_RETURN);
  try tok(T_TRY);
  catch tok(T_CATCH);
  finally tok(T_FINALLY);
  throw tok(T_THROW);
  use tok(T_USE);
  global tok(T_GLOBAL);
  static tok(T_STATIC);
  abstract tok(T_ABSTRACT);
  final tok(T_FINAL);
  private tok(T_PRIVATE);
  protected tok(T_PROTECTED);
  public tok(T_PUBLIC);
  var tok(T_VAR);
  unset tok(T_UNSET);
  isset tok(T_ISSET);
  empty tok(T_EMPTY);
  __halt_compiler tok(T_HALT_COMPILER);
  class tok(T_CLASS);
  interface tok(T_INTERFACE);
  extends tok(T_EXTENDS);
  implements tok(T_IMPLEMENTS);
  list tok(T_LIST);
  array tok(T_ARRAY);
  __class__ tok(T_CLASS_C);
  __method__ tok(T_METHOD_C);
  __function__ tok(T_FUNC_C);
  __line__ tok(T_LINE);
  __file__ tok(T_FILE);
  __trait__ tok(T_TRAIT_C);
  namespace tok(T_NAMESPACE);
  trait tok(T_TRAIT);
  insteadof tok(T_INSTEADOF);
  yield tok(T_YIELD);
  "..." tok(T_VARIADIC_PARAMETER);
  __namespace__ tok(T_NS_C);
  __dir__ tok(T_DIR);
  (?-i:"attribute") {
    if ((last_token() == '{' || last_token() == '}' || last_token() == ';')) {
      push_state(ST_XHP_ATTR_TYPE_DECL);
      tok(T_XHP_ATTRIBUTE);
    } else {
      tok(T_STRING);
    }
  }
  (?-i:"category") {
    if ((last_token() == '{' || last_token() == '}' || last_token() == ';')) {
      tok(T_XHP_CATEGORY);
    } else {
      tok(T_STRING);
    }
  }
  (?-i:"children") {
    if ((last_token() == '{' || last_token() == '}' || last_token() == ';')) {
      push_state(ST_XHP_CHILDREN_DECL);
      tok(T_XHP_CHILDREN);
    } else {
      tok(T_STRING);
    }
  }
}

 /* Operators */
<ST_PHP,ST_PHP_NO_RESERVED_WORDS,ST_PHP_NO_RESERVED_WORDS_PERSIST,ST_XHP_ATTR_TYPE_DECL,ST_XHP_CHILDREN_DECL>{
  "+=" tok(T_PLUS_EQUAL);
  "-=" tok(T_MINUS_EQUAL);
  "*=" tok(T_MUL_EQUAL);
  "/=" tok(T_DIV_EQUAL);
  ".=" tok(T_CONCAT_EQUAL);
  "%=" tok(T_MOD_EQUAL);
  "&=" tok(T_AND_EQUAL);
  "|=" tok(T_OR_EQUAL);
  "^=" tok(T_XOR_EQUAL);
  "<<=" tok(T_SL_EQUAL);
  ">>=" tok(T_SR_EQUAL);
  "||" tok(T_BOOLEAN_OR);
  "&&" tok(T_BOOLEAN_AND);
  "==" tok(T_IS_EQUAL);
  "!="|"<>" tok(T_IS_NOT_EQUAL);
  "===" tok(T_IS_IDENTICAL);
  "!==" tok(T_IS_NOT_IDENTICAL);
  "<=" tok(T_IS_SMALLER_OR_EQUAL);
  ">=" tok(T_IS_GREATER_OR_EQUAL);
  "<<" tok(T_SL);
  ">>" tok(T_SR);
  "++" tok(T_INC);
  "--" tok(T_DEC);
  "->" tok(T_OBJECT_OPERATOR);
  "=>" tok(T_DOUBLE_ARROW);
  "::" tok(T_PAAMAYIM_NEKUDOTAYIM);
  "\\" tok(T_NS_SEPARATOR);
  "??" tok(T_COALESCE);
  ":"{XHPLABEL} {
    int ntt = getNextTokenType(last_token());
    if (ntt & NextTokenType::XhpClassName) {
      yytext++; yyleng--; // skipping ":"
      tok(T_XHP_LABEL);
    }
    yyless(1);
    tok(':');
  }
  "%"{XHPLABEL} {
    int ntt = getNextTokenType(last_token());
    if (ntt & NextTokenType::XhpCategoryName) {
      yytext++; yyleng--; // skipping "%"
      tok(T_XHP_CATEGORY_LABEL);
    }
    yyless(1);
    tok('%');
  }
}

 /* Casts */
<ST_PHP,ST_PHP_NO_RESERVED_WORDS,ST_PHP_NO_RESERVED_WORDS_PERSIST>{
  "("{TABS_AND_SPACES}(int|integer){TABS_AND_SPACES}")" tok(T_INT_CAST);
  "("{TABS_AND_SPACES}(real|double|float){TABS_AND_SPACES}")" tok(T_DOUBLE_CAST);
  "("{TABS_AND_SPACES}string{TABS_AND_SPACES}")" tok(T_STRING_CAST);
  "("{TABS_AND_SPACES}unicode{TABS_AND_SPACES}")" tok(T_UNICODE_CAST);
  "("{TABS_AND_SPACES}binary{TABS_AND_SPACES}")" tok(T_BINARY_CAST);
  "("{TABS_AND_SPACES}array{TABS_AND_SPACES}")" tok(T_ARRAY_CAST);
  "("{TABS_AND_SPACES}object{TABS_AND_SPACES}")" tok(T_OBJECT_CAST);
  "("{TABS_AND_SPACES}(bool|boolean){TABS_AND_SPACES}")" tok(T_BOOL_CAST);
  "("{TABS_AND_SPACES}unset{TABS_AND_SPACES}")" tok(T_UNSET_CAST);
}

 /* Numbers */
<ST_PHP,ST_PHP_NO_RESERVED_WORDS,ST_PHP_NO_RESERVED_WORDS_PERSIST,ST_XHP_ATTR_TYPE_DECL,ST_XHP_IN_TAG>{
  {LNUM}|{HNUM} tok(T_LNUMBER);
  {DNUM}|{EXPONENT_DNUM} tok(T_DNUMBER);
}

 /* Scalars (parsing these doesn't really matter since we just pass them through literally) */
<ST_PHP,ST_PHP_NO_RESERVED_WORDS,ST_PHP_NO_RESERVED_WORDS_PERSIST,ST_XHP_ATTR_TYPE_DECL>{
  {LABEL} tok(T_STRING);
  "$"{LABEL} tok(T_VARIABLE);
  b?'(\\.|\\\n|[^\\']+)*'|b?\"(\\.|\\\n|[^\\\"]+)*\" {
    yy_scan_newlines(yytext, yyg);
    tok(T_CONSTANT_ENCAPSED_STRING);
  }
  `[^`]*` {
    yy_scan_newlines(yytext, yyg);
    tok(T_BACKTICKS_EXPR);
  }
}

<ST_PHP>{TOKENS}   tok(yytext[0]);
<ST_PHP>"{" {
  push_state(ST_PHP);
  tok('{');
}

<ST_PHP>"}"/":"[a-zA-Z_\x7f-\xff] {
  // We need to be robust against a '}' in PHP code with
  // no corresponding '{'
  //struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
  if (yyg->yy_start_stack_ptr) {
    pop_state();
    if (YY_START == ST_PHP) {
      /* If XHP is enabled and "}:" occurs (and "}" does not cause us
         to transition to some state other than ST_IN_SCRIPTING), drop
         into the ST_LOOKING_FOR_COLON state to avoid potentially
         treating ":" as the beginning of an XHP class name */
      BEGIN(ST_LOOKING_FOR_COLON);
    }
  }
  tok('}');
}
<ST_PHP>"}" {
  // We need to be robust against a '}' in PHP code with
  // no corresponding '{'
  //struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
  if (yyg->yy_start_stack_ptr) pop_state();
  tok('}');
}
<ST_LOOKING_FOR_COLON>":" {
  BEGIN(ST_PHP);
  tok(':');
}

 /* XHP */
<ST_PHP>"<"[a-zA-Z_\x7f-\xff] {
  int ntt = getNextTokenType(last_token());

  if (ntt & NextTokenType::XhpTag) {
    yyless(1);
    push_state(ST_XHP_IN_TAG);
    tok(T_XHP_TAG_LT);
  }

  if (ntt & NextTokenType::XhpTagMaybe) {
    // Shift to state state ST_LT_CHECK to do a more extensive check to
    // determine if this is the beginning of an XHP tag.
    yyless(0);
    BEGIN(ST_LT_CHECK);
    YY_BREAK;
  }

  yyless(1);
  tok('<');
}

<ST_LT_CHECK>"<"{XHPLABEL}(">"|"/>"|{WHITESPACE_AND_COMMENTS}(">"|"/>"|[a-zA-Z_\x7f-\xff])) {
  BEGIN(ST_PHP);
  yyless(1);
  push_state(ST_XHP_IN_TAG);
  tok(T_XHP_TAG_LT);
}
<ST_LT_CHECK>"<" {
  BEGIN(ST_PHP);
  tok('<');
}

 /* (HERE|NOW)DOC's */
<ST_PHP,ST_PHP_NO_RESERVED_WORDS,ST_PHP_NO_RESERVED_WORDS_PERSIST>b?"<<<"{TABS_AND_SPACES} {
  push_state(ST_PHP_HEREDOC_START);
  yyextra->heredoc_yyleng = yyleng;
  yymore();
}
<ST_PHP_HEREDOC_START>{
  "'"{LABEL}"'"|\"{LABEL}\" {
    // Create a new string for the heredoc label. Since we're using yymore above
    // yytext will actually start at the "<<<" and not the label. Use of
    // heredoc_yyleng jumps past that. Then we add 1 to get past the " or '. The
    // match is similar to calculate length.
    yyextra->heredoc_label = string(yytext + yyextra->heredoc_yyleng + 1, yyleng - yyextra->heredoc_yyleng - 2);
    set_state(ST_PHP_HEREDOC_NSTART);
    yyextra->heredoc_yyleng = yyleng;
    yymore();
  }
  {LABEL} {
    yyextra->heredoc_label = string(yytext + yyextra->heredoc_yyleng);
    set_state(ST_PHP_HEREDOC_NSTART);
    yyextra->heredoc_yyleng = yyleng;
    yymore();
  }
}
<ST_PHP_HEREDOC_NSTART>{NEWLINE} {
  ++yyextra->lineno;
  yyextra->heredoc_data = yytext + yyleng;
  set_state(ST_PHP_HEREDOC_DATA);
  yymore();
}
<ST_PHP_HEREDOC_DATA>{
  [^\r\n]*{NEWLINE} {
    ++yyextra->lineno;
    set_state(ST_PHP_HEREDOC_NEWLINE);
    yyextra->heredoc_yyleng = yyleng;
    yymore();
  }
}
<ST_PHP_HEREDOC_NEWLINE>{
  {LABEL};?{NEWLINE} {
    if (strncmp(yyextra->heredoc_label.c_str(), yytext + yyextra->heredoc_yyleng, yyextra->heredoc_label.size()) == 0) {
      switch (yytext[yyextra->heredoc_yyleng + yyextra->heredoc_label.size()]) {
        case ';': case '\n': case '\r':
          yyless(yyleng - (yyleng - yyextra->heredoc_yyleng - yyextra->heredoc_label.size()));
          pop_state();
          tok(T_HEREDOC);
      }
    }
    ++yyextra->lineno;
    yyextra->heredoc_yyleng = yyleng;
    yymore();
  }
  [^\r\n]+ {
    set_state(ST_PHP_HEREDOC_DATA);
    yyextra->heredoc_yyleng = yyleng;
    yymore();
  }
  {NEWLINE} {
    ++yyextra->lineno;
    yyextra->heredoc_yyleng = yyleng;
    yymore();
  }
}


 /* XHP */
<ST_XHP_IN_TAG>{
  {XHPLABEL} {
   tok(T_XHP_LABEL);
  }
  "=" {
    tok(yytext[0]);
  }
  ["][^"]*["] {
    // Need not to skipp the quite:
    // yytext++; yyleng--; yytext[yyleng-1] = 0; // skipping "quote"
    tok_consume();
    yylval->xhpDecode();
    tok_return(T_XHP_TEXT);
  }
  [{] {
    push_state(ST_PHP);
    tok('{');
  }
  ">" {
    BEGIN(ST_XHP_CHILD);
    tok(T_XHP_TAG_GT);
  }
  "/>" {
    BEGIN(ST_XHP_END_SINGLETON_TAG);
    yyless(1);
    tok('/');
  }
  {ANY_CHAR} {
    // This rule ensures we get a reasonable syntax error message
    // when unexpected characters occur inside XHP tags
    if (!yyextra->terminated) {
      yyextra->error = string("Invalid entity: (") + yytext + ")";
      yyextra->terminated = true;
    }
    tok(yytext[0]);
  }
}

<ST_XHP_END_SINGLETON_TAG>">" {
  pop_state();
  tok(T_XHP_TAG_GT);
}

<ST_XHP_CHILD>{
  [^{<]+ {
    yy_scan_newlines(yytext, yyg);
    tok_consume();
    if (!yylval->htmlTrim()) {
      YY_BREAK;
    }
    yylval->xhpDecode();
    tok_return(T_XHP_TEXT);
  }
  "{" {
    push_state(ST_PHP);
    tok('{');
  }
  "<" {
    yy_scan_newlines(yytext, yyg);
    push_state(ST_XHP_IN_TAG);
    tok(T_XHP_TAG_LT);
  }
  "</" {
    BEGIN(ST_XHP_END_CLOSE_TAG);
    yyless(yyleng - 1);
    tok(T_XHP_TAG_LT);
  }
}

<ST_XHP_END_CLOSE_TAG>{
  "/" tok('/');
  {XHPLABEL} tok(T_XHP_LABEL);
  ">" { pop_state(); tok(T_XHP_TAG_GT); }
}

<ST_XHP_CHILD>"<!--" {
  push_state(ST_XHP_COMMENT);
  yymore();
}

<ST_XHP_COMMENT>{
  [^-]+ {
    yymore();
  }
  "-->" {
    pop_state();
    tok(T_COMMENT);
  }
  "-" {
    yymore();
  }
}

 /* Other */
<*>{ANY_CHAR} {
  tok(yytext[0]);

  // fix unused function warnings
  yy_top_state(NULL);
  yyunput(0, 0, NULL);
}

%%

#ifdef DEBUG
static const char* yy_state_name(int state) {
  switch (state) {
    case INITIAL:
      return "INITIAL";
    case ST_PHP:
      return "ST_PHP";
    case ST_PHP_COMMENT:
      return "ST_PHP_COMMENT";
    case ST_PHP_EOL_COMMENT:
      return "ST_PHP_EOL_COMMENT";
    case ST_PHP_DOC_COMMENT:
      return "ST_PHP_DOC_COMMENT";
    case ST_PHP_HEREDOC_START:
      return "ST_PHP_HEREDOC_START";
    case ST_PHP_HEREDOC_NSTART:
      return "ST_PHP_HEREDOC_NSTART";
    case ST_PHP_HEREDOC_NEWLINE:
      return "ST_PHP_HEREDOC_NEWLINE";
    case ST_PHP_HEREDOC_DATA:
      return "ST_PHP_HEREDOC_DATA";
    case ST_PHP_NO_RESERVED_WORDS:
      return "ST_PHP_NO_RESERVED_WORDS";
    case ST_PHP_NO_RESERVED_WORDS_PERSIST:
      return "ST_PHP_NO_RESERVED_WORDS_PERSIST";
    case ST_XHP_CHILD:
      return "ST_XHP_CHILD";
    case ST_XHP_ATTR_TYPE_DECL:
      return "ST_XHP_ATTR_TYPE_DECL";
    case XHP_CHILDREN_DECL:
      return "XHP_CHILDREN_DECL";
    case ST_XHP_COMMENT:
      return "ST_XHP_COMMENT";
    default:
      return "???";
  }
}

static void yy_log_token(int tok) {
  const char* tokname = yytokname(tok);
  if (tokname) {
    fprintf(stderr, "--> %s\n", tokname);
  } else {
    fprintf(stderr, "--> '%c'\n", tok);
  }
}
#endif

static int yy_token(int tok, yyguts_t* yyg) {
  if (YY_START == ST_PHP_NO_RESERVED_WORDS) {
    pop_state();
  }

  switch (tok) {
    case T_OPEN_TAG:
    case T_OPEN_TAG_WITH_ECHO:
    case T_OPEN_TAG_FAKE:
      push_state(ST_PHP);
      break;

    case T_CLOSE_TAG:
      pop_state();
      return ';';

    // In PHP it's ok to use keywords such as 'if' as field names
    // or function names.
    case T_OBJECT_OPERATOR:
    case T_PAAMAYIM_NEKUDOTAYIM:
    case T_FUNCTION:
    case T_USE:
    case T_CLASS:
    case T_NS_SEPARATOR:
      push_state(ST_PHP_NO_RESERVED_WORDS);
      break;

    case T_XHP_LABEL:
      if (yyextra->return_all_tokens) {
        yylval->prepend(":");
      }
      break;

  }
#ifdef DEBUG
  yy_log_token(tok);
#endif
  if (shouldReturnToken(tok, false)) yyextra->last_token = tok;
  return tok;
}

static inline void yy_scan_newlines(const char* text, struct yyguts_t* yyg) {
  for (; *text; ++text) {
    if (*text == '\r') {
      if (text[1] == '\n') {
        ++text;
      }
      ++yyextra->lineno;
    } else if (*text == '\n') {
      ++yyextra->lineno;
    }
  }
}

void xhp_new_push_state(int s, struct yyguts_t* yyg) {
#ifdef DEBUG
  fprintf(stderr, "--> PUSH(%s -> %s)\n", yy_state_name(YY_START), yy_state_name(s));
#endif
  yy_push_state(s, yyg);
}

void xhp_new_pop_state(struct yyguts_t* yyg) {
#ifdef DEBUG
  int s = YY_START;
#endif
  yy_pop_state(yyg);
#ifdef DEBUG
  fprintf(stderr, "--> POP(%s -> %s)\n", yy_state_name(s), yy_state_name(YY_START));
#endif
}

void xhp_set_state(int s, struct yyguts_t* yyg) {
#ifdef DEBUG
  fprintf(stderr, "--> SET(%s)\n", yy_state_name(s));
#endif
  BEGIN(s);
}
