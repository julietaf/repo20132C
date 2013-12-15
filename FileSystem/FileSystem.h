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
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <fuse.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <commons/bitarray.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>

#define GFILEBYTABLE 1024 //Cantidad max archivos
#define GFILEBYBLOCK 1
#define GFILENAMELENGTH 71 //Tamaño max nombre de archivo
#define GHEADERBLOCKS 1
#define BLOQUEINDIRECT 1000 //Bloques indirectos
#define CONFIG_PATH "./FuseConf.txt"
#define LOG_PATH "./FuseLog.txt"
#define BLOCK_SIZE 4096

enum enum_estado{
	BORRADO,
	ARCHIVO,
	DIRECTORIO
};

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

// Defino la estructura del Disco
typedef struct {
	int fd;
	size_t tamanio;
	ptrGBloque *endBlock;
	ptrGBloque *firstBlock;
	uint8_t* mem;
} t_disco;

//Defino Nodo
typedef struct grasa_nodo_t { // un cuarto de bloque (256 bytes)
	uint8_t state; // 0: borrado, 1: archivo, 2: directorio
	unsigned char filename[GFILENAMELENGTH];
	ptrGBloque parent_dir_block;
	uint32_t file_size;
	uint64_t created;
	uint64_t modified;
	ptrGBloque blk_indirect[BLOQUEINDIRECT];
} GFile;

typedef struct __grasa_fs {
	GHeader* pHeader;
	t_bitarray* pBitmap;
	GFile* nodos;//1024
	t_disco* disco;
} t_grasa_fs;

typedef struct {
	char* puntoMontaje;
	char* binPath;
} configuracion_koopa_t;

t_log *logFile;
t_config* configFile;
configuracion_koopa_t *config;

void getConfiguracion();
t_grasa_fs* fileSystemCrear();
t_disco* discoCrear();

#endif /* FILESYSTEM_H_ */
