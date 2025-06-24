#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vfs.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <imagen> <archivo1> [archivo2...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *image_path = argv[1];

    for (int i = 2; i < argc; i++) {
        const char *filename = argv[i];

        // 1) Verificar existencia
        int inode_num = dir_lookup(image_path, filename);
        if (inode_num < 0) {
            fprintf(stderr, "Error: '%s' no existe\n", filename);
            continue;
        }

        // 2) Intentar leer el inodo
        struct inode in;
        int has_inode = 1;
        if (read_inode(image_path, inode_num, &in) < 0) {
            fprintf(stderr, "Advertencia: no se pudo leer inodo de '%s', eliminando entrada de todos modos\n", filename);
            has_inode = 0;
        }

        if (has_inode) {
            // 3) Comprobar que es archivo regular
            if ((in.mode & INODE_MODE_FILE) != INODE_MODE_FILE) {
                fprintf(stderr, "Error: '%s' no es un archivo regular\n", filename);
                continue;
            }

            // 4) Truncar datos (libera bloques)
            if (inode_trunc_data(image_path, &in) < 0) {
                fprintf(stderr, "Error: no se pudo truncar '%s'\n", filename);
                continue;
            }

            // 5) Liberar inodo
            if (free_inode(image_path, inode_num) < 0) {
                fprintf(stderr, "Error: no se pudo liberar inodo de '%s'\n", filename);
                continue;
            }
        }

        // 6) Eliminar de directorio
        if (remove_dir_entry(image_path, filename) < 0) {
            fprintf(stderr, "Error: no se pudo eliminar '%s' del directorio\n", filename);
            continue;
        }

        printf("Archivo '%s' eliminado del VFS.\n", filename);
    }

    return EXIT_SUCCESS;
}
