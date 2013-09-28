/*
 * deadlock_detection.c
 *
 *  Created on: 03/06/2013
 *      Author: utnso
 */

#include "deadlock_detection.h"
#include <stdlib.h>

t_dl_vector* CrearNodoVector(char id, int value) {
	t_dl_vector* temp;
	temp = malloc(sizeof(t_dl_vector));
	temp->id = id;
	temp->value = value;
	return temp;
}


t_dl_celda* CrearCelda(char id_r, char id_p, int alloc, int req) {
	t_dl_celda* temp;
	temp = malloc(sizeof(t_dl_celda));
	temp->process_id = id_p;
	temp->resources_id = id_r;
	temp->request_value = req;
	temp->allocation_value = alloc;
	return temp;
}


t_list* cargarMatriz(ITEM_NIVEL* listaR, ITEM_NIVEL* listaP) {

	ITEM_NIVEL* tempP = listaP;

	t_list* mat = list_create();

	while (tempP != NULL ) {
		ITEM_NIVEL* tempR = listaR;
		while (tempR != NULL ) {
			ITEM_ADQ* temp2 = tempP->objetosAdquiridos;
			while ((temp2 != NULL )&& (temp2->id != tempR->id)){
			temp2 = temp2->next;
		}
			if (temp2 == NULL ) {
				list_add(mat, CrearCelda(tempR->id, tempP->id, 0, 0));
			} else {
				list_add(mat,
						CrearCelda(tempR->id, tempP->id, temp2->quantity,
								temp2->actual_request));
			}
			tempR = tempR->next;

		}
		tempP = tempP->next;
	}
	return mat;
}

t_list* cargarAvaible(ITEM_NIVEL* listaR, t_list* mat) {
	ITEM_NIVEL* temp = listaR;
	t_list* avl = list_create();

	while (temp != NULL ) {

		if ((temp->item_type == RECURSO_ITEM_TYPE) && (temp != NULL )) {
			list_add(avl, CrearNodoVector(temp->id, temp->quantity));

		}
		temp = temp->next;
	}

	return avl;
}

t_list* obtenerAllocation(t_list* mat, char p_id) {

	t_list* temp;
	t_list* aloc_array;
	t_dl_celda* cel;
	bool _es_fila(t_dl_celda* celda) {
		return celda->process_id == p_id;
	}

	temp = list_filter(mat, (void*) _es_fila);
	aloc_array = list_create();

	while (!list_is_empty(temp)) {

		cel = list_remove(temp, 0);
		char r_id = cel->resources_id;
		int value = cel->allocation_value;
		list_add(aloc_array, CrearNodoVector(r_id, value));
	}

//	free(cel);
	list_destroy_and_destroy_elements(temp, (void *) free);
	return aloc_array;
}

t_list* obtenerRequest(t_list* mat, char p_id) {
	t_list* temp = NULL;
	t_list* aloc_array = NULL;
	t_dl_celda* cel;
	bool _es_fila(t_dl_celda* celda) {
		return celda->process_id == p_id;
	}
	temp = list_filter(mat, (void*) _es_fila);
	aloc_array = list_create();

	while (!list_is_empty(temp)) {
		cel = list_remove(temp, 0);
		char r_id = cel->resources_id;
		int value = cel->request_value;
		list_add(aloc_array, CrearNodoVector(r_id, value));
	}

	//free(cel);
	list_destroy_and_destroy_elements(temp, (void *) free);
	return aloc_array;
}

t_list* sumarVectores(t_list* vec1, t_list* vec2) {
	t_list* suma = list_create();
	t_dl_vector* sum1;
	t_dl_vector* sum2;

	int i;

	for (i = 0; i < list_size(vec1); i++) {

		sum1 = list_get(vec1, i);

		int j;
		for (j = 0; j < list_size(vec2); j++) {
			sum2 = list_get(vec2, j);
			if (sum2->id == sum1->id) {
				break;
			} else {
				sum2 = NULL;
			}

		}

		list_add(suma, CrearNodoVector(sum1->id, sum1->value + sum2->value));
	}

	free(sum1);
	free(sum2);

	return suma;
}

int esMenorIgual(t_list* vec1, t_list* vec2) {

	t_dl_vector* nodo1;
	t_dl_vector* nodo2;

	int i;
	int iguales = 1;

	for (i = 0; i < list_size(vec1); i++) {
		nodo1 = list_get(vec1, i);

		int j;
		for (j = 0; j < list_size(vec2); j++) {
			nodo2 = list_get(vec2, j);
			if (nodo1->id == nodo2->id) {
				if (nodo1->value > nodo2->value) {
					iguales = 0;// hay al menos uno que no cumple
				}
			}
		}
	}

	return iguales;// son todos <=
}

int esCero(t_list* vec1){

	t_dl_vector* nodo1;
	int i;
	for (i = 0; i < list_size(vec1); i++) {
		nodo1 = list_get(vec1, i);
		if (nodo1->value != 0) {
			return 0;
		}

	}
	return 1;
}

void inicializarFinish(ITEM_NIVEL* listaP, t_list* mat, t_list* finish){
	ITEM_NIVEL* temp = listaP;
	t_list* request;

	while (temp != NULL ) {

		if ((temp->item_type == PERSONAJE_ITEM_TYPE) && (temp != NULL )) {

			request = obtenerAllocation(mat, temp->id);
			if (esCero(request)) {
				list_add(finish, CrearNodoVector(temp->id, 1));
			} else {
				list_add(finish, CrearNodoVector(temp->id, 0));
			}
		}
		temp = temp->next;
	}

	list_destroy_and_destroy_elements(request, (void *) free);

}

int sumarAsiganadosPorRecurso(t_list* mat, char id_r){
	int suma;
	t_dl_celda* celda;
	int i;

	for (i = 0; i < mat->elements_count; i++) {

		celda = list_get(mat, i);
		if (celda->resources_id == id_r) {
			suma = suma + celda->allocation_value;
		}
	}

	return suma;

}

int obtenerFinishProceso(t_list* finish, char id_p){

	t_dl_vector* nodo;
	int i;
	for (i = 0; i < finish->elements_count; i++) {
		nodo = list_get(finish, i);
		if (nodo->id == id_p) {
			return nodo->value;
		}
	}
	return -1;
}

void setFinishProceso(t_list* finish, char id_p, int  value){

	t_dl_vector* nodo;
	int i;
	for (i = 0; i < finish->elements_count; i++) {
		nodo = list_get(finish, i);
		if (nodo->id == id_p) {
			nodo->value = value;
		}
	}
}

t_list* deadlockDetection(ITEM_NIVEL* listaR, ITEM_NIVEL* listaP){
	t_list* finish = NULL;
	t_list* request = NULL;
	t_list* avaible = NULL;
	t_list* work = NULL;
	t_list* allocation = NULL;
	t_list* matriz  = NULL;
	ITEM_NIVEL* tempP = listaP;

	finish = list_create();
	matriz = cargarMatriz(listaR, listaP);
	inicializarFinish(listaP, matriz, finish);
	avaible = cargarAvaible(listaR, matriz);

	work = avaible;

	while (tempP != NULL) {
		request = obtenerRequest(matriz, tempP->id);
		allocation = obtenerAllocation(matriz, tempP->id);
		int boolF = obtenerFinishProceso(finish, tempP->id) ;
		int boolM = esMenorIgual(request, work);
		if (!boolF && boolM) {
			work = sumarVectores(work, allocation);
			setFinishProceso(finish, tempP->id, 1);
		}
		tempP= tempP->next;
	}


//	list_destroy_and_destroy_elements(request, (void *) free);
	list_destroy(request);
	list_destroy_and_destroy_elements(avaible, (void *) free);
//	TODO: list_destroy_and_destroy_elements(work, (void *) free);
//	list_destroy_and_destroy_elements(allocation, (void *) free);
	list_destroy(allocation);
	list_destroy_and_destroy_elements(matriz, (void *) free);
	return finish;
}

t_list* obtenerPersonajesEnDL(ITEM_NIVEL* listaR, ITEM_NIVEL* listaP) {

	t_list* finalizados = NULL;
	t_list* bloqueados = list_create();
	t_dl_vector* personaje;
	int i;
	finalizados = deadlockDetection(listaR, listaP);

	for (i = 0; i < finalizados->elements_count; i++) {
		personaje = list_get(finalizados, i);

		if (!(personaje->value)) {
			list_add(bloqueados, CrearNodoVector(personaje->id, personaje->value));
		}
	}

	list_destroy_and_destroy_elements(finalizados, (void *) free);
	return bloqueados;
}




