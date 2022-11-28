#include "settings.h"
#include "process.h"
#include "gc.h"


#define RPOLICY 0 //0: fifo

/*check mapping table hit
 * if miss, do replacement (or load)
 */
int check_mtable(uint32_t lba, SSD* ssd, STATS* stats) {
	int i=0;
	int entry_index=-1;
	while (ssd->mtable[i].lba != UINT_MAX) {
		if (ssd->mtable[i].lba == lba) {
			entry_index = i;
			break;
		}
		i++;
		if (i == (ssd->mtable_size-1)) break;
	}
	if (entry_index == -1) {
		/* need replacement
		 */
		stats->cache_miss++;
		if (RPOLICY == 0) {
			entry_index = m_fifo(lba, ssd, stats);
			ssd->mtable[entry_index].lba = lba;
			return entry_index;
		} else if (RPOLICY == 1) {
			//TODO add replacement policy
		}
	} else {
		stats->cache_hit++;
		return entry_index;
	}
}

int update_mtable(int index, uint32_t ppa, SSD* ssd) {
	ssd->mtable[index].ppa = ppa;
	ssd->mtable[index].dirty = true;
	return 0;
}

int findex=0;
int m_fifo(uint32_t lba, SSD* ssd, STATS* stats) {
	if (ssd->mtable[findex].dirty == true) {
		//writeback dirty data
		//TODO scaling time value when accessing fmtable
		ssd->fmtable[ssd->mtable[findex].lba] = ssd->mtable[findex].ppa;
		stats->writeback++;
	}
	ssd->mtable[findex].dirty = false;
	ssd->mtable[findex].lba = UINT_MAX;
	int ret = findex;
	findex++;
	if (findex == (ssd->mtable_size)) findex = 0;
	return ret;
}

void read(uint32_t lba, SSD *ssd, STATS* stats) {
	int mindex = check_mtable(lba, ssd, stats);
	uint32_t ppa = ssd->mtable[mindex].ppa;
	if (ppa == UINT_MAX) {
		printf("READ) There is no ppa info in LBA: %u\n", lba);
		return;
	}

	//TODO add flash access latency
	
	return;
}

void write(uint32_t lba, SSD* ssd, STATS* stats) {
	stats->write++;
	int mindex = check_mtable(lba, ssd, stats);
	uint32_t ppa = ssd->mtable[mindex].ppa;
	if (ppa != UINT_MAX) {
		invalidate_ppa(ppa, ssd);
	}
	ppa = get_ppa(ssd, stats);
	update_mtable(mindex, ppa, ssd);
	validate_ppa(ppa, lba, ssd);

	//TODO add flash access latency
}

void trim(uint32_t lba, SSD* ssd, STATS* stats) {
	int mindex = check_mtable(lba, ssd, stats);
	uint32_t ppa = ssd->mtable[mindex].ppa;
	if (ppa != UINT_MAX) invalidate_ppa(ppa, ssd);
}

int submit_io(SSD *ssd, STATS *stats, user_request *req) {
	int ret;
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
	}else return 1;

	return 0;
}
