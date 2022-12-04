#include <cassert>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "settings.h"
#include "map.h"

queue<int> freeq;
list<int> lba_list;
STATS* stats;
int RPOLICY; //0: FIFO, 1: LRU, 2: Clock-based NRU
char* cRPOLICY; //0: FIFO, 1: LRU, 2: Clock-based NRU
char* workload;

static off_t fdlength(int fd)
{
	struct stat st;
	off_t cur, ret;

	if (!fstat(fd, &st) && S_ISREG(st.st_mode))
		return st.st_size;

	cur = lseek(fd, 0, SEEK_CUR);
	ret = lseek(fd, 0, SEEK_END);
	lseek(fd, cur, SEEK_SET);

	return ret;
}

void ssd_init(SSD* ssd, STATS* stats) {
	ssd->itable = (bool*)calloc(NOP, sizeof(bool));
	ssd->oob = (OOB*)calloc(NOP, sizeof(OOB));
	ssd->mtable = (m_unit*)malloc(sizeof(m_unit)*ssd->mtable_size);
	ssd->lkuptable = (uint32_t*)malloc(sizeof(uint32_t)*LBANUM);
	ssd->fmtable = (uint32_t*)malloc(sizeof(uint32_t)*LBANUM);
	for (int i=0;i<ssd->mtable_size;i++) {
		ssd->mtable[i].lba = UINT_MAX;
		ssd->mtable[i].ppa = UINT_MAX;
		ssd->mtable[i].dirty = false;
		ssd->mtable[i].recently_used = false;

	}
	for (int i=0;i<LBANUM;i++) {
		ssd->fmtable[i] = UINT_MAX;
		ssd->lkuptable[i] = -1;
	}
	ssd->ictable = (int*)calloc(NOB, sizeof(int));

	ssd->active.index = 0;
	ssd->active.page = 0;

	for (int i=1; i<NOB;i++) {
		freeq.push(i);
	}
	printf("* Queue size: %ld\n", freeq.size());

	/* status value for experiment
	 */
	stats->cur_req = 0;
	stats->latency=0;
	stats->tot_req=0;
	
	stats->write=0;
	stats->read=0;
	stats->trim=0;
	stats->trash=0;
	stats->copy=0;
	stats->writeback=0;
	stats->cache_miss=0;
	stats->cache_hit=0;
	
	printf("* Device size: %.2f GB \n", (double)DEVSIZE/G);
	printf("* Logical size: %.2f GB\n", (double)LOGSIZE/G);
}

void simul_info(SSD *ssd, STATS *stats, int progress) {
	double waf = double(stats->write + stats->copy)/double(stats->write);
	printf("\n");
	printf("[progress: %d%%] WAF: %.3f\n", progress, waf);
	printf("WRITEBACK: %llu\nCACHE MISS: %llu\nCACHE HIT: %llu\n", 
			stats->writeback, stats->cache_miss, stats->cache_hit);
	printf("=====================\n");
}

void display_result(STATS* stats) {
	printf("\n=======================================================\n");
	printf("Experimental result [ %s ]\n\n", cRPOLICY);
	printf("(workload: %s)\n", workload);
	printf("* # of request : %llu / %llu\n", stats->cur_req, stats->tot_req);
	printf("* USER WRITE count : %llu\n", stats->write);
	printf("* USER READ count : %llu\n", stats->read);
	printf("* USER TRIM count : %llu\n", stats->trim);
	printf(" (trash: %llu)\n", stats->trash);
	printf("* GC COPY count : %llu (WAF: %.3f)\n\n", stats->copy, double(stats->write+stats->copy)/double(stats->write));
	printf("* # of writeback : %llu\n", stats->writeback);
	printf("* # of cache miss : %llu\n", stats->cache_miss);
	printf("* # of cache hit : %llu\n", stats->cache_hit);
	printf("=======================================================\n");
}

void sig_handler(int signum) {
	display_result(stats);
	exit(0);
}

void req_processing(char *raw_req, user_request *req) {
	//char* tmp = strtok(raw_req, " \t"); //time
	req->op = (int)(atoi(strtok(raw_req, " \t")));
	//req->op = (int)(atoi(strtok(NULL, " \t")));
	req->lba = strtoul(strtok(NULL, " \t"), NULL, 10);
	req->io_size = atoi(strtok(NULL, " \t"));
	//printf("op: %d, lba: %u, size: %d\n", req->op, req->lba, req->io_size);
}

/*processing trace file
 */
void ssd_simulation( SSD *ssd, STATS *stats, char* workload) {
	int readfd;
	char *buf;
	long cur, read_len;
	//char load_mark[128] = "LOAD_END\n"; // Not supported yet
	user_request *ureq;

	readfd = open(workload, O_RDONLY);
	if (readfd < 0) {
		perror("Failed to open read file");
		exit(1);
	}

	read_len = fdlength(readfd);
	buf = (char*)mmap(NULL, read_len, PROT_READ, MAP_SHARED, readfd, 0);
	if (buf == MAP_FAILED) {
		perror("Failed to mmap read file");
		exit(1);
	}
	close(readfd);

	madvise(buf, read_len, MADV_SEQUENTIAL | MADV_WILLNEED);

	stats->tot_req = *(long*)buf;
	cur = sizeof(long);

	int progress=0;
	while (cur < read_len) {
		ureq = (user_request*)(buf + cur);
		assert(0 <= ureq->op && ureq->op <= 3);

		cur += sizeof(user_request);

		stats->cur_req++;
		if (stats->write%262144==0) printf("\rwrite size: %lldGB    ", stats->write/262144);
		if ((stats->cur_req%(stats->tot_req/50))==0) {
			progress += 2;
			simul_info(ssd, stats, progress);
		}
		submit_io(ssd, stats, ureq);
	}

	munmap(buf, read_len);
}

int main(int argc, char **argv) {
	
	SSD *ssd = (SSD*)malloc(sizeof(SSD));
	stats = (STATS*)malloc(sizeof(STATS));

	if (argc < 2) {
		printf("./simulation <workload> <FIFO/LRU/cNRU> <mapping table size ratio>\n");
		abort();
	}
	workload = argv[1];
	cRPOLICY = argv[2];
	
	if(strcmp(cRPOLICY, "FIFO")==0)	RPOLICY = 0;
	else if(strcmp(cRPOLICY, "LRU")==0)	RPOLICY = 1;
	else if(strcmp(cRPOLICY, "cNRU")==0)	RPOLICY = 2;

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	
	signal(SIGINT, sig_handler);	
	//signal(SIGABRT, sig_handler);
	/* initialize SSD structures
	 */
	ssd->mtable_size = atoi(argv[3]);
	printf("***SIMULATION SETUP***\n");
	printf("* Victim Selection Policy: %s\n", cRPOLICY);
	printf("* Workload: %s\n* Mapping table size ratio: %d", workload, ssd->mtable_size);
	ssd->mtable_size = (int)LBANUM/ssd->mtable_size;
	printf(" (size: %d)\n", ssd->mtable_size);
	ssd_init(ssd, stats);

	printf("SSD initialization complete\n");
	printf("**********************\n");
	
	ssd_simulation(ssd, stats, workload);

	display_result(stats);
	return 0;
}
