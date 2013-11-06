//----------------------------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------------------------
#include "tad_items.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

//----------------------------------------------------------------------------------------------
// FUNCIONES PORPIETARIAS
//----------------------------------------------------------------------------------------------
ITEM_NIVEL* _search_item_by_id(t_list* items, char id);
ITEM_NIVEL* _search_item_by_id_enemigo(t_list* items, int id);

void CrearItem(t_list* ListaItems, char id, int x, int y, char tipo, int cant,
		int socket, int idEnemigo) {
	ITEM_NIVEL * temp = malloc(sizeof(ITEM_NIVEL));

	temp->id = id;
	temp->posx = x;
	temp->posy = y;
	temp->item_type = tipo;
	temp->quantity = cant; //vidas en el caso del personaje
	temp->objetosAdquiridos = NULL;
	temp->socket = socket;
	temp->idEnemigo = idEnemigo;

	list_add(ListaItems, temp);

}

void CrearAdquirido(ITEM_ADQ** ListaAdq, char id, char request, int cant) {
	ITEM_ADQ * temp;
	temp = malloc(sizeof(ITEM_ADQ));

	temp->id = id;
	temp->quantity = cant;
	temp->actual_request = request;
	temp->next = *ListaAdq;

	*ListaAdq = temp;
}

void CrearPersonaje(t_list* ListaItems, char id, int x, int y, int vidas,
		int socket) {
	CrearItem(ListaItems, id, x, y, PERSONAJE_ITEM_TYPE, vidas, socket, -1);
}

void CrearCaja(t_list* ListaItems, char id, int x, int y, int cant) {
	CrearItem(ListaItems, id, x, y, RECURSO_ITEM_TYPE, cant, -1, -1);
}

void CrearEnemigo(t_list* ListaItems, char id, int x, int y, int idEnemigo) {
	CrearItem(ListaItems, id, x, y, ENEMIGO_ITEM_TYPE, -1, -1, idEnemigo);
}

void BorrarItem(t_list* items, char id) {
	bool _search_by_id(ITEM_NIVEL* item) {
		return item->id == id;
	}

	list_remove_by_condition(items, (void*) _search_by_id);

}

void BorrarAdquirido(ITEM_ADQ** ListaAdq, char id) {
	ITEM_ADQ * temp = *ListaAdq;
	ITEM_ADQ * oldtemp;

	if ((temp != NULL )&& (temp->id == id)){
	*ListaAdq = (*ListaAdq)->next;
	free(temp);
} else {
	while((temp != NULL) && (temp->id != id)) {
		oldtemp = temp;
		temp = temp->next;
	}
	if ((temp != NULL) && (temp->id == id)) {
		oldtemp->next = temp->next;
		free(temp);
	}
}
}

void MoverPersonaje(t_list* items, char id, int x, int y) {

	ITEM_NIVEL* item = _search_item_by_id(items, id);

	if (item != NULL ) {
		item->posx = x;
		item->posy = y;
	} else {
		printf("WARN: Item %c no existente\n", id);
	}

}

void moverEnemigo(t_list* ListaItems, int idEnemigo, int x, int y) {

	ITEM_NIVEL * temp = _search_item_by_id_enemigo(ListaItems, idEnemigo);


	if ((temp != NULL )&& (temp->idEnemigo == idEnemigo)){
	temp->posx = x;
	temp->posy = y;
}

}

void restarRecurso(t_list* items, char id) {

	ITEM_NIVEL* item = _search_item_by_id(items, id);

	if (item != NULL ) {
		item->quantity = item->quantity > 0 ? item->quantity - 1 : 0;
	} else {
		printf("WARN: Item %c no existente\n", id);
	}

}

void restarRecursos(t_list* items, char id, int cant) {

	ITEM_NIVEL* item = _search_item_by_id(items, id);

	if (item != NULL ) {
		item->quantity = item->quantity > cant ? item->quantity - cant : 0;
	} else {
		printf("WARN: Item %c no existente\n", id);
	}

}

coordenada_t* obtenerCoordenadas(t_list* items, char id) {
	coordenada_t* resul = NULL;

	ITEM_NIVEL* item = _search_item_by_id(items, id);

	if (item != NULL ) {
		resul = malloc(sizeof(coordenada_t));
		resul->ejeX = item->posx;
		resul->ejeY = item->posy;
	} else {
		printf("WARN: Item %c no existente\n", id);
	}

	return resul;
}

void matarPersonaje(t_list* personajes, t_list* recursos, char id) {
	//encuentro personaje
	ITEM_NIVEL* personaje = _search_item_by_id(personajes, id);

	//Libero intems
	if ((personaje != NULL )&& (personaje->item_type==PERSONAJE_ITEM_TYPE)){
	while (personaje->objetosAdquiridos != NULL) {
		incrementarRecurso(recursos, personaje->objetosAdquiridos->id, personaje->objetosAdquiridos->quantity);

		BorrarAdquirido(&personaje->objetosAdquiridos, personaje->objetosAdquiridos->id);
	}
//	//Cierro el socket
//	close(temp->socket);
//	//Borro Personaje
	BorrarItem(personajes, id);
}

}

int darRecursoPersonaje(t_list* personajes, t_list* recursos, char id,
		char objetoId) {

	ITEM_NIVEL * personaje = _search_item_by_id(personajes, id);
	int respuesta;

	if ((personaje != NULL )&& (personaje->id == id) && (personaje->item_type==PERSONAJE_ITEM_TYPE)){
	ITEM_ADQ* temp2 = (personaje->objetosAdquiridos);

	while (temp2 != NULL && temp2->id != objetoId) { //Busco Recurso en el personaje
		temp2 = temp2->next;
	}
	if (temp2 == NULL) { //si no lo encontre lo agrego si no lo incremento
		//Lo Creo
		if (cantidadItem(recursos, objetoId)>0) {//valido que haya recursos disponibles para dar
			CrearAdquirido(&personaje->objetosAdquiridos,objetoId,0,1);
			respuesta = 1;
		} else {		// si no hay seteo flag peticion actura (request)
			CrearAdquirido(&personaje->objetosAdquiridos,objetoId,1,0);
			respuesta = 0;
		}
	} else {
		//Incremento
		if (cantidadItem(recursos, objetoId)>0) {
			temp2->quantity = temp2->quantity+1;
			temp2->actual_request = 0;
			respuesta = 1;
		} else {
			temp2->actual_request = 1;
			respuesta = 0;
		}

	}
	restarRecurso(recursos, objetoId);
	//printf("Cantidad recursos: %d\n", temp2->quantity);

}
	return respuesta;

}

void incrementarRecurso(t_list* items, char id, int cant) {

	ITEM_NIVEL* item = _search_item_by_id(items, id);

	if (item != NULL ) {
		item->quantity = item->quantity + cant;
	} else {
		printf("WARN: Item %c no existente\n", id);
	}

}

int cantidadItem(t_list *items, char id) {

	int resul;
	ITEM_NIVEL* item = _search_item_by_id(items, id);

	if ((item != NULL )&& (item->id == id)){
	resul = item->quantity;
	return resul;
}
	return resul = -1;
}

ITEM_ADQ* getObjetosAdquiridos(t_list* personajes, char idP) {
	ITEM_ADQ* adquiridos = NULL;
	ITEM_NIVEL* personaje = _search_item_by_id(personajes, idP);

	if ((personaje != NULL )&& (personaje->id == idP)){
	adquiridos = personaje->objetosAdquiridos;
}

	return adquiridos;
}

t_list* getObjetosAdquiridosSerializable(t_list* listaP, char idP) {

	ITEM_ADQ* tempAdquiridos = NULL;
	t_list* adquiridos = list_create();
	ITEM_NIVEL* personaje = _search_item_by_id(listaP, idP);

	if ((personaje != NULL )&& (personaje->id == idP)){
		tempAdquiridos = personaje->objetosAdquiridos;
		while (tempAdquiridos != NULL ) {
			list_add(adquiridos, crearNodoRecurso(tempAdquiridos->id, tempAdquiridos->quantity));
			tempAdquiridos = tempAdquiridos->next;
		}
	}

	return adquiridos;

//	t_list* adquiridos = NULL;
//	ITEM_NIVEL* tempP = NULL;
//	adquiridos = list_create();
//	tempP = listaP;
//	while (tempP != NULL ) {
//		if (tempP->id == idP) {
//			ITEM_ADQ* tempA = NULL;
//			tempA = tempP->objetosAdquiridos;
//			while (tempA != NULL ) {
////				listaRecursos_add(&adquiridos, tempA->id,tempA->quantity);
//				list_add(adquiridos,
//						crearNodoRecurso(tempA->id, tempA->quantity));
//				tempA = tempA->next;
//
//			}
//		}
//		tempP = tempP->next;
//	}
//
//	return adquiridos;
}

void destroyItems(t_list* ListaItems) {

	list_destroy_and_destroy_elements(ListaItems, (void*)free);

}

void destroyAdquirido(ITEM_ADQ** ListaAdq) {

	while (*ListaAdq != NULL ) {
		ITEM_ADQ * temp = *ListaAdq;
		*ListaAdq = (*ListaAdq)->next;
		free(temp);
	}
}

ITEM_NIVEL* _search_item_by_id(t_list* items, char id) {
	bool _search_by_id(ITEM_NIVEL* item) {
		return item->id == id;
	}

	return list_find(items, (void*) _search_by_id);
}

ITEM_NIVEL* _search_item_by_id_enemigo(t_list* items, int id) {
	bool _search_by_id(ITEM_NIVEL* item) {
		return item->idEnemigo == id;
	}

	return list_find(items, (void*) _search_by_id);
}
