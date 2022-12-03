#include <signal.h>
#include "settings.h"
#include "map.h"

queue<int> freeq;
STATS* stats;
char RPOLICY = 0; //0: fifo
char* workload;

void ssd_init(SSD* ssd, STATS* stats) {
	ssd->itable = (bool*)calloc(NOP, sizeof(bool));
	ssd->oob = (OOB*)calloc(NOP, sizeof(OOB));
	ssd->mtable = (m_unit*)malloc(sizeof(m_unit)*ssd->mtable_size);
	ssd->fmtable = (uint32_t*)malloc(sizeof(uint32_t)*LBANUM);
	for (int i=0;i<ssd->mtable_size;i++) {
		ssd->mtable[i].lba = UINT_MAX;
		ssd->mtable[i].ppa = UINT_MAX;
		ssd->mtable[i].dirty = false;
	}
	for (int i=0;i<LBANUM;i++) {
		ssd->fmtable[i] = UINT_MAX;
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

long get_workloadline(char *workload) {
	string data;
	char cmd_char[128] = "wc -l ";
	strcat(cmd_char, workload);
	string cmd = cmd_char;
	FILE *stream;
	const int max_buffer = 256;
	char buffer[max_buffer];
	cmd.append(" 2>&1");

	stream = popen(cmd.c_str(), "r");

	if (stream) {
		while (!feof(stream)) {
			if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
		}
		pclose(stream);
	}
	return stol(data);
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
	printf("Experimental result [ %s ]\n\n", RPOLICY? "test":"FIFO");
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
	char raw_req[128];
	char load_mark[128] = "LOAD_END\n";
	user_request *ureq = (user_request*)malloc(sizeof(user_request));
	FILE *wktrace = fopen(workload, "r");
	
	int progress=0;
	while (fgets(raw_req, 128, wktrace)) {
		if (!strcmp(raw_req, load_mark)) {
			stats->tot_req -= 1;
			continue;
		}
		stats->cur_req++;
		if (stats->write%262144==0) printf("\rwrite size: %lldGB    ", stats->write/262144);
		if ((stats->cur_req%(stats->tot_req/50))==0) {
			progress += 2;
			simul_info(ssd, stats, progress);
		}
		req_processing(raw_req, ureq);
		submit_io(ssd, stats, ureq);

	}

}

int main(int argc, char **argv) {
	
	SSD *ssd = (SSD*)malloc(sizeof(SSD));
	stats = (STATS*)malloc(sizeof(STATS));

	if (argc < 2) {
		printf("./simulation <workload> <mapping table size ratio>\n");
		abort();
	}
	workload = argv[1];

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	
	signal(SIGINT, sig_handler);	
	//signal(SIGABRT, sig_handler);
	/* initialize SSD structures
	 */
	ssd->mtable_size = atoi(argv[2]);
	printf("***SIMULATION SETUP***\n");
	printf("* Workload: %s\n* Mapping table size ratio: %d", workload, ssd->mtable_size);
	ssd->mtable_size = (int)LBANUM/ssd->mtable_size;
	printf(" (size: %d)\n", ssd->mtable_size);
	ssd_init(ssd, stats);
	stats->tot_req = get_workloadline(workload);
	printf("* Workload line number: %llu\n", stats->tot_req);
	printf("SSD initialization complete\n");
	printf("**********************\n");
	
	ssd_simulation(ssd, stats, workload);

	display_result(stats);
	return 0;
}
