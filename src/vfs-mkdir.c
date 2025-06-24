#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vfs.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <imagen> <nombre_directorio>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *image_path = argv[1];
    const char *dirname = argv[2];

    // Validar nombre de directorio
    if (!name_is_valid(dirname)) {
        fprintf(stderr, "Error: nombre inválido '%s'\n", dirname);
        return EXIT_FAILURE;
    }

    // Verificar que no exista ya
    if (dir_lookup(image_path, dirname) > 0) {
        fprintf(stderr, "Error: '%s' ya existe\n", dirname);
        return EXIT_FAILURE;
    }

    // Crear inodo vacío con permisos de directorio
    int inode_num = create_empty_file_in_free_inode(image_path, DEFAULT_PERM);
    if (inode_num < 0) {
        fprintf(stderr, "Error: no se pudo reservar inodo para '%s'\n", dirname);
        return EXIT_FAILURE;
    }

    // Leer y modificar inodo para directorio
    struct inode in;
    if (read_inode(image_path, inode_num, &in) < 0) {
        fprintf(stderr, "Error: no se pudo leer inodo de '%s'\n", dirname);
        free_inode(image_path, inode_num);
        return EXIT_FAILURE;
    }
    // Ajustar modo a directorio
    in.mode = (in.mode & 0x0FFF) | INODE_MODE_DIR;

    // Asignar un bloque de datos para el directorio
    int block_num = bitmap_set_first_free(image_path);
    if (block_num < 0) {
        fprintf(stderr, "Error: no se pudo asignar bloque para '%s'\n", dirname);
        free_inode(image_path, inode_num);
        return EXIT_FAILURE;
    }
    in.direct[0] = block_num;
    in.blocks = 1;

    // Escribir el inodo actualizado
    if (write_inode(image_path, inode_num, &in) < 0) {
        fprintf(stderr, "Error: no se pudo escribir inodo de '%s'\n", dirname);
        free_inode(image_path, inode_num);
        bitmap_free_block(image_path, block_num);
        return EXIT_FAILURE;
    }

    // Inicializar bloque de directorio con entradas . y ..
    struct dir_entry entries[DIR_ENTRIES_PER_BLOCK];
    memset(entries, 0, sizeof(entries));
    // Entrada "."
    entries[0].inode = inode_num;
    strncpy(entries[0].name, ".", FILENAME_MAX_LEN);
    // Entrada ".." (apunta a directorio raíz)
    entries[1].inode = ROOTDIR_INODE;
    strncpy(entries[1].name, "..", FILENAME_MAX_LEN);
    
    if (write_block(image_path, block_num, entries) < 0) {
        fprintf(stderr, "Error: no se pudo inicializar bloque de directorio para '%s'\n", dirname);
        // revertir
        free_inode(image_path, inode_num);
        bitmap_free_block(image_path, block_num);
        return EXIT_FAILURE;
    }

    // Agregar la entrada al directorio raíz (inode 1)
    if (add_dir_entry(image_path, dirname, inode_num) < 0) {
        fprintf(stderr, "Error: no se pudo agregar '%s' al directorio raíz\n", dirname);
        // revertir
        free_inode(image_path, inode_num);
        bitmap_free_block(image_path, block_num);
        return EXIT_FAILURE;
    }

    printf("Directorio '%s' creado exitosamente.\n", dirname);
    return EXIT_SUCCESS;
}
