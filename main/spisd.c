

// Дефайны для сдКарты
#define PIN_NUM_MISO 6
#define PIN_NUM_MOSI 7
#define PIN_NUM_CLK 15
#define PIN_NUM_CS 16

#include "bsp/board.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include <dirent.h>
#include <sys/stat.h>

//#define FORCE_SD_40MHZ

esp_err_t sdmmc_host_set_card_clk(int slot, uint32_t freq_khz);

static const char *TAG = "SDSPI";

#define MOUNT_POINT "/sdcard"
#define SPI_DMA_CHAN    SPI_DMA_CH_AUTO
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

sdmmc_host_t host = SDSPI_HOST_DEFAULT();
sdmmc_card_t* card;
sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

int spisd_deinit()
{
    return 1;
}

int spisd_mount_fs()
{
    int         result = -1;
    esp_err_t   ret;
    const char mount_point[] = MOUNT_POINT;

    ESP_LOGD(TAG,"Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret == ESP_OK)
    {
#ifdef FORCE_SD_40MHZ
        card->max_freq_khz = 40000;
        sdmmc_host_set_card_clk(&host.slot, 40000);
        sdspi_host_set_card_clk(host.slot, 40000);
#endif

    	ESP_LOGD(TAG,"Filesystem mounted:");
        //sdmmc_card_print_info(stdout, card);

        result = ESP_OK;
    }
    else
    {
        if (ret == ESP_FAIL) {
        	ESP_LOGI(TAG,"Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.\n");
        } else {
        	ESP_LOGI(TAG,"Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.\n", esp_err_to_name(ret));
        }
    }

    return result;
}

int spisd_init()
{
    int         result  = -1;
    esp_err_t   ret;

    ESP_LOGD(TAG,"Initializing SD card");

#ifdef FORCE_SD_40MHZ
    host.max_freq_khz  = 40000;
#endif

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CHAN);
    if (ret == ESP_OK) 
    {
        // This initializes the slot without card detect (CD) and write protect (WP) signals.
        // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
        slot_config.gpio_cs = PIN_NUM_CS;
        slot_config.host_id = host.slot;

        result = ESP_OK;
    }
    else
    	ESP_LOGE(TAG,"Failed to initialize bus");

    return spisd_mount_fs();
}


int spisd_get_sector_size()
{
    return card->csd.sector_size;
}
int spisd_get_sector_num()
{
    return card->csd.capacity;
}

int spisd_sectors_read(void * dst, uint32_t start_sector, uint32_t num)
{
    int result          = -1;

    if (sdmmc_read_sectors(card, dst, start_sector, num) == ESP_OK)
    {
        result = 1;
    }

    return result;
}
int spisd_sectors_write(void * dst, uint32_t start_sector, uint32_t num)
{
    int result          = -1;

    if (sdmmc_write_sectors(card, dst, start_sector, num) == ESP_OK)
    {
        result = 1;
    }

    return result;
}

void usbprintf(char * msg, ...);

void spisd_list_root()
{
    struct dirent *entry;
    struct stat entry_stat;

    DIR *dir = opendir(MOUNT_POINT);

    if (dir != NULL)
    {
        usbprintf("\n\nRoot dir list:\n\n");

        while ((entry = readdir(dir)) != NULL) 
        {
            usbprintf("%s%s%s\n", 
                entry->d_type == DT_DIR ? "[" : "",
                entry->d_name,
                entry->d_type == DT_DIR ? "]" : "");
        }
        closedir(dir);
    }
    else
        usbprintf("cannot open dir!\n");
}

