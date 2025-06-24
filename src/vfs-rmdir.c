#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vfs.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <imagen> <directorio>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *image_path = argv[1];
    const char *dirname = argv[2];

    // 1) Verificar existencia
    int inode_num = dir_lookup(image_path, dirname);
    if (inode_num < 0) {
        fprintf(stderr, "Error: '%s' no existe\n", dirname);
        return EXIT_FAILURE;
    }

    // 2) Leer inodo
    struct inode in;
    if (read_inode(image_path, inode_num, &in) < 0) {
        fprintf(stderr, "Error: no se pudo leer inodo de '%s'\n", dirname);
        return EXIT_FAILURE;
    }

    // 3) Comprobar que es directorio
    if ((in.mode & INODE_MODE_DIR) != INODE_MODE_DIR) {
        fprintf(stderr, "Error: '%s' no es un directorio\n", dirname);
        return EXIT_FAILURE;
    }

    // 4) Leer bloque de directorio y verificar vacío (solo . y ..)
    uint8_t buffer[BLOCK_SIZE];
    if (read_block(image_path, in.direct[0], buffer) < 0) {
        fprintf(stderr, "Error: no se pudo leer bloque de '%s'\n", dirname);
        return EXIT_FAILURE;
    }
    struct dir_entry *entries = (struct dir_entry *)buffer;
    for (size_t i = 2; i < DIR_ENTRIES_PER_BLOCK; i++) {
        if (entries[i].inode != 0) {
            fprintf(stderr, "Error: directorio '%s' no está vacío\n", dirname);
            return EXIT_FAILURE;
        }
    }

    // 5) Liberar bloque de datos
    if (bitmap_free_block(image_path, in.direct[0]) < 0) {
        fprintf(stderr, "Error: no se pudo liberar bloque de '%s'\n", dirname);
        return EXIT_FAILURE;
    }

    // 6) Liberar inodo
    if (free_inode(image_path, inode_num) < 0) {
        fprintf(stderr, "Error: no se pudo liberar inodo de '%s'\n", dirname);
        return EXIT_FAILURE;
    }

    // 7) Eliminar de directorio raíz
    if (remove_dir_entry(image_path, dirname) < 0) {
        fprintf(stderr, "Error: no se pudo eliminar '%s' del directorio raíz\n", dirname);
        return EXIT_FAILURE;
    }

    printf("Directorio '%s' eliminado con éxito.\n", dirname);
    return EXIT_SUCCESS;
}
