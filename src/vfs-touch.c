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

        if (dir_lookup(image_path, filename) > 0) {
            fprintf(stderr, "Error: el archivo '%s' ya existe\n", filename);
            continue;
        }

        if (!name_is_valid(filename)) {
            fprintf(stderr, "Error: nombre inválido '%s'\n", filename);
            continue;
        }

        int inodo_nro = create_empty_file_in_free_inode(image_path, 0644);
        if (inodo_nro < 0) {
            fprintf(stderr, "Error: no se pudo crear inodo para '%s'\n", filename);
            continue;
        }

        if (add_dir_entry(image_path, filename, inodo_nro) < 0) {
            fprintf(stderr, "Error: no se pudo agregar entrada de directorio '%s'\n", filename);
            free_inode(image_path, inodo_nro);
            continue;
        }

        printf("Archivo '%s' creado exitosamente.\n", filename);
    }

    return EXIT_SUCCESS;
}
