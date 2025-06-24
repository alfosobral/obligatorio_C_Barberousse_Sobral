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

        // 2) Leer el nodo-i
        struct inode in;
        if (read_inode(image_path, inode_num, &in) < 0) {
            fprintf(stderr, "Error: no se pudo leer inodo de '%s'\n", filename);
            continue;
        }

        // 3) Comprobar que sea archivo regular
        if ((in.mode & INODE_MODE_FILE) != INODE_MODE_FILE) {
            fprintf(stderr, "Error: '%s' no es un archivo regular\n", filename);
            continue;
        }

        // 4) Truncar datos (libera bloques)
        if (inode_trunc_data(image_path, &in) < 0) {
            fprintf(stderr, "Error: no se pudo truncar '%s'\n", filename);
            continue;
        }

        // 5) Guardar el inode actualizado
        if (write_inode(image_path, inode_num, &in) < 0) {
            fprintf(stderr, "Error: no se pudo actualizar inodo de '%s'\n", filename);
            continue;
        }

        printf("Archivo '%s' truncado (ahora tamaño 0).\n", filename);
    }

    return EXIT_SUCCESS;
}
