uint32_t get_ppa(SSD* ssd, STATS* stats);
int do_gc(SSD* ssd, STATS* stats);
int validate_ppa(uint32_t ppa, uint32_t lba, SSD* ssd);
int invalidate_ppa(uint32_t ppa, SSD* ssd);

