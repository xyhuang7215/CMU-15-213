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

bool isVerboseMode = false;

unsigned nIdxBits, nAssoc, nBlockBits;
char *traceFile;

CacheLine **cache;
unsigned nSets;

int hit, eviction, miss;

inline void* Calloc(int, int);

inline unsigned Pow(unsigned, unsigned);

void printUsage();

void parseCommand(int argc, char **argv);

void parseLine(char *, int *, int *, char [MAXLEN]);

inline void pushFront(int idx, CacheLine *);

inline void deleteNode(CacheLine *);

void accessCache(unsigned, unsigned);

int main(int argc, char **argv)
{
    parseCommand(argc, argv);
    // printf("%u %u %u \n", nIdxBits, nAssoc, nBlockBits);

    // Allocate cache 
    // Elements would point to dummyHead of cachelines
    nSets = Pow(2, nIdxBits);
    cache = (CacheLine **) Calloc(nSets, sizeof(CacheLine*));
    for (unsigned i = 0; i < nSets; i++)
        cache[i] = (CacheLine*) Calloc(1, sizeof(CacheLine));


    char buf[MAXLEN];
    FILE *fp = fopen(traceFile, "r");
    while (fgets(buf, MAXLEN-1, fp) != NULL)
    {
        // skip Instruction Load & empty string
        if (*buf == 'I' || *buf == '\0') continue;
        if (isVerboseMode) printf("%s", buf);

        char type;
        int idx, tag;
        parseLine(&type, &idx, &tag, buf);

        accessCache(idx, tag);
        if (type == 'M') accessCache(idx, tag);
            
        if (isVerboseMode) putchar('\n');
    }

    printSummary(hit, miss, eviction);
    return 0;
}

void* Calloc(int n, int s)
{
    void *p = calloc(n, s);
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


void printUsage()
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


void parseCommand(int argc, char **argv)
{
    int cmd_opt = 0;
    while((cmd_opt = getopt(argc, argv, "hvs:E:b:t:")) != -1)
    {
        switch (cmd_opt) 
        {
            case 'h':
                printUsage();
                exit(0);
            case 'v':
                isVerboseMode = true;
                break;
            case 's':
                nIdxBits = atoi(optarg);
                break;
            case 'E':
                nAssoc = atoi(optarg);
                break;
            case 'b':
                nBlockBits = atoi(optarg);
                break;
            case 't':
                traceFile = optarg;
                break;

            case '?':
            default:
                printUsage();
                exit(0);
        }
    }
}

void parseLine(char *pType, int *pIdx, int *pTag, char buf[MAXLEN])
{
    // skip white spaces
    while (*buf && *buf == ' ') buf++;

    // Get type
    *pType = *buf;
    buf += 2;

    // Parse address [ tag | idx | offset ] into tag and index  
    uint64_t address;
    sscanf(buf, "%lx", &address); // !! lx

    // shift offset bits out -> address : [0..00 | tag | idx ]
    address >>= nBlockBits;

    // idxMask : [0...01..1]
    uint64_t idxMask = -1;
    idxMask >>= 64 - nIdxBits;

    *pIdx = address & idxMask;
    *pTag = address >> nIdxBits;
}


void pushFront(int idx, CacheLine *p)
{
    CacheLine *oldHead = cache[idx]->next;
    p->next = oldHead;
    p->prev = cache[idx];
    cache[idx]->next = p;
    if (oldHead != NULL) oldHead->prev = p;
}

void deleteNode(CacheLine *p)
{
    p->prev->next = p->next;
    if (p->next)
        p->next->prev = p->prev;
}



void accessCache(unsigned idx, unsigned tag)
{
    CacheLine* cLinePtr = cache[idx]->next, *prevPtr;
    unsigned count;
    for (count = 0; count < nAssoc; count++)
    {
        if (cLinePtr == NULL) break;
        if (cLinePtr->tag == tag)
        {
            // delete from list, and insert at head
            deleteNode(cLinePtr);
            pushFront(idx, cLinePtr);
            ++hit;
            if (isVerboseMode) printf(" hit ");
            return;
        }
        prevPtr = cLinePtr;
        cLinePtr = cLinePtr->next;
    }

    // miss
    ++miss;
    if (isVerboseMode) printf(" miss ");

    if (count == nAssoc)
    {
        deleteNode(prevPtr);
        free(prevPtr);
        ++eviction;
        if (isVerboseMode) printf(" eviction ");
    }

    // allocate new node, and insert at head
    cLinePtr = (CacheLine*) Calloc(1, sizeof(CacheLine));
    cLinePtr->tag = tag;
    pushFront(idx, cLinePtr);
}