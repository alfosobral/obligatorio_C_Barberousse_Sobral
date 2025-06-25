    #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
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
    // Chequear parámetros mínimos
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <imagen> [permisos_octal] <archivo1> [archivo2...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *image_path = argv[1];

    // Verificar que el archivo de imagen exista
    if (access(image_path, F_OK) != 0) {
        fprintf(stderr, "Error: imagen '%s' no encontrada\n", image_path);
        return EXIT_FAILURE;
    }

    // Verificar que sea una imagen VFS válida
    struct superblock sb;
    if (read_superblock(image_path, &sb) < 0) {
        fprintf(stderr, "Error: '%s' no es una imagen VFS válida\n", image_path);
        return EXIT_FAILURE;
    }

    // Verificar que haya al menos un archivo a crear
    if (argc < 3) {
        fprintf(stderr, "Error: falta al menos un nombre de archivo\n");
        return EXIT_FAILURE;
    }

    uint16_t perms = DEFAULT_PERM;
    int file_start = 2;

    // Interpretar permisos octales si se proporcionan
    if (argc >= 4 && is_octal(argv[2])) {
        perms = (uint16_t)strtol(argv[2], NULL, 8);
        file_start = 3;
    }

    // Procesar cada archivo
    for (int i = file_start; i < argc; i++) {
        const char *filename = argv[i];

        // Chequear existencia previa
        int existing = dir_lookup(image_path, filename);
        if (existing > 0) {
            fprintf(stderr, "Error: el archivo '%s' ya existe\n", filename);
            continue;
        } else if (existing < 0) {
            fprintf(stderr, "Error: no se pudo acceder al VFS para buscar '%s'\n", filename);
            continue;
        }

        // Validar nombre
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

        // Agregar al directorio
        if (add_dir_entry(image_path, filename, inode_nbr) < 0) {
            fprintf(stderr, "Error: no se pudo agregar '%s' al directorio\n", filename);
            free_inode(image_path, inode_nbr);
            continue;
        }

        printf("Archivo '%s' creado exitosamente con permisos %o\n", filename, perms);
    }

    return EXIT_SUCCESS;
}
