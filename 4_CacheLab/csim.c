#include "cachelab.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

typedef unsigned long long address_t;

// not really cache the memory thus not need data filed
// each line: 1 valid bit | (64 - s - b) bits tag
// for alignment, use 1 char for valid bit, then remained bytes for bits tag
// each set: E lines
// (1 << s) set
typedef struct Line
{
    struct Line *next;
    address_t tag;
    bool valid;
} Line, *LinePtr;

static char USAGE_STRING[] =
    "Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n"
    "  -h: Optional help flag that prints usage info\n"
    "  -v: Optional verbose flag that displays trace info\n"
    "  -s <s>: Number of set index bits (S = 2^s is the number of sets)\n"
    "  -E <E>: Associativity (number of lines per set)\n"
    "  -b <b>: Number of block bits (B = 2^b is the block size)\n"
    "  -t <tracefile>: Name of the valgrindtrace to replay";

static FILE *file;
static LinePtr *sets;
static int verbose;
static size_t set_size;
static size_t associativity;
static size_t block_bit;
static address_t set_mask;
static address_t tag_mask;

static size_t hit_count;
static size_t miss_count;
static size_t evict_count;

void *Malloc(size_t size)
{
    void *p = malloc(size);
    if (!p)
    {
        puts("Bad alloc.");
        exit(-1);
    }

    return p;
}

FILE *Fopen(char *filename, char *mode)
{
    FILE *file = fopen(filename, mode);

    if (!file)
    {
        puts("fopen error.");
        exit(-1);
    }

    return file;
}

bool has_flag(size_t argc, char **argv, char *flag)
{
    for (size_t i = 0; i < argc; ++i)
    {
        if (strcmp(argv[i], flag) == 0)
        {
            return true;
        }
    }

    return false;
}

char *parse_flag(size_t argc, char **argv, char *flag)
{
    for (size_t i = 0; i < argc; ++i)
    {
        if (strcmp(argv[i], flag) == 0 && i + 1 < argc)
        {
            return argv[i + 1];
        }
    }

    return "";
}

void init_sets()
{
    sets = (LinePtr *)Malloc(set_size * sizeof(LinePtr *));

    for (size_t i = 0; i < set_size; ++i)
    {
        sets[i] = (LinePtr)Malloc(sizeof(Line));
        LinePtr prev = sets[i];
        memset(prev, 0, sizeof(Line));

        for (size_t j = 0; j < associativity; ++j)
        {
            prev->next = (LinePtr)Malloc(sizeof(Line));
            memset(prev->next, 0, sizeof(Line));
            prev = prev->next;
        }
    }
}

void release_sets()
{
    for (size_t i = 0; i < set_size; ++i)
    {
        LinePtr node = sets[i];

        while (node != NULL)
        {
            LinePtr next = node->next;
            free(node);
            node = next;
        }
    }

    free(sets);
}

address_t get_tag(address_t address)
{
    return address & tag_mask;
}

size_t get_set(address_t address)
{
    return (address & set_mask) >> block_bit;
}

LinePtr find_line(size_t set, address_t tag)
{
    LinePtr node = sets[set]->next;

    while (node != NULL)
    {
        if (node->tag == tag && node->valid)
        {
            return node;
        }

        node = node->next;
    }

    return NULL;
}

bool insert_line(size_t set, address_t tag)
{
    bool evicted = true;
    // input santitizaion guaranteed the first line is non-null
    LinePtr prev = sets[set];
    LinePtr node = prev->next;

    while (node->next != NULL)
    {
        prev = node;
        node = node->next;
    }

    // node could always hit a invalid line or last line(i.e., least recent used)
    evicted = node->valid;
    node->tag = tag;
    node->valid = true;

    // no neet to move line if target line is the first line
    if (node != sets[set]->next)
    {
        prev->next = node->next;
        node->next = sets[set]->next;
        sets[set]->next = node;
    }

    return evicted;
}

void parse_trace(char *buffer, char *type, address_t *address)
{
    // assume that memory accesses are aligned properly
    // request sizes in the valgrind traces could be ignored.

    for (size_t i = 0; buffer[i] != '\0'; ++i)
    {
        if (buffer[i] == ',')
        {
            buffer[i] = '\0';
            break;
        }
    }

    char address_buffer[17];
    sscanf(buffer, " %c %s", type, address_buffer);
    *address = strtoull(address_buffer, NULL, 16);
}

void cache_hit()
{
    ++hit_count;
    if (verbose)
    {
        printf(" hit");
    }
}

void cache_miss()
{
    ++miss_count;
    if (verbose)
    {
        printf(" miss");
    }
}

void cache_evict(bool evicted)
{
    evict_count += evicted;
    if (evicted && verbose)
    {
        printf(" eviction");
    }
}

void do_load(address_t address)
{
    size_t set = get_set(address);
    address_t tag = get_tag(address);

    LinePtr p = find_line(set, tag);

    if (p)
    {
        cache_hit();
        return;
    }

    cache_miss();
    bool evicted = insert_line(set, tag);
    cache_evict(evicted);
}

// use write-back and write-allocate
void do_store(address_t address)
{
    size_t set = get_set(address);
    address_t tag = get_tag(address);

    LinePtr p = find_line(set, tag);

    if (p)
    {
        cache_hit();
        return;
    }

    cache_miss();
    bool evicted = insert_line(set, tag);
    cache_evict(evicted);
    cache_hit();
}

void do_modify(address_t address)
{
    do_store(address);
}

void do_trace(char type, address_t address)
{
    switch (type)
    {
    case 'L':
        do_load(address);
        break;
    case 'S':
        do_store(address);
        break;
    case 'M':
        do_modify(address);
        break;
    default:
        break;
    }
}

int run_sim()
{
    char buffer[100];
    while (fgets(buffer, 99, file))
    {
        if (buffer[0] != ' ')
        {
            continue;
        }

        size_t len = strlen(buffer);
        buffer[len - 1] = '\0';

        if (verbose)
        {
            printf("%s", buffer);
        }

        char type;
        address_t address;
        parse_trace(buffer, &type, &address);
        do_trace(type, address);

        if (verbose)
        {
            printf("\n");
        }
    }
}

int main(int argc, char **argv)
{
    if (has_flag(argc, argv, "-h"))
    {
        puts(USAGE_STRING);
        return 0;
    }

    verbose = has_flag(argc, argv, "-v");
    int s = atoi(parse_flag(argc, argv, "-s"));
    int E = atoi(parse_flag(argc, argv, "-E"));
    int b = atoi(parse_flag(argc, argv, "-b"));
    char *filename = parse_flag(argc, argv, "-t");

    if (s <= 0 || E <= 0 || b <= 0 || strlen(filename) == 0)
    {
        puts(USAGE_STRING);
        return -1;
    }

    // x86-64 64bits address restriction
    if (s + b >= 64)
    {
        puts(USAGE_STRING);
        return -1;
    }

    set_size = 1UL << s;
    associativity = E;
    block_bit = b;
    set_mask = ((1UL << (s + block_bit)) - 1) - ((1UL << block_bit) - 1);
    tag_mask = ~((1UL << (s + b)) - 1);
    file = Fopen(filename, "r");

    init_sets(1 << s);
    run_sim();
    printSummary(hit_count, miss_count, evict_count);
    release_sets(1 << s);
    fclose(file);

    return 0;
}
