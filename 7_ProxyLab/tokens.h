#ifndef __TOKENS_H__
#define __TOKENS_H__

#include "csapp.h"

/* helper struct */

/* help to parse request and response line */
typedef struct
{
    char *token;
    size_t size;
} token_t;

int split_line(token_t tokens[], size_t max_token, char *usrbuf, size_t maxlen, char delim);
void rstrip_token(token_t *token);

#endif