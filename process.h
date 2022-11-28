int check_mtable(uint32_t lba, SSD* ssd, STATS* stats);
int update_mtable(int index, uint32_t ppa, SSD* ssd);
int m_fifo(uint32_t lba, SSD* ssd, STATS* stats);

void read(uint32_t lba, SSD* ssd, STATS* stats);
void write(uint32_t lba, SSD* ssd, STATS* stats);
void trim(uint32_t lba, SSD* ssd, STATS* stats);
int submit_io(SSD* ssd, STATS* stats, user_request* req);


