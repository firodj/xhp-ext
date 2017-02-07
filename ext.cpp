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

#include "xhp/xhp_preprocess.hpp"
#include "xhp/xhp.hpp"
#include "ext.hpp"
#include "php.h"
#include "php_ini.h"
#include "zend.h"
#include "zend_API.h"
#include "zend_compile.h"
#include "zend_operators.h"
#include "zend_hash.h"
#include "zend_extensions.h"
#include "ext/standard/info.h"
#include <string>
#include <sstream>
#include <iostream>
using namespace std;

//
// Decls
typedef zend_op_array* (zend_compile_file_t)(zend_file_handle*, int TSRMLS_DC);
typedef zend_op_array* (zend_compile_string_t)(zval*, char* TSRMLS_DC);
static zend_compile_file_t* dist_compile_file;
static zend_compile_string_t* dist_compile_string;

typedef struct {
  const char* str;
  size_t pos;
  size_t len;
} xhp_stream_t;

//
// Globals
ZEND_BEGIN_MODULE_GLOBALS(xhp)
  bool idx_expr;
  bool include_debug;
  bool force_global_namespace;
  bool moderate_parse;
ZEND_END_MODULE_GLOBALS(xhp)
ZEND_DECLARE_MODULE_GLOBALS(xhp)

#ifdef ZTS
# define XHPG(i) TSRMG(xhp_globals_id, zend_xhp_globals*, i)
#else
# define XHPG(i) (xhp_globals.i)
#endif

//
// PHP 5.3 helper functions
#if PHP_VERSION_ID >= 50300

// These functions wese made static to zend_stream.c in r255174. These an inline copies of those functions.
static int zend_stream_getc(zend_file_handle *file_handle TSRMLS_DC) {
  char buf;

  if (file_handle->handle.stream.reader(file_handle->handle.stream.handle, &buf, sizeof(buf) TSRMLS_CC)) {
    return (int)buf;
  }
  return EOF;
}

static size_t zend_stream_read(zend_file_handle *file_handle, char *buf, size_t len TSRMLS_DC) {
  if (file_handle->type != ZEND_HANDLE_MAPPED && file_handle->handle.stream.isatty) {
    int c = '*';
    size_t n;

#ifdef NETWARE
    /*
      c != 4 check is there as fread of a character in NetWare LibC gives 4 upon ^D character.
      Ascii value 4 is actually EOT character which is not defined anywhere in the LibC
      or else we can use instead of hardcoded 4.
    */
    for (n = 0; n < len && (c = zend_stream_getc(file_handle TSRMLS_CC)) != EOF && c != 4 && c != '\n'; ++n) {
#else
    for (n = 0; n < len && (c = zend_stream_getc(file_handle TSRMLS_CC)) != EOF && c != '\n'; ++n)  {
#endif
        buf[n] = (char)c;
    }
    if (c == '\n') {
      buf[n++] = (char)c;
    }

    return n;
  }
  return file_handle->handle.stream.reader(file_handle->handle.stream.handle, buf, len TSRMLS_CC);
}
#endif

//
// XHP Streams
size_t xhp_stream_reader(xhp_stream_t* handle, char* buf, size_t len TSRMLS_DC) {
  if (len > handle->len - handle->pos) {
    len = handle->len - handle->pos;
  }
  if (len) {
    memcpy(buf, handle->str + handle->pos, len);
    buf[len] = 0;
    handle->pos += len;
    return len;
  } else {
    return 0;
  }
}

long xhp_stream_fteller(xhp_stream_t* handle TSRMLS_DC) {
  return (long)handle->pos;
}

//
// PHP compilation intercepter
static zend_op_array* xhp_compile_file(zend_file_handle* f, int type TSRMLS_DC) {

  if (!f || open_file_for_scanning(f TSRMLS_CC) == FAILURE) {
    // If opening the file fails just send it to the original func
    return dist_compile_file(f, type TSRMLS_CC);
  }

  // Grab code from zend file handle
  string original_code;
#if PHP_VERSION_ID >= 50300
  if (f->type == ZEND_HANDLE_MAPPED) {
    original_code = f->handle.stream.mmap.buf;
  }
#else
  if (0);
#endif
  else {

    // Read full program from zend stream
    char read_buf[4096];
    size_t len;
    while (len = zend_stream_read(f, (char*)&read_buf, 4095 TSRMLS_CC)) {
      read_buf[len] = 0;
      original_code += read_buf;
    }
  }

  // Process XHP
  XHPResult result;
  xhp_flags_t flags;
  string rewrit, error_str;
  uint32_t error_lineno;
  string* code_to_give_to_php;

  memset(&flags, 0, sizeof(xhp_flags_t));
  flags.short_tags = 0;
  flags.idx_expr = XHPG(idx_expr);
  flags.include_debug = XHPG(include_debug);
#if PHP_VERSION_ID >= 50300
  flags.force_global_namespace = XHPG(force_global_namespace);
#else
  flags.force_global_namespace = false;
#endif
  flags.moderate_parse = XHPG(moderate_parse);
  result = xhp_preprocess(original_code, rewrit, error_str, error_lineno, flags);

  if (result == XHPErred) {
    // Bubble error up to PHP
    CG(in_compilation) = true;
    CG(zend_lineno) = error_lineno;
    zend_string *str = zend_string_init(f->filename, strlen(f->filename), 0);
    zend_set_compiled_filename(str  TSRMLS_CC);
    zend_string_release(str);
    zend_error(E_PARSE, "%s", error_str.c_str());
    zend_bailout();
  } else if (result == XHPRewrote) {
    code_to_give_to_php = &rewrit;
  } else {
    code_to_give_to_php = &original_code;
  }

  // Create a fake file to give back to PHP to handle
  zend_file_handle fake_file;
  memset(&fake_file, 0, sizeof(zend_file_handle));

  fake_file.type = ZEND_HANDLE_MAPPED;

  fake_file.opened_path = f->opened_path ? zend_string_copy(f->opened_path) : NULL;
  fake_file.filename = f->filename;
  fake_file.free_filename = false;

  fake_file.handle.stream.isatty = 0;
  fake_file.handle.stream.mmap.pos = 0;
  fake_file.handle.stream.mmap.buf = const_cast<char*>(code_to_give_to_php->c_str());
  fake_file.handle.stream.mmap.len = code_to_give_to_php->size();
  fake_file.handle.stream.closer = NULL;

  // TODO: should check for bailout
  zend_op_array* ret = dist_compile_file(&fake_file, type TSRMLS_CC);

  zend_destroy_file_handle(&fake_file);

  return ret;
}

static zend_op_array* xhp_compile_string(zval* str, char *filename TSRMLS_DC) {

  // Cast to str
  zval tmp;
  char* val;
  if (Z_TYPE_P(str) != IS_STRING) {
    tmp = *str;
    zval_copy_ctor(&tmp);
    convert_to_string(&tmp);
    val = Z_STRVAL(tmp);
  } else {
    val = Z_STRVAL_P(str);
  }

  // Process XHP
  string rewrit, error_str;
  string* code_to_give_to_php;
  uint32_t error_lineno;
  string original_code(val);
  xhp_flags_t flags;

  memset(&flags, 0, sizeof(xhp_flags_t));
  flags.short_tags = 0;
  flags.idx_expr = XHPG(idx_expr);
  flags.include_debug = XHPG(include_debug);
  flags.force_global_namespace = XHPG(force_global_namespace);
  flags.eval = true;
  flags.moderate_parse = false;
  XHPResult result = xhp_preprocess(original_code, rewrit, error_str, error_lineno, flags);

  // Destroy temporary in the case of non-string input (why?)
  if (Z_TYPE_P(str) != IS_STRING) {
    zval_dtor(&tmp);
  }

  if (result == XHPErred) {

    // Bubble error up to PHP
    bool original_in_compilation = CG(in_compilation);
    CG(in_compilation) = true;
    CG(zend_lineno) = error_lineno;
    zend_error(E_PARSE, "%s", error_str.c_str());
    CG(unclean_shutdown) = 1;
    CG(in_compilation) = original_in_compilation;
    return NULL;
  } else if (result == XHPRewrote) {

    // Create another tmp zval with the rewritten PHP code and pass it to the original function
    ZVAL_STRINGL(&tmp, rewrit.c_str(), rewrit.size());
    zend_op_array* ret = dist_compile_string(&tmp, filename TSRMLS_CC);
    zval_dtor(&tmp);
    return ret;
  } else {
    return dist_compile_string(str, filename TSRMLS_CC);
  }
}

//
// tokenize
static zend_bool tokenize(zval *return_value, zend_string *source)
{
  array_init(return_value);

  char *val = ZSTR_VAL(source);
  string in(val);

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
  int tok;
  size_t lineno;

  while (tok = xhplex(&new_code, scanner)) {
    size_t lineno = extra.lineno;
    code_str = (char*)new_code.c_str();
    string code_s(code_str);

    if (tok >= 256) {
        zval keyword;
        array_init(&keyword);
        add_next_index_long(&keyword, tok);
        add_next_index_stringl(&keyword, code_s.c_str(), code_s.length());
        add_next_index_long(&keyword, lineno);
        add_next_index_zval(return_value, &keyword);
    } else {
        if (tok >= 0x20 && tok <= 0x7E) {
            add_next_index_stringl(return_value, code_s.c_str(), code_s.length());
        } else {
            add_next_index_long(return_value, tok);
        }
    }
  }

  xhplex_destroy(scanner);
  ////xhp_destroy_lexical_state(lex_state);

  return 1;
}

void xhp_tokenizer_register_constants(INIT_FUNC_ARGS);

//
// globals initialization
static void php_xhp_init_globals(zend_xhp_globals* xhp_globals) {
  xhp_globals->idx_expr = false;
  xhp_globals->include_debug = true;
  xhp_globals->force_global_namespace = true;
}

//
// ini entry
PHP_INI_BEGIN()
  STD_PHP_INI_BOOLEAN("xhp.idx_expr", "0", PHP_INI_PERDIR, OnUpdateBool, idx_expr, zend_xhp_globals, xhp_globals)
  STD_PHP_INI_BOOLEAN("xhp.include_debug", "1", PHP_INI_PERDIR, OnUpdateBool, include_debug, zend_xhp_globals, xhp_globals)
  STD_PHP_INI_BOOLEAN("xhp.force_global_namespace", "1", PHP_INI_PERDIR, OnUpdateBool, force_global_namespace, zend_xhp_globals, xhp_globals)
  STD_PHP_INI_BOOLEAN("xhp.moderate_parse", "1", PHP_INI_PERDIR, OnUpdateBool, moderate_parse, zend_xhp_globals, xhp_globals)
PHP_INI_END()

//
// Extension entry
static PHP_MINIT_FUNCTION(xhp) {

  ZEND_INIT_MODULE_GLOBALS(xhp, php_xhp_init_globals, NULL);

  REGISTER_INI_ENTRIES();

#ifndef ZEND_ENGINE_3
  // APC has this crazy magic api you can use to avoid the race condition for when an extension overwrites
  // the compile_file function. The desired order here is APC -> XHP -> PHP, that way APC can cache the
  // file as usual.
  zend_module_entry *apc_lookup;
  zend_constant *apc_magic;
  if (zend_hash_str_find(&module_registry, "apc", sizeof("apc")-1 /* (void**)&apc_lookup*/) != NULL &&
      zend_hash_str_find(EG(zend_constants), "\000apc_magic", 10 /* (void**)&apc_magic */) != NULL) {
    zend_compile_file_t* (*apc_set_compile_file)(zend_compile_file_t*) = (zend_compile_file_t* (*)(zend_compile_file_t*))apc_magic->value.value.lval;
    dist_compile_file = apc_set_compile_file(NULL);
    apc_set_compile_file(xhp_compile_file);
  } else
#endif
  {
    dist_compile_file = zend_compile_file;
    zend_compile_file = xhp_compile_file;
  }

  xhp_tokenizer_register_constants(INIT_FUNC_ARGS_PASSTHRU);

  // For eval
  dist_compile_string = zend_compile_string;
  zend_compile_string = xhp_compile_string;
  return SUCCESS;
}

static PHP_MSHUTDOWN_FUNCTION(xhp) {
  UNREGISTER_INI_ENTRIES();
  return SUCCESS;
}

//
// phpinfo();
static PHP_MINFO_FUNCTION(xhp) {
  php_info_print_table_start();
  php_info_print_table_row(2, "Version", PHP_XHP_VERSION);
  php_info_print_table_row(2, "Include Debug Info Into XHP Classes", XHPG(include_debug) ? "enabled" : "disabled");
  php_info_print_table_row(2, "Manual Support For func_call()['key'] Syntax", XHPG(idx_expr) ? "enabled" : "disabled");
  php_info_print_table_row(2, "Force XHP Into The Global Namespace", XHPG(force_global_namespace) ? "enabled" : "disabled");
  php_info_print_table_row(2, "XHP Parser in Moderate", XHPG(moderate_parse) ? "enabled" : "disabled");
  php_info_print_table_end();
}

//
// __xhp_idx
ZEND_FUNCTION(__xhp_idx) {
  zval *dict, *offset;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &dict, &offset) == FAILURE) {
    RETURN_NULL();
  }

  switch (Z_TYPE_P(dict)) {
    //
    // These are always NULL
    case IS_NULL:
    case IS_TRUE:
    case IS_FALSE:
    case IS_LONG:
    case IS_DOUBLE:
    default:
      RETURN_NULL();
      break;

    //
    // array()[] -- Array index
    case IS_ARRAY:
      switch (Z_TYPE_P(offset)) {
        case IS_RESOURCE:
          zend_error(E_STRICT, "Resource ID#%ld used as offset, casting to integer (%ld)", Z_LVAL_P(offset), Z_LVAL_P(offset));
          /* Fall Through */
        case IS_DOUBLE:
        case IS_TRUE:
        case IS_FALSE:
        case IS_LONG:
          long loffset;
          zval *value;
          if (Z_TYPE_P(offset) == IS_DOUBLE) {
            loffset = (long)Z_DVAL_P(offset);
          } else if (Z_TYPE_P(offset) == IS_TRUE) {
            loffset = 1;
          } else if (Z_TYPE_P(offset) == IS_FALSE) {
            loffset = 0;
          } else {
            loffset = Z_LVAL_P(offset);
          }
          value = zend_hash_index_find(Z_ARRVAL_P(dict), loffset);
          if (value != NULL) {
            *return_value = *value;
            break;
          }
          zend_error(E_NOTICE, "Undefined offset:  %ld", loffset);
          RETURN_NULL();
          break;

        case IS_STRING:
          value = zend_symtable_find(Z_ARRVAL_P(dict), Z_STR_P(offset));
          if (value != NULL) {
            *return_value = *value;
            break;
          }
          zend_error(E_NOTICE, "Undefined index:  %s", Z_STRVAL(*offset));
          RETURN_NULL();
          break;

        case IS_NULL:
          value = zend_hash_str_find(Z_ARRVAL_P(dict), "", sizeof("") - 1);
          if (value != NULL) {
            *return_value = *value;
            break;
          }
          zend_error(E_NOTICE, "Undefined index:  ");
          RETURN_NULL();
          break;

        default:
          zend_error(E_WARNING, "Illegal offset type");
          RETURN_NULL();
          break;
      }
      break;

    //
    // 'string'[] -- String offset
    case IS_STRING:
      long loffset;
      switch (Z_TYPE_P(offset)) {
        case IS_LONG:
        case IS_TRUE:
          loffset = Z_LVAL_P(offset);
          break;

        case IS_DOUBLE:
          loffset = (long)Z_DVAL_P(offset);
          break;

        case IS_FALSE:
        case IS_NULL:
          loffset = 0;
          break;

        case IS_STRING: {
          zval tmp = *offset;
          zval_copy_ctor(&tmp);
          convert_to_long(&tmp);
          loffset = Z_LVAL(tmp);
          zval_dtor(&tmp);
          break;
        }

        default:
          zend_error(E_WARNING, "Illegal offset type");
          RETURN_NULL();
          break;
      }
      if (loffset < 0 || Z_STRLEN_P(dict) <= loffset) {
        zend_error(E_NOTICE, "Uninitialized string offset: %ld", loffset);
        RETURN_NULL();
      }
      RETURN_STRINGL(Z_STRVAL_P(dict) + loffset, 1);
      break;

    //
    // (new foo)[] -- Object overload (ArrayAccess)
    case IS_OBJECT:
      if (!Z_OBJ_HT_P(dict)->read_dimension) {
        zend_error(E_ERROR, "Cannot use object as array");
        RETURN_NULL();
      } else {
        zval rv;
        zval* overloaded_result = Z_OBJ_HT_P(dict)->read_dimension(dict, offset, BP_VAR_R, &rv /* TSRMLS_CC */);
        if (overloaded_result) {
          *return_value = *overloaded_result;
        } else {
          RETURN_NULL();
        }
      }
      break;
  }
  zval_copy_ctor(return_value);
}

//
// xhp_preprocess_code
ZEND_FUNCTION(xhp_preprocess_code) {
  // Parse zend params
  char *code;
  size_t code_len;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &code, &code_len) == FAILURE) {
    RETURN_NULL();
  }
  string rewrit, error;
  uint32_t error_line;

  // Mangle code
  string code_str(code, code_len);
  XHPResult result = xhp_preprocess(code_str, rewrit, false, error, error_line);

  // Build return code
  array_init(return_value);
  if (result == XHPErred) {
    add_assoc_string(return_value, "error", const_cast<char*>(error.c_str()));
    add_assoc_long(return_value, "error_line", error_line);
  } else if (result == XHPRewrote) {
    add_assoc_string(return_value, "new_code", const_cast<char*>(rewrit.c_str()));
  }
}

//
// xhp_token_get_all
PHP_FUNCTION(xhp_token_get_all) {
  zend_string *source;
  zend_bool success;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &source) == FAILURE) {
    return;
  }

  success = tokenize(return_value, source);

  if (!success) RETURN_FALSE;
}

//
// xhp_token_name
PHP_FUNCTION(xhp_token_name) {
  zend_long type;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &type) == FAILURE) {
    return;
  }

  RETVAL_STRING(xhp_get_token_type_name(type));
}

// xhp_rename_function
PHP_FUNCTION(xhp_rename_function)
{
  zend_string *orig_fname, *new_fname;
  zval *func;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "SS", &orig_fname, &new_fname) == FAILURE) {
    return;
  }

  func = zend_hash_find(EG(function_table), orig_fname);
  if (func == NULL) {
    zend_error(E_WARNING, "%s() failed: original '%s' does not exist!",
                 get_active_function_name(TSRMLS_C),ZSTR_VAL(orig_fname));
    RETURN_FALSE;
  }

  // zend_error(E_WARNING, "name: %s", ZSTR_VAL(Z_FUNC_P(func)->common.function_name));

  if (zend_hash_exists(EG(function_table), new_fname)) {
    zend_error(E_WARNING, "%s() failed: new '%s' already exist!",
                 get_active_function_name(TSRMLS_C),ZSTR_VAL(new_fname));
    RETURN_FALSE;
  }

  if (zend_hash_add(EG(function_table), new_fname, func) == NULL) {
    zend_error(E_WARNING, "%s() failed to insert '%s' into function table",
                 get_active_function_name(TSRMLS_C), ZSTR_VAL(new_fname));
    RETURN_FALSE;
  }

  if(zend_hash_del(EG(function_table), orig_fname) == FAILURE) {
    zend_error(E_WARNING, "%s() failed to remove '%s' from function table",
              get_active_function_name(TSRMLS_C),
              ZSTR_VAL(orig_fname));

    zend_hash_del(EG(function_table), new_fname);
    RETURN_FALSE;
  }

  RETURN_TRUE;
}

//
// Module description
zend_function_entry xhp_functions[] = {
  ZEND_FE(__xhp_idx, NULL)
  ZEND_FE(xhp_preprocess_code, NULL)
  PHP_FE(xhp_token_get_all, NULL)
  PHP_FE(xhp_token_name, NULL)
  PHP_FE(xhp_rename_function, NULL)
  {NULL, NULL, NULL}
};

zend_module_entry xhp_module_entry = {
  STANDARD_MODULE_HEADER,
  PHP_XHP_EXTNAME,
  xhp_functions,
  PHP_MINIT(xhp),
  PHP_MSHUTDOWN(xhp),
  NULL,
  NULL,
  PHP_MINFO(xhp),
  PHP_XHP_VERSION,
  STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_XHP
ZEND_GET_MODULE(xhp)
#endif
