// Copyright 2019 @htz. Released under the MIT license.

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "hcc.h"

static node_t *additive_expression(lex_t *lex);
static node_t *multiplicative_expression(lex_t *lex);
static node_t *primary_expression(lex_t *lex);
static node_t *expression(lex_t *lex);

static node_t *additive_expression(lex_t *lex) {
  node_t *node = multiplicative_expression(lex);
  for (;;) {
    int op;
    if (lex_next_keyword_is(lex, '+')) {
      op = '+';
    } else if (lex_next_keyword_is(lex, '-')) {
      op = '-';
    } else {
      break;
    }
    node = node_new_binary_op(op, node, multiplicative_expression(lex));
  }
  return node;
}

static node_t *multiplicative_expression(lex_t *lex) {
  node_t *node = primary_expression(lex);
  for (;;) {
    int op;
    if (lex_next_keyword_is(lex, '*')) {
      op = '*';
    } else if (lex_next_keyword_is(lex, '/')) {
      op = '/';
    } else if (lex_next_keyword_is(lex, '%')) {
      op = '%';
    } else {
      break;
    }
    node = node_new_binary_op(op, node, primary_expression(lex));
  }
  return node;
}

static node_t *primary_expression(lex_t *lex) {
  token_t *token = lex_get_token(lex);
  switch (token->kind) {
  case TOKEN_KIND_KEYWORD:
    if (token->keyword == '(') {
      node_t *node = expression(lex);
      lex_expect_keyword_is(lex, ')');
      return node;
    }
    errorf("unknown keyword");
  case TOKEN_KIND_INT:
    return node_new_int(token->ival);
  }
  errorf("unknown token: %s", token_str(token));
}

static node_t *expression(lex_t *lex) {
  return additive_expression(lex);
}

node_t *parse_file(FILE *fp) {
  lex_t *lex = lex_new(fp);
  node_t *node = expression(lex);
  lex_expect_token_is(lex, TOKEN_KIND_EOF);
  lex_free(lex);
  return node;
}
