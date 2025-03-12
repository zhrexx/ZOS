#ifndef DISK_H
#define DISK_H

#include "../msstd.h"

#define ATA_PRIMARY_DATA        0x1F0
#define ATA_PRIMARY_ERR         0x1F1
#define ATA_PRIMARY_SECCOUNT    0x1F2
#define ATA_PRIMARY_SECNUM      0x1F3
#define ATA_PRIMARY_CYLLOW      0x1F4
#define ATA_PRIMARY_CYLHIGH     0x1F5
#define ATA_PRIMARY_DRIVE       0x1F6
#define ATA_PRIMARY_CMD         0x1F7
#define ATA_PRIMARY_STATUS      0x1F7

#define ATA_CMD_READ_SECTORS    0x20
#define ATA_CMD_WRITE_SECTORS   0x30

#define SECTOR_SIZE             512
#define MAX_FILES               64
#define MAX_FILENAME            64
#define FIRST_DATA_SECTOR       10

#define DISK_ERROR -1
#define DISK_NOT_FOUND 0 

static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t __inb(uint16_t port) {
    uint8_t value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t value;
    asm volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outw(uint16_t port, uint16_t value) {
    asm volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

typedef struct {
    char filename[MAX_FILENAME];
    uint32_t size;
    uint32_t first_sector;
    uint32_t num_sectors;
    uint8_t in_use;
} FileEntry;

typedef struct {
    char magic[4];
    uint32_t num_files;
    FileEntry files[MAX_FILES];
} FileTable;

void ata_wait_ready() {
    int timeout = 100000;
    while ((inb(ATA_PRIMARY_STATUS) & 0x80) != 0 && --timeout);
    timeout = 100000;
    while ((inb(ATA_PRIMARY_STATUS) & 0x40) == 0 && --timeout);
}

void disk_read_sector(uint32_t lba, uint8_t *buffer) {
    ata_wait_ready();
    
    outb(ATA_PRIMARY_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_ERR, 0x00);
    outb(ATA_PRIMARY_SECCOUNT, 1);
    outb(ATA_PRIMARY_SECNUM, lba & 0xFF);
    outb(ATA_PRIMARY_CYLLOW, (lba >> 8) & 0xFF);
    outb(ATA_PRIMARY_CYLHIGH, (lba >> 16) & 0xFF);
    outb(ATA_PRIMARY_CMD, ATA_CMD_READ_SECTORS);
    
    ata_wait_ready();
    
    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(ATA_PRIMARY_DATA);
        buffer[i*2] = data & 0xFF;
        buffer[i*2+1] = (data >> 8) & 0xFF;
    }
}

void disk_write_sector(uint32_t lba, const uint8_t *buffer) {
    ata_wait_ready();
    
    outb(ATA_PRIMARY_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_ERR, 0x00);
    outb(ATA_PRIMARY_SECCOUNT, 1);
    outb(ATA_PRIMARY_SECNUM, lba & 0xFF);
    outb(ATA_PRIMARY_CYLLOW, (lba >> 8) & 0xFF);
    outb(ATA_PRIMARY_CYLHIGH, (lba >> 16) & 0xFF);
    outb(ATA_PRIMARY_CMD, ATA_CMD_WRITE_SECTORS);
    
    ata_wait_ready();
    
    for (int i = 0; i < 256; i++) {
        outw(ATA_PRIMARY_DATA, buffer[i*2] | (buffer[i*2+1] << 8));
    }
}


int fs_init() {
    FileTable ft;
    uint8_t buffer[SECTOR_SIZE];

    disk_read_sector(1, buffer);
    memcpy(&ft, buffer, sizeof(FileTable));

    if (ft.num_files > MAX_FILES) {  
        memset(&ft, 0, sizeof(FileTable));
        ft.num_files = 0;
        memcpy(buffer, &ft, sizeof(FileTable));
        disk_write_sector(1, buffer);
        return 1;
    } else {
        return 0;
    }
}
void fs_list_files() {
    FileTable ft;
    uint8_t buffer[SECTOR_SIZE];
    
    disk_read_sector(1, buffer);
    memcpy(&ft, buffer, sizeof(FileTable));
    
    if (strncmp(ft.magic, K_MAGIC, 4) != 0) {
        printf("Filesystem not initialized!\n");
        return;
    }
    
    printf("Files on disk: %d\n", ft.num_files);
    for (uint32_t i = 0; i < MAX_FILES; i++) {
        if (ft.files[i].in_use) {
            if (ft.files[i].filename[0] == '\0') break;
            printf("%s - %d bytes\n", ft.files[i].filename, ft.files[i].size);
        }
    }
}

uint32_t fs_find_free_sector() {
    FileTable ft;
    uint8_t buffer[SECTOR_SIZE];
    uint32_t highest_sector = FIRST_DATA_SECTOR;
    
    disk_read_sector(1, buffer);
    memcpy(&ft, buffer, sizeof(FileTable));
    
    for (uint32_t i = 0; i < MAX_FILES; i++) {
        if (ft.files[i].in_use) {
            uint32_t last_sector = ft.files[i].first_sector + ft.files[i].num_sectors;
            if (last_sector > highest_sector) {
                highest_sector = last_sector;
            }
        }
    }
    
    return highest_sector;
}

int fs_create_file(const char *filename, const uint8_t *data, uint32_t size) {
    FileTable ft;
    uint8_t buffer[SECTOR_SIZE];
    
    if (strlen(filename) >= MAX_FILENAME) {
        printf("Filename too long\n");
        return -1;
    }
    
    disk_read_sector(1, buffer);
    memcpy(&ft, buffer, sizeof(FileTable));
    
    int free_entry = -1;
    for (uint32_t i = 0; i < MAX_FILES; i++) {
        if (ft.files[i].in_use && strcmp(ft.files[i].filename, filename) == 0) {
            printf("File already exists\n");
            return -1;
        }
        if (!ft.files[i].in_use && free_entry == -1) {
            free_entry = i;
        }
    }
    
    if (free_entry == -1) {
        printf("No free file entries\n");
        return -1;
    }
    
    uint32_t num_sectors = (size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    uint32_t first_sector = fs_find_free_sector();
    
    strcpy(ft.files[free_entry].filename, filename);
    ft.files[free_entry].size = size;
    ft.files[free_entry].first_sector = first_sector;
    ft.files[free_entry].num_sectors = num_sectors;
    ft.files[free_entry].in_use = 1;
    ft.num_files++;
    
    memcpy(buffer, &ft, sizeof(FileTable));
    disk_write_sector(1, buffer);
    
    for (uint32_t i = 0; i < num_sectors; i++) {
        uint32_t bytes_to_write = (i == num_sectors - 1) ? 
                                  (size % SECTOR_SIZE ? size % SECTOR_SIZE : SECTOR_SIZE) : 
                                  SECTOR_SIZE;
        
        memset(buffer, 0, SECTOR_SIZE);
        memcpy(buffer, data + (i * SECTOR_SIZE), bytes_to_write);
        disk_write_sector(first_sector + i, buffer);
    }
    
    return 0;
}

int fs_read_file(const char *filename, uint8_t *buffer, uint32_t *size) {
    FileTable ft;
    uint8_t sector_buffer[SECTOR_SIZE];
    
    disk_read_sector(1, sector_buffer);
    memcpy(&ft, sector_buffer, sizeof(FileTable));
    
    int file_index = -1;
    for (uint32_t i = 0; i < MAX_FILES; i++) {
        if (ft.files[i].in_use && strcmp(ft.files[i].filename, filename) == 0) {
            file_index = i;
            break;
        }
    }
    
    if (file_index == -1) {
        printf("File not found\n");
        return -1;
    }
    
    *size = ft.files[file_index].size;
    
    for (uint32_t i = 0; i < ft.files[file_index].num_sectors; i++) {
        disk_read_sector(ft.files[file_index].first_sector + i, sector_buffer);
        
        uint32_t bytes_to_copy = (i == ft.files[file_index].num_sectors - 1) ? 
                               (ft.files[file_index].size % SECTOR_SIZE ? 
                                ft.files[file_index].size % SECTOR_SIZE : SECTOR_SIZE) : 
                               SECTOR_SIZE;
        
        memcpy(buffer + (i * SECTOR_SIZE), sector_buffer, bytes_to_copy);
    }
    
    return 0;
}

int fs_delete_file(const char *filename) {
    FileTable ft;
    uint8_t buffer[SECTOR_SIZE];
    
    disk_read_sector(1, buffer);
    memcpy(&ft, buffer, sizeof(FileTable));
    
    int file_index = -1;
    for (uint32_t i = 0; i < MAX_FILES; i++) {
        if (ft.files[i].in_use && strcmp(ft.files[i].filename, filename) == 0) {
            file_index = i;
            break;
        }
    }
    
    if (file_index == -1) {
        printf("File not found\n");
        return -1;
    }
    
    ft.files[file_index].in_use = 0;
    ft.num_files--;
    
    memcpy(buffer, &ft, sizeof(FileTable));
    disk_write_sector(1, buffer);
    
    return 0;
}

uint32_t fs_get_file_size(const char *filename) {
    FileTable ft;
    uint8_t buffer[SECTOR_SIZE];
    
    disk_read_sector(1, buffer);
    memcpy(&ft, buffer, sizeof(FileTable));
    
    if (strncmp(ft.magic, K_MAGIC, 4) != 0) {
        kernel_panic("Filesystem not initialized!\n");
        return DISK_ERROR;
    }
    
    for (uint32_t i = 0; i < MAX_FILES; i++) {
        if (ft.files[i].in_use) {
            if (strcmp(filename, ft.files[i].filename) == 0) {
                return ft.files[i].size;
            }
        }
    }
    return DISK_NOT_FOUND;
}

int fs_file_exists(const char *filename) {
    FileTable ft;
    uint8_t buffer[SECTOR_SIZE];
    disk_read_sector(1, buffer);
    memcpy(&ft, buffer, sizeof(FileTable));
    for (uint32_t i = 0; i < MAX_FILES; i++) {
        if (ft.files[i].in_use && strcmp(ft.files[i].filename, filename) == 0) {
            return 1;
        }
    }
    return 0;
}


int fs_edit_file(const char *filename, const uint8_t *data, uint32_t new_size) {
    FileTable ft;
    uint8_t buffer[SECTOR_SIZE];
    disk_read_sector(1, buffer);
    memcpy(&ft, buffer, sizeof(FileTable));
    int file_index = -1;
    for (uint32_t i = 0; i < MAX_FILES; i++) {
        if (ft.files[i].in_use && strcmp(ft.files[i].filename, filename) == 0) {
            file_index = i;
            break;
        }
    }
    if (file_index == -1) {
        printf("File not found\n");
        return -1;
    }
    uint32_t current_allocated_bytes = ft.files[file_index].num_sectors * SECTOR_SIZE;
    uint32_t new_num_sectors = (new_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    if (new_size <= current_allocated_bytes) {
        for (uint32_t i = 0; i < ft.files[file_index].num_sectors; i++) {
            uint32_t offset = i * SECTOR_SIZE;
            uint32_t bytes_to_write;
            if (i == ft.files[file_index].num_sectors - 1) {
                bytes_to_write = (new_size - offset < SECTOR_SIZE)? (new_size - offset) : SECTOR_SIZE;
            } else {
                bytes_to_write = SECTOR_SIZE;
            }
            memset(buffer, 0, SECTOR_SIZE);
            if (offset < new_size) {
                memcpy(buffer, data + offset, bytes_to_write);
            }
            disk_write_sector(ft.files[file_index].first_sector + i, buffer);
        }
        ft.files[file_index].size = new_size;
        for (uint32_t i = new_num_sectors; i < ft.files[file_index].num_sectors; i++) {
            memset(buffer, 0, SECTOR_SIZE);
            disk_write_sector(ft.files[file_index].first_sector + i, buffer);
        }
        ft.files[file_index].num_sectors = new_num_sectors;
    } else {
        uint32_t free_sectors = fs_find_free_sector() - ft.files[file_index].first_sector - ft.files[file_index].num_sectors;
        if (new_num_sectors > free_sectors) {
            printf("Not enough free space\n");
            return -1;
        }
        uint32_t new_first_sector = ft.files[file_index].first_sector;
        for (uint32_t i = 0; i < new_num_sectors; i++) {
            uint32_t offset = i * SECTOR_SIZE;
            uint32_t bytes_to_write;
            if (i == new_num_sectors - 1) {
                bytes_to_write = (new_size - offset < SECTOR_SIZE)? (new_size - offset) : SECTOR_SIZE;
            } else {
                bytes_to_write = SECTOR_SIZE;
            }
            memset(buffer, 0, SECTOR_SIZE);
            if (offset < new_size) {
                memcpy(buffer, data + offset, bytes_to_write);
            }
            disk_write_sector(new_first_sector + i, buffer);
        }
        ft.files[file_index].first_sector = new_first_sector;
        ft.files[file_index].num_sectors = new_num_sectors;
        ft.files[file_index].size = new_size;
    }
    memcpy(buffer, &ft, sizeof(FileTable));
    disk_write_sector(1, buffer);

    return 0;
}

#endif
