#include "settings.h"
#include "map.h"
#include "gc.h"

extern int RPOLICY;
extern uint32_t target_ppa;
extern queue<int> freeq;
extern list<int> lba_list;

int cmp = 0;

/*check mapping table hit
 * if miss, do replacement (or load)
 */
int check_mtable(uint32_t lba, SSD* ssd, STATS* stats) {
	int entry_index=-1;
	/* check cached mapping table
	 */
	for (int i=0;i<ssd->mtable_size; i++) {
		if (ssd->mtable[i].lba == lba) {
			entry_index = i;
			break;
		}
	}
	if (entry_index == -1) {
		/* need replacement
		 */
		stats->cache_miss++;
		/*select replacement policy
	/	 */
		if (RPOLICY == 0) {
			entry_index = m_fifo(lba, ssd, stats);
		} else if (RPOLICY == 1) {
			if ((int) lba_list.size() < ssd->mtable_size){	//Do not need to replacement
				entry_index = m_fifo(lba, ssd, stats);
				update_lba_list(lba, ssd, stats, true);
			}else{
				entry_index = m_lru(lba, ssd, stats);
				if (entry_index != cmp){
				//printf("%d != %d\n", cmp, entry_index);
				}
				cmp += 1;
				if (cmp >= ssd->mtable_size) cmp = cmp -838;
	//			printf("entry_index: %d with list size: %d\n", entry_index, lba_list.size());
			}
			//TODO ADD REPLACEMENT POLICY
		} else if (RPOLICY == 2){
			entry_index = m_cnru(lba, ssd, stats);
		}
		//TODO add flash access latency
		ssd->mtable[entry_index].lba = lba;
		ssd->mtable[entry_index].ppa = ssd->fmtable[lba];
		ssd->mtable[entry_index].recently_used = true;
	} else {
		/*cache hit
		 */
		stats->cache_hit++;
		if (RPOLICY == 1){
			update_lba_list(lba, ssd, stats, false);
		}
	}

	return entry_index;
}

int update_mtable(int index, uint32_t ppa, SSD* ssd) {
	ssd->mtable[index].ppa = ppa;
	ssd->mtable[index].dirty = true;
	ssd->mtable[index].recently_used = true;
	return 0;
}

uint32_t update_lba_list(uint32_t lba, SSD* ssd, STATS* stats, bool ismiss){ // LRU, update lba-Linked list for finding lru-lba as victim
	uint32_t victim_lba = -1;
	list<int>::iterator it;
	bool find_flag = false;
	if((int) lba_list.size() < ssd->mtable_size){
		it = find(lba_list.begin(), lba_list.end(), lba);
		if (it != lba_list.end()){
			lba_list.erase(it);
	//		printf("find lba\n");
		}
		lba_list.push_front(lba);
	}else{
		if (ismiss){
			victim_lba = lba_list.back();
			lba_list.pop_back();
		}
		if(victim_lba == lba){
			lba_list.push_front(lba);
		}else{
			it = find(lba_list.begin(), lba_list.end(), lba);
			if (it != lba_list.end()){
				lba_list.erase(it);
				lba_list.push_front(lba);
				find_flag = true;
	//			printf("find lba\n");
			}
			if(!find_flag){
				lba_list.push_front(lba);
			}	
		}
	}
	return victim_lba;
}

int m_lru(uint32_t lba, SSD* ssd, STATS* stats){
	//TODO. Apply Hashing, too!
	int victim_lba = update_lba_list(lba, ssd, stats, true);
//	printf("victim_lba: %lld\n");
	int enindex = -1;
	for (int i=0;i<ssd->mtable_size; i++) {
		if ((int) ssd->mtable[i].lba == victim_lba) {
			enindex = i;
			if(ssd->mtable[i].dirty == true){
				ssd->fmtable[ssd->mtable[i].lba] = ssd->mtable[enindex].ppa;
				stats->writeback++;
			}
			break;
		}
	}
	if (enindex == -1){
		printf("someting is wrong in LRU!!!! victim lba: %d\n", victim_lba);
	}
	ssd->mtable[enindex].dirty = false;
	ssd->mtable[enindex].ppa = UINT_MAX;
	ssd->mtable[enindex].lba = UINT_MAX;
	//if (enindex == (ssd->mtable_size)) enindex = 0;
	return enindex;
}

/* select victim using FIFO
 */
int findex=0;
int m_fifo(uint32_t lba, SSD* ssd, STATS* stats) {
	if (ssd->mtable[findex].dirty == true) {
		//writeback dirty data
		//TODO scaling time value when accessing fmtable
		ssd->fmtable[ssd->mtable[findex].lba] = ssd->mtable[findex].ppa;
		stats->writeback++;
	}
	ssd->mtable[findex].dirty = false;
	ssd->mtable[findex].ppa = UINT_MAX;
	ssd->mtable[findex].lba = UINT_MAX;
	int ret = findex;
	findex++;
	if (findex == (ssd->mtable_size)) findex = 0;
	return ret;
}

int n_findex=0;
int m_cnru(uint32_t lba, SSD* ssd, STATS* stats) {
	int ret;
	while(1){
		if(ssd->mtable[n_findex].recently_used == true){
			ssd->mtable[n_findex].recently_used = false;
			n_findex++;
			if (n_findex == (ssd->mtable_size)) n_findex = 0;
		}else{
			if (ssd->mtable[n_findex].dirty == true) {
			//writeback dirty data
			//TODO scaling time value when accessing fmtable
				ssd->fmtable[ssd->mtable[n_findex].lba] = ssd->mtable[n_findex].ppa;
				stats->writeback++;
			}
			ssd->mtable[n_findex].dirty = false;
			ssd->mtable[n_findex].ppa = UINT_MAX;
			ssd->mtable[n_findex].lba = UINT_MAX;
			ssd->mtable[n_findex].recently_used = false;
			ret = n_findex;
			n_findex++;
			if (n_findex == (ssd->mtable_size)) n_findex = 0;
			break;
		}
	}
	return ret;
}

void read(uint32_t lba, SSD *ssd, STATS* stats) {
	stats->read++;
	//update_lba_list(lba, ssd, stats);
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
	//update_lba_list(lba, ssd, stats);
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
