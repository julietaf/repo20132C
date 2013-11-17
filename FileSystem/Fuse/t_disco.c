#include "t_disco.h"

t_disco* disco_crear(char* path, t_log* log) {

	t_disco* this = (t_disco*) malloc(sizeof(t_disco));

	this->fd  = open(path,O_RDWR);
	this->log = log;

	struct stat estadistica;
	fstat(this->fd,&estadistica);
	this->tamanio = estadistica.st_size;
	this->endBlock = this->tamanio / sizeof(GFile);

	this->mem = mmap(NULL, this->tamanio, PROT_READ | PROT_WRITE, MAP_SHARED, this->fd, 0);

	if (this->mem == MAP_FAILED)
			return NULL;

	this->curr_block = (pBloque) &this->mem[0];


	log_info(this->log,"disco: [%s] tam: [%d]",path,this->tamanio);

	return this;
}

void disco_destroy(t_disco* this) {

	log_info(this->log,"fin lectura disco");

	munmap(this->mem,this->tamanio);
	close(this->fd);

	free(this);
}

void disco_no_desalojo_bloques(t_disco* this,size_t tam_no_desalojo) {
	posix_madvise(this->mem,tam_no_desalojo,POSIX_MADV_WILLNEED);
}

pBloque disco_seek(t_disco* this, ptrGBloque nBloque) {
	if (this->endBlock < nBloque)
		return NULL;

//	log_info(this->log,"disco_seek [%d]",nBloque);

	this->curr_block = this->curr_block + nBloque;

	return this->curr_block;
}

uint32_t _disco_bloque_to_index(ptrGBloque nBloque) {
	return nBloque*sizeof(GHeader);
}
