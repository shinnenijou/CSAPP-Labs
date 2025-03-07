#include "csapp.h"
#include "tokens.h"

/*
 * split_line - split null-terminated string to tokens by given delim
 * return the number of tokens
 */
int split_line(token_t tokens[], size_t max_token, char *usrbuf, size_t maxlen, char delim)
{
    size_t cnt = 0;
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
            tokens[cnt].token = usrbuf + start;
            tokens[cnt].size = i - start;
            ++cnt;
            start = ~0;

            if (cnt == max_token)
            {
                break;
            }
        }
    }

    if (start != ~0)
    {
        tokens[cnt].token = usrbuf + start;
        tokens[cnt].size = i - start;
        ++cnt;
    }

    return cnt;
}

void rstrip_token(token_t *token)
{
    if (!token)
    {
        return;
    }

    for (size_t i = token->size - 1; i < token->size; --i)
    {
        if (token->token[i] != '\r' && token->token[i] != '\n')
        {
            token->size = i + 1;
            return;
        }
    }
}