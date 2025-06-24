#include <stdio.h>
#include <stdlib.h>
#include "vfs.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <imagen> <permisos_octal> <archivo>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *image_path = argv[1];
    const char *perm_str = argv[2];
    const char *filename = argv[3];

    // Validar permisos en octal
    char *endptr;
    long perms = strtol(perm_str, &endptr, 8);
    if (*endptr != '\0' || perms < 0 || perms > 0777) {
        fprintf(stderr, "Permisos inválidos: %s\n", perm_str);
        return EXIT_FAILURE;
    }

    // Buscar inodo del archivo
    int inode_num = dir_lookup(image_path, filename);
    if (inode_num < 0) {
        fprintf(stderr, "Error: '%s' no existe\n", filename);
        return EXIT_FAILURE;
    }

    // Leer inodo
    struct inode in;
    if (read_inode(image_path, inode_num, &in) < 0) {
        fprintf(stderr, "Error: no se pudo leer inodo de '%s'\n", filename);
        return EXIT_FAILURE;
    }

    // Verificar que sea archivo regular o directorio
    // Permitimos chmod tanto en archivos como en directorios
    if (!((in.mode & INODE_MODE_FILE) == INODE_MODE_FILE || (in.mode & INODE_MODE_DIR) == INODE_MODE_DIR)) {
        fprintf(stderr, "Error: '%s' no es un archivo ni directorio válido\n", filename);
        return EXIT_FAILURE;
    }

    // Mantener el bit de tipo y actualizar permisos
    in.mode = (in.mode & (INODE_MODE_FILE | INODE_MODE_DIR)) | (perms & 0777);

    // Escribir inodo actualizado
    if (write_inode(image_path, inode_num, &in) < 0) {
        fprintf(stderr, "Error: no se pudo escribir inodo de '%s'\n", filename);
        return EXIT_FAILURE;
    }

    printf("Permisos de '%s' cambiados a %o\n", filename, (unsigned)perms);
    return EXIT_SUCCESS;
}
