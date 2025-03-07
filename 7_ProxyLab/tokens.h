#ifndef __TOKENS_H__
#define __TOKENS_H__

#include "csapp.h"

/* helper struct */

/* help to parse request and response line */
typedef struct _token
{
    char *token;
    size_t size;
    struct _token *prev;
    struct _token *next;
} token_t;

typedef struct
{
    token_t dummy;
} tokens_t;

tokens_t *create_tokens();
void add_token(tokens_t *tokens, char *buf, size_t n);
void free_tokens(tokens_t *tokens);
tokens_t *split_line(char *usrbuf, size_t maxlen, char delim);

#endif