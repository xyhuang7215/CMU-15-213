#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#define MAXLEN 4096

typedef struct CacheLine
{
    struct CacheLine *prev, *next;
    int tag;
} CacheLine;

// global variables
CacheLine **cache;
bool is_verbose_mode = false;
unsigned n_idx_bits, n_assoc, n_block_bits;
unsigned n_sets;
int hit, eviction, miss;
char *traceFile;

inline void* Calloc(int, size_t);

inline unsigned Pow(unsigned, unsigned);

void PrintUsage();

void ParseCommand(int argc, char **argv);

void ParseTrace(char *pType, int *pIdx, int *pTag, char *buf);

void CacheAccess(unsigned idx, unsigned tag);

inline void CachePushFront(int idx, CacheLine *p);

inline void CacheEraseNode(CacheLine *p);

int main(int argc, char **argv)
{
    ParseCommand(argc, argv);
    // printf("%u %u %u \n", n_idx_bits, n_assoc, n_block_bits);

    // Allocate cache 
    // Cache[i] points to dummy head
    n_sets = Pow(2, n_idx_bits);
    cache = (CacheLine **) Calloc(n_sets, sizeof(CacheLine*));
    for (unsigned i = 0; i < n_sets; i++)
        cache[i] = (CacheLine*) Calloc(1, sizeof(CacheLine));


    char buf[MAXLEN];
    FILE *fp = fopen(traceFile, "r");
    while (fgets(buf, MAXLEN-1, fp) != NULL)
    {
        // Skip loading instruction or Empty string
        if (*buf == 'I' || *buf == '\0') continue;
        if (is_verbose_mode) printf("%s", buf);

        char type;
        int idx, tag;
        ParseTrace(&type, &idx, &tag, buf);

        CacheAccess(idx, tag);
        if (type == 'M') CacheAccess(idx, tag);
            
        if (is_verbose_mode) putchar('\n');
    }

    printSummary(hit, miss, eviction);
    return 0;
}


void* Calloc(int num, size_t size)
{
    void *p = calloc(num, size);
    if (p == NULL)
    {
        fprintf(stderr, "Bad calloc : %s", strerror(errno));
        exit(0);
    }
    return p;
}


unsigned Pow(unsigned base, unsigned exp)
{
    unsigned result = 1;
    while(exp-- > 0)
        result *= base;
    return result;    
}


void PrintUsage()
{
    printf("./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("  -h         Print this help message.\n"
           "  -v         Optional verbose flag.\n"
           "  -s <num>   Number of set index bits.\n"
           "  -E <num>   Number of lines per set.\n"
           "  -b <num>   Number of block offset bits.\n"
           "  -t <file>  Trace file.\n");
}


void ParseCommand(int argc, char **argv)
{
    int cmd_opt = 0;
    while((cmd_opt = getopt(argc, argv, "hvs:E:b:t:")) != -1)
    {
        switch (cmd_opt) 
        {
            case 'h':
                PrintUsage();
                exit(0);
            case 'v':
                is_verbose_mode = true;
                break;
            case 's':
                n_idx_bits = atoi(optarg);
                break;
            case 'E':
                n_assoc = atoi(optarg);
                break;
            case 'b':
                n_block_bits = atoi(optarg);
                break;
            case 't':
                traceFile = optarg;
                break;

            case '?':
            default:
                PrintUsage();
                exit(0);
        }
    }
}


void ParseTrace(char *pType, int *pIdx, int *pTag, char *buf)
{
    // Skip white spaces
    while (*buf && *buf == ' ') ++buf;

    // Get type
    *pType = *buf;
    buf += 2;

    // Parse address [ tag | idx | offset ] into tag and index  
    uint64_t address;
    sscanf(buf, "%lx", &address); // hex

    // Shift out offset bits -> current address : [0..00 | tag | idx ]
    address >>= n_block_bits;

    // idx_mask : [0...00 | 0...00 | 1...11]
    uint64_t idx_mask = -1;
    idx_mask >>= 64 - n_idx_bits;

    *pIdx = address & idx_mask;
    *pTag = address >> n_idx_bits;
}


void CacheAccess(unsigned idx, unsigned tag)
{
    CacheLine *cache_line = cache[idx]->next, *prev_cache_line;
    unsigned count;
    for (count = 0; count < n_assoc; count++)
    {
        // Not found
        if (cache_line == NULL) break;
        // Hit
        if (cache_line->tag == tag)
        {
            ++hit;
            if (is_verbose_mode) printf(" hit ");

            // delete from list, and insert at head
            CacheEraseNode(cache_line);
            CachePushFront(idx, cache_line);
            return;
        }
        prev_cache_line = cache_line;
        cache_line = cache_line->next;
    }

    // miss
    ++miss;
    CacheLine *new_cache_line;
    if (is_verbose_mode) printf(" miss ");

    // if full 
    if (count == n_assoc)
    {
        ++eviction;
        if (is_verbose_mode) printf(" eviction ");

        // delete last node
        CacheEraseNode(prev_cache_line);
        new_cache_line = prev_cache_line;
        new_cache_line->tag = tag;
        CachePushFront(idx, new_cache_line);
        return;
    }

    // allocate new node, and insert at head
    new_cache_line = (CacheLine*) Calloc(1, sizeof(CacheLine));
    new_cache_line->tag = tag;
    CachePushFront(idx, new_cache_line);
}


void CachePushFront(int idx, CacheLine *p)
{
    p->next = cache[idx]->next;
    p->prev = cache[idx];
    if (cache[idx]->next != NULL) 
        cache[idx]->next->prev = p;
    cache[idx]->next = p;
}


void CacheEraseNode(CacheLine *p)
{
    p->prev->next = p->next;
    if (p->next)
        p->next->prev = p->prev;
}
