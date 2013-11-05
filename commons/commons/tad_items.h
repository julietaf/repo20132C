#ifndef TAD_ITEMS_H_
#define TAD_ITEMS_H_

#include "nivel-gui.h"
#include "sockets.h"
#include "collections/list.h"

void BorrarItem(t_list* i, char id);
void restarRecurso(t_list* i, char id);
void restarRecursos(t_list* ListaItems, char id, int cant);
void MoverPersonaje(t_list* i, char personaje, int x, int y);
void moverEnemigo(t_list* ListaItems, int idEnemigo, int x, int y);
void CrearPersonaje(t_list* i, char id, int x , int y, int vidas, int socket);
void CrearCaja(t_list* i, char id, int x , int y, int cant);
void CrearEnemigo(t_list* i, char id, int x, int y, int idEnemigo);
void CrearItem(t_list* i, char id, int x, int y, char tipo, int cant, int socket, int idEnemigo);
coordenada_t *obtenerCoordenadas(t_list* i, char id);
void matarPersonaje(t_list* i,t_list* j, char id);
void incrementarRecurso(t_list* i, char id, int cant);
int darRecursoPersonaje(t_list* i, t_list* j, char id, char objetoId);
int cantidadItem(t_list *ListaItems, char id);
void CrearAdquirido(ITEM_ADQ** ListaAdq, char id, char request, int cant);
void BorrarAdquirido(ITEM_ADQ** ListaAdq, char id);
ITEM_ADQ* getObjetosAdquiridos(t_list* listaP, char idP);
t_list* getObjetosAdquiridosSerializable(t_list* listaP, char idP);
void destroyItems(t_list* ListaItems);
void destroyAdquirido(ITEM_ADQ** ListaAdq);
#endif
