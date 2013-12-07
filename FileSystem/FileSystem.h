/*
 * FileSystem.h
 *
 *  Created on: 24/09/2013
 *      Author: utnso
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "grasa.h"
#include <commons/log.h>
#include <commons/string.h>
#include <commons/bitarray.h>
#include <unistd.h>
#include <stdint.h>

#define GFILEBYTABLE 1024 //Cantidad max archivos
#define GFILEBYBLOCK 1
#define GFILENAMELENGTH 71 //Tamaño max nombre de archivo
#define GHEADERBLOCKS 1
#define BLOQUEINDIRECT 1000 //Bloques indirectos
#define CONFIG_PATH "./FuseProc.log"


enum enum_estado{
	BORRADO,
	ARCHIVO,
	DIRECTORIO
};

// Defino la estructura del Disco
typedef struct {
	int fd;
	size_t tamanio;
	ptrGBloque endBlock;
	unsigned char* mem;
	GHeader *current_block;
} t_disco;


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
typedef struct grasa_nodo_t { // un cuarto de bloque (256 bytes)
	uint8_t state; // 0: borrado, 1: archivo, 2: directorio
	unsigned char filename[GFILENAMELENGTH];
	uint32_t parent_dir_block;
	uint32_t file_size;
	uint64_t created;
	uint64_t modified;
	uint32_t blk_indirect[BLOQUEINDIRECT];
} GNodo;

typedef struct __grasa_fs {
	GHeader* pHeader;
	t_bitarray* pBitmap;
	GNodo* nodos;
	t_disco* disco;
} t_grasa_fs;


t_disco *disco_crear(char *path);
void disco_destroy(t_disco*);
//GHeader *disco_seek(t_disco*, ptrGBloque);
void disco_no_desalojo_bloques(t_disco*, size_t);
t_log *logFile;
t_grasa_fs* grasa_fs_crear(char*);
void grasa_fs_destroy(t_grasa_fs*);
void grasa_fs_log_estructuras(t_grasa_fs*);

#endif /* FILESYSTEM_H_ */
