#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vfs.h"

// Función recursiva para imprimir árbol de un directorio
void print_tree(const char *image_path, uint32_t inode_num, const char *prefix) {
    struct inode in;
    if (read_inode(image_path, inode_num, &in) < 0) return;

    // Leer bloque de directorio
    uint8_t buffer[BLOCK_SIZE];
    if (read_block(image_path, in.direct[0], buffer) < 0) return;
    struct dir_entry *entries = (struct dir_entry *)buffer;

    // Contar entradas válidas
    int total = 0;
    for (int i = 0; i < DIR_ENTRIES_PER_BLOCK; i++) {
        if (entries[i].inode != 0 &&
            strcmp(entries[i].name, ".")  != 0 &&
            strcmp(entries[i].name, "..") != 0) {
            total++;
        }
    }

    // Recorrer e imprimir con prefijos
    int count = 0;
    for (int i = 0; i < DIR_ENTRIES_PER_BLOCK; i++) {
        if (entries[i].inode == 0) continue;
        if (strcmp(entries[i].name, ".")  == 0) continue;
        if (strcmp(entries[i].name, "..") == 0) continue;

        count++;
        int last = (count == total);
        printf("%s%s %s\n",
               prefix,
               last ? "└──" : "├──",
               entries[i].name);

        // Si es directorio, recursar
        struct inode child;
        if (read_inode(image_path, entries[i].inode, &child) == 0 &&
            (child.mode & INODE_MODE_DIR) == INODE_MODE_DIR) {
            char new_prefix[256];
            snprintf(new_prefix, sizeof(new_prefix),
                     "%s%s   ",
                     prefix,
                     last ? "    " : "│   ");
            print_tree(image_path, entries[i].inode, new_prefix);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <imagen>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *image_path = argv[1];

    printf(".\n");
    // Empezar desde el directorio raíz (inode 1)
    print_tree(image_path, ROOTDIR_INODE, "");

    return EXIT_SUCCESS;
}
