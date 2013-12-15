/*
 * FileSystem.c
 *
 *  Created on: 24/09/2013
 *      Author: utnso
 */

#include "FileSystem.h"
#include <stdlib.h>

int main(void){

	getConfiguracion();
	logFile = log_create(LOG_PATH, "GRASA" ,true ,LOG_LEVEL_INFO);

	t_grasa_fs* garasaFS = fileSystemCrear();


	return EXIT_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
// Propietarias
//-------------------------------------------------------------------------------------------------

void getConfiguracion(void) {

	configFile = config_create(CONFIG_PATH);
	config = malloc(sizeof(configuracion_koopa_t));

	config->binPath = malloc(strlen(config_get_string_value(configFile, "binPath")) + 1);
	strcpy(config->binPath, config_get_string_value(configFile, "binPath"));

	config->puntoMontaje = malloc(strlen(config_get_string_value(configFile, "	")) + 1);
	strcpy(config->puntoMontaje, config_get_string_value(configFile, "puntoMontaje"));

}

//-------------------------------------------------------------------------------------------------
// File System
//-------------------------------------------------------------------------------------------------

t_grasa_fs* fileSystemCrear(){
	t_grasa_fs* fileSys = malloc(sizeof(t_grasa_fs));

	fileSys->disco = discoCrear();
	fileSys->pHeader = (ptrGBloque)&fileSys->disco->mem[0];
	int blkBitmap = fileSys->pHeader->blk_bitmap; //BLoque de inicio del bitmap
	int tamanioBitmap = fileSys->pHeader->size_bitmap*BLOCK_SIZE;
	fileSys->pBitmap = bitarray_create(fileSys->disco->mem+(blkBitmap*BLOCK_SIZE), tamanioBitmap);

	int j=fileSys->pHeader->size_bitmap+1;
	fileSys->nodos = (GFile*)&fileSys->disco->mem[j*BLOCK_SIZE];

	return fileSys;
}

//---------------------------------------------------------------------------------------------------

t_disco* discoCrear(){
	t_disco* disco = malloc(sizeof(t_disco));

	disco->fd  = open(config->binPath,O_RDWR);

	struct stat estadistica;

	stat(config->binPath,&estadistica);
	disco->tamanio = estadistica.st_size;

	disco->mem = mmap(NULL, disco->tamanio, PROT_READ | PROT_WRITE, MAP_SHARED, disco->fd, 0);
	int endBlock = (disco->tamanio / BLOCK_SIZE) - 1;
	disco->endBlock =  (ptrGBloque)&disco->mem[endBlock];
	disco->firstBlock=  (ptrGBloque)&disco->mem[0];

	if (disco->mem == MAP_FAILED){
		log_error(logFile,"No se pudo mapean el disco");
		return NULL;
	}


	log_info(logFile,"disco: [%s] tam: [%d]",config->binPath,disco->tamanio);

	return disco;

}



//-------------------------------------------------------------------------------------------------
// FUSE
//-------------------------------------------------------------------------------------------------

static int grasa_getattr(const char *path, struct stat *stbuf){
	 //TODO:
	return 1;
}

//-------------------------------------------------------------------------------------------------

static int grasa_readdir(){
	//TODO: Not implemented exception
	return 1;
}

//-------------------------------------------------------------------------------------------------

static int grasa_read(){
	//TODO: Not implemented exception
	return 1;
}

//-------------------------------------------------------------------------------------------------

static int grasa_open(){
	//TODO: Not implemented exception
	return 1;
}

//-------------------------------------------------------------------------------------------------

static int grasa_mkdir(){
	//TODO: Not implemented exception
	return 1;
}

//-------------------------------------------------------------------------------------------------

static int grasa_create(){
	//TODO: Not implemented exception
	return 1;
}

//-------------------------------------------------------------------------------------------------

static int grasa_rmdir(){
	//TODO: Not implemented exception
	return 1;
}

//-------------------------------------------------------------------------------------------------

static int grasa_unlink(){
	//TODO: Not implemented exception
	return 1;
}

//-------------------------------------------------------------------------------------------------

static int grasa_truncate(){
	//TODO: Not implemented exception
	return 1;
}

//-------------------------------------------------------------------------------------------------

static int grasa_write(){
	//TODO: Not implemented exception
	return 1;
}

//-------------------------------------------------------------------------------------------------

static int grasa_utimens(){
	//TODO: Not implemented exception
	return 1;
}



static struct fuse_operations grasa_oper = {
											.getattr = grasa_getattr,
//											.readdir = grasa_readdir,
											.read = grasa_read,
											.open = grasa_open,
											.mkdir = grasa_mkdir,
//											.create = grasa_create,
											.rmdir = grasa_rmdir,
											.unlink = grasa_unlink,
											.truncate = grasa_truncate,
											.write = grasa_write,
//											.utimens = grasa_utimens,
											};

enum {
		KEY_VERSION, KEY_HELP,
		};


static struct fuse_opt fuse_options[] = {

   FUSE_OPT_KEY("-V", KEY_VERSION),
   FUSE_OPT_KEY("--version", KEY_VERSION),
   FUSE_OPT_KEY("-h", KEY_HELP),
   FUSE_OPT_KEY("--help", KEY_HELP),
   FUSE_OPT_END,
};



