#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#define SECTOR_SIZE 512
#define MAX_FILES 64
#define MAX_FILENAME 64
#define FIRST_DATA_SECTOR 10
#define K_MAGIC "ZOS1"

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

void create_disk_image(const char* filename, uint32_t size_mb) {
    uint8_t buffer[SECTOR_SIZE];
    memset(buffer, 0, SECTOR_SIZE);
    
    FILE* file = fopen(filename, "wb");
    if (!file) {
        printf("Error creating disk image\n");
        return;
    }
    
    for (uint32_t i = 0; i < size_mb * 1024 * 2; i++) {
        fwrite(buffer, SECTOR_SIZE, 1, file);
    }
    
    fclose(file);
    printf("Created disk image %s of size %dMB\n", filename, size_mb);
}

void init_filesystem(const char* disk_image) {
    FILE* disk = fopen(disk_image, "r+b");
    if (!disk) {
        printf("Could not open disk image\n");
        return;
    }
    
    FileTable ft;
    memset(&ft, 0, sizeof(FileTable));
    memcpy(ft.magic, K_MAGIC, 4);
    ft.num_files = 0;
    
    fseek(disk, SECTOR_SIZE, SEEK_SET);
    fwrite(&ft, sizeof(FileTable), 1, disk);
    fclose(disk);
    
    printf("Filesystem initialized on %s\n", disk_image);
}

void list_files(const char* disk_image) {
    FILE* disk = fopen(disk_image, "rb");
    if (!disk) {
        printf("Could not open disk image\n");
        return;
    }
    
    FileTable ft;
    fseek(disk, SECTOR_SIZE, SEEK_SET);
    fread(&ft, sizeof(FileTable), 1, disk);
    
    if (strncmp(ft.magic, K_MAGIC, 4) != 0) {
        printf("Invalid filesystem or not initialized\n");
        fclose(disk);
        return;
    }
    
    printf("Files on disk: %d\n", ft.num_files);
    for (uint32_t i = 0; i < MAX_FILES; i++) {
        if (ft.files[i].in_use) {
            printf("%s - %d bytes (sectors %d-%d)\n", 
                   ft.files[i].filename, 
                   ft.files[i].size,
                   ft.files[i].first_sector,
                   ft.files[i].first_sector + ft.files[i].num_sectors - 1);
        }
    }
    
    fclose(disk);
}

void write_file_to_image(const char* disk_image, const char* filename, const char* source_file) {
    FILE* disk = fopen(disk_image, "r+b");
    if (!disk) {
        printf("Could not open disk image\n");
        return;
    }
    
    FileTable ft;
    fseek(disk, SECTOR_SIZE, SEEK_SET);
    fread(&ft, sizeof(FileTable), 1, disk);
    
    if (strncmp(ft.magic, K_MAGIC, 4) != 0) {
        printf("Invalid filesystem or not initialized\n");
        fclose(disk);
        return;
    }
    
    int free_entry = -1;
    for (uint32_t i = 0; i < MAX_FILES; i++) {
        if (ft.files[i].in_use && strcmp(ft.files[i].filename, filename) == 0) {
            printf("File %s already exists, replacing it\n", filename);
            free_entry = i;
            break;
        }
        if (!ft.files[i].in_use && free_entry == -1) {
            free_entry = i;
        }
    }
    
    if (free_entry == -1) {
        printf("No free file entries\n");
        fclose(disk);
        return;
    }
    
    FILE* src = fopen(source_file, "rb");
    if (!src) {
        printf("Could not open source file %s\n", source_file);
        fclose(disk);
        return;
    }
    
    fseek(src, 0, SEEK_END);
    uint32_t size = ftell(src);
    fseek(src, 0, SEEK_SET);
    
    uint8_t* buffer = malloc(size);
    fread(buffer, 1, size, src);
    fclose(src);
    
    uint32_t num_sectors = (size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    uint32_t highest_sector = FIRST_DATA_SECTOR;
    
    for (uint32_t i = 0; i < MAX_FILES; i++) {
        if (ft.files[i].in_use && i != free_entry) {
            uint32_t last_sector = ft.files[i].first_sector + ft.files[i].num_sectors;
            if (last_sector > highest_sector) {
                highest_sector = last_sector;
            }
        }
    }
    
    strncpy(ft.files[free_entry].filename, filename, MAX_FILENAME-1);
    ft.files[free_entry].size = size;
    ft.files[free_entry].first_sector = highest_sector;
    ft.files[free_entry].num_sectors = num_sectors;
    
    if (!ft.files[free_entry].in_use) {
        ft.files[free_entry].in_use = 1;
        ft.num_files++;
    }
    
    fseek(disk, SECTOR_SIZE, SEEK_SET);
    fwrite(&ft, sizeof(FileTable), 1, disk);
    
    uint8_t sector_buffer[SECTOR_SIZE];
    for (uint32_t i = 0; i < num_sectors; i++) {
        memset(sector_buffer, 0, SECTOR_SIZE);
        
        uint32_t bytes_to_write = (i == num_sectors - 1) ? 
                                (size % SECTOR_SIZE ? size % SECTOR_SIZE : SECTOR_SIZE) : 
                                SECTOR_SIZE;
        
        memcpy(sector_buffer, buffer + (i * SECTOR_SIZE), bytes_to_write);
        
        fseek(disk, (highest_sector + i) * SECTOR_SIZE, SEEK_SET);
        fwrite(sector_buffer, SECTOR_SIZE, 1, disk);
    }
    
    free(buffer);
    fclose(disk);
    
    printf("File %s written to disk image (%d bytes, %d sectors)\n", filename, size, num_sectors);
}

void extract_file(const char* disk_image, const char* filename, const char* output_file) {
    FILE* disk = fopen(disk_image, "rb");
    if (!disk) {
        printf("Could not open disk image\n");
        return;
    }
    
    FileTable ft;
    fseek(disk, SECTOR_SIZE, SEEK_SET);
    fread(&ft, sizeof(FileTable), 1, disk);
    
    if (strncmp(ft.magic, K_MAGIC, 4) != 0) {
        printf("Invalid filesystem or not initialized\n");
        fclose(disk);
        return;
    }
    
    int file_index = -1;
    for (uint32_t i = 0; i < MAX_FILES; i++) {
        if (ft.files[i].in_use && strcmp(ft.files[i].filename, filename) == 0) {
            file_index = i;
            break;
        }
    }
    
    if (file_index == -1) {
        printf("File not found\n");
        fclose(disk);
        return;
    }
    
    FILE* out = fopen(output_file, "wb");
    if (!out) {
        printf("Could not create output file\n");
        fclose(disk);
        return;
    }
    
    uint32_t size = ft.files[file_index].size;
    uint32_t num_sectors = ft.files[file_index].num_sectors;
    uint32_t first_sector = ft.files[file_index].first_sector;
    
    uint8_t buffer[SECTOR_SIZE];
    for (uint32_t i = 0; i < num_sectors; i++) {
        fseek(disk, (first_sector + i) * SECTOR_SIZE, SEEK_SET);
        fread(buffer, SECTOR_SIZE, 1, disk);
        
        uint32_t bytes_to_write = (i == num_sectors - 1) ? 
                                (size % SECTOR_SIZE ? size % SECTOR_SIZE : SECTOR_SIZE) : 
                                SECTOR_SIZE;
        
        fwrite(buffer, bytes_to_write, 1, out);
    }
    
    fclose(out);
    fclose(disk);
    
    printf("File %s extracted to %s (%d bytes)\n", filename, output_file, size);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage:\n");
        printf("  %s create <disk_image> <size_mb>\n", argv[0]);
        printf("  %s format <disk_image>\n", argv[0]);
        printf("  %s list <disk_image>\n", argv[0]);
        printf("  %s write <disk_image> <file_on_disk> <source_file>\n", argv[0]);
        printf("  %s extract <disk_image> <file_on_disk> <output_file>\n", argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "create") == 0) {
        if (argc != 4) {
            printf("Usage: %s create <disk_image> <size_mb>\n", argv[0]);
            return 1;
        }
        create_disk_image(argv[2], atoi(argv[3]));
    }
    else if (strcmp(argv[1], "format") == 0) {
        if (argc != 3) {
            printf("Usage: %s format <disk_image>\n", argv[0]);
            return 1;
        }
        init_filesystem(argv[2]);
    }
    else if (strcmp(argv[1], "list") == 0) {
        if (argc != 3) {
            printf("Usage: %s list <disk_image>\n", argv[0]);
            return 1;
        }
        list_files(argv[2]);
    }
    else if (strcmp(argv[1], "write") == 0) {
        if (argc != 5) {
            printf("Usage: %s write <disk_image> <file_on_disk> <source_file>\n", argv[0]);
            return 1;
        }
        write_file_to_image(argv[2], argv[3], argv[4]);
    }
    else if (strcmp(argv[1], "extract") == 0) {
        if (argc != 5) {
            printf("Usage: %s extract <disk_image> <file_on_disk> <output_file>\n", argv[0]);
            return 1;
        }
        extract_file(argv[2], argv[3], argv[4]);
    }
    else {
        printf("Unknown command %s\n", argv[1]);
        return 1;
    }
    
    return 0;
}
