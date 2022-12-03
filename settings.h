#ifndef __F_SETTING__
#define __F_SETTING__

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <climits>
#include <queue>
#include <string>
#include <cstring>
#include <iostream>

#define K (1024)
#define M (1024*K)
#define G (1024*M)
#define T (1024L*G)

#define LOGSIZE 32L*G
#define OP 10
#define DEVSIZE (LOGSIZE + LOGSIZE/100*OP)

#define PGSIZE 4096
#define PPB 512
#define BLKSIZE (PGSIZE*PPB)
#define NOB (DEVSIZE/BLKSIZE)
#define NOP (NOB*PPB)
#define LBANUM (LOGSIZE/PGSIZE)

using namespace std;

typedef struct mapping_unit{
	uint32_t lba;
	uint32_t ppa;
	bool dirty;
}m_unit;

typedef struct user_request{
	uint32_t lba;
	int op;
	int io_size;
}user_request;

typedef struct block {
	int index;
	int page;
}BLOCK;

typedef struct OOB {
	uint32_t lba;
}OOB;

typedef struct SSD {
        bool *itable; //valid table, valid=1
        int mtable_size; //mapping table size ratio (flash/memory)
        m_unit *mtable; // mapping table in memory
        uint32_t *__mtable; // internal table used for victim eviction policy
        uint32_t *fmtable; // mapping table in flash device
        int *ictable; //invalid page count per block
	BLOCK active;
	OOB *oob;
	queue<int> freeq;		
}SSD;

typedef struct STATS {
        unsigned long long cur_req; //# of done request
        unsigned long long latency; //current latency of accessing mapping table
        unsigned long long tot_req; //# of total request
        
	unsigned long long write;
	unsigned long long read;
	unsigned long long trim;
	unsigned long long trash;
	unsigned long long copy; // # of copy (for calculating WAF)
	unsigned long long writeback;
	unsigned long long cache_miss;
	unsigned long long cache_hit;
}STATS;

#endif
