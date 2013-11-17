#ifndef GRASA_H_
#define GRASA_H_

#include <stdint.h>

#define GFILEBYTABLE 1024 //Cantidad max archivos
#define GFILEBYBLOCK 1
#define GFILENAMELENGTH 71 //Tamaño max nombre de archivo
#define GHEADERBLOCKS 1
#define BLOQUEINDIRECT 1000 //Bloques indirectos

//Puntero a bloque
typedef uint32_t ptrGBloque;

//Defino Header
typedef struct grasa_header_t { // un bloque
	unsigned char grasa[5];
	uint32_t version;
	uint32_t blk_bitmap;
	uint32_t size_bitmap; // en bloques
	unsigned char padding[4073];
} GHeader;

//Defino Nodo
typedef struct grasa_file_t { // un cuarto de bloque (256 bytes)
	uint8_t state; // 0: borrado, 1: archivo, 2: directorio
	unsigned char filename[GFILENAMELENGTH];
	uint32_t parent_dir_block;
	uint32_t file_size;
	uint64_t created;
	uint64_t modified;
	ptrGBloque blk_indirect[BLOQUEINDIRECT];
} GFile;


#endif /* GRASA_H_ */
