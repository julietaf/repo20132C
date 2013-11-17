#include <stdio.h>
#include <stdlib.h>
#include "t_grasa_fs.h"

#define BLOCK_SIZE sizeof(GFile)


int main(int argc, char **argv) {

	t_grasa_fs* gfs = grasa_fs_crear(argv[1]);
	grasa_fs_imprimir_estructuras(gfs);
	grasa_fs_destroy(gfs);

	return 0;
}
