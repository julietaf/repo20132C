#ifndef T_GRASA_FS_H_
#define T_GRASA_FS_H_

//Defino el tamaño del Bloque: 4096 bytes
#define BLOCK_SIZE sizeof(GFile)

#include "t_disco.h"
#include "grasa.h"
#include <commons/string.h>
#include <commons/bitarray.h>
#include <commons/log.h>

typedef struct __grasa_fs {
	GHeader* pHeader;
	t_bitarray* pBitmap;
	GFile* nodos[1024];
	t_disco* disco;
	t_log* log;
} t_grasa_fs;

t_grasa_fs* grasa_fs_crear(char* ) ;
void grasa_fs_destroy(t_grasa_fs*);
void grasa_fs_imprimir_estructuras(t_grasa_fs*);


#endif /* T_GRASA_FS_H_ */
