#ifndef __SPISD_H__
#define __SPISD_H__

int spisd_get_sector_size();
int spisd_get_sector_num();
int spisd_init();
int spisd_deinit();
int spisd_sectors_read(void * dst, uint32_t start_sector, uint32_t num);
int spisd_sectors_write(void * dst, uint32_t start_sector, uint32_t num);
int spisd_mount_fs();
void spisd_list_root();


#endif // __SPISD_H__