/*
 * FileSystem.c
 *
 *  Created on: 24/09/2013
 *      Author: utnso
 */

#define FUSE_USE_VERSION 27
#define _FILE_OFFSET_BITS 64

#include "FileSystem.h"

#define CUSTOM_FUSE_OPT_KEY(t, p, v) { t, offsetof(struct t_runtime_options, p), v }

//-------------------------------------------------------------------------------------------------
// Propietarias
//-------------------------------------------------------------------------------------------------

void getConfiguracion(void) {

	configFile = config_create(CONFIG_PATH);
	config = malloc(sizeof(configuracion_koopa_t));

	config->binPath = malloc(
			strlen(config_get_string_value(configFile, "binPath")) + 1);
	strcpy(config->binPath, config_get_string_value(configFile, "binPath"));

	config->puntoMontaje = malloc(
			strlen(config_get_string_value(configFile, "	")) + 1);
	strcpy(config->puntoMontaje,
			config_get_string_value(configFile, "puntoMontaje"));

}

//-------------------------------------------------------------------------------------------------
// File System
//-------------------------------------------------------------------------------------------------

void fileSystemCrear() {

	grasaFS = malloc(sizeof(t_grasa_fs));

	grasaFS->disco = discoCrear();
	grasaFS->pHeader = (GHeader*) grasaFS->disco->mem;
	int blkBitmap = grasaFS->pHeader->blk_bitmap; //BLoque de inicio del bitmap
	int tamanioBitmap = grasaFS->pHeader->size_bitmap * BLOCK_SIZE;
	grasaFS->pBitmap = bitarray_create(
			(char*) grasaFS->disco->mem + (blkBitmap * BLOCK_SIZE),
			tamanioBitmap);

	int j = grasaFS->pHeader->size_bitmap + 1;
	grasaFS->nodos = (GFile*) &grasaFS->disco->mem[j * BLOCK_SIZE];

}

//---------------------------------------------------------------------------------------------------

t_disco* discoCrear() {
	t_disco* disco = malloc(sizeof(t_disco));

	disco->fd = open(config->binPath, O_RDWR);

	struct stat estadistica;

	stat(config->binPath, &estadistica);
	disco->tamanio = estadistica.st_size;

	disco->mem = mmap(NULL, disco->tamanio, PROT_READ | PROT_WRITE, MAP_SHARED,
			disco->fd, 0);
	int endBlock = (disco->tamanio / BLOCK_SIZE) - 1;
	disco->endBlock = (ptrGBloque) &disco->mem[endBlock];
	disco->firstBlock = (ptrGBloque) &disco->mem[0];

	if (disco->mem == MAP_FAILED ) {
		log_error(logFile, "No se pudo mapean el disco");
		return NULL ;
	}

	log_info(logFile, "disco: [%s] tam: [%d]", config->binPath, disco->tamanio);

	return disco;

}

//----------------------------------------------------------------------------------------------------------

int rutaToNumberBlock(const char* ruta) {

	char** vector_path = rutaToArray(ruta);

	int parent_dir = 0;
	int pos_archivo = 0;

	int j;
	int i = 0;

	while (vector_path[i] != NULL ) {
		for (j = 0; j < 1024; j++) {
			if ((strcmp(grasaFS->nodos[j].filename, vector_path[i]) == 0)
					&& (grasaFS->nodos[j].parent_dir_block == parent_dir)) {
				parent_dir = grasaFS->nodos[j].parent_dir_block;
				pos_archivo = j;
				break;
			}
		}
		i++;
	}

	if (j == 1024) {
		return -1;
	}
	return pos_archivo;

}

int padreRutaToNumberBlock(const char* path) {
	char* temp;
	temp = string_duplicate((char*) path);
	int i, j, ret;

	for (i = 0; i < strlen(path); i++) {
		if (temp[i] == '/') {
			j = i;
		}
	}
	temp = string_substring_until((char*) path, j);
	ret = rutaToNumberBlock(temp);
	free(temp);
	return ret;
}

//----------------------------------------------------------------------------------------------------------

int buscarNodoDisponible() {
	//TODO:
	int retval = 0, i = 0;

	pthread_mutex_lock(&mutex);
	for (i = 0; i < grasaFS->pHeader->size_bitmap; i++) {
		if (bitarray_test_bit(grasaFS->pBitmap, i) == 1)
			i++;
		else
			break;
	}

	if (i < grasaFS->pHeader->size_bitmap) {
		bitarray_set_bit(grasaFS->pBitmap, i);
		retval = i;
	} else
		retval = -1;
	pthread_mutex_unlock(&mutex);

	return retval;
}

//----------------------------------------------------------------------------------------------------------

char** rutaToArray(const char* text) {
	int length_value = strlen(text) - 1;
	char* temp = string_duplicate((char*) text);
	char* value_without_brackets = string_substring(temp, 1, length_value);
	char **array_values = string_split(value_without_brackets, "/");

	free(value_without_brackets);
	free(temp);

	return array_values;
}

//----------------------------------------------------------------------------------------------------------

int directorioVacio(int blkDirectorio){

	int ret = 1;
	int i;

	for (i = 0; i < 1024; i++){
		if (grasaFS->nodos[i].parent_dir_block == blkDirectorio){
			ret = 0;
			break;
		}
	}

	return ret;
}

//----------------------------------------------------------------------------------------------------------

void disponerNodo(int blkDirectorio){
	//TODO:
}

//-------------------------------------------------------------------------------------------------
// FUSE
//-------------------------------------------------------------------------------------------------

static int grasa_getattr(const char *path, struct stat *stbuf) {
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));

	//Si path es igual a "/" nos estan pidiendo los atributos del punto de montaje
	if (strcmp(path, DIRECTORIO_RAIZ) == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else {
//			todo: 	mutear???
		int pos_archivo = rutaToNumberBlock(path);
		if (pos_archivo == -1) {
			return -ENOENT;
		}
		if (pos_archivo != -1) {
			if (grasaFS->nodos[pos_archivo].state == ARCHIVO) {
				stbuf->st_mode = S_IFREG | 0444;
				stbuf->st_nlink = 1;
				stbuf->st_size = grasaFS->nodos[pos_archivo].file_size;
			} else if (grasaFS->nodos[pos_archivo].state == DIRECTORIO) {
				stbuf->st_mode = S_IFDIR | 0755;
				stbuf->st_nlink = 1;
			}
		} else {
			res = -ENOENT;
		}
//			todo: mutear????
	}

	return res;
}

//-------------------------------------------------------------------------------------------------

static int grasa_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi) {
	(void) offset;
	(void) fi;

	int i;
	int blkDirPadre = rutaToNumberBlock(path);
	if (blkDirPadre == -1) {
		return -ENOENT;
	}

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	//TODO: Mutear

	if (strcmp(path, DIRECTORIO_RAIZ) == 0) {
		for (i = 0; i < 1024; i++) {
			if (grasaFS->nodos[i].parent_dir_block == 0
					&& grasaFS->nodos[i].state != BORRADO) {
				filler(buf, (char*) grasaFS->nodos[i].filename, NULL, 0);
			}
		}
	} else {
		for (i = 0; i < 1024; i++) {
			if (grasaFS->nodos[i].parent_dir_block == blkDirPadre
					&& grasaFS->nodos[i].state != BORRADO) {
				filler(buf, (char*) grasaFS->nodos[i].filename, NULL, 0);
			}
		}
	}

	return 0;
}

//-------------------------------------------------------------------------------------------------

static int grasa_open(const char *path, struct fuse_file_info *fi) {

	int blkDir = rutaToNumberBlock(path);

	if (blkDir == -1)
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}
//-------------------------------------------------------------------------------------------------

static int grasa_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi) {
//	TODO: Refactorizar esta mierda
	size_t len;
	(void) fi;

	int blkDir = rutaToNumberBlock(path);
	int nroBlk, blkIndirecto, blkDirecto, offsetBlk;

	if (blkDir == -1)
		return -ENOENT;

	len = grasaFS->nodos[blkDir].file_size;
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;

		nroBlk = offset / BLOCK_SIZE;
		offsetBlk = offset % BLOCK_SIZE;
		blkIndirecto = nroBlk / 1024;
		blkDirecto = nroBlk % 1024;

//    		pthread_mutex_lock(&mutex);
		ptrGBloque *blk_direct =
				(ptrGBloque*) (grasaFS->disco->mem
						+ grasaFS->nodos[blkDir].blk_indirect[blkIndirecto]
								* BLOCK_SIZE); //blk_direct = Array[1024]
//    		pthread_mutex_unlock(&mutex);

		bzero(buf, size); //Limpio el buffer
		int bytes_leidos = 0;

		if ((BLOCK_SIZE - offsetBlk) >= size)  // Si entra en el prime bloque
				{
//    			pthread_mutex_lock(&mutex);
			memcpy(buf,
					(char *) (grasaFS->disco->mem
							+ blk_direct[blkDirecto] * BLOCK_SIZE + offsetBlk),
					size);
//    			pthread_mutex_unlock(&mutex);
			bytes_leidos = size;
		} else {
//    			pthread_mutex_lock(&mutex);
			memcpy(buf,
					(char *) (grasaFS->disco->mem
							+ blk_direct[blkDirecto] * BLOCK_SIZE + offsetBlk),
					BLOCK_SIZE - offsetBlk); // Copia lo que puesdas
//    			pthread_mutex_unlock(&mutex);
			bytes_leidos += (BLOCK_SIZE - offsetBlk);
			size -= (BLOCK_SIZE - offsetBlk);
			blkDirecto++;

			if (blkDirecto == 1024) // voy al proximo puntero indirecto y me coloco al principio
					{
				blkDirecto = 0;
				blkIndirecto++;
//    				pthread_mutex_lock(&mutex);
				blk_direct = (ptrGBloque*) (grasaFS->disco->mem
						+ grasaFS->nodos[blkDir].blk_indirect[blkDirecto]
								* BLOCK_SIZE);
//    				pthread_mutex_unlock(&mutex);
			}

			int cant_bloques_por_leer = size / BLOCK_SIZE;
			int bytes_por_leer = size % BLOCK_SIZE;

			if (cant_bloques_por_leer == 0) {
//    				pthread_mutex_lock(&mutex);
				memcpy(buf + bytes_leidos,
						(char *) (grasaFS->disco->mem
								+ blk_direct[blkDirecto] * BLOCK_SIZE),
						bytes_por_leer);
//    				pthread_mutex_unlock(&mutex);
				bytes_leidos += bytes_por_leer;
				size -= bytes_por_leer;
			} else {
				int k;
				for (k = 1; k <= cant_bloques_por_leer; k++) {
//    					pthread_mutex_lock(&mutex);
					memcpy(buf + bytes_leidos,
							(char *) (grasaFS->disco->mem
									+ (blk_direct[blkDirecto] * BLOCK_SIZE)),
							BLOCK_SIZE);
//    					pthread_mutex_unlock(&mutex);
					bytes_leidos += BLOCK_SIZE;
					size -= BLOCK_SIZE;
					++blkDirecto;

					if (blkDirecto == 1024) {
						blkDirecto = 0;
						blkIndirecto++;
//    						pthread_mutex_lock(&mutex);
						blk_direct =
								(ptrGBloque*) (grasaFS->disco->mem
										+ grasaFS->nodos[blkDir].blk_indirect[blkIndirecto]
												* BLOCK_SIZE);
//    						pthread_mutex_unlock(&mutex);
					}
				}

				if (bytes_por_leer > 0) {
//    					pthread_mutex_lock(&mutex);
					memcpy(buf + bytes_leidos,
							(char *) (grasaFS->disco->mem
									+ (blk_direct[blkDirecto] * BLOCK_SIZE)),
							bytes_por_leer);
//    					pthread_mutex_unlock(&mutex);
					bytes_leidos += bytes_por_leer;
					size -= bytes_por_leer;
				}
			}
		}

		return bytes_leidos;
	} else
		size = 0;

	return size;
}

//-------------------------------------------------------------------------------------------------

static int grasa_mkdir(const char *path, mode_t mode) {

	mode = S_IFDIR | 0755;

//	todo: mutuar aca si o si
	int nroNodo = buscarNodoDisponible();

	if (nroNodo != -1) {
		strcpy((char*) grasaFS->nodos[nroNodo].filename,
				strrchr(path, '/') + 1);
		grasaFS->nodos[nroNodo].state = DIRECTORIO;
//		int blkPadre = padreRutaToNumberBlock(path);
		grasaFS->nodos[nroNodo].parent_dir_block = padreRutaToNumberBlock(path);
		return 0;
	} else {
		return -ENOENT;
	}

}

//-------------------------------------------------------------------------------------------------

static int grasa_create(const char *path, mode_t mode,
		struct fuse_file_info *fi) {
	//TODO: Not implemented exception
	int retval = 0, i = 0, nroNodo = buscarNodoDisponible();
	mode = S_IFREG | 0444;

	if (nroNodo == -1)
		retval = -ENOENT;
	else {
		strcpy((char*) grasaFS->nodos[nroNodo].filename,
				strrchr(path, '/') + 1);
		grasaFS->nodos[nroNodo].state = ARCHIVO;
		grasaFS->nodos[nroNodo].parent_dir_block = padreRutaToNumberBlock(path);
		grasaFS->nodos[nroNodo].file_size = 0;

		for (i = 0; i < 1000; i++)
			grasaFS->nodos[nroNodo].blk_indirect[i] = 0;
	}

	return retval;
}

//-------------------------------------------------------------------------------------------------

static int grasa_rmdir(const char *path) {

	int blkDirectorio = rutaToNumberBlock(path);

	if (blkDirectorio == -1){
		return -ENOENT;
	}

	if (directorioVacio(blkDirectorio)){
		bzero(grasaFS->nodos[blkDirectorio].filename, 71);
		grasaFS->nodos[blkDirectorio].state = BORRADO;
		grasaFS->nodos[blkDirectorio].parent_dir_block = 0;
	}

	disponerNodo(blkDirectorio);

	return 0;

}

//-------------------------------------------------------------------------------------------------

static int grasa_unlink() {
	//TODO: Not implemented exception
	return 1;
}

//-------------------------------------------------------------------------------------------------

static int grasa_truncate() {
	//TODO: Not implemented exception
	return 1;
}

//-------------------------------------------------------------------------------------------------

static int grasa_write() {
	//TODO: Not implemented exception
	return 1;
}

//-------------------------------------------------------------------------------------------------

static int grasa_utimens() {
	//TODO: Not implemented exception
	return 1;
}

static struct fuse_operations grasa_oper = { .getattr = grasa_getattr,
		.readdir = grasa_readdir, .read = grasa_read, .open = grasa_open,
		.mkdir = grasa_mkdir, .create = grasa_create, .rmdir = grasa_rmdir,
		.unlink = grasa_unlink, .truncate = grasa_truncate,
		.write = grasa_write, .utimens = grasa_utimens, };

enum {
	KEY_VERSION, KEY_HELP,
};

static struct fuse_opt fuse_options[] = {

FUSE_OPT_KEY("-V", KEY_VERSION), FUSE_OPT_KEY("--version", KEY_VERSION),
		FUSE_OPT_KEY("-h", KEY_HELP), FUSE_OPT_KEY("--help", KEY_HELP),
		FUSE_OPT_END, };

//-------------------------------------------------------------------------------------------------

int main(int argc, char* argv[]) {

	getConfiguracion();
	logFile = log_create(LOG_PATH, "GRASA", true, LOG_LEVEL_INFO);

	fileSystemCrear();

	return fuse_main(argc-1, &argv[1], &grasa_oper, NULL);

}

