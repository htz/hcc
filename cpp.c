// Copyright 2019 @htz. Released under the MIT license.

#include <string.h>
#include "hcc.h"

static void preprocessor_define(parse_t *parse);
static void preprocessor_undef(parse_t *parse);
static vector_t *preprocessor_if(parse_t *parse, bool cond);
static vector_t *preprocessor_ifdef(parse_t *parse);
static vector_t *preprocessor_ifndef(parse_t *parse);
static void preprocessor_skip_body(parse_t *parse);
static vector_t *preprocessor_if_body(parse_t *parse, bool cond);
static vector_t *preprocessor_else(parse_t *parse, bool cond);
static token_t *cpp_get_token_new_line(parse_t *parse);

static token_t *macro_param(parse_t *parse, int position, bool is_vaargs) {
  token_t *arg = token_new(parse->lex, TOKEN_MACRO_PARAM);
  arg->position = position;
  arg->is_vaargs = is_vaargs;
  return arg;
}

static macro_t *read_macro(parse_t *parse) {
  macro_t *macro = macro_new();
  token_t *token = lex_next_token_is(parse->lex, TOKEN_KIND_KEYWORD);
  if (token == NULL || token->keyword != '(' || token->is_space) {
    if (token != NULL) {
      lex_unget_token(parse->lex, token);
    }
  } else {
    macro->args = map_new();
    if (!lex_next_keyword_is(parse->lex, ')')) {
      for (;;) {
        token = lex_get_token(parse->lex);
        if (token->kind == TOKEN_KIND_IDENTIFIER) {
          if (map_get(macro->args, token->identifier) != NULL) {
            errorf("duplicate macro parameter name '%s'", token->identifier);
          }
          token_t *arg = macro_param(parse, macro->args->size, false);
          map_add(macro->args, token->identifier, arg);
          if (lex_next_keyword_is(parse->lex, ')')) {
            break;
          }
          lex_expect_keyword_is(parse->lex, ',');
          continue;
        }
        if (token->kind == TOKEN_KIND_KEYWORD && token->keyword == TOKEN_KEYWORD_ELLIPSIS) {
          macro->is_vaargs = true;
          token_t *arg = macro_param(parse, macro->args->size, true);
          map_add(macro->args, "__VA_ARGS__", arg);
          lex_expect_keyword_is(parse->lex, ')');
          break;
        }
        errorf("invalid token in macro parameter list");
      }
    }
  }
  for (;;) {
     token_t *token = lex_get_token(parse->lex);
     if (token->kind == TOKEN_KIND_NEWLINE || token->kind == TOKEN_KIND_EOF) {
       break;
     }
     vector_push(macro->tokens, token);
  }
  return macro;
}

static void preprocessor_define(parse_t *parse) {
  token_t *name = lex_expect_token_is(parse->lex, TOKEN_KIND_IDENTIFIER);
  macro_t *macro = read_macro(parse);
  macro_t *old_macro = (macro_t *)map_get(parse->macros, name->identifier);
  if (old_macro != NULL) {
    warnf("'%s macro redefined", name->identifier);
    macro_free(old_macro);
    map_delete(parse->macros, name->identifier);
  }
  map_add(parse->macros, name->identifier, macro);
}

static void preprocessor_undef(parse_t *parse) {
  token_t *name = lex_expect_token_is(parse->lex, TOKEN_KIND_IDENTIFIER);
  macro_t *macro = (macro_t *)map_get(parse->macros, name->identifier);
  if (macro != NULL) {
    macro_free(macro);
  }
  map_delete(parse->macros, name->identifier);
  token_t *token = lex_get_token(parse->lex);
  if (token->kind != TOKEN_KIND_NEWLINE && token->kind != TOKEN_KIND_EOF) {
    warnf("extra tokens at end of #undef directive");
    do {
      token = lex_get_token(parse->lex);
    } while (token->kind != TOKEN_KIND_NEWLINE && token->kind != TOKEN_KIND_EOF);
  }
}

static bool read_defined_op(parse_t *parse) {
  token_t *name;
  if (lex_next_keyword_is(parse->lex, '(')) {
    name = lex_expect_token_is(parse->lex, TOKEN_KIND_IDENTIFIER);
    lex_expect_keyword_is(parse->lex, ')');
  } else {
    name = lex_expect_token_is(parse->lex, TOKEN_KIND_IDENTIFIER);
  }
  return map_get(parse->macros, name->identifier) != NULL;
}

static bool read_constexpr(parse_t *parse) {
  vector_t *tokens = vector_new();
  for (int i = 0;; i++) {
    token_t *token = cpp_get_token_new_line(parse);
    if (token->kind == TOKEN_KIND_NEWLINE) {
      break;
    } else if (token->kind == TOKEN_KIND_IDENTIFIER) {
      if (strcmp("defined", token->identifier) == 0) {
        token = token_new(parse->lex, TOKEN_KIND_INT);
        token->ival = read_defined_op(parse);
      } else {
        token = token_new(parse->lex, TOKEN_KIND_INT);
        token->ival = 0;
      }
    }
    vector_push(tokens, token);
  }
  vector_push(tokens, token_new(parse->lex, TOKEN_KIND_EOF));
  for (int i = tokens->size - 1; i >= 0; i--) {
    token_t *token = (token_t *)tokens->data[i];
    lex_unget_token(parse->lex, token);
  }
  node_t *cond = parse_constant_expression(parse);
  cpp_next_token_is(parse, TOKEN_KIND_EOF);
  assert(cond->kind == NODE_KIND_LITERAL);
  if (type_is_int(cond->type)) {
    return cond->ival != 0;
  }
  if (type_is_float(cond->type)) {
    return cond->fval != 0;
  }
  return true;
}

static vector_t *preprocessor_if(parse_t *parse, bool cond) {
  cond = !cond && read_constexpr(parse);
  return preprocessor_if_body(parse, cond);
}

static vector_t *preprocessor_ifdef(parse_t *parse) {
  token_t *name = lex_expect_token_is(parse->lex, TOKEN_KIND_IDENTIFIER);
  lex_expect_token_is(parse->lex, TOKEN_KIND_NEWLINE);
  bool cond = map_get(parse->macros, name->identifier) != NULL;
  return preprocessor_if_body(parse, cond);
}

static vector_t *preprocessor_ifndef(parse_t *parse) {
  token_t *name = lex_expect_token_is(parse->lex, TOKEN_KIND_IDENTIFIER);
  lex_expect_token_is(parse->lex, TOKEN_KIND_NEWLINE);
  bool cond = map_get(parse->macros, name->identifier) == NULL;
  return preprocessor_if_body(parse, cond);
}

static void preprocessor_skip_body(parse_t *parse) {
  int nest = 0;
  for (;;) {
    bool bol = parse->lex->column == 1;
    lex_skip_whitespace(parse->lex);
    char c = lex_get_char(parse->lex);
    if (c == '\0') {
      break;
    }
    if (c == '\'') {
      lex_skip_char(parse->lex);
      continue;
    }
    if (c == '"') {
      lex_skip_string(parse->lex);
      continue;
    }
    if (c != '#' || !bol) {
      continue;
    }
    lex_unget_char(parse->lex, c);
    token_t *sharp_token = lex_expect_keyword_is(parse->lex, '#');
    token_t *token = lex_get_token(parse->lex);
    if (nest == 0 && (strcmp("else", token->str) == 0 || strcmp("elif", token->str) == 0 || strcmp("endif", token->str) == 0)) {
      lex_unget_token(parse->lex, token);
      lex_unget_token(parse->lex, sharp_token);
      break;
    }
    if (strcmp("if", token->str) == 0 || strcmp("ifdef", token->str) == 0 || strcmp("ifndef", token->str) == 0) {
      nest++;
    } else if (strcmp("endif", token->str) == 0) {
      nest--;
    }
    lex_skip_line(parse->lex);
  }
  if (nest > 0) {
    errorf("unterminated conditional directive");
  }
}

static vector_t *preprocessor_if_body(parse_t *parse, bool cond) {
  vector_t *tokens = NULL;
  if (cond) {
    tokens = vector_new();
    for (;;) {
      token_t *token = cpp_get_token(parse);
      if (token->kind == TOKEN_KIND_EOF) {
        break;
      }
      vector_push(tokens, token);
    }
  } else {
    preprocessor_skip_body(parse);
  }

  lex_next_keyword_is(parse->lex, '#');
  token_t *token = lex_get_token(parse->lex);
  assert(token->str != NULL);
  if (strcmp("elif", token->str) == 0) {
    vector_t *tmp = preprocessor_if(parse, cond);
    if (!cond) {
      assert(tokens == NULL && tmp != NULL);
      tokens = tmp;
    }
  } else if (strcmp("else", token->str) == 0) {
    vector_t *tmp = preprocessor_else(parse, cond);
    if (!cond) {
      assert(tokens == NULL && tmp != NULL);
      tokens = tmp;
    }
  } else {
    assert(strcmp("endif", token->str) == 0);
  }
  if (tokens == NULL) {
    tokens = vector_new();
  }
  return tokens;
}

static vector_t *preprocessor_else(parse_t *parse, bool cond) {
  cond = !cond;

  vector_t *tokens = NULL;
  if (cond) {
    tokens = vector_new();
    for (;;) {
      token_t *token = cpp_get_token(parse);
      if (token->kind == TOKEN_KIND_EOF) {
        break;
      }
      vector_push(tokens, token);
    }
  } else {
    preprocessor_skip_body(parse);
  }

  lex_next_keyword_is(parse->lex, '#');
  token_t *token = lex_get_token(parse->lex);
  assert(token->str != NULL);
  assert(strcmp("endif", token->str) == 0);
  return tokens;
}

static token_t *preprocessor(parse_t *parse, token_t *hash_token) {
  if (hash_token->kind != TOKEN_KIND_KEYWORD || hash_token->keyword != '#' || hash_token->column > 1) {
    return hash_token;
  }

  token_t *token = lex_get_token(parse->lex);
  if (token->str == NULL) {
    lex_unget_token(parse->lex, token);
    return NULL;
  } else if (strcmp("define", token->str) == 0) {
    preprocessor_define(parse);
    return NULL;
  } else if (strcmp("undef", token->str) == 0) {
    preprocessor_undef(parse);
    return NULL;
  } else if (strcmp("", token->str) == 0) {
    return NULL;
  } else {
    vector_t *tokens = NULL;
    if (strcmp("if", token->str) == 0) {
      tokens = preprocessor_if(parse, false);
    } else if (strcmp("ifdef", token->str) == 0) {
      tokens = preprocessor_ifdef(parse);
    } else if (strcmp("ifndef", token->str) == 0) {
      tokens = preprocessor_ifndef(parse);
    }
    if (tokens != NULL) {
      for (int i = tokens->size - 1; i >= 0; i--) {
        token_t *token = (token_t *)tokens->data[i];
        lex_unget_token(parse->lex, token);
      }
      vector_free(tokens);
      return NULL;
    }
  }
  lex_unget_token(parse->lex, token);

  if (
    strcmp("elif", token->str) == 0 ||
    strcmp("else", token->str) == 0 ||
    strcmp("endif", token->str) == 0
  ) {
    lex_unget_token(parse->lex, hash_token);
    return token_new(parse->lex, TOKEN_KIND_EOF);
  } else {
    errorf("invalid preprocessing directive: %s", token->str);
  }
  return NULL;
}

static vector_t *expand_tokens(parse_t *parse, vector_t *tokens, bool is_space) {
  lex_unget_token(parse->lex, token_new(parse->lex, TOKEN_KIND_EOF));
  for (int i = tokens->size - 1; i >= 0; i--) {
    token_t *token = (token_t *)tokens->data[i];
    lex_unget_token(parse->lex, token);
  }
  vector_t *expanded_tokens = vector_new();
  for (int i = 0;; i++) {
    token_t *token = cpp_get_token(parse);
    if (token->kind == TOKEN_KIND_EOF) {
      break;
    }
    token = token_dup(parse->lex, token);
    if (i == 0) {
      token->is_space = is_space;
    }
    vector_push(expanded_tokens, token);
  }
  return expanded_tokens;
}

static token_t *stringize_tokens(parse_t *parse, vector_t *tokens) {
  token_t *token = token_new(parse->lex, TOKEN_KIND_STRING);
  string_t *str = string_new();
  for (int i = 0; i < tokens->size; i++) {
    token_t *t = (token_t *)tokens->data[i];
    if (i > 0 && t->is_space) {
      string_add(str, ' ');
    }
    string_append(str, t->str);
  }
  token->sval = str;
  return token;
}

static token_t *glue_tokens(parse_t *parse, token_t *before_token, token_t *token) {
  string_t *token_str = string_new();
  string_appendf(token_str, "%s%s", before_token->str, token->str);
  lex_t *lex = lex_new_string(token_str);
  token_t *glue_token = lex_get_token(lex);
  if (lex_get_token(lex)->kind != TOKEN_KIND_EOF) {
    errorf("unconsumed glued token");
  }
  glue_token = token_dup(parse->lex, glue_token);
  glue_token->line = before_token->line;
  glue_token->column = before_token->column;
  glue_token->is_space = before_token->is_space;
  glue_token->hideset = NULL;
  lex_free(lex);
  return glue_token;
}

static vector_t *subst(parse_t *parse, macro_t *macro, vector_t *args, vector_t *hideset) {
  vector_t *tokens = vector_new();
  for (int i = 0; i < macro->tokens->size; i++) {
    token_t *token = (token_t *)macro->tokens->data[i];
    token_t *next_token = NULL;
    if (i + 1 < macro->tokens->size) {
      next_token = (token_t *)macro->tokens->data[i + 1];
    }
    if (token->kind == TOKEN_KIND_KEYWORD && token->keyword == '#' && next_token != NULL && next_token->kind == TOKEN_KIND_IDENTIFIER) {
      token_t *arg = NULL;
      if (macro->args != NULL) {
        arg = map_get(macro->args, next_token->identifier);
      }
      if (arg != NULL) {
        vector_t *arg_tokens = args->data[arg->position];
        token_t *str_token = stringize_tokens(parse, arg_tokens);
        str_token->is_space = token->is_space;
        vector_push(tokens, str_token);
        i++;
        continue;
      }
    } else if (token->kind == TOKEN_KIND_KEYWORD && token->keyword == OP_HASHHASH && next_token != NULL) {
      token_t *arg = NULL;
      if (next_token->kind == TOKEN_KIND_IDENTIFIER) {
        arg = map_get(macro->args, next_token->identifier);
      }
      if (arg != NULL) {
        vector_t *arg_tokens = (vector_t *)args->data[arg->position];
        token_t *tail_token = NULL;
        if (tokens->size > 0) {
          tail_token = (token_t *)tokens->data[tokens->size - 1];
        }
        if (arg->is_vaargs && tail_token != NULL && tail_token->kind == TOKEN_KIND_KEYWORD && tail_token->keyword == ',') {
          if (arg_tokens->size > 0) {
            vector_push(tokens, arg);
          } else {
            vector_pop(tokens);
          }
        } else {
          if (arg_tokens->size > 0) {
            token_t *before_token = (token_t *)vector_pop(tokens);
            token_t *glue_token = glue_tokens(parse, before_token, (token_t *)arg_tokens->data[0]);
            vector_push(tokens, glue_token);
            for (int j = 1; j < arg_tokens->size; j++) {
              vector_push(tokens, (token_t *)arg_tokens->data[j]);
            }
          }
        }
      } else {
        token_t *before_token = (token_t *)vector_pop(tokens);
        token_t *glue_token = glue_tokens(parse, before_token, next_token);
        vector_push(tokens, glue_token);
      }
      i++;
      continue;
    } else if (token->kind == TOKEN_KIND_IDENTIFIER) {
      token_t *arg = NULL;
      if (macro->args != NULL) {
        arg = map_get(macro->args, token->identifier);
      }
      if (arg != NULL) {
        if (next_token != NULL && next_token->kind == TOKEN_KIND_KEYWORD && next_token->keyword == OP_HASHHASH) {
          vector_t *arg_tokens = args->data[arg->position];
          if (arg_tokens->size == 0) {
            i++;
          } else {
            for (int j = 0; j < arg_tokens->size; j++) {
              vector_push(tokens, (token_t *)arg_tokens->data[j]);
            }
          }
          continue;
        }
        vector_t *arg_tokens = (vector_t *)args->data[arg->position];
        vector_t *expanded_tokens = expand_tokens(parse, arg_tokens, token->is_space);
        for (int j = 0; j < expanded_tokens->size; j++) {
          vector_push(tokens, (token_t *)expanded_tokens->data[j]);
        }
        vector_free(expanded_tokens);
        continue;
      }
    }
    vector_push(tokens, token);
  }
  for (int i = 0; i < tokens->size; i++) {
    token_t *org_token = (token_t *)tokens->data[i];
    token_t *token = token_dup(parse->lex, org_token);
    if (hideset != NULL) {
      token->hideset = vector_dup(hideset);
    }
    token_union_hideset(token, org_token->hideset);
    token_add_hideset(token, macro);
    tokens->data[i] = token;
  }
  return tokens;
}

static void expand_macro(parse_t *parse, macro_t *macro, vector_t *args, vector_t *hideset, bool is_space) {
  vector_t *tokens = subst(parse, macro, args, hideset);
  if (macro->args != NULL) {
    assert(args != NULL);
    if (args->size > macro->args->size) {
      errorf("too many arguments provided to function-like macro invocation");
    } else if (args->size < macro->args->size) {
      errorf("too few arguments provided to function-like macro invocation");
    }
  }
  for (int i = tokens->size - 1; i >= 0; i--) {
    token_t *token = (token_t *)tokens->data[i];
    if (i == 0) {
      token->is_space = is_space;
    }
    lex_unget_token(parse->lex, token);
  }
  vector_free(tokens);
}

static vector_t *function_list_arg(parse_t *parse, bool is_vaargs) {
  vector_t *tokens = vector_new();
  int level = 0;
  for (;;) {
    token_t *token = lex_get_token(parse->lex);
    if (token->kind == TOKEN_KIND_EOF) {
      errorf("unterminated function-like macro invocation");
    }
    if (token->kind == TOKEN_KIND_NEWLINE) {
      continue;
    }
    if (token->kind != TOKEN_KIND_KEYWORD) {
      vector_push(tokens, token);
      continue;
    }
    if (level == 0) {
      if (token->keyword == ')') {
        lex_unget_token(parse->lex, token);
        break;
      }
      if (!is_vaargs && token->keyword == ',') {
        lex_unget_token(parse->lex, token);
        break;
      }
    }
    if (token->keyword == '(') {
      level++;
    } else if (token->keyword == ')') {
      level--;
    }
    vector_push(tokens, token);
  }
  return tokens;
}

static vector_t *function_like_args(parse_t *parse, macro_t *macro) {
  if (!lex_next_keyword_is(parse->lex, '(')) {
    return NULL;
  }
  vector_t *args = vector_new();
  token_t *token = lex_next_keyword_is(parse->lex, ')');
  if (token != NULL) {
    if (macro->args->size == 0) {
      return args;
    }
    lex_unget_token(parse->lex, token);
  }
  for (map_entry_t *e = macro->args->top; e != NULL; e = e->next) {
    token_t *arg = (token_t *)e->val;
    vector_push(args, function_list_arg(parse, arg->is_vaargs));
    if (lex_next_keyword_is(parse->lex, ')')) {
      break;
    }
    lex_expect_keyword_is(parse->lex, ',');
  }
  if (macro->args->size > 0) {
    token_t *last_arg = (token_t *)macro->args->bottom->val;
    if (last_arg->is_vaargs && args->size == macro->args->size - 1) {
      vector_push(args, vector_new());
    }
  }
  return args;
}

static token_t *cpp_get_token_new_line(parse_t *parse) {
  for (;;) {
    token_t *token = lex_get_token(parse->lex);
    if (token->kind == TOKEN_KIND_IDENTIFIER) {
      macro_t *macro = (macro_t *)map_get(parse->macros, token->identifier);
      if (macro != NULL) {
        if (token_exists_hideset(token, macro)) {
          return token;
        }
        if (macro->args == NULL) {
          expand_macro(parse, macro, NULL, token->hideset, token->is_space);
        } else {
          vector_t *args = function_like_args(parse, macro);
          if (args == NULL) {
            return token;
          }
          expand_macro(parse, macro, args, token->hideset, token->is_space);
          for (int i = 0; i < args->size; i++) {
            vector_t *arg = (vector_t *)args->data[i];
            vector_free(arg);
          }
          vector_free(args);
        }
        continue;
      }
    }
    token = preprocessor(parse, token);
    if (token != NULL) {
      return token;
    }
  }
  return NULL;
}

token_t *cpp_get_token(parse_t *parse) {
  for (;;) {
    token_t *token = cpp_get_token_new_line(parse);
    if (token->kind != TOKEN_KIND_NEWLINE) {
      return token;
    }
  }
}

void cpp_unget_token(parse_t *parse, token_t *token) {
  lex_unget_token(parse->lex, token);
}

token_t *cpp_next_token_is(parse_t *parse, int kind) {
  token_t *token = cpp_get_token(parse);
  if (token->kind == kind) {
    return token;
  }
  cpp_unget_token(parse, token);
  return NULL;
}

token_t *cpp_expect_token_is(parse_t *parse, int k) {
  token_t *token = cpp_get_token(parse);
  if (token->kind != k) {
    errorf("%s expected, but got %c", token_name(k), token_str(token));
  }
  return token;
}

token_t *cpp_next_keyword_is(parse_t *parse, int k) {
  token_t *token = cpp_get_token(parse);
  if (token->kind == TOKEN_KIND_KEYWORD && token->keyword == k) {
    return token;
  }
  cpp_unget_token(parse, token);
  return NULL;
}

void cpp_expect_keyword_is(parse_t *parse, int k) {
  token_t *token = cpp_get_token(parse);
  if (token->kind != TOKEN_KIND_KEYWORD) {
    errorf("%c expected, but not got keyword: %s", k, token_str(token));
  }
  if (token->keyword != k) {
    errorf("%c expected, but got %c", k, token->keyword);
  }
}
