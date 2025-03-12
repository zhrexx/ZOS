#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>


#define XBIN_MAGIC 0x4E494258  


#define XBIN_FLAG_PRIVILEGED  0x01
#define XBIN_FLAG_SHARED      0x02
#define XBIN_FLAG_RELOCATABLE 0x04
#define XBIN_FLAG_HAS_SYMBOLS 0x08


#define RELOC_ABSOLUTE        0x01
#define RELOC_RELATIVE        0x02
#define RELOC_SYMBOL          0x03


#define SYMBOL_UNDEFINED     0x00
#define SYMBOL_FUNCTION      0x01
#define SYMBOL_DATA          0x02
#define SYMBOL_CONSTANT      0x03

#define BINDING_LOCAL        0x00
#define BINDING_GLOBAL       0x01
#define BINDING_EXTERNAL     0x02


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


typedef struct {
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint32_t eip;
    uint32_t eflags;
} CPUState;


typedef struct {
    uint8_t* code;
    uint8_t* data;
    uint8_t* bss;
    uint8_t* stack;
    
    uint32_t code_size;
    uint32_t data_size;
    uint32_t bss_size;
    uint32_t stack_size;
    
    uint32_t base_address;  
    CPUState cpu;
    
    
    char* string_table;
    XBINSymbol* symbols;
    uint32_t symbol_count;
    
    
    void** external_symbols;
    uint32_t external_count;
    
    bool debug_mode;
} RuntimeEnvironment;


static int load_xbin_file(const char* filename, RuntimeEnvironment* env);
static int apply_relocations(RuntimeEnvironment* env, XBINRelocation* relocs, uint32_t reloc_count);
static int resolve_external_symbols(RuntimeEnvironment* env);
static int execute(RuntimeEnvironment* env);
static void cleanup(RuntimeEnvironment* env);
static void init_environment(RuntimeEnvironment* env);
static void dump_state(RuntimeEnvironment* env);


uint32_t ext_print_num(uint32_t num) {
    printf("External function called with: %u\n", num);
    return num;
}

uint32_t ext_print_str(const char* str) {
    printf("Message: %s\n", str);
    return strlen(str);
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <xbin_file> [--debug]\n", argv[0]);
        return 1;
    }
    
    const char* filename = argv[1];
    bool debug_mode = false;
    
    
    if (argc > 2 && strcmp(argv[2], "--debug") == 0) {
        debug_mode = true;
        printf("Debug mode enabled\n");
    }
    
    RuntimeEnvironment env;
    init_environment(&env);
    env.debug_mode = debug_mode;
    
    
    if (load_xbin_file(filename, &env) != 0) {
        fprintf(stderr, "Failed to load XBIN file: %s\n", filename);
        return 1;
    }
    
    printf("XBIN file loaded successfully:\n");
    printf("  Code size: %u bytes\n", env.code_size);
    printf("  Data size: %u bytes\n", env.data_size);
    printf("  BSS size: %u bytes\n", env.bss_size);
    printf("  Stack size: %u bytes\n", env.stack_size);
    printf("  Entry point: 0x%08X\n", env.cpu.eip);
    
    
    printf("\nStarting execution...\n");
    printf("----------------------------------------\n");
    
    int result = execute(&env);
    
    printf("----------------------------------------\n");
    if (result == 0) {
        printf("Program executed successfully\n");
        printf("Exit code: %d\n", env.cpu.eax);
    } else {
        fprintf(stderr, "Execution failed with error code %d\n", result);
    }
    
    if (debug_mode) {
        dump_state(&env);
    }
    
    
    cleanup(&env);
    
    return result;
}


static void init_environment(RuntimeEnvironment* env) {
    memset(env, 0, sizeof(RuntimeEnvironment));
    
    
    env->external_count = 2;
    env->external_symbols = malloc(sizeof(void*) * env->external_count);
    env->external_symbols[0] = (void*)ext_print_num;
    env->external_symbols[1] = (void*)ext_print_str;
}


static int load_xbin_file(const char* filename, RuntimeEnvironment* env) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return 1;
    }
    
    
    XBINHeader header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fprintf(stderr, "Error: Failed to read XBIN header\n");
        fclose(file);
        return 1;
    }
    
    
    if (header.magic != XBIN_MAGIC) {
        fprintf(stderr, "Error: Invalid XBIN file (incorrect magic number)\n");
        fclose(file);
        return 1;
    }
    
    
    if (header.version != 1) {
        fprintf(stderr, "Error: Unsupported XBIN version: %u\n", header.version);
        fclose(file);
        return 1;
    }
    
    
    env->code_size = header.code_size;
    env->data_size = header.data_size;
    env->bss_size = header.bss_size;
    env->stack_size = header.stack_size;
    
    env->code = malloc(env->code_size);
    env->data = malloc(env->data_size);
    env->bss = calloc(1, env->bss_size);  
    env->stack = malloc(env->stack_size);
    
    if (!env->code || !env->data || !env->bss || !env->stack) {
        fprintf(stderr, "Error: Failed to allocate memory for XBIN segments\n");
        cleanup(env);
        fclose(file);
        return 1;
    }
    
    
    if (fread(env->code, 1, env->code_size, file) != env->code_size) {
        fprintf(stderr, "Error: Failed to read code segment\n");
        cleanup(env);
        fclose(file);
        return 1;
    }
    
    
    if (fread(env->data, 1, env->data_size, file) != env->data_size) {
        fprintf(stderr, "Error: Failed to read data segment\n");
        cleanup(env);
        fclose(file);
        return 1;
    }
    
    
    XBINRelocation* relocations = NULL;
    if (header.reloc_count > 0) {
        relocations = malloc(sizeof(XBINRelocation) * header.reloc_count);
        if (!relocations) {
            fprintf(stderr, "Error: Failed to allocate memory for relocations\n");
            cleanup(env);
            fclose(file);
            return 1;
        }
        
        if (fread(relocations, sizeof(XBINRelocation), header.reloc_count, file) != header.reloc_count) {
            fprintf(stderr, "Error: Failed to read relocations\n");
            free(relocations);
            cleanup(env);
            fclose(file);
            return 1;
        }
    }
    
    
    if (header.flags & XBIN_FLAG_HAS_SYMBOLS) {
        
        env->string_table = malloc(header.string_table_size);
        if (!env->string_table) {
            fprintf(stderr, "Error: Failed to allocate memory for string table\n");
            free(relocations);
            cleanup(env);
            fclose(file);
            return 1;
        }
        
        fseek(file, header.string_table_offset, SEEK_SET);
        if (fread(env->string_table, 1, header.string_table_size, file) != header.string_table_size) {
            fprintf(stderr, "Error: Failed to read string table\n");
            free(relocations);
            cleanup(env);
            fclose(file);
            return 1;
        }
        
        
        env->symbol_count = header.symbol_count;
        env->symbols = malloc(sizeof(XBINSymbol) * env->symbol_count);
        if (!env->symbols) {
            fprintf(stderr, "Error: Failed to allocate memory for symbols\n");
            free(relocations);
            cleanup(env);
            fclose(file);
            return 1;
        }
        
        if (fread(env->symbols, sizeof(XBINSymbol), env->symbol_count, file) != env->symbol_count) {
            fprintf(stderr, "Error: Failed to read symbols\n");
            free(relocations);
            cleanup(env);
            fclose(file);
            return 1;
        }
    }
    
    fclose(file);
    
    
    memset(&env->cpu, 0, sizeof(CPUState));
    env->cpu.eip = header.entry_point;
    env->cpu.esp = env->stack_size - 4;  
    
    
    if (header.flags & XBIN_FLAG_RELOCATABLE && relocations) {
        if (apply_relocations(env, relocations, header.reloc_count) != 0) {
            fprintf(stderr, "Error: Failed to apply relocations\n");
            free(relocations);
            return 1;
        }
    }
    
    
    if (relocations) {
        free(relocations);
    }
    
    
    if (resolve_external_symbols(env) != 0) {
        fprintf(stderr, "Error: Failed to resolve external symbols\n");
        return 1;
    }
    
    return 0;
}


static int apply_relocations(RuntimeEnvironment* env, XBINRelocation* relocs, uint32_t reloc_count) {
    for (uint32_t i = 0; i < reloc_count; i++) {
        uint32_t offset = relocs[i].offset;
        uint32_t type = relocs[i].type;
        
        if (offset >= env->code_size) {
            fprintf(stderr, "Error: Relocation offset out of bounds: %u\n", offset);
            return 1;
        }
        
        uint32_t* addr = (uint32_t*)(env->code + offset);
        
        switch (type) {
            case RELOC_ABSOLUTE:
                
                *addr += (uint32_t)env->code;
                break;
                
            case RELOC_RELATIVE:
                
                *addr += (uint32_t)env->code + offset + 4;
                break;
                
            case RELOC_SYMBOL:
                
                break;
                
            default:
                fprintf(stderr, "Error: Unknown relocation type: %u\n", type);
                return 1;
        }
    }
    
    return 0;
}


static int resolve_external_symbols(RuntimeEnvironment* env) {
    if (!env->symbols || !env->string_table) {
        return 0;  
    }
    
    for (uint32_t i = 0; i < env->symbol_count; i++) {
        XBINSymbol* sym = &env->symbols[i];
        
        if (sym->binding == BINDING_EXTERNAL) {
            const char* name = env->string_table + sym->name_offset;
            
            
            bool found = false;
            
            if (strcmp(name, "print_num") == 0 && env->external_count > 0) {
                sym->value = (uint32_t)env->external_symbols[0];
                found = true;
            } else if (strcmp(name, "print_str") == 0 && env->external_count > 1) {
                sym->value = (uint32_t)env->external_symbols[1];
                found = true;
            }
            
            if (!found) {
                fprintf(stderr, "Error: Unresolved external symbol: %s\n", name);
                return 1;
            }
        }
    }
    
    return 0;
}


static int execute(RuntimeEnvironment* env) {
    
    while (env->cpu.eip < env->code_size) {
        uint8_t opcode = env->code[env->cpu.eip];
        
        if (env->debug_mode) {
            printf("EIP: 0x%08X, Opcode: 0x%02X\n", env->cpu.eip, opcode);
        }
        
        switch (opcode) {
            case 0x90:  
                env->cpu.eip++;
                break;
                
            case 0xC3:  
                
                return 0;
                
            case 0xB8: case 0xB9: case 0xBA: case 0xBB:
            case 0xBC: case 0xBD: case 0xBE: case 0xBF: {
                
                uint32_t reg_idx = opcode - 0xB8;
                uint32_t value = *(uint32_t*)(env->code + env->cpu.eip + 1);
                
                
                uint32_t* regs = &env->cpu.eax;
                regs[reg_idx] = value;
                
                env->cpu.eip += 5;  
                break;
            }
                
            case 0x01: {
                
                uint8_t modrm = env->code[env->cpu.eip + 1];
                uint8_t dst_reg = modrm & 0x07;
                uint8_t src_reg = (modrm >> 3) & 0x07;
                
                
                uint32_t* regs = &env->cpu.eax;
                regs[dst_reg] += regs[src_reg];
                
                env->cpu.eip += 2;  
                break;
            }
                
            case 0x29: {
                
                uint8_t modrm = env->code[env->cpu.eip + 1];
                uint8_t dst_reg = modrm & 0x07;
                uint8_t src_reg = (modrm >> 3) & 0x07;
                
                
                uint32_t* regs = &env->cpu.eax;
                regs[dst_reg] -= regs[src_reg];
                
                env->cpu.eip += 2;  
                break;
            }
                
            case 0x50: case 0x51: case 0x52: case 0x53:
            case 0x54: case 0x55: case 0x56: case 0x57: {
                
                uint32_t reg_idx = opcode - 0x50;
                uint32_t* regs = &env->cpu.eax;
                
                
                env->cpu.esp -= 4;
                
                
                if (env->cpu.esp < env->stack_size) {
                    *(uint32_t*)(env->stack + env->cpu.esp) = regs[reg_idx];
                } else {
                    fprintf(stderr, "Error: Stack overflow\n");
                    return 1;
                }
                
                env->cpu.eip++;
                break;
            }
                
            case 0x58: case 0x59: case 0x5A: case 0x5B:
            case 0x5C: case 0x5D: case 0x5E: case 0x5F: {
                
                uint32_t reg_idx = opcode - 0x58;
                uint32_t* regs = &env->cpu.eax;
                
                
                if (env->cpu.esp < env->stack_size) {
                    regs[reg_idx] = *(uint32_t*)(env->stack + env->cpu.esp);
                } else {
                    fprintf(stderr, "Error: Stack underflow\n");
                    return 1;
                }
                
                
                env->cpu.esp += 4;
                
                env->cpu.eip++;
                break;
            }
                
            case 0xE8: {
                
                int32_t offset = *(int32_t*)(env->code + env->cpu.eip + 1);
                
                
                if (offset > (int32_t)env->code_size || offset < 0) {
                    
                    typedef uint32_t (*ExtFunc)(uint32_t);
                    ExtFunc func = (ExtFunc)offset;
                    
                    
                    env->cpu.eax = func(env->cpu.eax);
                } else {
                    
                    env->cpu.esp -= 4;
                    if (env->cpu.esp < env->stack_size) {
                        *(uint32_t*)(env->stack + env->cpu.esp) = env->cpu.eip + 5;
                    } else {
                        fprintf(stderr, "Error: Stack overflow during CALL\n");
                        return 1;
                    }
                    
                    
                    env->cpu.eip += offset + 5;
                    continue;  
                }
                
                env->cpu.eip += 5;  
                break;
            }
                
            case 0xEB: {
                
                int8_t offset = (int8_t)env->code[env->cpu.eip + 1];
                env->cpu.eip += offset + 2;  
                continue;  
            }
                
            default:
                fprintf(stderr, "Error: Unknown opcode 0x%02X at offset 0x%08X\n", 
                        opcode, env->cpu.eip);
                return 1;
        }
    }
    
    fprintf(stderr, "Error: Execution went beyond code bounds\n");
    return 1;
}


static void dump_state(RuntimeEnvironment* env) {
    printf("\nCPU State:\n");
    printf("  EAX: 0x%08X   ECX: 0x%08X   EDX: 0x%08X   EBX: 0x%08X\n",
           env->cpu.eax, env->cpu.ecx, env->cpu.edx, env->cpu.ebx);
    printf("  ESP: 0x%08X   EBP: 0x%08X   ESI: 0x%08X   EDI: 0x%08X\n",
           env->cpu.esp, env->cpu.ebp, env->cpu.esi, env->cpu.edi);
    printf("  EIP: 0x%08X   EFLAGS: 0x%08X\n", env->cpu.eip, env->cpu.eflags);
    
    printf("\nStack dump (top 64 bytes):\n");
    uint32_t stack_top = env->cpu.esp;
    uint32_t dump_size = (stack_top + 64 < env->stack_size) ? 64 : env->stack_size - stack_top;
    
    for (uint32_t i = 0; i < dump_size; i += 4) {
        if (i % 16 == 0) {
            printf("\n  %08X: ", stack_top + i);
        }
        printf("%08X ", *(uint32_t*)(env->stack + stack_top + i));
    }
    printf("\n");
    
    
    if (env->symbols && env->string_table) {
        printf("\nSymbols:\n");
        for (uint32_t i = 0; i < env->symbol_count; i++) {
            const char* name = env->string_table + env->symbols[i].name_offset;
            const char* binding = "";
            switch (env->symbols[i].binding) {
                case BINDING_LOCAL: binding = "LOCAL"; break;
                case BINDING_GLOBAL: binding = "GLOBAL"; break;
                case BINDING_EXTERNAL: binding = "EXTERNAL"; break;
            }
            
            const char* type = "";
            switch (env->symbols[i].type) {
                case SYMBOL_UNDEFINED: type = "UNDEFINED"; break;
                case SYMBOL_FUNCTION: type = "FUNCTION"; break;
                case SYMBOL_DATA: type = "DATA"; break;
                case SYMBOL_CONSTANT: type = "CONSTANT"; break;
            }
            
            printf("  %s: 0x%08X  [%s, %s]\n", name, env->symbols[i].value, binding, type);
        }
    }
}


static void cleanup(RuntimeEnvironment* env) {
    if (env->code) free(env->code);
    if (env->data) free(env->data);
    if (env->bss) free(env->bss);
    if (env->stack) free(env->stack);
    if (env->string_table) free(env->string_table);
    if (env->symbols) free(env->symbols);
    if (env->external_symbols) free(env->external_symbols);
    
    memset(env, 0, sizeof(RuntimeEnvironment));
}
