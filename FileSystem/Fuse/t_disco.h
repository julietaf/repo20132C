#ifndef T_DISCO_H_
#define T_DISCO_H_

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
#include <unistd.h>

typedef GHeader* pBloque;

// Defino la estructura del Disco
typedef struct __disco {
	int fd;
	size_t tamanio;
	ptrGBloque endBlock;
	unsigned char* mem;
	t_log* log;
	pBloque curr_block;
} t_disco;

t_disco* disco_crear(char*, t_log*);
void disco_destroy(t_disco*);
pBloque disco_seek(t_disco* , ptrGBloque );
void disco_no_desalojo_bloques(t_disco* ,size_t );

#endif /* T_DISCO_H_ */
