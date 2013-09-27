/*
 * deadlock_detection.h
 *
 *  Created on: 03/06/2013
 *      Author: utnso
 */

#ifndef DEADLOCK_DETECTION_H_
#define DEADLOCK_DETECTION_H_
//------------------------------------------------------------------------------------------------
// 	INCLUDES
//------------------------------------------------------------------------------------------------
#include "collections/list.h"
#include "nivel-gui.h"
//------------------------------------------------------------------------------------------------
// 	ESTRUCTURAS
//------------------------------------------------------------------------------------------------
struct dl_fila{
	char process_id;
	struct t_list* columna;
};

typedef struct dl_fila t_dl_fila;

struct dl_celda {
	char process_id;
	char resources_id;
	int allocation_value;
	int request_value;
};
typedef struct dl_celda t_dl_celda;

struct dl_vector{
	char id;
	int value;
};
typedef struct dl_vector t_dl_vector;

//------------------------------------------------------------------------------------------------
// 	FIRMAS
//------------------------------------------------------------------------------------------------
t_dl_vector* CrearNodoVector(char id, int value);
t_dl_celda* CrearCelda(char id_r, char id_p, int alloc, int req);
t_dl_fila* CrearFila(char id, t_list* col);
t_list* cargarMatriz(ITEM_NIVEL* listaR, ITEM_NIVEL* listaP);
t_list* obtenerAllocation(t_list* mat, char p_id);
t_list* obtenerRequest(t_list* mat, char p_id);
t_list* sumarVectores(t_list* vec1, t_list* vec2);
int esMenorIgual(t_list* vec1, t_list* vec2);
int esCero(t_list* vec1);
void inicializarFinish(ITEM_NIVEL* listaP, t_list* mat, t_list* finish);
int sumarAsiganadosPorRecurso(t_list* mat, char id_r);
int obtenerFinishProceso(t_list* finish, char id_p);
void setFinishProceso(t_list* finish, char id_p, int  value);
t_list* deadlockDetection(ITEM_NIVEL* listaR, ITEM_NIVEL* listaP);
t_list* obtenerPersonajesEnDL(ITEM_NIVEL* listaR, ITEM_NIVEL* listaP);
#endif /* DEADLOCK_DETECTION_H_ */
