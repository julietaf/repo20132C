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

void CrearItem(ITEM_NIVEL** ListaItems, char id, int x, int y, char tipo,
		int cant, int socket) {
	ITEM_NIVEL * temp;
	temp = malloc(sizeof(ITEM_NIVEL));

	temp->id = id;
	temp->posx = x;
	temp->posy = y;
	temp->item_type = tipo;
	temp->quantity = cant; //vidas en el caso del personaje
	temp->objetosAdquiridos = NULL;
	temp->socket = socket;
	temp->next = *ListaItems;
	*ListaItems = temp;
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

void CrearPersonaje(ITEM_NIVEL** ListaItems, char id, int x, int y, int vidas,
		int socket) {
	CrearItem(ListaItems, id, x, y, PERSONAJE_ITEM_TYPE, vidas, socket);
}

void CrearCaja(ITEM_NIVEL** ListaItems, char id, int x, int y, int cant) {
	CrearItem(ListaItems, id, x, y, RECURSO_ITEM_TYPE, cant, -1);
}

void BorrarItem(ITEM_NIVEL** ListaItems, char id) {
	ITEM_NIVEL * temp = *ListaItems;
	ITEM_NIVEL * oldtemp;

	if ((temp != NULL )&& (temp->id == id)){
	*ListaItems = (*ListaItems)->next;

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

void MoverPersonaje(ITEM_NIVEL* ListaItems, char id, int x, int y) {

	ITEM_NIVEL * temp;
	temp = ListaItems;

	while ((temp != NULL )&& (temp->id != id)){
	temp = temp->next;
}
	if ((temp != NULL )&& (temp->id == id)){
	temp->posx = x;
	temp->posy = y;
}

}

void restarRecurso(ITEM_NIVEL* ListaItems, char id) {

	ITEM_NIVEL * temp;
	temp = ListaItems;

	while ((temp != NULL )&& (temp->id != id)){
	temp = temp->next;
}
	if ((temp != NULL )&& (temp->id == id)){
	if ((temp->item_type) && (temp->quantity > 0)) {
		//temp->quantity = temp->quantity--;
		(*temp).quantity = temp->quantity-1;
	}
}

}

void restarRecursos(ITEM_NIVEL* ListaItems, char id, int cant) {

	ITEM_NIVEL * temp;
	temp = ListaItems;

	while ((temp != NULL )&& (temp->id != id)){
	temp = temp->next;
}
	if ((temp != NULL )&& (temp->id == id)){
	if (temp->item_type) {

		(*temp).quantity = temp->quantity-cant;
	}
}

}

coordenada_t* obtenerCoordenadas(ITEM_NIVEL* ListaItems, char id) {
	ITEM_NIVEL * temp = ListaItems;
	coordenada_t* resul = NULL;

	while ((temp != NULL )&& (temp->id != id)){
	temp = temp->next;
}
	if ((temp != NULL )&& (temp->id == id)){
	resul=malloc(sizeof(coordenada_t));
	resul->ejeX = temp->posx;
	resul->ejeY = temp->posy;
}
	return resul;
}

void matarPersonaje(ITEM_NIVEL** ListaP, ITEM_NIVEL** ListaR, char id) {
	//encuentro personaje
	ITEM_NIVEL * temp = *ListaP;
	while ((temp != NULL )&& (temp->id != id)){
	temp = temp->next;
}
	//Libero intems
	if ((temp != NULL )&& (temp->id == id) && (temp->item_type==PERSONAJE_ITEM_TYPE)){
	while (temp->objetosAdquiridos != NULL) {
		incrementarRecurso(*ListaR, temp->objetosAdquiridos->id, temp->objetosAdquiridos->quantity);

		BorrarAdquirido(&temp->objetosAdquiridos, temp->objetosAdquiridos->id);
	}
	//Cierro el socket
	close(temp->socket);
	//Borro Personaje
	BorrarItem(ListaP, id);
}

}

int darRecursoPersonaje(ITEM_NIVEL** ListaP, ITEM_NIVEL** ListaR, char id,
		char objetoId) {

	ITEM_NIVEL * tempP = *ListaP;
	int respuesta;

	while ((tempP != NULL )&& (tempP->id != id)){ //encuentro personaje
	tempP = tempP->next;
}

	if ((tempP != NULL )&& (tempP->id == id) && (tempP->item_type==0)){
	ITEM_ADQ* temp2 = (tempP->objetosAdquiridos);

	while (temp2 != NULL && temp2->id != objetoId) { //Busco Recurso en el personaje
		temp2 = temp2->next;
	}
	if (temp2 == NULL) { //si no lo encontre lo agrego si no lo incremento
		//Lo Creo
		if (cantidadItem(*ListaR, objetoId)>0) {//valido que haya recursos disponibles para dar
			CrearAdquirido(&tempP->objetosAdquiridos,objetoId,0,1);
			respuesta = 1;
		} else {// si no hay seteo flag peticion actura (request)
			CrearAdquirido(&tempP->objetosAdquiridos,objetoId,1,0);
			respuesta = 0;
		}
	} else {
		//Incremento
		if (cantidadItem(*ListaR, objetoId)>0) {
			temp2->quantity = temp2->quantity+1;
			temp2->actual_request = 0;
			respuesta = 1;
		} else {
			temp2->actual_request = 1;
			respuesta = 0;
		}

	}
	restarRecurso(*ListaR, objetoId);
	//printf("Cantidad recursos: %d\n", temp2->quantity);

}
	return respuesta;

}

void incrementarRecurso(ITEM_NIVEL* ListaItems, char id, int cant) {

	ITEM_NIVEL * temp;
	temp = ListaItems;

	while ((temp != NULL )&& (temp->id != id)){
	temp = temp->next;
}
	if ((temp != NULL )&& (temp->id == id)){
	if (temp->item_type) {

		(*temp).quantity = temp->quantity+cant;
	}
}

}

int cantidadItem(ITEM_NIVEL *ListaItems, char id) {
	ITEM_NIVEL * temp = ListaItems;
	int resul;

	while ((temp != NULL )&& (temp->id != id)){
	temp = temp->next;
}
	if ((temp != NULL )&& (temp->id == id)){
	resul = temp->quantity;
	return resul;
}
	return resul = -1;
}

ITEM_ADQ* getObjetosAdquiridos(ITEM_NIVEL* listaP, char idP) {
	ITEM_ADQ* adquiridos;
	ITEM_NIVEL* tempP = listaP;
	while (tempP != NULL ) {
		if (tempP->id == idP) {
			adquiridos = tempP->objetosAdquiridos;
			break;
		}
		tempP = tempP->next;
	}

	return adquiridos;
}

t_list* getObjetosAdquiridosSerializable(ITEM_NIVEL* listaP, char idP) {
	t_list* adquiridos = NULL;
	ITEM_NIVEL* tempP = NULL;
	adquiridos = list_create();
	tempP = listaP;
	while (tempP != NULL ) {
		if (tempP->id == idP) {
			ITEM_ADQ* tempA = NULL;
			tempA = tempP->objetosAdquiridos;
			while (tempA != NULL ) {
//				listaRecursos_add(&adquiridos, tempA->id,tempA->quantity);
				list_add(adquiridos,
						crearNodoRecurso(tempA->id, tempA->quantity));
				tempA = tempA->next;

			}
		}
		tempP = tempP->next;
	}

	return adquiridos;
}

void destroyItems(ITEM_NIVEL** ListaItems){

	while(*ListaItems != NULL) {
		ITEM_NIVEL * temp = *ListaItems;
		destroyAdquirido(&temp->objetosAdquiridos);
		*ListaItems = (*ListaItems)->next;
		free(temp);
	}

}

void destroyAdquirido(ITEM_ADQ** ListaAdq) {

	while (*ListaAdq != NULL ){
		ITEM_ADQ * temp = *ListaAdq;
		*ListaAdq = (*ListaAdq)->next;
		free(temp);
	}
}
