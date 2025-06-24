#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vfs.h"

#define BUF_SIZE 512  // Ajustable: bytes a leer por iteración

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <imagen> <archivo1> [archivo2...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *image_path = argv[1];
    struct superblock sb;
    if (read_superblock(image_path, &sb) < 0) {
        fprintf(stderr, "No se pudo leer el superbloque\n");
        return EXIT_FAILURE;
    }

    // Buffer de lectura
    char buffer[BUF_SIZE];

    // Para cada archivo pasado por parámetro
    for (int f = 2; f < argc; f++) {
        const char *filename = argv[f];

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

        // 3) Leer contenido por bloques (o offsets)
        uint32_t remaining = in.size;
        size_t offset = 0;

        while (remaining > 0) {
            size_t to_read = (remaining < BUF_SIZE) ? remaining : BUF_SIZE;
            int bytes = inode_read_data(image_path, inode_num, buffer, to_read, offset);
            if (bytes < 0) {
                fprintf(stderr, "Error leyendo datos de '%s'\n", filename);
                break;
            }
            // 4) Escribir al stdout
            fwrite(buffer, 1, bytes, stdout);

            remaining -= bytes;
            offset    += bytes;
        }
    }

    return EXIT_SUCCESS;
}
