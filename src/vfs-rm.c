#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "vfs.h"

int main(int argc, char *argv[]) {
    // Verificar parámetros mínimos
    if (argc < 2) {
        fprintf(stderr, "Error: falta el parámetro de la imagen\n");
        fprintf(stderr, "Uso: %s <imagen> <archivo1> [archivo2...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *image_path = argv[1];

    // Verificar que la imagen exista
    if (access(image_path, F_OK) != 0) {
        fprintf(stderr, "Error: la imagen '%s' no existe\n", image_path);
        return EXIT_FAILURE;
    }

    // Verificar que sea una imagen VFS válida
    struct superblock sb;
    if (read_superblock(image_path, &sb) < 0) {
        fprintf(stderr, "Error: '%s' no es una imagen VFS válida\n", image_path);
        return EXIT_FAILURE;
    }

    // Verificar al menos un archivo a eliminar
    if (argc < 3) {
        fprintf(stderr, "Error: falta al menos un nombre de archivo\n");
        return EXIT_FAILURE;
    }

    for (int i = 2; i < argc; i++) {
        const char *filename = argv[i];

        // Verificar existencia en directorio raíz
        int inode_num = dir_lookup(image_path, filename);
        if (inode_num <= 0) {
            fprintf(stderr, "No se ha encontrado el archivo '%s' en el directorio\n", filename);
            continue;
        }

        // Leer inodo
        struct inode in;
        if (read_inode(image_path, inode_num, &in) < 0) {
            fprintf(stderr, "Error: no se pudo leer inodo de '%s'\n", filename);
            continue;
        }

        // Verificar archivo regular
        if ((in.mode & INODE_MODE_FILE) != INODE_MODE_FILE) {
            fprintf(stderr, "Error: '%s' no es un archivo regular\n", filename);
            continue;
        }

        // Truncar datos (liberar bloques)
        if (inode_trunc_data(image_path, &in) < 0) {
            fprintf(stderr, "Error: no se pudo truncar '%s'\n", filename);
            continue;
        }

        // Liberar inodo
        if (free_inode(image_path, inode_num) < 0) {
            fprintf(stderr, "Error: no se pudo liberar inodo de '%s'\n", filename);
            continue;
        }

        // Leer bloque de directorio raíz
        struct inode root_in;
        if (read_inode(image_path, ROOTDIR_INODE, &root_in) < 0) {
            fprintf(stderr, "Error: no se pudo leer inodo raíz\n");
            continue;
        }
        uint32_t block_num = root_in.direct[0];

        uint8_t buffer[BLOCK_SIZE];
        if (read_block(image_path, block_num, buffer) < 0) {
            fprintf(stderr, "Error: no se pudo leer bloque de directorio\n");
            continue;
        }
        struct dir_entry *entries = (struct dir_entry *)buffer;

        // Eliminar entrada: inode y nombre
        int found = 0;
        for (size_t j = 0; j < DIR_ENTRIES_PER_BLOCK; j++) {
            if (entries[j].inode == (uint32_t)inode_num && strcmp(entries[j].name, filename) == 0) {
                entries[j].inode = 0;  // limpiar inodo
                memset(entries[j].name, 0, FILENAME_MAX_LEN); // limpiar nombre
                found = 1;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "No se ha encontrado el archivo '%s' en el directorio\n", filename);
            continue;
        }

        // Escribir bloque actualizado
        if (write_block(image_path, block_num, buffer) < 0) {
            fprintf(stderr, "Error: no se pudo actualizar bloque de directorio\n");
            continue;
        }

        printf("Archivo '%s' eliminado del VFS.\n", filename);
    }

    return EXIT_SUCCESS;
}
