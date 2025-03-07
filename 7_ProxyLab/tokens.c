#include "csapp.h"
#include "tokens.h"

tokens_t *create_tokens()
{
    tokens_t *tokens = Malloc(sizeof(tokens_t));
    tokens->dummy.next = &tokens->dummy;
    tokens->dummy.prev = &tokens->dummy;
    tokens->dummy.token = NULL;
    tokens->dummy.size = 0;

    return tokens;
}

void add_token(tokens_t *tokens, char *buf, size_t n)
{
    token_t *t = Malloc(sizeof(token_t));
    t->token = Malloc(n + 1);
    memcpy(t->token, buf, n);
    t->size = n;
    t->token[n] = '\0';

    token_t *last = tokens->dummy.prev;

    last->next = t;
    tokens->dummy.prev = t;
    t->prev = last;
    t->next = &tokens->dummy;
}

void free_tokens(tokens_t *tokens)
{
    if (!tokens)
    {
        return;
    }

    token_t *p = tokens->dummy.next;

    while (p != &tokens->dummy)
    {
        token_t *next = p->next;
        Free(p->token);
        Free(p);
        p = next;
    }

    Free(tokens);
}

/* split_line - split null-terminated string to tokens by given delim */
tokens_t *split_line(char *usrbuf, size_t maxlen, char delim)
{
    tokens_t *tokens = create_tokens();

    size_t i = 0;
    size_t start = ~0;

    for (; i < maxlen && usrbuf[i] != '\0'; ++i)
    {
        int is_delim = delim == ' ' ? isspace(usrbuf[i]) : usrbuf[i] == delim;

        if (!is_delim && start == ~0)
        {
            start = i;
        }

        if (is_delim && start != ~0)
        {
            add_token(tokens, usrbuf + start, i - start);
            start = ~0;
        }
    }

    if (start != ~0)
    {
        add_token(tokens, usrbuf + start, i - start);
    }

    return tokens;
}
