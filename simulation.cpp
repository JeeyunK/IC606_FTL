#include "settings.h"

void ssd_init(SSD* ssd, STATS* stats) {
	ssd->itable = (bool*)calloc(NOP, sizeof(bool))
	ssd->mmtable = (m_unit*)malloc(sizeof(m_unit)*ssd->mtable_size);
	ssd->fmtable = (uint32_t*)malloc(sizeof(uint32_t)*LBANUM);
	for (int i=0;i<ssd->mtable_size;i++) {
		ssd->mmtable[i].lba = UINT_MAX;
	}
	for (int i=0;i<LBANUM;i++) {
		ssd->fmtable[i] = UINT_MAX;
	}
	ssd->ictable = (int*)calloc(NOB, sizeof(int));

	/* status value for experiment
	 */
	stats->cur_req = 0;
	stats->latency=0;
	stats->copy=0;
	stats->writeback=0;
	stats->cache_miss=0;
	stats->cache_hit=0;
	
	printf("Device size: %.2f GB \n", (double)DEVSIZE/G);
	printf("Logical size: %.2f GB\n", (double)LOGSIZE/G);
}

long get_workloadline(char *workload) {
	string data;
	char cmd_char[128] = "wc -l ";
	strcat(cmd_char, workload);
	FILE *stream;
	const int max_buffer = 256;
	char buffer[max_buffer];
	cmd.append(" 2>&1");

	stream popen(cmd.c_str(), "r");

	if (stream) {
		while (!feop(stream)) {
			if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
		}
		pclose(stream);
	}
	return stol(data);
}

void simul_info(SSD *ssd, STATS *stats, int progress) {
	double waf = double(stats->cur_req + stats->copy)/double(stats->cur_wp);
	printf("\n")
	printf("[progress: %d%%] WAF: %.3f\n"progress, waf);
	printf("WRITEBACK: %llu\nCACHE MISS: %llu\nCACHE HIT: %llu\n", 
			stats->writeback, stats->cache_miss, stats->cache_hit);
}

void req_processing(char *raw_req, user_request *req) {
	int tmp = strtok(raw_req, " \t"); //time
	req->op = (int)(atoi(strtok(NULL, " \t")));
	req->lba = strtoul(atoi(strtok(NULL, " \t"), NULL, 10));
	req->io_size = atoi(strtok(NULL, " \t"));
}

/*processing trace file
 */
void ssd_simulation( SSD *ssd, STATS *stats, char* workload) {
	char raw_req[128];
	char load_mark[128];
	user_request *ureq = malloc(sizeof(user_request));
	FILE *wktrace = fopen(workload, "r");
	
	int progress=0;
	while (fgets(raw_req, 128, wktrace)) {
		if (!strcmp(raw_req, load_mark)) {
			stats->tot_req -= 1;
			continue;
		}
		stats->cur_req++;
		if (stats->cur_req%(stats->tot_req/50)==0) {
			progress += 2;
			simul_info(ssd, stats, progress);
		}
		req_processing(raw_req, ureq);
		submit_io(ssd, stats, ureq);
		stats->cur_req++;

	}

}

int main(int argc, char **argv) {
	
	SSD *ssd = (SSD*)malloc(sizeof(SSD));
	STATS *stats = (STATS*)malloc(sizeof(stats));

	if (argc < 2) {
		printf("./simulation <workload> <mapping table size ratio>\n");
		abort();
	}
	char* workload = argv[1];
	
	/* initialize SSD structures
	 */
	ssd->mtable_size = atoi(argv[2]);
	printf("***SIMULATION SETUP***\n");
	printf("*Workload: %s\n*Mapping table size ratio: %d\n", workload, ssd->mtable_size);
	ssd->mtable_size = (int)LBANUM/ssd->mtable_size;
	ssd_init(ssd, stats);
	stats->tot_req = get_workloadline(workload)
	printf("SSD initialization complete\n");
	printf("**********************\n");
	
	ssd_simulation(ssd, stats, workload);

	return 0;
}
