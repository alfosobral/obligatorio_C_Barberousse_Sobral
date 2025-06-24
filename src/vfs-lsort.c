#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vfs.h"

struct dir_entry_with_inode {
    struct dir_entry entry;
    struct inode inode;
};

int cmp_by_name(const void *a, const void *b) {
    const struct dir_entry_with_inode *e1 = a;
    const struct dir_entry_with_inode *e2 = b;
    return strcmp(e1->entry.name, e2->entry.name);
}

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
    struct dir_entry_with_inode files[DIR_ENTRIES_PER_BLOCK];
    int file_count = 0;

    for (size_t i = 0; i < DIR_ENTRIES_PER_BLOCK; i++) {
        if (entry[i].inode == 0) continue;
        if (strcmp(entry[i].name, ".") == 0 || strcmp(entry[i].name, "..") == 0) continue;

        struct inode in;
        if (read_inode(image_path, entry[i].inode, &in) < 0) {
            fprintf(stderr, "No se pudo leer el inodo del archivo '%s'\n", entry[i].name);
            continue;
        }

        files[file_count].entry = entry[i];
        files[file_count].inode = in;
        file_count++;
    }

    qsort(files, file_count, sizeof(struct dir_entry_with_inode), cmp_by_name);

    for (int i = 0; i < file_count; i++) {
        print_inode(&files[i].inode, files[i].entry.inode, files[i].entry.name);
    }

    return EXIT_SUCCESS;
}
