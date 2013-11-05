
#ifndef NIVEL_GHI_H_
#define NIVEL_GHI_H_

#define PERSONAJE_ITEM_TYPE 0
#define RECURSO_ITEM_TYPE 1
#define ENEMIGO_ITEM_TYPE 2

#include "collections/list.h"

struct elementos {
	char id;
	int posx;
	int posy;
	char item_type; // PERSONAJE o CAJA_DE_RECURSOS
	int quantity;
	struct adquiridos *objetosAdquiridos; // Estos campos solo son validos para item_type 0
	int socket;
	int idEnemigo;
//	struct elementos *next;
};

struct adquiridos{
	char id;
	int quantity;
	int actual_request; //0 False 1 True
	struct adquiridos *next;
};

typedef struct adquiridos ITEM_ADQ;
typedef struct elementos ITEM_NIVEL;

int nivel_gui_dibujar(t_list* items, char* nombre_nivel);
int nivel_gui_terminar(void);
int nivel_gui_inicializar(void);
int nivel_gui_get_area_nivel(int * filas, int * columnas);

#endif /*NIVEL_GHI_H_*/

