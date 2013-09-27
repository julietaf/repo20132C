#define PERSONAJE_ITEM_TYPE 0
#define RECURSO_ITEM_TYPE 1

#ifndef NIVEL_GHI_H_
#define NIVEL_GHI_H_

struct elementos {
	char id;
	int posx;
	int posy;
	char item_type; // PERSONAJE o CAJA_DE_RECURSOS
	int quantity;
	struct adquiridos *objetosAdquiridos; // Estos campos solo son validos para item_type 0
	int socket;
	struct elementos *next;
};

struct adquiridos{
	char id;
	int quantity;
	int actual_request; //0 False 1 True
	struct adquiridos *next;
};

typedef struct adquiridos ITEM_ADQ;
typedef struct elementos ITEM_NIVEL;

int nivel_gui_dibujar(ITEM_NIVEL* items);
int nivel_gui_terminar(void);
int nivel_gui_inicializar(void);
int nivel_gui_get_area_nivel(int * rows, int * cols);

#endif /*NIVEL_GHI_H_*/

