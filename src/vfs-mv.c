#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vfs.h"

// Busca y devuelve el número de inodo de 'name' dentro del directorio 'parent_inode'
int dir_lookup_in(const char *image_path, uint32_t parent_inode, const char *name) {
    struct inode din;
    if (read_inode(image_path, parent_inode, &din) < 0) return -1;
    uint8_t buffer[BLOCK_SIZE];
    if (read_block(image_path, din.direct[0], buffer) < 0) return -1;
    struct dir_entry *entries = (struct dir_entry *)buffer;
    for (int i = 0; i < DIR_ENTRIES_PER_BLOCK; i++) {
        if (entries[i].inode != 0 && strcmp(entries[i].name, name) == 0) {
            return entries[i].inode;
        }
    }
    return 0; // no encontrado
}

// Agrega una entrada al directorio dado por parent_inode
int add_dir_entry_to(const char *image_path, uint32_t parent_inode,
                     const char *name, uint32_t inode_num) {
    struct inode din;
    if (read_inode(image_path, parent_inode, &din) < 0) return -1;
    uint8_t buffer[BLOCK_SIZE];
    if (read_block(image_path, din.direct[0], buffer) < 0) return -1;
    struct dir_entry *entries = (struct dir_entry *)buffer;
    for (int i = 0; i < DIR_ENTRIES_PER_BLOCK; i++) {
        if (entries[i].inode == 0) {
            entries[i].inode = inode_num;
            strncpy(entries[i].name, name, FILENAME_MAX_LEN);
            write_block(image_path, din.direct[0], buffer);
            return 0;
        }
    }
    return -1; // directorio lleno
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <imagen> <origen> <destino>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *image = argv[1];
    const char *src_path = argv[2];
    const char *dst_path = argv[3];

    // Obtener nombre y parent para origen (su padre siempre root en FS plano)
    int src_inode = dir_lookup(image, src_path);
    if (src_inode <= 0) {
        fprintf(stderr, "Error: origen '%s' no encontrado\n", src_path);
        return EXIT_FAILURE;
    }

    // Partir dst_path en parent y name
    char dst_copy[256];
    strncpy(dst_copy, dst_path, sizeof(dst_copy));
    char *slash = strrchr(dst_copy, '/');
    uint32_t parent_inode = ROOTDIR_INODE;
    char *dst_name;
    if (slash) {
        // extraer parent y nombre
        *slash = '\0';
        dst_name = slash + 1;
        int pin = dir_lookup(image, dst_copy);
        if (pin <= 0) {
            fprintf(stderr, "Error: directorio '%s' no encontrado\n", dst_copy);
            return EXIT_FAILURE;
        }
        parent_inode = pin;
    } else {
        dst_name = dst_copy;
    }

    // Validar nombre destino
    if (!name_is_valid(dst_name)) {
        fprintf(stderr, "Error: nombre inválido '%s'\n", dst_name);
        return EXIT_FAILURE;
    }

    // Si existe destino, error
    int dst_inode = dir_lookup_in(image, parent_inode, dst_name);
    if (dst_inode > 0) {
        fprintf(stderr, "Error: destino '%s' ya existe\n", dst_name);
        return EXIT_FAILURE;
    }

    // Añadir en nuevo directorio
    if (add_dir_entry_to(image, parent_inode, dst_name, src_inode) < 0) {
        fprintf(stderr, "Error: no se pudo agregar '%s' a inodo %u\n", dst_name, parent_inode);
        return EXIT_FAILURE;
    }

    // Eliminar del root o de su padre original
    if (remove_dir_entry(image, src_path) < 0) {
        // intentar eliminar del parent original si no era root
        remove_dir_entry(image, src_path);
    }

    printf("Movido '%s' a '%s' satisfactoriamente.\n", src_path, dst_path);
    return EXIT_SUCCESS;
}
