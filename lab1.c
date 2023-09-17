#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SYMBOL_LEN 16
#define MAX_DEF_USE_COUNT 16
#define MAX_INSTRUCTION_COUNT 512
#define MAX_SYMBOL_TABLE_SIZE 256
#define MAX_MODULE_TABLE_SIZE 128

// Data structures to store symbol and module information
typedef struct
{
    char name[MAX_SYMBOL_LEN + 1];
    int absolute_address;
    int used;
    int redefined;
} Symbol;

typedef struct
{
    char name[MAX_SYMBOL_LEN + 1];
    int relative_address;
    int module_num;
} Definition;

typedef struct
{
    char name[MAX_SYMBOL_LEN + 1];
    int used;
} Use;

typedef struct
{
    char addr_mode;
    int instr;
} Instruction;

// Global data structures
Symbol symbol_table[MAX_SYMBOL_TABLE_SIZE];
Definition definition_list[MAX_DEF_USE_COUNT];
Use use_list[MAX_DEF_USE_COUNT];
Instruction program_text[MAX_INSTRUCTION_COUNT];
int module_base[MAX_MODULE_TABLE_SIZE];

// Error message catalog
const char *error_messages[] = {
    "Syntax error at line %d, offset %d\n",
    "Error: Symbol %s is defined multiple times. Using the value from the first definition.\n",
    "Warning: Symbol %s is defined but not used.\n",
    "Warning: Symbol %s is used in an E-instruction but not defined anywhere. Using absolute zero.\n",
    "Warning: Symbol %s appears in a use list but is not used in the module.\n",
    "Warning: Address in first definition of symbol %s exceeds module size. Using relative address 0.\n",
    "Error: External operand is too large to reference an entry in the use list. Using immediate.\n",
    "Error: Absolute address exceeds machine size. Using absolute zero.\n",
    "Error: Relative address exceeds module size. Using relative address 0.\n",
    "Error: Illegal immediate operand encountered (>= 900). Using operand value 999.\n",
    "Error: Illegal opcode encountered (>= 10). Using <opcode, operand> value 9999.\n",
    "Error: Module operand is invalid. Using module 0.\n"};

int symbol_count = 0;
int definition_count = 0;
int use_count = 0;
int instruction_count = 0;

// Function to print errors and warnings
void print_error(int error_code, int line_num, int offset, const char *symbol_name)
{
    fprintf(stderr, error_messages[error_code], line_num, offset, symbol_name);
}

// Function to add a symbol to the symbol table
void add_symbol(const char *name, int value, int used)
{
    if (symbol_count >= MAX_SYMBOL_TABLE_SIZE)
    {
        fprintf(stderr, "Error: Symbol table size exceeded.\n");
        exit(1);
    }

    strcpy(symbol_table[symbol_count].name, name);
    symbol_table[symbol_count].absolute_address = value;
    symbol_table[symbol_count].used = used;
    symbol_table[symbol_count].redefined = 0;
    symbol_count++;
}

// Function to find a symbol in the symbol table by name
int find_symbol(const char *name)
{
    for (int i = 0; i < symbol_count; i++)
    {
        if (strcmp(symbol_table[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

// Function to add a definition to the definition list
void add_definition(const char *name, int value, int module_num)
{
    if (definition_count >= MAX_DEF_USE_COUNT)
    {
        fprintf(stderr, "Error: Definition list size exceeded.\n");
        exit(1);
    }

    strcpy(definition_list[definition_count].name, name);
    definition_list[definition_count].relative_address = value;
    definition_list[definition_count].module_num = module_num;
    definition_count++;
}

// Function to find a definition in the definition list by name
int find_definition(const char *name)
{
    for (int i = 0; i < definition_count; i++)
    {
        if (strcmp(definition_list[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

// Function to add a use to the use list
void add_use(const char *name, int used)
{
    if (use_count >= MAX_DEF_USE_COUNT)
    {
        fprintf(stderr, "Error: Use list size exceeded.\n");
        exit(1);
    }

    strcpy(use_list[use_count].name, name);
    use_list[use_count].used = used;
    use_count++;
}

// Function to find a use in the use list by name
int find_use(const char *name)
{
    for (int i = 0; i < use_count; i++)
    {
        if (strcmp(use_list[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

// Function to process pass one and build the symbol and module tables
void pass_one(FILE *input)
{
    int line_num = 1;
    int offset = 1;
    int module_num = 0;
    int relative_address = 0;

    while (1)
    {
        char token[256];
        int token_len = 0;
        char c = fgetc(input);

        // Skip leading whitespace and newlines
        while (c == ' ' || c == '\t' || c == '\n')
        {
            if (c == '\n')
            {
                line_num++;
                offset = 1;
            }
            else
            {
                offset++;
            }
            c = fgetc(input);
        }

        // Check for end of file
        if (c == EOF)
        {
            break;
        }

        // Read a token
        while (c != ' ' && c != '\t' && c != '\n' && c != EOF)
        {
            if (token_len < sizeof(token) - 1)
            {
                token[token_len++] = c;
            }
            offset++;
            c = fgetc(input);
        }
        token[token_len] = '\0';

        // Process the token based on its type
        if (isalpha(token[0]))
        {
            // Token is a symbol
            if (token_len > MAX_SYMBOL_LEN)
            {
                print_error(0, line_num, offset - token_len, "");
                continue;
            }

            // Check for symbol redefinition
            int symbol_index = find_symbol(token);
            if (symbol_index != -1)
            {
                if (!symbol_table[symbol_index].redefined)
                {
                    print_error(1, line_num, offset - token_len, token);
                }
                symbol_table[symbol_index].redefined = 1;
            }

            add_symbol(token, relative_address + module_base[module_num], 0);
        }
        else if (isdigit(token[0]))
        {
            // Token is an instruction
            int instruction = atoi(token);
            int opcode = instruction / 1000;
            int operand = instruction % 1000;

            if (opcode >= 10)
            {
                print_error(9, line_num, offset - token_len, "");
                opcode = 9;
            }

            if (operand >= 10000)
            {
                print_error(8, line_num, offset - token_len, "");
                operand = 0;
            }

            relative_address++;

            // Store the instruction in the program text
            if (instruction_count >= MAX_INSTRUCTION_COUNT)
            {
                fprintf(stderr, "Error: Instruction count exceeded.\n");
                exit(1);
            }

            program_text[instruction_count].addr_mode = 'A'; // Absolute
            program_text[instruction_count].instr = instruction;
            instruction_count++;
        }
        else if (token[0] == 'E')
        {
            // Token is an external reference
            int operand = atoi(token + 1);

            if (operand >= use_count)
            {
                print_error(6, line_num, offset - token_len, "");
                operand = 0;
            }

            // Store the instruction in the program text
            if (instruction_count >= MAX_INSTRUCTION_COUNT)
            {
                fprintf(stderr, "Error: Instruction count exceeded.\n");
                exit(1);
            }

            program_text[instruction_count].addr_mode = 'E'; // External
            program_text[instruction_count].instr = operand;
            instruction_count++;
        }
        else if (token[0] == 'R')
        {
            // Token is a relative address
            int operand = atoi(token + 1);

            if (operand >= MAX_INSTRUCTION_COUNT)
            {
                print_error(7, line_num, offset - token_len, "");
                operand = 0;
            }

            // Store the instruction in the program text
            if (instruction_count >= MAX_INSTRUCTION_COUNT)
            {
                fprintf(stderr, "Error: Instruction count exceeded.\n");
                exit(1);
            }

            program_text[instruction_count].addr_mode = 'R'; // Relative
            program_text[instruction_count].instr = operand;
            instruction_count++;
        }
        else if (token[0] == 'I')
        {
            // Token is an immediate operand
            int operand = atoi(token + 1);

            if (operand >= 900)
            {
                print_error(8, line_num, offset - token_len, "");
                operand = 999;
            }

            // Store the instruction in the program text
            if (instruction_count >= MAX_INSTRUCTION_COUNT)
            {
                fprintf(stderr, "Error: Instruction count exceeded.\n");
                exit(1);
            }

            program_text[instruction_count].addr_mode = 'I'; // Immediate
            program_text[instruction_count].instr = operand;
            instruction_count++;
        }
        else if (token[0] == 'M')
        {
            // Token is a module size indicator
            int size = atoi(token + 1);

            if (size <= 0 || size >= MAX_INSTRUCTION_COUNT)
            {
                print_error(10, line_num, offset - token_len, "");
                size = 0;
            }

            // Set the base address for the current module
            module_base[module_num + 1] = module_base[module_num] + size;
            module_num++;
            relative_address = 0;
        }
        else if (token[0] == 'A')
        {
            // Token is an absolute address
            int operand = atoi(token + 1);

            if (operand >= 512)
            {
                print_error(7, line_num, offset - token_len, "");
                operand = 0;
            }

            // Store the instruction in the program text
            if (instruction_count >= MAX_INSTRUCTION_COUNT)
            {
                fprintf(stderr, "Error: Instruction count exceeded.\n");
                exit(1);
            }

            program_text[instruction_count].addr_mode = 'A'; // Absolute
            program_text[instruction_count].instr = operand;
            instruction_count++;
        }
        else
        {
            // Token is invalid
            print_error(0, line_num, offset - token_len, "");
        }
    }
}
// Function to process pass two and generate the memory map
void pass_two()
{
    int line_num = 1;
    int offset = 1;
    int module_num = 0;
    int relative_address = 0;

    printf("Symbol Table\n");
    for (int i = 0; i < symbol_count; i++)
    {
        if (!symbol_table[i].used)
        {
            printf("%s=%d Warning: This symbol is defined but not used.\n", symbol_table[i].name, symbol_table[i].absolute_address);
        }
        else
        {
            printf("%s=%d\n", symbol_table[i].name, symbol_table[i].absolute_address);
        }
    }

    printf("Memory Map\n");

    for (int i = 0; i < instruction_count; i++)
    {
        char addr_mode = program_text[i].addr_mode;
        int operand = program_text[i].instr;

        if (addr_mode == 'A')
        {
            if (operand >= 512)
            {
                operand = 0;
                print_error(7, line_num, offset, "");
            }
            printf("%03d: %04d\n", i, operand);
        }
        else if (addr_mode == 'R')
        {
            int absolute_addr = module_base[module_num] + operand;
            if (operand >= MAX_INSTRUCTION_COUNT || absolute_addr >= module_base[module_num + 1])
            {
                absolute_addr = module_base[module_num];
                print_error(8, line_num, offset, "");
            }
            printf("%03d: %04d\n", i, absolute_addr);
        }
        else if (addr_mode == 'I')
        {
            if (operand >= 900)
            {
                operand = 999;
                print_error(9, line_num, offset, "");
            }
            printf("%03d: %04d\n", i, operand);
        }
        else if (addr_mode == 'E')
        {
            if (operand >= use_count)
            {
                operand = 0;
                print_error(6, line_num, offset, "");
            }
            int symbol_index = find_symbol(use_list[operand].name);
            if (symbol_index == -1)
            {
                printf("%03d: %04d Warning: Symbol %s is used but not defined.\n", i, 0, use_list[operand].name);
            }
            else
            {
                int absolute_addr = symbol_table[symbol_index].absolute_address;
                printf("%03d: %04d\n", i, absolute_addr);
            }
        }

        offset++;
        if (program_text[i].addr_mode == 'M')
        {
            module_num++;
            relative_address = 0;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s input_file\n", argv[0]);
        return 1;
    }

    FILE *input = fopen(argv[1], "r");
    if (input == NULL)
    {
        perror("Error opening input file");
        return 1;
    }

    // Initialize module base for the first module
    module_base[0] = 0;

    // Pass one: Build symbol table and module base
    pass_one(input);

    // Rewind the input file for pass two
    rewind(input);

    // Pass two: Generate memory map
    pass_two();

    fclose(input);

    return 0;
}
