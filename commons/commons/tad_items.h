#ifndef TAD_ITEMS_H_
#define TAD_ITEMS_H_

#include "nivel-gui.h"
#include "sockets.h"
#include "collections/list.h"

void BorrarItem(ITEM_NIVEL** i, char id);
void restarRecurso(ITEM_NIVEL* i, char id);
void restarRecursos(ITEM_NIVEL* ListaItems, char id, int cant);
void MoverPersonaje(ITEM_NIVEL* i, char personaje, int x, int y);
void moverEnemigo(ITEM_NIVEL* ListaItems, int idEnemigo, int x, int y);
void CrearPersonaje(ITEM_NIVEL** i, char id, int x , int y, int vidas, int socket);
void CrearCaja(ITEM_NIVEL** i, char id, int x , int y, int cant);
void CrearEnemigo(ITEM_NIVEL** i, char id, int x, int y, int idEnemigo);
void CrearItem(ITEM_NIVEL** i, char id, int x, int y, char tipo, int cant, int socket, int idEnemigo);
coordenada_t *obtenerCoordenadas(ITEM_NIVEL* i, char id);
void matarPersonaje(ITEM_NIVEL** i,ITEM_NIVEL** j, char id);
void incrementarRecurso(ITEM_NIVEL* i, char id, int cant);
int darRecursoPersonaje(ITEM_NIVEL** i, ITEM_NIVEL** j, char id, char objetoId);
int cantidadItem(ITEM_NIVEL *ListaItems, char id);
void CrearAdquirido(ITEM_ADQ** ListaAdq, char id, char request, int cant);
void BorrarAdquirido(ITEM_ADQ** ListaAdq, char id);
ITEM_ADQ* getObjetosAdquiridos(ITEM_NIVEL* listaP, char idP);
t_list* getObjetosAdquiridosSerializable(ITEM_NIVEL* listaP, char idP);
void destroyItems(ITEM_NIVEL** ListaItems);
void destroyAdquirido(ITEM_ADQ** ListaAdq);
#endif
