#ifndef __F_SETTING__
#define __F_SETTING__

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <climits>
#include <queue>
#include <list>
#include <string>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <sys/types.h>

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

#define __maybe_unused __attribute__((unused))

using namespace std;

typedef struct mapping_unit{
	uint32_t lba;
	uint32_t ppa;
	bool dirty;
	bool recently_used;
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
        uint32_t *lkuptable; // lookup table for searching LBA fast! (index: lba, element: index of mtable);
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

static __maybe_unused inline void timespec_sub(
		const struct timespec * const a,
		const struct timespec * const b,
		struct timespec * const result) {
	result->tv_sec  = a->tv_sec  - b->tv_sec;
	result->tv_nsec = a->tv_nsec - b->tv_nsec;
	if (result->tv_nsec < 0) {
		--result->tv_sec;
		result->tv_nsec += 1000000000L;
	}
}

static __maybe_unused inline void timespec_add(
		const struct timespec * const a,
		const struct timespec * const b,
		struct timespec * const result) {
	result->tv_sec  = a->tv_sec + b->tv_sec;
	result->tv_nsec = a->tv_nsec + b->tv_nsec;
	if (result->tv_nsec > 1000000000L) {
		result->tv_sec += result->tv_nsec / 1000000000L;
		result->tv_nsec %= 1000000000L;
	}
}

static __maybe_unused struct timespec curtime(void)
{
	struct timespec ret;
	int err = clock_gettime(CLOCK_MONOTONIC_RAW, &ret);
	if (err < 0) {
		perror("clock_gettime() error");
		exit(1);
	}
	return ret;
}

static __maybe_unused void print_time(const struct timespec * const time)
{
	printf("%4ld.%06lds\n", time->tv_sec, time->tv_nsec / 1000L);
}

static __maybe_unused void print_time_diff(const struct timespec * const start, const struct timespec * const end)
{
	struct timespec diff;
	timespec_sub(end, start, &diff);

	print_time(&diff);
}

#endif
