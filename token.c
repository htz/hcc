// Copyright 2019 @htz. Released under the MIT license.

#include <stdlib.h>
#include "hcc.h"

static const char *token_names[] = {
  "INT",
  "KEYWORD",
  "EOF",
  "UNKNOWN",
};

token_t *token_new(lex_t *lex, int kind) {
  token_t *token = (token_t *)malloc(sizeof (token_t));
  token->kind = kind;
  token->line = lex->mark_line;
  token->column = lex->mark_column;
  vector_push(lex->tokens, (void *)token);
  return token;
}

void token_free(token_t *token) {
  free(token);
}

const char *token_name(int kind) {
  if (kind >= TOKEN_KIND_UNKNOWN) {
    return token_names[TOKEN_KIND_UNKNOWN];
  }
  return token_names[kind];
}

const char *token_str(token_t *token) {
  return token_name(token->kind);
}
