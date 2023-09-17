#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct
{
    char name[10 + 1];
    int absolute_address;
    int used;
    int redefined;
} Symbol;

int main(int argc, char *argv[])
{
    Symbol symbol_table[10];

    return 0;
}
