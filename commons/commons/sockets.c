/*
 * sockets.c
 *
 *  Created on: 09/05/2013
 *      Author: utnso
 */

#include "sockets.h"
#include <stdlib.h>
#include <string.h>
#include "string.h"

/**
 * @NAME: sockets_getSocket
 * @DESC: crea un socket y retorna su descriptor. Retorna -1 en caso de error.
 */
int sockets_getSocket(void) {
	int yes = 1;
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		close(sockfd);
		return -1;
	}

	return sockfd;
}

/**
 * @NAME: sockets_bind
 * @DESC: Bindea el socket pasado por parámetro a la dirección ip y puerto pasados por parámetro.
 * Retorna -1 en caso de error.
 */
int sockets_bind(int sockfd, char *addr, char *port) {
	struct sockaddr_in my_addr;

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(atoi(port));
	my_addr.sin_addr.s_addr = inet_addr(addr);
	memset(&(my_addr.sin_zero), '\0', 8);

	return bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr));
}

/**
 * @NAME: sockets_listen
 * @DESC: Pone a la escucha el socket pasado por parámetro con el backlog pasado por parámetro.
 * Retorna -1 en caso de error.
 */
int sockets_listen(int sockfd, int backlog) {
	return listen(sockfd, backlog);
}

/**
 * @NAME: sockets_accept
 * @DESC: Acepta una conexion al socket enviado por parámetro y retorna el nuevo descriptor de socket.
 * Retorna -1 en caso de error.
 */
int sockets_accept(int sockfd) {
	struct sockaddr_in their_addr;
	int sin_size = sizeof(struct sockaddr_in);

	return accept(sockfd, (struct sockaddr *) &their_addr,
			(socklen_t *) &sin_size);
}

/**
 * @NAME: sockets_connect
 * @DESC: Conecta el socket a la dirección y puerto pasados por parámetro.
 * Retorna -1 en caso de error.
 */
int sockets_connect(int sockfd, char *addr, char *port) {
	struct sockaddr_in their_addr;

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(atoi(port));
	their_addr.sin_addr.s_addr = inet_addr(addr);
	memset(&(their_addr.sin_zero), '\0', 8);

	return connect(sockfd, (struct sockaddr*) &their_addr,
			sizeof(struct sockaddr));
}

/**
 * @NAME: sockets_send
 * @DESC: Serializa el header y el string de datos y lo envía al scoket pasado por parámetro.
 * Retorna la cantidad de bytes enviados.
 */
int sockets_send(int sockfd, header_t *header, char *data) {
	int bytesEnviados, offset = 0, tmp_len = 0;
	char *packet = malloc(sizeof(header_t) + header->length);

	memcpy(packet, &header->type, tmp_len = sizeof(int8_t));
	offset = tmp_len;

	memcpy(packet + offset, &header->length, tmp_len = sizeof(int16_t));
	offset += tmp_len;

	memcpy(packet + offset, data, tmp_len = header->length);
	offset += tmp_len;

	bytesEnviados = send(sockfd, packet, offset, MSG_NOSIGNAL);
	free(packet);

	return bytesEnviados;
}

/**
 *
 */
int sockets_createServer(char *addr, char *port, int backlog) {
	int sockfd = sockets_getSocket();

	if (sockets_bind(sockfd, addr, port) == -1) {
		close(sockfd);
		return -1;
	}

	if (sockets_listen(sockfd, backlog) == -1) {
		close(sockfd);
		return -1;
	}

	return sockfd;
}

/**
 *
 */
int sockets_createClient(char *addr, char *port) {
	int sockfd = sockets_getSocket();

	if (sockets_connect(sockfd, addr, port) == -1) {
		close(sockfd);
		return -1;
	}

	return sockfd;
}

//------------------------------------------ipInfo------------------------------------------------------------

char *ipInfo_serializer(ip_info_t *self, int16_t *length) {
	char *serialized = malloc(strlen(self->addr) + strlen(self->port) + 2);
	int offset = 0, tmp_size = 0;

	memcpy(serialized, self->addr, tmp_size = strlen(self->addr) + 1);
	offset = tmp_size;

	memcpy(serialized + offset, self->port, tmp_size = strlen(self->port) + 1);
	offset += tmp_size;

	*length = offset;

	return serialized;
}

ip_info_t *ipInfo_deserializer(char *serialized) {
	ip_info_t *self = malloc(sizeof(ip_info_t));
	int offset = 0, tmp_len = 0;

	for (tmp_len = 1; (serialized)[tmp_len - 1] != '\0'; tmp_len++)
		;
	self->addr = malloc(tmp_len);

	memcpy(self->addr, serialized, tmp_len);
	offset = tmp_len;

	for (tmp_len = 1; (serialized + offset)[tmp_len - 1] != '\0'; tmp_len++)
		;
	self->port = malloc(tmp_len);
	memcpy(self->port, serialized + offset, tmp_len);

	return self;
}

void ipInfo_destroy(ip_info_t *ipInfo) {
	free(ipInfo->addr);
	free(ipInfo->port);
	free(ipInfo);
}

//------------------------------------------coordenadas------------------------------------------------------------

char *coordenadas_serializer(coordenada_t *self, int16_t *length) {
	char *serialized = malloc(2 * sizeof(int));
	int offset = 0, tmp_size = 0;

	memcpy(serialized, &self->ejeX, tmp_size = sizeof(int));
	offset = tmp_size;

	memcpy(serialized + offset, &self->ejeY, tmp_size = sizeof(int));
	offset += tmp_size;
	*length = offset;

	return serialized;
}

coordenada_t *coordenadas_deserializer(char *serialized) {
	coordenada_t *self = malloc(sizeof(coordenada_t));
	int offset = 0, tmp_size = 0;

	memcpy(&self->ejeX, serialized, tmp_size = sizeof(int));
	offset = tmp_size;

	memcpy(&self->ejeY, serialized + offset, tmp_size = sizeof(int));
	offset += tmp_size;

	return self;
}

void coordenadas_destroy(coordenada_t *self) {
	free(self);
}

//------------------------------------------indicaciones------------------------------------------------------------

char *indicaciones_serializer(indicacion_t *self, int16_t *length) {
	char *serialized = malloc(strlen(self->eje) + strlen(self->sentido) + 2);
	int offset = 0, tmp_size = 0;

	memcpy(serialized, self->eje, tmp_size = strlen(self->eje) + 1);
	offset = tmp_size;

	memcpy(serialized + offset, self->sentido,
			tmp_size = strlen(self->sentido) + 1);
	offset += tmp_size;

	*length = offset;

	return serialized;
}

indicacion_t *indicaciones_deserializer(char *serialized) {
	indicacion_t *self = malloc(sizeof(indicacion_t));
	int offset = 0, tmp_size = 0;

	for (tmp_size = 1; serialized[tmp_size - 1] != '\0'; tmp_size++)
		;
	self->eje = malloc(tmp_size);
	memcpy(self->eje, serialized, tmp_size);
	offset = tmp_size;

	for (tmp_size = 1; (serialized + offset)[tmp_size - 1] != '\0'; tmp_size++)
		;
	self->sentido = malloc(tmp_size);
	memcpy(self->sentido, serialized + offset, tmp_size);
	offset += tmp_size;

	return self;
}

void indicaciones_destroy(indicacion_t *self) {
	free(self->eje);
	free(self->sentido);
	free(self);
}

int enviarHandshake(int sockfd, int headerType) {
	header_t header;
	header.type = headerType;
	header.length = 0;

	return sockets_send(sockfd, &header, '\0');
}

//------------------------------------------Recursos------------------------------------------------------------

char* recursos_serializer(recurso_t* self, int16_t *lenght) {
	char id = self->id;
	char *serialized = malloc(sizeof(char) + sizeof(int));
	int offset = 0, tmp_size;

	memcpy(serialized, &id, tmp_size = sizeof(char));
	offset = tmp_size;

	memcpy(serialized + offset, &self->quantity, tmp_size = sizeof(int));
	offset += tmp_size;

	*lenght = offset;

	return serialized;
}

char* listaRecursos_serializer(t_list* self, int16_t *length) {
	int i = 0, offset = 0;
	recurso_t* nodo = NULL;
//	TODO:char* serialized = malloc((strlen(&nodo->id) + 1 + sizeof(int)) * i);
	char* serialized = malloc(
			(sizeof(char) + sizeof(int)) * self->elements_count);
	for (i = 0; i < self->elements_count; i++) {
		nodo = list_get(self, i);
		int16_t eachlength;
		char *unRecurso = recursos_serializer(nodo, &eachlength);
		memcpy(serialized + offset, unRecurso, eachlength);
		offset += eachlength;
		free(unRecurso);
	}

	*length = offset;

	return serialized;
}

t_list* recursos_deserializer(char* serialized) {
	t_list* p = NULL;
	return p;
}

t_list* listaRecursos_deserializer(char* serialized, int16_t length) {
	t_list* self = NULL;
	self = list_create();
	int offset = 0, tmp_size = 0, cant = 0;
	char id;

	while (offset < length) {
		memcpy(&id, serialized + offset, tmp_size = sizeof(char));
		offset += tmp_size;

		memcpy(&cant, serialized + offset, tmp_size = sizeof(int));
		offset += tmp_size;
		list_add(self, crearNodoRecurso(id, cant));
	}
	return self;
}

void listaRecursos_destroy(t_list* self) {
	list_destroy_and_destroy_elements(self, (void *) free);
}

void recurso_destroy(recurso_t *recurso) {
	free(recurso);
}

//------------------------------------------Personaje Interbloqueado------------------------------------------------------------

char* personajesInterbloqueados_serializer(t_list* self, int16_t *length) {
	char* serialized = malloc(self->elements_count);
	int i, offset = 0, tmp_size = 0;
	personaje_interbloqueado_t* personaje;

	for (i = 0; i < self->elements_count; i++) {
		personaje = list_get(self, i);
		memcpy(serialized + offset, &personaje->id, tmp_size = 1);
		offset += tmp_size;
	}

	*length = offset;

	return serialized;
}

t_list* personajesInterbloqueados_deserializer(char*serialized, int16_t length) {
	t_list* bloqueados = list_create();
	int offset = 0, tmp_size = 1;
	char id;

	while (offset < length) {
		memcpy(&id, serialized + offset, tmp_size);
		offset += tmp_size;
		list_add(bloqueados, crearNodoInterbloqueado(id));
	}

	return bloqueados;

}

recurso_t* crearNodoRecurso(char id, int cant) {
	recurso_t* temp;
	temp = malloc(sizeof(recurso_t));
	temp->id = id;
	temp->quantity = cant;
	return temp;
}

personaje_interbloqueado_t* crearNodoInterbloqueado(char id) {
	personaje_interbloqueado_t* temp;
	temp = malloc(sizeof(personaje_interbloqueado_t));
	temp->id = id;
	return temp;
}

//-------------------------------Personaje-Recurso------------------------------

char *personajeRecurso_serializer(personaje_recurso_t *self, int16_t *length) {
	char *serialized = malloc(sizeof(char) + sizeof(char));
	int offset = 0, tmp_size = 0;

	memcpy(serialized, &self->idPersonaje, tmp_size = sizeof(char));
	offset = tmp_size;

	memcpy(serialized + offset, &self->idRecurso, tmp_size = sizeof(char));
	offset += tmp_size;
	*length = offset;

	return serialized;
}

personaje_recurso_t *personajeRecurso_deserializer(char *data) {
	personaje_recurso_t *personaje = malloc(sizeof(personaje_recurso_t));
	int offset = 0, tmp_size = 0;

	memcpy(&personaje->idPersonaje, data, tmp_size = sizeof(char));
	offset = tmp_size;

	memcpy(&personaje->idRecurso, data + offset, tmp_size = sizeof(char));

	return personaje;
}

char *notificacionDatosPersonaje_serializer(
		notificacion_datos_personaje_t *datos, int16_t *length) {
	char *serialized = malloc(sizeof(char) + strlen(datos->nombreNivel) + 1);
	int offset = 0, tmp_size = 0;

	memcpy(serialized, &datos->simbolo, tmp_size = sizeof(char));
	offset = tmp_size;

	memcpy(serialized + offset, datos->nombreNivel,
			tmp_size = strlen(datos->nombreNivel) + 1);
	offset += tmp_size;

	*length = offset;

	return serialized;
}

notificacion_datos_personaje_t *notificacionDatosPersonaje_deserializer(
		char *serialized) {
	notificacion_datos_personaje_t *datos = malloc(
			sizeof(notificacion_datos_personaje_t));
	int offset = 0, tmp_size = 0;

	memcpy(&datos->simbolo, serialized, tmp_size = sizeof(char));
	offset = tmp_size;

	for (tmp_size = 1; serialized[tmp_size - 1] != '\0'; tmp_size++)
		;
	datos->nombreNivel = malloc(tmp_size);
	memcpy(datos->nombreNivel, serialized + offset, tmp_size);

	return datos;
}

void notificacionDatosPersonaje_destroy(notificacion_datos_personaje_t *self) {
	free(self->nombreNivel);
	free(self);
}

char *informacionPlanificacion_serializer(informacion_planificacion_t *self,
		int16_t *length) {
	char *serialized = malloc(strlen(self->nombreNivel) + 1 + 3 * sizeof(int));
	int offset = 0, tmp_size = 0;

	memcpy(serialized, self->nombreNivel,
			tmp_size = strlen(self->nombreNivel) + 1);
	offset = tmp_size;

	memcpy(serialized + offset, &self->algoritmo, tmp_size = sizeof(int));
	offset += tmp_size;

	memcpy(serialized + offset, &self->quantum, tmp_size = sizeof(int));
	offset += tmp_size;

	memcpy(serialized + offset, &self->retardo, tmp_size = sizeof(int));
	offset += tmp_size;

	*length = offset;

	return serialized;
}

informacion_planificacion_t *informacionPlanificacion_deserializer(
		char *serialized) {
	informacion_planificacion_t *self = malloc(
			sizeof(informacion_planificacion_t));
	int offset = 0, tmp_size = 0;

	for (tmp_size = 1; serialized[tmp_size - 1] != '\0'; tmp_size++)
		;
	self->nombreNivel = malloc(tmp_size);
	memcpy(self->nombreNivel, serialized + offset, tmp_size);
	offset = tmp_size;

	memcpy(&self->algoritmo, serialized + offset, tmp_size = sizeof(int));
	offset += tmp_size;

	memcpy(&self->quantum, serialized + offset, tmp_size = sizeof(int));
	offset += tmp_size;

	memcpy(&self->retardo, serialized + offset, tmp_size = sizeof(int));
	offset += tmp_size;

	return self;
}

void informacionPlanificacion_destroy(informacion_planificacion_t*self) {
	free(self->nombreNivel);
	free(self);
}

//

