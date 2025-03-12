#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>

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


#define MAX_LINE_LENGTH       1024
#define MAX_LABEL_LENGTH      64
#define MAX_INSTRUCTION_COUNT 10000
#define MAX_LABELS            1000
#define MAX_DATA_SIZE         1048576  
#define MAX_CODE_SIZE         1048576
#define MAX_RELOCS            1000
#define MAX_SYMBOLS           1000     
#define MAX_STRING_TABLE_SIZE 32768    


typedef enum {
    OP_NONE,
    OP_REGISTER,
    OP_IMMEDIATE,
    OP_MEMORY,
    OP_LABEL
} OperandType;


typedef enum {
    REG_EAX, REG_ECX, REG_EDX, REG_EBX,
    REG_ESP, REG_EBP, REG_ESI, REG_EDI,
    REG_AX, REG_CX, REG_DX, REG_BX,
    REG_SP, REG_BP, REG_SI, REG_DI,
    REG_AL, REG_CL, REG_DL, REG_BL,
    REG_AH, REG_CH, REG_DH, REG_BH,
    REG_CS, REG_DS, REG_ES, REG_FS, REG_GS, REG_SS,
    REG_UNKNOWN
} Register;


typedef struct {
    OperandType type;
    union {
        Register reg;
        int32_t immediate;
        struct {
            Register base_reg;
            Register index_reg;
            int32_t scale;
            int32_t displacement;
        } mem;
        char label[MAX_LABEL_LENGTH];
    } value;
} Operand;


typedef struct {
    char mnemonic[16];
    Operand operands[3];
    int operand_count;
    uint32_t address;
    uint32_t size;
    bool needs_relocation;
    int reloc_type;
    int reloc_offset;
} Instruction;


typedef struct {
    char name[MAX_LABEL_LENGTH];
    uint32_t address;
    bool is_data;
    uint8_t binding;        
    uint8_t symbol_type;    
} Label;


typedef struct {
    uint8_t* data;
    uint32_t size;
    uint32_t address;
} DataItem;


typedef struct {
    Instruction instructions[MAX_INSTRUCTION_COUNT];
    int instruction_count;
    
    Label labels[MAX_LABELS];
    int label_count;
    
    uint8_t code[MAX_CODE_SIZE];
    uint32_t code_size;
    
    uint8_t data[MAX_DATA_SIZE];
    uint32_t data_size;
    
    uint32_t bss_size;
    uint32_t stack_size;
    
    XBINRelocation relocations[MAX_RELOCS];
    int relocation_count;
    
    
    XBINSymbol symbols[MAX_SYMBOLS];
    int symbol_count;
    
    char string_table[MAX_STRING_TABLE_SIZE];
    uint32_t string_table_size;
    
    uint8_t flags;
    char* entry_point;
    int current_pass;
} AssemblerState;


struct RegEntry {
    const char* name;
    Register reg;
};

static const struct RegEntry register_table[] = {
    {"eax", REG_EAX}, {"ecx", REG_ECX}, {"edx", REG_EDX}, {"ebx", REG_EBX},
    {"esp", REG_ESP}, {"ebp", REG_EBP}, {"esi", REG_ESI}, {"edi", REG_EDI},
    {"ax", REG_AX}, {"cx", REG_CX}, {"dx", REG_DX}, {"bx", REG_BX},
    {"sp", REG_SP}, {"bp", REG_BP}, {"si", REG_SI}, {"di", REG_DI},
    {"al", REG_AL}, {"cl", REG_CL}, {"dl", REG_DL}, {"bl", REG_BL},
    {"ah", REG_AH}, {"ch", REG_CH}, {"dh", REG_DH}, {"bh", REG_BH},
    {"cs", REG_CS}, {"ds", REG_DS}, {"es", REG_ES}, {"fs", REG_FS},
    {"gs", REG_GS}, {"ss", REG_SS},
    {NULL, REG_UNKNOWN}
};


static void init_assembler(AssemblerState* state);
static int parse_file(AssemblerState* state, FILE* file);
static int encode_instructions(AssemblerState* state);
static int write_output(AssemblerState* state, const char* filename);
static int add_label(AssemblerState* state, const char* name, uint32_t address, bool is_data);
static Label* find_label(AssemblerState* state, const char* name);
static Register get_register(const char* reg_name);
static int parse_operand(char* token, Operand* operand);
static void strip_comments(char* line);
static char* trim(char* str);
static int parse_directive(AssemblerState* state, char* line);
static int parse_instruction(AssemblerState* state, char* line, uint32_t address);
static int encode_instruction(AssemblerState* state, Instruction* instr);
static int add_relocation(AssemblerState* state, uint32_t offset, uint32_t type);

static int add_symbol(AssemblerState* state, const char* name, uint32_t value, uint8_t type, uint8_t binding);
static uint32_t add_to_string_table(AssemblerState* state, const char* str);
static int generate_symbols(AssemblerState* state);


int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.asm> <output.xbin>\n", argv[0]);
        return 1;
    }
    
    FILE* input_file = fopen(argv[1], "r");
    if (!input_file) {
        fprintf(stderr, "Error: Cannot open input file '%s'\n", argv[1]);
        return 1;
    }
    
    AssemblerState state;
    init_assembler(&state);
    
    
    state.current_pass = 1;
    if (parse_file(&state, input_file) != 0) {
        fclose(input_file);
        return 1;
    }
    
    
    rewind(input_file);
    
    
    state.current_pass = 2;
    if (parse_file(&state, input_file) != 0) {
        fclose(input_file);
        return 1;
    }
    
    fclose(input_file);
    
    
    if (encode_instructions(&state) != 0) {
        return 1;
    }
    
    
    if (generate_symbols(&state) != 0) {
        return 1;
    }
    
    
    if (write_output(&state, argv[2]) != 0) {
        return 1;
    }
    
    printf("Assembly complete. Output written to %s\n", argv[2]);
    printf("Code size: %u bytes\n", state.code_size);
    printf("Data size: %u bytes\n", state.data_size);
    printf("BSS size: %u bytes\n", state.bss_size);
    printf("Stack size: %u bytes\n", state.stack_size);
    printf("Relocations: %d\n", state.relocation_count);
    printf("Symbols: %d\n", state.symbol_count);
    printf("String table size: %u bytes\n", state.string_table_size);
    
    return 0;
}


static void init_assembler(AssemblerState* state) {
    memset(state, 0, sizeof(AssemblerState));
    state->stack_size = 4096;  
    state->flags = 0;
    state->entry_point = NULL;
    state->string_table_size = 1; 
    state->string_table[0] = '\0'; 
}


static int parse_file(AssemblerState* state, FILE* file) {
    char line[MAX_LINE_LENGTH];
    uint32_t code_address = 0;
    uint32_t data_address = 0;
    
    while (fgets(line, sizeof(line), file)) {
        
        strip_comments(line);
        char* trimmed = trim(line);
        
        if (*trimmed == '\0') {
            continue;  
        }
        
        
        char* colon = strchr(trimmed, ':');
        if (colon) {
            *colon = '\0';
            char* label_name = trim(trimmed);
            
            
            bool is_data = false;
            if (state->current_pass == 1) {
                char* next_line = colon + 1;
                next_line = trim(next_line);
                is_data = (strncmp(next_line, "db", 2) == 0 || 
                          strncmp(next_line, "dw", 2) == 0 || 
                          strncmp(next_line, "dd", 2) == 0 ||
                          strncmp(next_line, "resb", 4) == 0 ||
                          strncmp(next_line, "resw", 4) == 0 ||
                          strncmp(next_line, "resd", 4) == 0);
            }
            
            
            if (state->current_pass == 1) {
                uint32_t addr = is_data ? data_address : code_address;
                if (add_label(state, label_name, addr, is_data) != 0) {
                    return 1;
                }
            }
            
            
            trimmed = colon + 1;
            trimmed = trim(trimmed);
            if (*trimmed == '\0') {
                continue;  
            }
        }
        
        
        if (*trimmed == '.') {
            if (parse_directive(state, trimmed) != 0) {
                return 1;
            }
            continue;
        }
        
        
        if (state->current_pass == 2) {
            Instruction* instr = &state->instructions[state->instruction_count];
            if (parse_instruction(state, trimmed, code_address) != 0) {
                return 1;
            }
            
            code_address += instr->size;
            state->instruction_count++;
        } else {
            
            
            code_address += 4;  
        }
    }
    
    return 0;
}


static void strip_comments(char* line) {
    char* comment = strchr(line, ';');
    if (comment) {
        *comment = '\0';
    }
}


static char* trim(char* str) {
    char* end;
    
    
    while (isspace((unsigned char)*str)) {
        str++;
    }
    
    if (*str == 0) {  
        return str;
    }
    
    
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }
    
    
    *(end + 1) = 0;
    
    return str;
}


static int parse_directive(AssemblerState* state, char* line) {
    char directive[32];
    int result = sscanf(line, ".%31s", directive);
    
    if (result != 1) {
        fprintf(stderr, "Error: Invalid directive format: %s\n", line);
        return 1;
    }
    
    if (strcmp(directive, "entry") == 0) {
        char entry_point[MAX_LABEL_LENGTH];
        if (sscanf(line, ".entry %63s", entry_point) != 1) {
            fprintf(stderr, "Error: Invalid entry point directive\n");
            return 1;
        }
        
        if (state->current_pass == 1) {
            state->entry_point = strdup(entry_point);
        }
    } else if (strcmp(directive, "stack") == 0) {
        int size;
        if (sscanf(line, ".stack %d", &size) != 1) {
            fprintf(stderr, "Error: Invalid stack directive\n");
            return 1;
        }
        
        if (state->current_pass == 1) {
            state->stack_size = (uint32_t)size;
        }
    } else if (strcmp(directive, "privileged") == 0) {
        if (state->current_pass == 1) {
            state->flags |= XBIN_FLAG_PRIVILEGED;
        }
    } else if (strcmp(directive, "shared") == 0) {
        if (state->current_pass == 1) {
            state->flags |= XBIN_FLAG_SHARED;
        }
    } else if (strcmp(directive, "relocatable") == 0) {
        if (state->current_pass == 1) {
            state->flags |= XBIN_FLAG_RELOCATABLE;
        }
    } else if (strcmp(directive, "bss") == 0) {
        int size;
        if (sscanf(line, ".bss %d", &size) != 1) {
            fprintf(stderr, "Error: Invalid bss directive\n");
            return 1;
        }
        
        if (state->current_pass == 1) {
            state->bss_size = (uint32_t)size;
        }
    } else if (strcmp(directive, "global") == 0) {
        
        char symbol_name[MAX_LABEL_LENGTH];
        if (sscanf(line, ".global %63s", symbol_name) != 1) {
            fprintf(stderr, "Error: Invalid global directive\n");
            return 1;
        }
        
        if (state->current_pass == 1) {
            
            for (int i = 0; i < state->label_count; i++) {
                if (strcmp(state->labels[i].name, symbol_name) == 0) {
                    state->labels[i].binding = BINDING_GLOBAL;
                    break;
                }
            }
        }
    } else if (strcmp(directive, "extern") == 0) {
        
        char symbol_name[MAX_LABEL_LENGTH];
        if (sscanf(line, ".extern %63s", symbol_name) != 1) {
            fprintf(stderr, "Error: Invalid extern directive\n");
            return 1;
        }
        
        if (state->current_pass == 1) {
            
            if (add_label(state, symbol_name, 0, false) != 0) {
                return 1;
            }
            
            
            state->labels[state->label_count-1].binding = BINDING_EXTERNAL;
            state->labels[state->label_count-1].symbol_type = SYMBOL_UNDEFINED;
        }
    } else {
        fprintf(stderr, "Warning: Unknown directive: %s\n", directive);
    }
    
    return 0;
}


static int add_label(AssemblerState* state, const char* name, uint32_t address, bool is_data) {
    if (state->label_count >= MAX_LABELS) {
        fprintf(stderr, "Error: Too many labels\n");
        return 1;
    }
    
    
    for (int i = 0; i < state->label_count; i++) {
        if (strcmp(state->labels[i].name, name) == 0) {
            fprintf(stderr, "Error: Duplicate label: %s\n", name);
            return 1;
        }
    }
    
    
    Label* label = &state->labels[state->label_count++];
    strncpy(label->name, name, MAX_LABEL_LENGTH - 1);
    label->name[MAX_LABEL_LENGTH - 1] = '\0';
    label->address = address;
    label->is_data = is_data;
    label->binding = BINDING_LOCAL; 
    label->symbol_type = is_data ? SYMBOL_DATA : SYMBOL_FUNCTION; 
    
    return 0;
}


static Label* find_label(AssemblerState* state, const char* name) {
    for (int i = 0; i < state->label_count; i++) {
        if (strcmp(state->labels[i].name, name) == 0) {
            return &state->labels[i];
        }
    }
    return NULL;
}


static Register get_register(const char* reg_name) {
    for (int i = 0; register_table[i].name != NULL; i++) {
        if (strcasecmp(register_table[i].name, reg_name) == 0) {
            return register_table[i].reg;
        }
    }
    return REG_UNKNOWN;
}


static int parse_operand(char* token, Operand* operand) {
    token = trim(token);
    
    
    Register reg = get_register(token);
    if (reg != REG_UNKNOWN) {
        operand->type = OP_REGISTER;
        operand->value.reg = reg;
        return 0;
    }
    
    
    if (token[0] == '#') {
        operand->type = OP_IMMEDIATE;
        operand->value.immediate = (int32_t)strtol(token + 1, NULL, 0);
        return 0;
    }
    
    
    if (token[0] == '$') {
        operand->type = OP_IMMEDIATE;
        operand->value.immediate = (int32_t)strtol(token + 1, NULL, 0);
        return 0;
    }
    
    
    if (isdigit(token[0]) || 
        (token[0] == '-' && isdigit(token[1])) || 
        (token[0] == '0' && (token[1] == 'x' || token[1] == 'X')) ||
        (token[0] == '0' && isdigit(token[1]))) {
        operand->type = OP_IMMEDIATE;
        operand->value.immediate = (int32_t)strtol(token, NULL, 0);
        return 0;
    }
    
    
    if (token[0] == '[') {
        char* closing = strchr(token, ']');
        if (!closing) {
            fprintf(stderr, "Error: Missing closing bracket in memory operand\n");
            return 1;
        }
        
        *closing = '\0';
        char* contents = token + 1;
        
        
        operand->type = OP_MEMORY;
        operand->value.mem.base_reg = REG_UNKNOWN;
        operand->value.mem.index_reg = REG_UNKNOWN;
        operand->value.mem.scale = 0;
        operand->value.mem.displacement = 0;
        
        
        Register base = get_register(contents);
        if (base != REG_UNKNOWN) {
            operand->value.mem.base_reg = base;
        } else {
            
            operand->value.mem.displacement = (int32_t)strtol(contents, NULL, 0);
        }
        
        return 0;
    }
    
    
    operand->type = OP_LABEL;
    strncpy(operand->value.label, token, MAX_LABEL_LENGTH - 1);
    operand->value.label[MAX_LABEL_LENGTH - 1] = '\0';
    
    return 0;
}


static int parse_instruction(AssemblerState* state, char* line, uint32_t address) {
    if (state->instruction_count >= MAX_INSTRUCTION_COUNT) {
        fprintf(stderr, "Error: Too many instructions\n");
        return 1;
    }
    
    Instruction* instr = &state->instructions[state->instruction_count];
    instr->address = address;
    instr->operand_count = 0;
    instr->needs_relocation = false;
    
    
    char* space = strchr(line, ' ');
    if (space) {
        int mnemonic_len = space - line;
        if (mnemonic_len >= 16) {
            fprintf(stderr, "Error: Mnemonic too long: %.*s\n", mnemonic_len, line);
            return 1;
        }
        
        strncpy(instr->mnemonic, line, mnemonic_len);
        instr->mnemonic[mnemonic_len] = '\0';
        
        
        char* operand_str = space + 1;
        char* comma;
        
        while ((operand_str = trim(operand_str)) && *operand_str) {
            if (instr->operand_count >= 3) {
                fprintf(stderr, "Error: Too many operands\n");
                return 1;
            }
            
            comma = strchr(operand_str, ',');
            if (comma) {
                *comma = '\0';
            }
            
            if (parse_operand(operand_str, &instr->operands[instr->operand_count]) != 0) {
                return 1;
            }
            
            instr->operand_count++;
            
            if (comma) {
                operand_str = comma + 1;
            } else {
                break;
            }
        }
    } else {
        
        strncpy(instr->mnemonic, line, 15);
        instr->mnemonic[15] = '\0';
    }
    
    
    instr->size = 4;  
    
    
    if (state->current_pass == 2) {
        for (int i = 0; i < instr->operand_count; i++) {
            if (instr->operands[i].type == OP_LABEL) {
                Label* label = find_label(state, instr->operands[i].value.label);
                if (!label) {
                    fprintf(stderr, "Error: Undefined label: %s\n", instr->operands[i].value.label);
                    return 1;
                }
                
                if (state->flags & XBIN_FLAG_RELOCATABLE) {
                    instr->needs_relocation = true;
                    if (label->binding == BINDING_EXTERNAL) {
                        instr->reloc_type = RELOC_SYMBOL;
                    } else {
                        instr->reloc_type = RELOC_ABSOLUTE;
                    }
                    instr->reloc_offset = 1;
                }
            }
        }
    }
    
    return 0;
}

static int encode_instructions(AssemblerState* state) {
    state->code_size = 0;
    
    for (int i = 0; i < state->instruction_count; i++) {
        if (encode_instruction(state, &state->instructions[i]) != 0) {
            return 1;
        }
    }
    
    return 0;
}

static int encode_instruction(AssemblerState* state, Instruction* instr) {
    uint8_t* code = &state->code[state->code_size];
    uint32_t initial_size = state->code_size;
    
    uint8_t opcode = 0x90;
    
    if (strcmp(instr->mnemonic, "mov") == 0) {
        opcode = 0xB8;
    } else if (strcmp(instr->mnemonic, "add") == 0) {
        opcode = 0x01;
    } else if (strcmp(instr->mnemonic, "sub") == 0) {
        opcode = 0x29;
    } else if (strcmp(instr->mnemonic, "push") == 0) {
        opcode = 0x50;
    } else if (strcmp(instr->mnemonic, "pop") == 0) {
        opcode = 0x58;
    } else if (strcmp(instr->mnemonic, "call") == 0) {
        opcode = 0xE8;
    } else if (strcmp(instr->mnemonic, "ret") == 0) {
        opcode = 0xC3;
    } else if (strcmp(instr->mnemonic, "jmp") == 0) {
        opcode = 0xEB;
    } else if (strcmp(instr->mnemonic, "nop") == 0) {
        opcode = 0x90;
    }
    
    code[0] = opcode;
    
    if (instr->operand_count > 0 && instr->operands[0].type == OP_REGISTER) {
        if (strcmp(instr->mnemonic, "push") == 0 || strcmp(instr->mnemonic, "pop") == 0) {
            code[0] += (instr->operands[0].value.reg & 0x07);
        }
    }
    
    if (instr->operand_count >= 2) {
        uint8_t modrm = 0xC0; 
        
        if (instr->operands[0].type == OP_REGISTER && instr->operands[1].type == OP_REGISTER) {
            modrm |= ((instr->operands[0].value.reg & 0x07) << 3);  
            modrm |= (instr->operands[1].value.reg & 0x07);  
            code[1] = modrm;
            instr->size = 2;
        } else if (instr->operands[0].type == OP_REGISTER && instr->operands[1].type == OP_IMMEDIATE) {
            modrm |= (instr->operands[0].value.reg & 0x07);
            code[1] = modrm;
            
            *(int32_t*)(&code[2]) = instr->operands[1].value.immediate;
            instr->size = 6;
        } else if (instr->operands[0].type == OP_REGISTER && instr->operands[1].type == OP_LABEL) {
            Label* label = find_label(state, instr->operands[1].value.label);
            if (!label) {
                fprintf(stderr, "Error: Undefined label: %s\n", instr->operands[1].value.label);
                return 1;
            }
            
            modrm |= ((instr->operands[0].value.reg & 0x07) << 3);
            modrm |= 5;
            code[1] = modrm;
            
            if (label->binding == BINDING_EXTERNAL) {
                
                for (int i = 0; i < state->symbol_count; i++) {
                    if (strcmp(state->string_table + state->symbols[i].name_offset, label->name) == 0) {
                        *(uint32_t*)(&code[2]) = i; 
                        break;
                    }
                }
            } else {
                *(uint32_t*)(&code[2]) = label->address;
            }
            
            if (instr->needs_relocation) {
                add_relocation(state, state->code_size + 2, instr->reloc_type);
            }
            
            instr->size = 6;  
        }
    } else if (instr->operand_count == 1) {
        if (instr->operands[0].type == OP_LABEL) {
            Label* label = find_label(state, instr->operands[0].value.label);
            if (!label) {
                fprintf(stderr, "Error: Undefined label: %s\n", instr->operands[0].value.label);
                return 1;
            }
            
            if (strcmp(instr->mnemonic, "call") == 0 || strcmp(instr->mnemonic, "jmp") == 0) {
                if (label->binding == BINDING_EXTERNAL) {
                    for (int i = 0; i < state->symbol_count; i++) {
                        if (strcmp(state->string_table + state->symbols[i].name_offset, label->name) == 0) {
                            *(uint32_t*)(&code[2]) = i; 
                            break;
                        }
                    }
                } else {
                    int32_t offset = label->address - (instr->address + instr->size);
                    *(int32_t*)(&code[1]) = offset;
                }
                
                if (instr->needs_relocation) {
                    add_relocation(state, state->code_size + 1, instr->reloc_type);
                }
                
                instr->size = 5;
            }
        }
    }
    
    state->code_size += instr->size;
    return 0;
}


static int add_relocation(AssemblerState* state, uint32_t offset, uint32_t type) {
    if (state->relocation_count >= MAX_RELOCS) {
        fprintf(stderr, "Error: Too many relocations\n");
        return 1;
    }
    
    XBINRelocation* reloc = &state->relocations[state->relocation_count++];
    reloc->offset = offset;
    reloc->type = type;
    
    return 0;
}


static int generate_symbols(AssemblerState* state) {
    if (!(state->flags & XBIN_FLAG_HAS_SYMBOLS)) {
        state->flags |= XBIN_FLAG_HAS_SYMBOLS;  
    }
    
    for (int i = 0; i < state->label_count; i++) {
        Label* label = &state->labels[i];
        if (add_symbol(state, label->name, label->address, label->symbol_type, label->binding) != 0) {
            return 1;
        }
    }
    
    return 0;
}


static int add_symbol(AssemblerState* state, const char* name, uint32_t value, uint8_t type, uint8_t binding) {
    if (state->symbol_count >= MAX_SYMBOLS) {
        fprintf(stderr, "Error: Too many symbols\n");
        return 1;
    }
    
    XBINSymbol* symbol = &state->symbols[state->symbol_count++];
    symbol->name_offset = add_to_string_table(state, name);
    symbol->value = value;
    symbol->type = type;
    symbol->binding = binding;
    
    return 0;
}


static uint32_t add_to_string_table(AssemblerState* state, const char* str) {
    uint32_t offset = state->string_table_size;
    size_t len = strlen(str) + 1;  
    
    if (offset + len > MAX_STRING_TABLE_SIZE) {
        fprintf(stderr, "Error: String table overflow\n");
        return 0;
    }
    
    memcpy(state->string_table + offset, str, len);
    state->string_table_size += len;
    
    return offset;
}


static int write_output(AssemblerState* state, const char* filename) {
    FILE* output_file = fopen(filename, "wb");
    if (!output_file) {
        fprintf(stderr, "Error: Cannot open output file '%s'\n", filename);
        return 1;
    }
    
    XBINHeader header;
    memset(&header, 0, sizeof(header));
    
    header.magic = XBIN_MAGIC;
    header.version = 1;
    header.code_size = state->code_size;
    header.data_size = state->data_size;
    header.bss_size = state->bss_size;
    header.stack_size = state->stack_size;
    header.reloc_count = state->relocation_count;
    header.flags = state->flags;
    
    
    if (state->flags & XBIN_FLAG_HAS_SYMBOLS) {
        header.string_table_size = state->string_table_size;
        header.symbol_count = state->symbol_count;
        header.string_table_offset = sizeof(XBINHeader) + 
                                   header.code_size + 
                                   header.data_size + 
                                   (header.reloc_count * sizeof(XBINRelocation));
    }
    
    
    if (state->entry_point) {
        Label* entry = find_label(state, state->entry_point);
        if (entry) {
            header.entry_point = entry->address;
        } else {
            fprintf(stderr, "Warning: Entry point label '%s' not found\n", state->entry_point);
            header.entry_point = 0;
        }
    } else {
        
        header.entry_point = 0;
    }
    
    
    fwrite(&header, sizeof(header), 1, output_file);
    
    
    fwrite(state->code, 1, state->code_size, output_file);
    
    
    fwrite(state->data, 1, state->data_size, output_file);
    
    
    fwrite(state->relocations, sizeof(XBINRelocation), state->relocation_count, output_file);
    
    
    if (state->flags & XBIN_FLAG_HAS_SYMBOLS) {
        fwrite(state->string_table, 1, state->string_table_size, output_file);
        fwrite(state->symbols, sizeof(XBINSymbol), state->symbol_count, output_file);
    }
    
    fclose(output_file);
    return 0;
}
