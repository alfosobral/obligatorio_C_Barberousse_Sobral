#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "vfs.h"

// Busca un inodo dentro de un directorio dado
int dir_lookup_in(const char *image_path, uint32_t parent_inode, const char *name) {
    struct inode din;
    if (read_inode(image_path, parent_inode, &din) < 0) return -1;
    uint8_t buffer[BLOCK_SIZE];
    if (read_block(image_path, din.direct[0], buffer) < 0) return -1;
    struct dir_entry *entries = (struct dir_entry *)buffer;
    for (size_t i = 0; i < DIR_ENTRIES_PER_BLOCK; i++) {
        if (entries[i].inode != 0 && strcmp(entries[i].name, name) == 0) {
            return entries[i].inode;
        }
    }
    return 0;
}

// Añade una entrada a un directorio específico
int add_dir_entry_to(const char *image_path, uint32_t parent_inode,
                     const char *name, uint32_t inode_num) {
    struct inode din;
    if (read_inode(image_path, parent_inode, &din) < 0) return -1;
    uint8_t buffer[BLOCK_SIZE];
    if (read_block(image_path, din.direct[0], buffer) < 0) return -1;
    struct dir_entry *entries = (struct dir_entry *)buffer;
    for (size_t i = 0; i < DIR_ENTRIES_PER_BLOCK; i++) {
        if (entries[i].inode == 0) {
            entries[i].inode = inode_num;
            strncpy(entries[i].name, name, FILENAME_MAX_LEN);
            if (write_block(image_path, din.direct[0], buffer) < 0) return -1;
            return 0;
        }
    }
    return -1;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <imagen> <origen> <destino>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *image = argv[1];
    const char *src_path = argv[2];
    const char *dst_path = argv[3];

    // Verificar imagen VFS
    struct superblock sb;
    if (read_superblock(image, &sb) < 0) {
        fprintf(stderr, "Error: '%s' no es una imagen VFS válida\n", image);
        return EXIT_FAILURE;
    }

    // Inodo origen
    int src_inode = dir_lookup(image, src_path);
    if (src_inode <= 0) {
        fprintf(stderr, "Error: origen '%s' no encontrado\n", src_path);
        return EXIT_FAILURE;
    }

    // Separar ruta destino en directorio padre y nombre
    char buf[256];
    strncpy(buf, dst_path, sizeof(buf));
    char *slash = strrchr(buf, '/');
    uint32_t parent_inode = ROOTDIR_INODE;
    char *dst_name;
    if (slash) {
        *slash = '\0';
        dst_name = slash + 1;
        int pin = dir_lookup(image, buf);
        if (pin <= 0) {
            fprintf(stderr, "Error: directorio '%s' no encontrado\n", buf);
            return EXIT_FAILURE;
        }
        parent_inode = pin;
    } else {
        dst_name = buf;
    }

    // Validar nombre
    if (!name_is_valid(dst_name)) {
        fprintf(stderr, "Error: nombre inválido '%s'\n", dst_name);
        return EXIT_FAILURE;
    }

    // Si destino existe, eliminar sin warning
    int dst_inode = dir_lookup_in(image, parent_inode, dst_name);
    if (dst_inode > 0) {
        struct inode tmp;
        if (read_inode(image, dst_inode, &tmp) == 0) {
            inode_trunc_data(image, &tmp);
            free_inode(image, dst_inode);
        }
        remove_dir_entry(image, dst_path);
    }

    // Añadir entrada en destino
    if (add_dir_entry_to(image, parent_inode, dst_name, src_inode) < 0) {
        fprintf(stderr, "Error: no se pudo crear la entrada '%s'\n", dst_name);
        return EXIT_FAILURE;
    }

    // Eliminar la entrada antigua
    remove_dir_entry(image, src_path);

    printf("Renombrado '%s' a '%s' exitosamente.\n", src_path, dst_path);
    return EXIT_SUCCESS;
}
