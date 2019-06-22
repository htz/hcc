// Copyright 2019 @htz. Released under the MIT license.

#include <stdlib.h>
#include <string.h>
#include "hcc.h"

static const char *token_names[] = {
  "CHAR",
  "INT",
  "UINT",
  "LONG",
  "ULONG",
  "LLONG",
  "ULLONG",
  "FLOAT",
  "DOUBLE",
  "STRING",
  "KEYWORD",
  "IDENTIFIER",
  "EOF",
  "UNKNOWN",
};

token_t *token_new(lex_t *lex, int kind) {
  token_t *token = (token_t *)malloc(sizeof (token_t));
  token->kind = kind;
  token->line = lex->mark_line;
  token->column = lex->mark_column;
  token->is_space = lex->is_space;
  token->hideset = NULL;
  if (kind == TOKEN_MACRO_PARAM || kind == TOKEN_KIND_EOF || kind == TOKEN_KIND_NEWLINE) {
    token->str = strdup("");
  } else {
    token->str = malloc(lex->p - lex->mark_p + 1);
    strncpy(token->str, lex->mark_p, lex->p - lex->mark_p);
    token->str[lex->p - lex->mark_p] = '\0';
  }
  vector_push(lex->tokens, (void *)token);
  return token;
}

void token_free(token_t *token) {
  switch (token->kind) {
  case TOKEN_KIND_IDENTIFIER:
    free(token->identifier);
    break;
  case TOKEN_KIND_STRING:
    string_free(token->sval);
    break;
  }
  if (token->hideset != NULL) {
    vector_free(token->hideset);
  }
  free(token->str);
  free(token);
}

token_t *token_dup(lex_t *lex, token_t *token) {
  token_t *dup = (token_t *)malloc(sizeof (token_t));
  memcpy(dup, token, sizeof (token_t));
  switch (token->kind) {
  case TOKEN_KIND_IDENTIFIER:
    dup->identifier = strdup(token->identifier);
    break;
  case TOKEN_KIND_STRING:
    dup->sval = string_dup(token->sval);
    break;
  }
  dup->hideset = NULL;
  dup->str = strdup(token->str);
  vector_push(lex->tokens, (void *)dup);
  return dup;
}

void token_add_hideset(token_t *token, macro_t *macro) {
  if (token->hideset == NULL) {
    token->hideset = vector_new();
  }
  vector_push(token->hideset, macro);
}

bool token_exists_hideset(token_t *token, macro_t *macro) {
  if (token->hideset == NULL) {
    return false;
  }
  return vector_exists(token->hideset, macro);
}

void token_union_hideset(token_t *token, vector_t *hideset) {
  if (hideset == NULL) {
    return;
  }
  for (int i = 0; i < hideset->size; i++) {
    macro_t *macro = (macro_t *)hideset->data[i];
    if (!token_exists_hideset(token, macro)) {
      token_add_hideset(token, macro);
    }
  }
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
