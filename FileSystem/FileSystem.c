/*
 * FileSystem.c
 *
 *  Created on: 24/09/2013
 *      Author: utnso
 */

#define FUSE_USE_VERSION 27
#define _FILE_OFFSET_BITS 64
#define FILE_MAX_SIZE 4187593113,6

#include "FileSystem.h"

#define CUSTOM_FUSE_OPT_KEY(t, p, v) { t, offsetof(struct t_runtime_options, p), v }
#define FILE_MAX_SIZE 4187593113,6

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
			strlen(config_get_string_value(configFile, "puntoMontaje")) + 1);
	strcpy(config->puntoMontaje,
			config_get_string_value(configFile, "puntoMontaje"));

}

void inicializarLog() {

	remove(LOG_PATH);
	logFile = log_create(LOG_PATH, "GRASA", true, LOG_LEVEL_DEBUG);
	log_info(logFile,
			"------------------------------- Grasa -------------------------------");
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

int rutaToNumberBlock(const char* ruta) { //aunque en realidad devuelve el numero de nodo

	char** vector_path = rutaToArray(ruta);

	int parent_dir = 0;
	int pos_archivo = 0;

	int j;
	int i = 0;

	while (vector_path[i] != NULL ) {
		for (j = 0; j < 1024; j++) {
			if ((strcmp((char*) grasaFS->nodos[j].filename,
					(char*) vector_path[i]) == 0)
					&& (grasaFS->nodos[j].parent_dir_block == parent_dir)) {
				parent_dir = j;
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

//----------------------------------------------------------------------------------------------------------

int padreRutaToNumberBlock(const char* path) {
	char* temp;
	temp = string_duplicate((char*) path);
	int i, j, ret;

	for (i = 0; i < strlen(path); i++) {
		if (temp[i] == '/') {
			j = i;
		}
	}

	if (j == 0)
		j = 1;
	temp = string_substring_until((char*) path, j);
	ret = rutaToNumberBlock(temp);
	free(temp);
	return ret;
}

//----------------------------------------------------------------------------------------------------------

//int buscarBloqueDisponible() {
//	//TODO: ver esto
//	int retval = 0, i = 0;
//
//	pthread_mutex_lock(&mutex);
//	for (i = 0; i < grasaFS->pHeader->size_bitmap; i++) {
//		if (bitarray_test_bit(grasaFS->pBitmap, i) == 1)
//			i++;
//		else
//			break;
//	}
//
//	if (i < grasaFS->pHeader->size_bitmap) {
//		bitarray_set_bit(grasaFS->pBitmap, i);
//		retval = i;
//	} else
//		retval = -1;
//	pthread_mutex_unlock(&mutex);
//
//	return retval;
//()}

void reservarBloqueDirecto(int nroNodo, int nroBloque) {

	int posBitmap = grasaFS->pHeader->blk_bitmap + 1024
			+ grasaFS->pHeader->size_bitmap;

	while (bitarray_test_bit(grasaFS->pBitmap, posBitmap)) {
		posBitmap++;
	}

	bitarray_set_bit(grasaFS->pBitmap, posBitmap);

	grasaFS->nodos[nroNodo].blk_indirect[nroBloque] = posBitmap;

	ptrGBloque* blkDirect = seek(nroNodo, nroBloque);

	int i;
	for (i = 0; i < 1024; i++) {
		blkDirect[i] = 0;
	}

}

//----------------------------------------------------------------------------------------------------------

void reservarBloqueDatos(ptrGBloque* blkDirect, int nroBlk) {

	int posBitmap = grasaFS->pHeader->blk_bitmap + 1024
			+ grasaFS->pHeader->size_bitmap;

	while (bitarray_test_bit(grasaFS->pBitmap, posBitmap)) {
		posBitmap++;
	}

	bitarray_set_bit(grasaFS->pBitmap, posBitmap);

	blkDirect[nroBlk] = posBitmap;
	char *bloque_dato = (char *) grasaFS->disco->mem
			+ blkDirect[nroBlk] * BLOCK_SIZE;
	bzero(bloque_dato, BLOCK_SIZE);

}

//----------------------------------------------------------------------------------------------------------

void disponerBloqueDirecto(int blkDirecto, ptrGBloque* blkDirect) {
	bitarray_clean_bit(grasaFS->pBitmap, blkDirect[blkDirecto]);
	char *bloque_dato = (char *) grasaFS->disco->mem
			+ blkDirect[blkDirecto] * BLOCK_SIZE;
	bzero(bloque_dato, BLOCK_SIZE);
	blkDirect[blkDirecto] = 0;
}

//----------------------------------------------------------------------------------------------------------

void disponerBloqueIndirecto(int nroNodo, int nroBlkIndirecto) {

	bitarray_clean_bit(grasaFS->pBitmap,
			grasaFS->nodos[nroNodo].blk_indirect[nroBlkIndirecto]); //Borra Array[1024]
	grasaFS->nodos[nroNodo].blk_indirect[nroBlkIndirecto] = 0;
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

int directorioVacio(int blkDirectorio) {

	int ret = 1;
	int i;

	for (i = 0; i < 1024; i++) {
		if (grasaFS->nodos[i].parent_dir_block == blkDirectorio) {
			ret = 0;
			break;
		}
	}

	return ret;
}

//----------------------------------------------------------------------------------------------------------

int buscarNodoDisponible() {
	int i;
	for (i = 1; i < 1024; i++) {
		if (grasaFS->nodos[i].state == BORRADO) {
			return i;
		}
	}
	return -1;
}

//----------------------------------------------------------------------------------------------------------

void disponerNodo(int blkDirectorio) {
	bzero(grasaFS->nodos[blkDirectorio].filename, 71);
	grasaFS->nodos[blkDirectorio].state = BORRADO;
	grasaFS->nodos[blkDirectorio].parent_dir_block = 0;
}

//----------------------------------------------------------------------------------------------------------

ptrGBloque* seek(int nroNodo, int nroBLkIndirecto) {
	return (ptrGBloque*) (grasaFS->disco->mem
			+ grasaFS->nodos[nroNodo].blk_indirect[nroBLkIndirecto] * BLOCK_SIZE);
}

//-------------------------------------------------------------------------------------------------
// FUSE
//-------------------------------------------------------------------------------------------------

static int grasa_getattr(const char *path, struct stat *stbuf) {

	log_debug(logFile, "Ejecuntando gtattr");
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));

	//Si path es igual a "/" nos estan pidiendo los atributos del punto de montaje
	if (strcmp(path, DIRECTORIO_RAIZ) == 0) {
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_nlink = 2;
	} else {
		pthread_mutex_lock(&mutex);
		int pos_archivo = rutaToNumberBlock(path);

		if (pos_archivo != -1) {
			if (grasaFS->nodos[pos_archivo].state == ARCHIVO) {
				stbuf->st_mode = S_IFREG | 0666;
				stbuf->st_nlink = 1;
				stbuf->st_size = grasaFS->nodos[pos_archivo].file_size;
			} else if (grasaFS->nodos[pos_archivo].state == DIRECTORIO) {
				stbuf->st_mode = S_IFDIR | 0777;
				stbuf->st_nlink = 1;
			}
		} else {
			res = -ENOENT;
		}
		pthread_mutex_unlock(&mutex);
	}

	return res;
}

//-------------------------------------------------------------------------------------------------

static int grasa_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi) {

	log_debug(logFile, "Ejecuntando grasa_readdir");
	(void) offset;
	(void) fi;

	int i;
	int blkDirPadre = rutaToNumberBlock(path);
	if (blkDirPadre == -1) {
		return -ENOENT;
	}

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	pthread_mutex_lock(&mutex);

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

	pthread_mutex_unlock(&mutex);

	return 0;
}

//-------------------------------------------------------------------------------------------------

static int grasa_open(const char *path, struct fuse_file_info *fi) {

	log_debug(logFile, "Ejecuntando open");

	pthread_mutex_lock(&mutex);
	int blkDir = rutaToNumberBlock(path);
	pthread_mutex_unlock(&mutex);

	if (blkDir == -1)
		return -ENOENT;

	return 0;
}
//-------------------------------------------------------------------------------------------------

static int grasa_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi) {
	log_debug(logFile, "Ejecuntando read");
//	TODO: Refactorizar esta mierda
	size_t len;
	(void) fi;

	pthread_mutex_lock(&mutex);
	int blkDir = rutaToNumberBlock(path);
	pthread_mutex_unlock(&mutex);

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

		pthread_mutex_lock(&mutex);
		ptrGBloque *blk_direct = seek(blkDir, blkIndirecto);
		pthread_mutex_unlock(&mutex);

		bzero(buf, size); //Limpio el buffer
		int bytes_leidos = 0;

		if ((BLOCK_SIZE - offsetBlk) >= size) {  // Si entra en el prime bloque

			pthread_mutex_lock(&mutex);
			memcpy(buf,
					(char *) (grasaFS->disco->mem
							+ blk_direct[blkDirecto] * BLOCK_SIZE + offsetBlk),
					size);
			pthread_mutex_unlock(&mutex);
			bytes_leidos = size;
		} else {
			pthread_mutex_lock(&mutex);
			memcpy(buf,
					(char *) (grasaFS->disco->mem
							+ blk_direct[blkDirecto] * BLOCK_SIZE + offsetBlk),
					BLOCK_SIZE - offsetBlk); // Copia lo que puesdas
			pthread_mutex_unlock(&mutex);
			bytes_leidos += (BLOCK_SIZE - offsetBlk);
			size -= (BLOCK_SIZE - offsetBlk);
			blkDirecto++;

			if (blkDirecto == 1024) { // voy al proximo puntero indirecto y me coloco al principio

				blkDirecto = 0;
				blkIndirecto++;

				pthread_mutex_lock(&mutex);
				blk_direct = seek(blkDir, blkDirecto);
				pthread_mutex_unlock(&mutex);

			}

			int cant_bloques_por_leer = size / BLOCK_SIZE;
			int bytes_por_leer = size % BLOCK_SIZE;

			if (cant_bloques_por_leer == 0) {

				pthread_mutex_lock(&mutex);
				memcpy(buf + bytes_leidos,
						(char *) (grasaFS->disco->mem
								+ blk_direct[blkDirecto] * BLOCK_SIZE),
						bytes_por_leer);
				pthread_mutex_unlock(&mutex);

				bytes_leidos += bytes_por_leer;
				size -= bytes_por_leer;
			} else {
				int k;
				for (k = 1; k <= cant_bloques_por_leer; k++) {

					pthread_mutex_lock(&mutex);
					memcpy(buf + bytes_leidos,
							(char *) (grasaFS->disco->mem
									+ (blk_direct[blkDirecto] * BLOCK_SIZE)),
							BLOCK_SIZE);
					pthread_mutex_unlock(&mutex);

					bytes_leidos += BLOCK_SIZE;
					size -= BLOCK_SIZE;
					++blkDirecto;

					if (blkDirecto == 1024) {
						blkDirecto = 0;
						blkIndirecto++;

						pthread_mutex_lock(&mutex);
						blk_direct = seek(blkDir, blkIndirecto);
						pthread_mutex_unlock(&mutex);
					}
				}

				if (bytes_por_leer > 0) {

					pthread_mutex_lock(&mutex);
					memcpy(buf + bytes_leidos,
							(char *) (grasaFS->disco->mem
									+ (blk_direct[blkDirecto] * BLOCK_SIZE)),
							bytes_por_leer);
					pthread_mutex_unlock(&mutex);

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

	log_debug(logFile, "Ejecuntando mkdir");

	mode = S_IFDIR | 0777;

	pthread_mutex_lock(&mutex);
	int nroNodo = buscarNodoDisponible();
	pthread_mutex_unlock(&mutex);

	if (nroNodo != -1) {
		strcpy((char*) grasaFS->nodos[nroNodo].filename,
				strrchr(path, '/') + 1);
		grasaFS->nodos[nroNodo].state = DIRECTORIO;
		grasaFS->nodos[nroNodo].parent_dir_block = padreRutaToNumberBlock(path);
		return 0;
	} else {
		return -ENOENT;
	}

}

//-------------------------------------------------------------------------------------------------

static int grasa_create(const char *path, mode_t mode,
		struct fuse_file_info *fi) {

	log_debug(logFile, "Ejecuntando create");

	int retval = 0, i = 0, nroNodo;

	pthread_mutex_lock(&mutex);
	nroNodo = buscarNodoDisponible();
	pthread_mutex_unlock(&mutex);
//	mode = S_IFREG | 0444;

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

	log_debug(logFile, "Ejecuntando rmdir");

	int blkDirectorio = rutaToNumberBlock(path);

	if (blkDirectorio == -1) {
		return -ENOENT;
	}

	if (directorioVacio(blkDirectorio)) {
		disponerNodo(blkDirectorio);
	}

	return 0;

}

//-------------------------------------------------------------------------------------------------

static int grasa_truncate(const char *path, off_t offset) {

	log_debug(logFile, "Ejecuntando truncate");

	pthread_mutex_lock(&mutex);
	int blkDirectorio = rutaToNumberBlock(path);
	pthread_mutex_unlock(&mutex);

	int nroBlk, blkIndirecto, blkDirecto, offsetBlk;

	if (offset < grasaFS->nodos[blkDirectorio].file_size) { //Achico el archivo
		nroBlk = offset / BLOCK_SIZE;
		offsetBlk = offset % BLOCK_SIZE;
		blkIndirecto = nroBlk / 1024;
		blkDirecto = nroBlk % 1024;

		if (offset == 0) {

			while ((grasaFS->nodos[blkDirecto].blk_indirect[blkIndirecto] != 0)
					&& (blkIndirecto < 1000)) {

				pthread_mutex_lock(&mutex);
				ptrGBloque *blkDirect = seek(blkDirectorio, blkIndirecto);
				pthread_mutex_unlock(&mutex);

				while ((blkDirect[blkDirecto] != 0) && (blkDirecto < 1024)) {
					pthread_mutex_lock(&mutex);
					disponerBloqueDirecto(blkDirecto, blkDirect);
					pthread_mutex_unlock(&mutex);
					++blkDirecto;
				}

				pthread_mutex_lock(&mutex);
				disponerBloqueIndirecto(blkDirectorio, blkIndirecto);
				pthread_mutex_unlock(&mutex);

				++blkIndirecto;
			}

			pthread_mutex_lock(&mutex);
			grasaFS->nodos[blkDirectorio].file_size = 0;
			pthread_mutex_unlock(&mutex);

			return 0;
		}

		if ((nroBlk == 0) && (offsetBlk > 0)) //Menos de un bloque
				{
			while ((grasaFS->nodos[blkDirectorio].blk_indirect[blkIndirecto]
					!= 0) && (blkIndirecto < 1000)) {

				pthread_mutex_lock(&mutex);
				ptrGBloque *blkDirect = seek(blkDirectorio, blkIndirecto);
				pthread_mutex_unlock(&mutex);

				++blkDirecto;

				while ((blkDirect[blkDirecto] != 0) && (blkDirecto < 1024)) {
					pthread_mutex_lock(&mutex);
					disponerBloqueDirecto(blkDirecto, blkDirect);
					pthread_mutex_unlock(&mutex);
					++blkDirecto;
				}
				if (blkIndirecto > 0) {
					pthread_mutex_lock(&mutex);
					disponerBloqueIndirecto(blkDirectorio, blkIndirecto);
					pthread_mutex_unlock(&mutex);
				}

				++blkIndirecto;
			}

			return 0;
		}

		if (nroBlk > 0) { //N Bloques enteros y un poco mas

			if (offsetBlk > 0) //Valido para pasar al siguiente
					{
				++blkDirecto;
				if (blkDirecto == 1024) {
					++blkIndirecto;
					if (blkIndirecto == 1000) {
						return -1;
					}
					blkDirecto = 0;
				}
			}

			while ((grasaFS->nodos[blkDirectorio].blk_indirect[blkIndirecto]
					!= 0) && (blkIndirecto < 1000)) {

				pthread_mutex_lock(&mutex);
				ptrGBloque *blkDirect = seek(blkDirecto, blkIndirecto);
				pthread_mutex_unlock(&mutex);

				while ((blkDirect[blkDirecto] != 0) && (blkDirecto < 1024)) {
					pthread_mutex_lock(&mutex);
					disponerBloqueDirecto(blkDirecto, blkDirect);
					pthread_mutex_unlock(&mutex);

					++blkDirecto;
				}
				if (blkDirect[0] == 0) //no borro el array[1024]
						{
					pthread_mutex_lock(&mutex);
					disponerBloqueIndirecto(blkDirectorio, blkIndirecto);
					pthread_mutex_unlock(&mutex);
				}
				++blkIndirecto;
			}

			return 0;
		}

	} else {	//Agrando el archivo

		pthread_mutex_lock(&mutex);

		nroBlk = (grasaFS->nodos[blkDirectorio].file_size / BLOCK_SIZE);
		offsetBlk = (grasaFS->nodos[blkDirectorio].file_size % BLOCK_SIZE);
		blkIndirecto = nroBlk / 1024;
		blkDirecto = nroBlk % 1024;

		pthread_mutex_unlock(&mutex);

		if (grasaFS->nodos[blkDirectorio].file_size == 0) //el archivo es nuevo
				{
			int bytes_por_reservar = offset;
			int bytes_reservados = 0;
			pthread_mutex_lock(&mutex);
			reservarBloqueDirecto(blkDirectorio, blkIndirecto); //Array[1024]
			ptrGBloque *blk_direct = seek(blkDirectorio, blkIndirecto);
			pthread_mutex_unlock(&mutex);

			while (bytes_por_reservar > bytes_reservados) {
				pthread_mutex_lock(&mutex);
				reservarBloqueDatos(blk_direct, blkDirecto);
				pthread_mutex_unlock(&mutex);
//				TOOD: Ver esto
				bytes_reservados += BLOCK_SIZE;
				bytes_por_reservar -= bytes_reservados;
				++blkDirecto;
				if (blkDirecto == 1024) {
					++blkIndirecto;
					if (blkIndirecto < 1000) {
						pthread_mutex_lock(&mutex);
						reservarBloqueDirecto(blkDirectorio, blkIndirecto);
						blk_direct = seek(blkDirectorio, blkIndirecto);
						pthread_mutex_unlock(&mutex);
						blkDirecto = 0;
					} else //No entra en el nodo
					{
						return -1;
					}
				}
			}

			return 0;
		}

		else //El archivo ya tiene datos grabados
		{
			int bytes_por_reservar = offset
					- grasaFS->nodos[blkDirectorio].file_size;
			int bytes_reservados = 0;

			while (bytes_por_reservar > bytes_reservados) {
				if (grasaFS->nodos[blkDirectorio].blk_indirect[blkIndirecto]
						== 0) {
					pthread_mutex_lock(&mutex);
					reservarBloqueDirecto(blkDirectorio, blkIndirecto);
					pthread_mutex_unlock(&mutex);
					blkIndirecto = 0;
				}

				pthread_mutex_lock(&mutex);
				ptrGBloque *blk_direct = seek(blkDirectorio, blkIndirecto);
				pthread_mutex_unlock(&mutex);

				if (blk_direct[blkDirecto] == 0) {
					pthread_mutex_lock(&mutex);
					reservarBloqueDatos(blk_direct, blkDirecto);
					pthread_mutex_unlock(&mutex);
					bytes_reservados += BLOCK_SIZE;
					bytes_por_reservar -= bytes_reservados;
				}

				++blkDirecto;

				if (blkDirecto == 1024) {
					++blkIndirecto;
					if (blkIndirecto < 1000) {
						pthread_mutex_lock(&mutex);
						reservarBloqueDirecto(blkDirectorio, blkIndirecto);
						blk_direct = seek(blkDirectorio, blkIndirecto);
						pthread_mutex_unlock(&mutex);
						blkDirecto = 0;
					} else {
						return -1;
					}
				} else {
					bytes_reservados += (offset - bytes_por_reservar);
					bytes_por_reservar -= bytes_reservados;
				}

			}
		}

		return 0;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------------

static int grasa_unlink(const char* path) {

	log_debug(logFile, "Ejecuntando unlink");

	pthread_mutex_lock(&mutex);
	int nroNodoFile = rutaToNumberBlock(path);
	pthread_mutex_unlock(&mutex);

	if (nroNodoFile == -1) {
		return -ENOENT;
	}

	grasa_truncate(path, 0); // OJO semaforos adentro!!!!

	pthread_mutex_lock(&mutex);
	disponerNodo(nroNodoFile);
	pthread_mutex_unlock(&mutex);

	return 0;

}

//-------------------------------------------------------------------------------------------------

static int grasa_write(const char* path, const char* buf, size_t size,
		off_t offset, struct fuse_file_info *fi) {

	log_debug(logFile, "Ejecuntando write");

	pthread_mutex_lock(&mutex);
	int nroNodoFile = rutaToNumberBlock(path);
	pthread_mutex_unlock(&mutex);

	int nroBlk, blkIndirecto, blkDirecto, offsetBlk;
	int long bytes_escritos = 0;

	grasa_truncate(path, size + offset); // OJO mutex adentro

	nroBlk = offset / BLOCK_SIZE;
	offsetBlk = offset % BLOCK_SIZE;
	blkIndirecto = nroBlk / 1024;
	blkDirecto = nroBlk % 1024;

	pthread_mutex_lock(&mutex);
	ptrGBloque* blk_direct = seek(nroNodoFile, blkIndirecto);
	pthread_mutex_unlock(&mutex);

	if (BLOCK_SIZE - offsetBlk >= size) {

		pthread_mutex_lock(&mutex);
		memcpy(
				(char *) (grasaFS->disco->mem
						+ blk_direct[blkDirecto] * BLOCK_SIZE + offsetBlk), buf,
				size);
		pthread_mutex_unlock(&mutex);
		bytes_escritos += size;
		size -= bytes_escritos;
		pthread_mutex_lock(&mutex);
		grasaFS->nodos[nroNodoFile].file_size += bytes_escritos;
		pthread_mutex_unlock(&mutex);
	} else {
		pthread_mutex_lock(&mutex);
		memcpy(
				(char *) (grasaFS->disco->mem
						+ blk_direct[blkDirecto] * BLOCK_SIZE + offsetBlk), buf,
				BLOCK_SIZE - offsetBlk);
		pthread_mutex_unlock(&mutex);
		bytes_escritos += (BLOCK_SIZE - offsetBlk);
		size -= bytes_escritos;
		pthread_mutex_lock(&mutex);
		grasaFS->nodos[nroNodoFile].file_size += bytes_escritos;
		pthread_mutex_unlock(&mutex);
		blkDirecto++;

		if (blkDirecto == 1024) {
			blkDirecto = 0;
			blkIndirecto++;

			pthread_mutex_lock(&mutex);
			blk_direct = seek(nroNodoFile, blkIndirecto);
			pthread_mutex_unlock(&mutex);
		}

		int cant_bloques_por_escribir = size / BLOCK_SIZE;
		int bytes_por_escribir = size % BLOCK_SIZE;

		if (cant_bloques_por_escribir == 0) {
			pthread_mutex_lock(&mutex);
			memcpy(
					(char *) (grasaFS->disco->mem
							+ blk_direct[blkDirecto] * BLOCK_SIZE),
					buf + bytes_escritos, bytes_por_escribir);
			pthread_mutex_unlock(&mutex);
			bytes_escritos += bytes_por_escribir;
			size -= bytes_por_escribir;
			pthread_mutex_lock(&mutex);
			grasaFS->nodos[nroNodoFile].file_size += bytes_escritos;
			pthread_mutex_unlock(&mutex);
		} else {
			int k;
			for (k = 0; k < cant_bloques_por_escribir; k++) {
				pthread_mutex_lock(&mutex);
				memcpy(
						(char *) (grasaFS->disco->mem
								+ (blk_direct[blkDirecto] * BLOCK_SIZE)),
						buf + bytes_escritos, BLOCK_SIZE);
				pthread_mutex_unlock(&mutex);
				bytes_escritos += BLOCK_SIZE;
				size -= BLOCK_SIZE;
				pthread_mutex_lock(&mutex);
				grasaFS->nodos[nroNodoFile].file_size += bytes_escritos;
				pthread_mutex_unlock(&mutex);
				++blkDirecto;

				if (blkDirecto == 1024) {
					blkDirecto = 0;
					blkIndirecto++;
					pthread_mutex_lock(&mutex);
					blk_direct = seek(nroNodoFile, blkIndirecto);
					pthread_mutex_unlock(&mutex);
				}
			}

			if (bytes_por_escribir > 0) {
				pthread_mutex_lock(&mutex);
				memcpy(
						(char *) (grasaFS->disco->mem
								+ (blk_direct[blkDirecto] * BLOCK_SIZE)),
						buf + bytes_escritos, bytes_por_escribir);
				pthread_mutex_unlock(&mutex);
				bytes_escritos += bytes_por_escribir;
				size -= bytes_por_escribir;
				pthread_mutex_lock(&mutex);
				grasaFS->nodos[nroNodoFile].file_size += bytes_escritos;
				pthread_mutex_unlock(&mutex);
			}
		}
	}

	return bytes_escritos;

}

//-------------------------------------------------------------------------------------------------

static int grasa_utimens(const char *path, const struct timespec tv[2]) {

	log_debug(logFile, "Ejecuntando utimens");
	int retval = 0;
	pthread_mutex_lock(&mutex);
	int nroNodoFile = rutaToNumberBlock(path);
	pthread_mutex_unlock(&mutex);

	if (nroNodoFile == -1)
		retval = -ENOENT;
	else {
		pthread_mutex_lock(&mutex);
		grasaFS->nodos[nroNodoFile].modified = tv[1].tv_nsec;
		pthread_mutex_unlock(&mutex);
	}

	return retval;
}

static struct fuse_operations grasa_oper = { .getattr = grasa_getattr,
		.readdir = grasa_readdir, .mkdir = grasa_mkdir, .create = grasa_create,
		.rmdir = grasa_rmdir, .unlink = grasa_unlink, .open = grasa_open,
		.read = grasa_read, .truncate = grasa_truncate, .write = grasa_write,
		.utimens = grasa_utimens, };

enum {
	KEY_VERSION, KEY_HELP,
};

//static struct fuse_opt fuse_options[] = {
//		// Estos son parametros por defecto que ya tiene FUSE
//		FUSE_OPT_KEY("-V", KEY_VERSION),
//		FUSE_OPT_KEY("--version", KEY_VERSION),
//		FUSE_OPT_KEY("-h", KEY_HELP),
//		FUSE_OPT_KEY("--help", KEY_HELP),
//		FUSE_OPT_END,
//};

//-------------------------------------------------------------------------------------------------

int main(int argc, char* argv[]) {
	getConfiguracion();

	inicializarLog();

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	fileSystemCrear();

	return fuse_main(args.argc, args.argv, &grasa_oper, NULL);

}

