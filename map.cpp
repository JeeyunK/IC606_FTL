#include "settings.h"
#include "map.h"
#include "gc.h"

#define XXH_INLINE_ALL
#define XXH_NO_STREAM
#include "xxhash/xxhash.h"
#if (XXH_VECTOR != XXH_AVX512) && (XXH_VECTOR != XXH_AVX2)
#error "Hash is not using x86 AVX"
#endif

extern char RPOLICY;
extern uint32_t target_ppa;
extern queue<int> freeq;

/*check mapping table hit
 * if miss, do replacement (or load)
 */
int check_mtable(uint32_t lba, SSD* ssd, STATS* stats) {
	long entry_index;

	/* check cached mapping table
	 */
	entry_index = XXH3_64bits(&lba, sizeof(lba)) % ssd->mtable_size;

	if (ssd->mtable[entry_index].lba != lba) {
		/* need replacement
		 */
		stats->cache_miss++;
		/*select replacement policy
		 */
		if (RPOLICY == 0) {
			entry_index = m_fifo(lba, ssd, stats);
		} else if (RPOLICY == 1) {
			//TODO ADD REPLACEMENT POLICY
		}
		//TODO add flash access latency
		ssd->mtable[entry_index].lba = lba;
		ssd->mtable[entry_index].ppa = ssd->fmtable[lba];
	} else {
		/*cache hit
		 */
		stats->cache_hit++;
	}

	return entry_index;
}

int update_mtable(int index, uint32_t ppa, SSD* ssd) {
	ssd->mtable[index].ppa = ppa;
	ssd->mtable[index].dirty = true;
	return 0;
}

/* select victim using FIFO
 */
long m_fifo(uint32_t lba, SSD* ssd, STATS* stats) {
	static long findex;

	long findex = XXH3_64bits(&__findex, sizeof(__findex));

	if (ssd->__mtable[findex

	if (ssd->mtable[findex].dirty == true) {
		//writeback dirty data
		//TODO scaling time value when accessing fmtable
		ssd->fmtable[ssd->mtable[findex].lba] = ssd->mtable[findex].ppa;
		stats->writeback++;
	}
	ssd->mtable[findex].dirty = false;
	ssd->mtable[findex].ppa = UINT_MAX;
	ssd->mtable[findex].lba = UINT_MAX;

	__findex++;
	if (__findex == (ssd->mtable_size)) __findex = 0;

	return findex;
}

void read(uint32_t lba, SSD *ssd, STATS* stats) {
	stats->read++;
	int mindex = check_mtable(lba, ssd, stats);
	uint32_t ppa = ssd->mtable[mindex].ppa;
	if (ppa == UINT_MAX) {
		//printf("READ) There is no ppa info in LBA: %u\n", lba);
		return;
	}

	//TODO add flash access latency
	
	return;
}

void write(uint32_t lba, SSD* ssd, STATS* stats) {
	stats->write++;
	int mindex = check_mtable(lba, ssd, stats);
	uint32_t ppa = ssd->mtable[mindex].ppa;
	if (ppa == target_ppa) printf("prev ppa is %u, lba: %u\n", target_ppa, lba);
	if (ppa != UINT_MAX) {
		invalidate_ppa(ppa, ssd);
	}
	uint32_t new_ppa = get_ppa(ssd, stats);
	if (new_ppa == target_ppa) {
		printf("lba: %u, prev ppa: %u, new ppa: %u\n", lba, ppa, new_ppa);
	}
	//printf("%d\n", ppa);
	update_mtable(mindex, new_ppa, ssd);
	validate_ppa(new_ppa, lba, ssd);
	if (freeq.size() == 0) do_gc(ssd, stats);	
	//TODO add flash access latency
}

void trim(uint32_t lba, SSD* ssd, STATS* stats) {
	stats->trim++;
	int mindex = check_mtable(lba, ssd, stats);
	uint32_t ppa = ssd->mtable[mindex].ppa;
	if (ppa != UINT_MAX) invalidate_ppa(ppa, ssd);
	update_mtable(mindex, UINT_MAX, ssd);
}

int submit_io(SSD *ssd, STATS *stats, user_request *req) {
	int req_num = req->io_size/(int)PGSIZE;

	if (req->op == 0) {
		//read
		for (int i=0;i<req_num;i++) read(req->lba+i, ssd, stats);
	} else if (req->op == 1) {
		//write
		for (int i=0;i<req_num;i++) write(req->lba+i, ssd, stats);
	} else if (req->op == 3) {
		//discard
		for (int i=0;i<req_num;i++) trim(req->lba+i, ssd, stats);
	}else {
		stats->trash++;
		return 1;
	}

	return 0;
}
