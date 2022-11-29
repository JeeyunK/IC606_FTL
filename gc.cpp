#include "settings.h"
#include "gc.h"
#include "map.h"

uint32_t target_ppa = 8437246;
extern queue<int> freeq;

uint32_t get_ppa(SSD* ssd, STATS* stats) {
	if (ssd->active.page == PPB) {
		//need new block
		if (freeq.size() == 0) {
			printf("GET_PPA) something's wrong with queue\n");
		}
		ssd->active.index = freeq.front();
		freeq.pop();
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
	int invalid_count = -1;

	/* select victim (GREEDY)
	 */
	for (int i=0;i<NOB;i++) {
		if (invalid_count < ssd->ictable[i]) {
			victim = i;
			invalid_count = ssd->ictable[i];
		}
	}

	//printf("victim block: %d, invalid cnt: %d\n\n", victim, invalid_count);
	
	/* copy valid pages
	 * i: ppa
	 */
	for (uint32_t i=(uint32_t)(victim*PPB);i<(uint32_t)((victim+1)*PPB);i++) {
		if (ssd->itable[i]) {
			stats->copy++;

			if (i == target_ppa) printf("ppa %u gc\n", target_ppa);
			uint32_t tmp_lba = ssd->oob[i].lba;
			invalidate_ppa(i, ssd);
			uint32_t new_ppa = ssd->active.index*PPB+ssd->active.page;
			ssd->active.page++;
			if (ssd->active.page == PPB) {
				printf("DO_GC) there is no empty space in active block\n");
				abort();
			}
			//access mapping table
			int index = check_mtable(tmp_lba, ssd, stats);
			if (ssd->mtable[index].ppa != i) {
				printf("mapping table has wrong ppa(lba: %u, oob ppa: %u, table ppa: %u)\n", 
						tmp_lba, i, ssd->mtable[index].ppa);
				abort();
			}
			update_mtable(index, new_ppa, ssd);
			validate_ppa(new_ppa, tmp_lba, ssd);
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
	if (ssd->itable[ppa] == true) {
		printf("valid information err in ppa %u\n", ppa);
		abort();
	}
	if (ppa == target_ppa) printf("%u validated\n", target_ppa);
	ssd->itable[ppa] = true;
	ssd->oob[ppa].lba = lba;
	return 0;
}


int invalidate_ppa(uint32_t ppa, SSD* ssd) {
	if (ssd->itable[ppa] == false) {
                printf("invalid information err in ppa %u\n", ppa);
                abort();
        }
	if (ppa == target_ppa) {
                printf("%u invalidated\n", target_ppa);
                //abort();
        }
        ssd->itable[ppa] = false;
	ssd->oob[ppa].lba = UINT_MAX;
        ssd->ictable[ppa/PPB]++;
        return 0;
}
