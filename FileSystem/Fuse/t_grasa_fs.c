#include "t_grasa_fs.h"

t_grasa_fs* grasa_fs_crear(char* path)  {

	t_grasa_fs* this = (t_grasa_fs *) malloc(sizeof(t_grasa_fs));
	this->log = log_create("./FuseProc.log","GRASA",true,LOG_LEVEL_INFO);
	this->disco = disco_crear(path,this->log);

	this->pHeader = disco_seek(this->disco,0);
	this->pBitmap = bitarray_create((char*)disco_seek(this->disco,this->pHeader->blk_bitmap),(this->pHeader->size_bitmap)*BLOCK_SIZE);

	int i,j;
	for (i=0,j=this->pHeader->size_bitmap+1; i < GFILEBYTABLE; i++,j++)
		this->nodos[i] = (GFile *) disco_seek(this->disco,j);

	disco_no_desalojo_bloques(this->disco,BLOCK_SIZE + this->pHeader->size_bitmap*BLOCK_SIZE+GFILEBYTABLE*BLOCK_SIZE);

	return this;

}
void grasa_fs_destroy(t_grasa_fs* this) {

	log_info(this->log,"fin lectura fs");

	bitarray_destroy(this->pBitmap);

	disco_destroy(this->disco);

	log_destroy(this->log);
	free(this);

}
void grasa_fs_imprimir_estructuras(t_grasa_fs* this) {

	log_info(this->log,"[HEADER]");
	log_info(this->log,"identificador: [%s]",this->pHeader->grasa);
	log_info(this->log,"version: [%d]",this->pHeader->version);
	log_info(this->log,"bloque bitmap: [%d]",this->pHeader->blk_bitmap);
	log_info(this->log,"bloque tam_bitmap: [%d]",this->pHeader->size_bitmap);
	log_info(this->log,"tam_header: [%d]",BLOCK_SIZE);

	log_info(this->log,"[BITMAP]:");
	int i;
	char* buff = string_new();
	log_info(this->log,"tam_bitarray: [%d]",bitarray_get_max_bit(this->pBitmap));
	for (i=0; i < bitarray_get_max_bit(this->pBitmap); i++) {
		if (i % 150 == 0)
			string_append_with_format(&buff,"\n");
		string_append_with_format(&buff,"%d",bitarray_test_bit(this->pBitmap, i) == true ? 1: 0);

	}
	log_info(this->log,buff);

}
