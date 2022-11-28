#ifndef __F_SETTING__
#define __F_SETTING__

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define K (1024)
#define M (1024*K)
#define G (1024*M)
#define T (1024L*G)

#define LOGSIZE 128L*G
#define OP 10
#define DEVSIZE (LOGSIZE + LOGSIZE/100*OP)

#define PGSIZE 4096
#define PPB 512
#define BLKSIZE (PGSIZE*PPB)
#define NOB (DEVSIZE/BLKSIZE)
#define NOP (NOB*PPB)
#define LBANUM (LOGSIZE/PGSIZE)

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

typedef struct SSD {
        bool *itable; //valid table, valid=1
        int mtable_size; //mapping table size ratio (flash/memory)
        m_unit *mtable; // mapping table in memory
        uint32_t *fmtable; // mapping table in flash device
        int *ictable; //invalid page count per block
}SSD;

typedef struct STATS {
        unsigned long long cur_req; //# of done request
        unsigned long long latency; //current latency of accessing mapping table
        unsigned long long tot_req; //# of total request
        
	unsigned long long write;
	unsigned long long copy; // # of copy (for calculating WAF)
	unsigned long long writeback;
	unsigned long long cache_miss;
	unsigned long long cache_hit;
}STATS;

#endif