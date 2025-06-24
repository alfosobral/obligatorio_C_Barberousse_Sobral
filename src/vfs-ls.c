#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vfs.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <imagen>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *image_path = argv[1];
    struct superblock sb;

    if (read_superblock(image_path, &sb) < 0) {
        fprintf(stderr, "No se pudo leer el superbloque\n");
        return EXIT_FAILURE;
    }

    struct inode root_inode;
    if (read_inode(image_path, ROOTDIR_INODE, &root_inode) < 0) {
        fprintf(stderr, "No se pudo leer el inodo del directorio raíz\n");
        return EXIT_FAILURE;
    }

    uint8_t buffer[BLOCK_SIZE];
    if (read_block(image_path, root_inode.direct[0], buffer) < 0) {
        fprintf(stderr, "No se pudo leer el bloque de directorio\n");
        return EXIT_FAILURE;
    }

    struct dir_entry *entry = (struct dir_entry *)buffer;
    int max_entries = DIR_ENTRIES_PER_BLOCK;

    for (int i = 0; i < max_entries; i++) {
        if (entry[i].inode == 0) continue;

        if (strcmp(entry[i].name, ".") == 0 || strcmp(entry[i].name, "..") == 0)
            continue;

        struct inode file_inode;
        if (read_inode(image_path, entry[i].inode, &file_inode) < 0) {
            fprintf(stderr, "No se pudo leer el inodo del archivo '%s'\n", entry[i].name);
            continue;
        }

        print_inode(&file_inode, entry[i].inode, entry[i].name);
    }

    return EXIT_SUCCESS;
}
