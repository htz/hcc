// Copyright 2019 @htz. Released under the MIT license.

#include <string.h>
#include "hcc.h"

static void preprocessor_define(parse_t *parse);
static void preprocessor_undef(parse_t *parse);

static macro_t *read_macro(parse_t *parse) {
  macro_t *macro = macro_new();
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

static token_t *preprocessor(parse_t *parse, token_t *hash_token) {
  if (hash_token->kind != TOKEN_KIND_KEYWORD || hash_token->keyword != '#' || hash_token->column > 1) {
    return hash_token;
  }

  token_t *token = lex_next_token_is(parse->lex, TOKEN_KIND_IDENTIFIER);
  if (token == NULL) {
    return NULL;
  }
  if (strcmp("define", token->identifier) == 0) {
    preprocessor_define(parse);
    return NULL;
  } else if (strcmp("undef", token->identifier) == 0) {
    preprocessor_undef(parse);
    return NULL;
  } else {
    lex_unget_token(parse->lex, token);
  }
  return NULL;
}

static vector_t *subst(parse_t *parse, macro_t *macro, vector_t *hideset) {
  vector_t *tokens = vector_new();
  for (int i = 0; i < macro->tokens->size; i++) {
    token_t *token = (token_t *)macro->tokens->data[i];
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

static void expand_macro(parse_t *parse, macro_t *macro, vector_t *hideset) {
  vector_t *tokens = subst(parse, macro, hideset);
  for (int i = tokens->size - 1; i >= 0; i--) {
    token_t *token = (token_t *)tokens->data[i];
    lex_unget_token(parse->lex, token);
  }
  vector_free(tokens);
}

token_t *cpp_get_token(parse_t *parse) {
  for (;;) {
    token_t *token = lex_get_token(parse->lex);
    if (token->kind == TOKEN_KIND_NEWLINE) {
      continue;
    }
    if (token->kind == TOKEN_KIND_IDENTIFIER) {
      macro_t *macro = (macro_t *)map_get(parse->macros, token->identifier);
      if (macro != NULL) {
        if (token_exists_hideset(token, macro)) {
          return token;
        }
        expand_macro(parse, macro, token->hideset);
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
