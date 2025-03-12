#ifndef XBIN_H
#define XBIN_H

#include "msstd.h"
#include "libs/disk.h"

#define XBIN_MAGIC 0x4E494258

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t entry_point;
    uint32_t code_size;
    uint32_t data_size;
    uint32_t bss_size;
    uint32_t stack_size;
    uint32_t reloc_count;
    uint8_t  flags;
    uint32_t string_table_offset;   
    uint32_t string_table_size;     
    uint32_t symbol_count;          
} XBINHeader;

typedef struct {
    uint32_t offset;
    uint32_t type;
} XBINRelocation;

typedef struct {
    uint32_t name_offset;    
    uint32_t value;          
    uint8_t  type;           
    uint8_t  binding;        
} XBINSymbol;


#define SYMBOL_UNDEFINED     0x00
#define SYMBOL_FUNCTION      0x01
#define SYMBOL_DATA          0x02
#define SYMBOL_CONSTANT      0x03


#define BINDING_LOCAL        0x00
#define BINDING_GLOBAL       0x01
#define BINDING_EXTERNAL     0x02


#define XBIN_FLAG_PRIVILEGED  0x01
#define XBIN_FLAG_SHARED      0x02
#define XBIN_FLAG_RELOCATABLE 0x04
#define XBIN_FLAG_HAS_SYMBOLS 0x08


#define RELOC_ABSOLUTE        0x01
#define RELOC_RELATIVE        0x02
#define RELOC_SYMBOL          0x03


typedef struct {
    char* name;
    uint32_t value;
    uint8_t type;
} GlobalSymbol;

#define MAX_GLOBAL_SYMBOLS 256
static GlobalSymbol g_symbols[MAX_GLOBAL_SYMBOLS];
static uint32_t g_symbol_count = 0;


int xbin_register_symbol(const char* name, uint32_t value, uint8_t type) {
    if (g_symbol_count >= MAX_GLOBAL_SYMBOLS) {
        return -1;
    }
    
    uint32_t name_len = strlen(name) + 1;
    g_symbols[g_symbol_count].name = (char*)malloc(name_len);
    if (!g_symbols[g_symbol_count].name) {
        return -1;
    }
    
    strcpy(g_symbols[g_symbol_count].name, name);
    g_symbols[g_symbol_count].value = value;
    g_symbols[g_symbol_count].type = type;
    g_symbol_count++;
    
    return 0;
}


uint32_t xbin_lookup_symbol(const char* name) {
    for (uint32_t i = 0; i < g_symbol_count; i++) {
        if (strcmp(g_symbols[i].name, name) == 0) {
            return g_symbols[i].value;
        }
    }
    return 0;
}


void xbin_cleanup_symbols() {
    for (uint32_t i = 0; i < g_symbol_count; i++) {
        free(g_symbols[i].name);
    }
    g_symbol_count = 0;
}


char* get_string(uint8_t* string_table, uint32_t offset) {
    return (char*)(string_table + offset);
}

uint32_t xbin_load(const char* filename, uint32_t* memory_size) {
    uint8_t* buffer;
    uint32_t size;
    XBINHeader header;
    
    if (!fs_file_exists(filename)) {
        printf("File not found: %s\n", filename);
        return 0;
    }
    
    size = fs_get_file_size(filename);
    if (size == DISK_ERROR || size == DISK_NOT_FOUND) {
        printf("Error accessing file: %s\n", filename);
        return 0;
    }
    
    buffer = (uint8_t*)malloc(size);
    if (!buffer) {
        printf("Memory allocation failed\n");
        return 0;
    }
    
    if (fs_read_file(filename, buffer, &size) != 0) {
        printf("Error reading file: %s\n", filename);
        free(buffer);
        return 0;
    }
    
    if (size < sizeof(XBINHeader)) {
        printf("Invalid XBIN file: too small\n");
        free(buffer);
        return 0;
    }
    
    memcpy(&header, buffer, sizeof(XBINHeader));
    if (header.magic != XBIN_MAGIC) {
        printf("Invalid XBIN file: incorrect magic number\n");
        free(buffer);
        return 0;
    }
    
    uint32_t total_size = header.code_size + header.data_size + header.bss_size + header.stack_size;
    uint8_t* exec_memory = (uint8_t*)malloc(total_size);
    if (!exec_memory) {
        printf("Failed to allocate memory for executable\n");
        free(buffer);
        return 0;
    }
    
    memset(exec_memory, 0, total_size);
    memcpy(exec_memory, buffer + sizeof(XBINHeader), header.code_size);
    memcpy(exec_memory + header.code_size, 
           buffer + sizeof(XBINHeader) + header.code_size, 
           header.data_size);

    
    if (header.flags & XBIN_FLAG_HAS_SYMBOLS && header.symbol_count > 0) {
        uint8_t* string_table = buffer + header.string_table_offset;
        XBINSymbol* symbols = (XBINSymbol*)(buffer + sizeof(XBINHeader) + 
                                           header.code_size + header.data_size + 
                                           header.reloc_count * sizeof(XBINRelocation));
        
        
        for (uint32_t i = 0; i < header.symbol_count; i++) {
            char* symbol_name = get_string(string_table, symbols[i].name_offset);
            
            
            if (symbols[i].binding == BINDING_EXTERNAL) {
                uint32_t external_value = xbin_lookup_symbol(symbol_name);
                if (external_value == 0) {
                    printf("Unresolved external symbol: %s\n", symbol_name);
                    free(exec_memory);
                    free(buffer);
                    return 0;
                }
                symbols[i].value = external_value;
            }
        }
        
        
        if (header.flags & XBIN_FLAG_RELOCATABLE) {
            XBINRelocation* relocs = (XBINRelocation*)(buffer + sizeof(XBINHeader) + 
                                                    header.code_size + header.data_size);
            
            for (uint32_t i = 0; i < header.reloc_count; i++) {
                uint32_t* target = (uint32_t*)(exec_memory + relocs[i].offset);
                
                switch (relocs[i].type) {
                    case RELOC_ABSOLUTE:
                        *target += (uint32_t)exec_memory;
                        break;
                    case RELOC_RELATIVE:
                        *target += (uint32_t)exec_memory - relocs[i].offset - 4;
                        break;
                    case RELOC_SYMBOL: {
                        
                        uint32_t symbol_index = *target;
                        if (symbol_index >= header.symbol_count) {
                            printf("Invalid symbol index in relocation: %d\n", symbol_index);
                            free(exec_memory);
                            free(buffer);
                            return 0;
                        }
                        *target = symbols[symbol_index].value;
                        break;
                    }
                    default:
                        printf("Unknown relocation type: %d\n", relocs[i].type);
                        break;
                }
            }
        }
    }
    
    free(buffer);
    *memory_size = total_size;
    return (uint32_t)exec_memory + header.entry_point;
}

int xbin_execute(uint32_t entry_point) {
    if (entry_point == 0) {
        return -1;
    }
    int (*entry_func)(void) = (int (*)(void))entry_point;
    return entry_func();
}

int xbin_run(const char* filename) {
    uint32_t memory_size;
    uint32_t entry_point = xbin_load(filename, &memory_size);
    if (entry_point == 0) {
        return -1;
    }
    int result = xbin_execute(entry_point);
    free((void*)(entry_point - sizeof(XBINHeader)));
    return result;
}

int xbin_validate(const char* filename) {
    uint8_t buffer[sizeof(XBINHeader)];
    uint32_t size;
    
    if (!fs_file_exists(filename)) {
        return 0;
    }
    
    if (fs_read_file(filename, buffer, &size) != 0 || size < sizeof(XBINHeader)) {
        return 0;
    }
    
    XBINHeader* header = (XBINHeader*)buffer;
    return (header->magic == XBIN_MAGIC);
}


#endif
