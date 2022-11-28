#include "settings.h"
#include "gc.h"
#include "process.h"

extern queue<int> freeq;

uint32_t get_ppa(SSD* ssd, STATS* stats) {
	if (ssd->active.page == PPB) {
		//need new block
		if (freeq.size() == 0) {
			printf("GET_PPA) something's wrong with queue\n");
		}
		ssd->active.index = freeq.front();
		ssd->active.page = 0;
	}

	if (freeq.size() == 0) {
		//need new free block
		do_gc(ssd, stats);
	}
	
	int ret = ssd->active.index*PPB+ssd->active.page;
	ssd->active.page++;
	return ret;
}

int do_gc(SSD* ssd, STATS* stats) {
	int victim = -1;
	int invalid_count = 0;

	/* select victim (GREEDY)
	 */
	for (int i=0;i<NOB;i++) {
		if (invalid_count < ssd->ictable[i]) {
			victim = i;
			invalid_count = ssd->ictable[i];
		}
	}
	
	/* copy valid pages
	 * i: ppa
	 */
	for (int i=victim*PPB;i<(victim+1)*PPB;i++) {
		if (ssd->itable[i]) {
			stats->copy++;

			int tmp_lba = ssd->oob[i].lba;
			invalidate_ppa(i, ssd);
			int new_ppa = ssd->active.index*PPB+ssd->active.page;
			validate_ppa(new_ppa, tmp_lba, ssd);
			ssd->active.page++;
			//access mapping table
			int index = check_mtable(tmp_lba, ssd, stats);
			update_mtable(index, new_ppa, ssd);
			//TODO add flash access latency
		}
	}

	if (ssd->ictable[victim] != PPB) {
		printf("DO_GC) somethings wrong in victim block: still valid pages remain\n");
		abort();
	}
	ssd->ictable[victim] = 0;
	freeq.push(victim);
	return 0;
}

int validate_ppa(uint32_t ppa, uint32_t lba, SSD* ssd) {
	if (ssd->itable[ppa] == 1) {
		printf("valid information err in ppa %u\n", ppa);
		abort();
	}
	ssd->itable[ppa] = true;
	ssd->oob[ppa].lba = lba;
	return 0;
}


int invalidate_ppa(uint32_t ppa, SSD* ssd) {
        if (ssd->itable[ppa] == false) {
                printf("invalid information err in ppa %u\n", ppa);
                abort();
        }
        ssd->itable[ppa] = false;
	ssd->oob[ppa].lba = UINT_MAX;
        ssd->ictable[ppa/PPB]++;
        return 0;
}
