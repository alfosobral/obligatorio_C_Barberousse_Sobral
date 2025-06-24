#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vfs.h"

// Función auxiliar para verificar si una cadena es un número octal válido
static int is_octal(const char *s) {
    if (*s == '0' && *(s + 1) != '\0') s++; // permitir prefijo 0
    while (*s) {
        if (*s < '0' || *s > '7') return 0;
        s++;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <imagen> [permisos_octal] <archivo1> [archivo2...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *image_path = argv[1];
    uint16_t perms = DEFAULT_PERM;
    int file_start = 2;

    // Si el segundo argumento es un octal, lo interpretamos como permisos
    if (argc >= 4 && is_octal(argv[2])) {
        perms = (uint16_t) strtol(argv[2], NULL, 8);
        file_start = 3;
    }

    for (int i = file_start; i < argc; i++) {
        const char *filename = argv[i];

        // Verificar si el nombre ya existe
        if (dir_lookup(image_path, filename) > 0) {
            fprintf(stderr, "Error: el archivo '%s' ya existe\n", filename);
            continue;
        }

        // Validar el nombre
        if (!name_is_valid(filename)) {
            fprintf(stderr, "Error: nombre inválido '%s'\n", filename);
            continue;
        }

        // Crear inodo vacío con permisos
        int inode_nbr = create_empty_file_in_free_inode(image_path, perms);
        if (inode_nbr < 0) {
            fprintf(stderr, "Error: no se pudo crear inodo para '%s'\n", filename);
            continue;
        }

        // Agregar entrada al directorio
        if (add_dir_entry(image_path, filename, inode_nbr) < 0) {
            fprintf(stderr, "Error: no se pudo agregar entrada de directorio '%s'\n", filename);
            // Revertir creación del inodo
            free_inode(image_path, inode_nbr);
            continue;
        }

        printf("Archivo '%s' creado exitosamente con permisos %o.\n", filename, perms);
    }

    return EXIT_SUCCESS;
}
