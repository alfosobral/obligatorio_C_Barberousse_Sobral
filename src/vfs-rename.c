#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vfs.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <imagen> <origen> <destino>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *image_path = argv[1];
    const char *oldname = argv[2];
    const char *newname = argv[3];

    // Verificar que el archivo origen exista
    int inode_num_src = dir_lookup(image_path, oldname);
    if (inode_num_src < 0) {
        fprintf(stderr, "Error: '%s' no existe\n", oldname);
        return EXIT_FAILURE;
    }

    // Si destino existe, eliminarlo primero para sobrescribir
    int inode_num_dst = dir_lookup(image_path, newname);
    if (inode_num_dst >= 0) {
        // Truncar datos y liberar inodo
        struct inode in_dst;
        if (read_inode(image_path, inode_num_dst, &in_dst) == 0) {
            if (inode_trunc_data(image_path, &in_dst) < 0) {
                fprintf(stderr, "Error: no se pudo truncar '%s'\n", newname);
                return EXIT_FAILURE;
            }
            if (free_inode(image_path, inode_num_dst) < 0) {
                fprintf(stderr, "Error: no se pudo liberar inodo de '%s'\n", newname);
                return EXIT_FAILURE;
            }
        }
        // Eliminar entrada de directorio
        remove_dir_entry(image_path, newname);
    }

    // Validar nuevo nombre
    if (!name_is_valid(newname)) {
        fprintf(stderr, "Error: nombre inválido '%s'\n", newname);
        return EXIT_FAILURE;
    }

    // Agregar entrada con nuevo nombre apuntando al mismo inodo fuente
    if (add_dir_entry(image_path, newname, inode_num_src) < 0) {
        fprintf(stderr, "Error: no se pudo crear la nueva entrada '%s'\n", newname);
        return EXIT_FAILURE;
    }

    // Eliminar entrada antigua
    if (remove_dir_entry(image_path, oldname) < 0) {
        fprintf(stderr, "Error: no se pudo eliminar la entrada antigua '%s'\n", oldname);
        // Intentar revertir: quitar la nueva entrada
        remove_dir_entry(image_path, newname);
        return EXIT_FAILURE;
    }

    printf("Renombrado '%s' a '%s' exitosamente.\n", oldname, newname);
    return EXIT_SUCCESS;
}
