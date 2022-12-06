int check_mtable(uint32_t lba, SSD* ssd, STATS* stats);
int update_mtable(int index, uint32_t ppa, SSD* ssd);
uint32_t update_lba_list(uint32_t lba, SSD* ssd, STATS* stats, bool ismiss);
int update_gc_table(uint32_t lba, uint32_t ppa, SSD* ssd);

int m_lru(uint32_t lba, SSD* ssd, STATS* stats);
int m_fifo(uint32_t lba, SSD* ssd, STATS* stats);
int m_cnru(uint32_t lba, SSD* ssd, STATS* stats);
int m_rand(uint32_t lba, SSD* ssd, STATS* stats);

void read(uint32_t lba, SSD* ssd, STATS* stats);
void write(uint32_t lba, SSD* ssd, STATS* stats);
void trim(uint32_t lba, SSD* ssd, STATS* stats);
int submit_io(SSD* ssd, STATS* stats, user_request* req);


