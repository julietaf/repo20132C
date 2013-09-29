/*
 * sockets.h
 *
 *  Created on: 09/05/2013
 *      Author: utnso
 */

#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <unistd.h>
#include "collections/list.h"

typedef struct {
	int8_t type;
	int16_t length;
}__attribute__((__packed__)) header_t;

typedef struct {
	char *addr;
	char *port;
}__attribute__((__packed__)) ip_info_t;

typedef struct {
	int ejeX;
	int ejeY;
}__attribute__((__packed__)) coordenadas_t;

typedef struct {
	char *eje;
	char *sentido;
}__attribute__((__packed__)) indicaciones_t;

typedef struct {
	char *addr;
	char *port;
	int8_t nivel;
	char fhm[6];
}__attribute__((__packed__)) nivel_ini_t;

typedef struct {
	char *addrN;
	char *portN;
	char *addrP;
	char *portP;
}__attribute__((__packed__)) personaje_ini_t;

typedef struct {
	char id;
	int quantity;
}__attribute__((__packed__)) recurso_t;

typedef struct {
	char id;
}__attribute__((__packed__)) personaje_interbloqueado_t;

typedef struct {
	char idPersonaje;
	char idRecurso;
}__attribute__((__packed__)) personaje_desbloqueado_t;

enum enum_protocolo {
	//	Handshakes
	HANDSHAKE_PERSONAJE = 1,
	HANDSHAKE_PLNIFICADOR = 2,
	HANDSHAKE_ORQUESTADOR = 3,
	HANDSHAKE_NIVEL = 4,
	// Hilo Personaje >>>> Planificador
	UBICACION_CAJA = 5,
	NOTIFICACION_MOVIMIENTO = 6,
	SOLICITAR_RECURSO = 7,
	FINALIZAR_NIVEL = 8,
	// Hilo Personaje <<<< Planificador
	TURNO_CONCEDIDO = 9,
	COORDENADA_CAJA = 10,
	OTORGAR_RECURSO = 11,
	NEGAR_RECURSO = 12,
	NOTIFICAR_MUERTE = 13,
	// Planificador >>>> Nivel
	// IDEM Hilo Personaje >>>> Planificador
	PERSONAJE_FINALIZO = 14,
	// Planificador <<<< Nivel
	// IDEM Hilo Personaje <<<< Planificador
	NOTIFICAR_ALGORITMO_PLANIFICACION = 15
};

int sockets_getSocket(void);
int sockets_bind(int, char *, char *);
int sockets_listen(int, int);
int sockets_accept(int);
int sockets_send(int, header_t *, char *);
int sockets_connect(int, char *, char *);
int sockets_createServer(char *, char *, int);
int sockets_createClient(char *, char *);
char *ipInfo_serializer(ip_info_t *self, int16_t *length);
ip_info_t *ipInfo_deserializer(char *serialized);
void ipInfo_destroy(ip_info_t *ipInfo);
char *coordenadas_serializer(coordenadas_t *self, int16_t *length);
coordenadas_t *coordenadas_deserializer(char *serialized);
void coordenadas_destroy(coordenadas_t *self);
char *indicaciones_serializer(indicaciones_t *self, int16_t *length);
indicaciones_t *indicaciones_deserializer(char *serialized);
void indicaciones_destroy(indicaciones_t *self);
char *nivelIni_serializer(nivel_ini_t *self, int16_t *length);
nivel_ini_t *nivelIni_deserializer(char *serialized);
void nivelIni_destroy(nivel_ini_t *self);
char *personajeIni_serializer(personaje_ini_t *self, int16_t *length);
personaje_ini_t *personajeIni_deserializer(char *serialized);
void personajeIni_destroy(personaje_ini_t *self);
int enviarHandshake(int sockfd, int headerType);
char* recursos_serializer(recurso_t* self, int16_t *lenght);
char* listaRecursos_serializer(t_list* self, int16_t *lenght);
t_list* recursos_deserializer(char* serialized);
t_list* listaRecursos_deserializer(char* serialized, int16_t length);
void listaRecursos_destroy(t_list* self);
void recurso_destroy(recurso_t *recurso);
recurso_t* crearNodoRecurso(char id, int cant);
char* personajesInterbloqueados_serializer(t_list* self, int16_t *length);
t_list* personajesInterbloqueados_deserializer(char*serialized, int16_t length);
personaje_interbloqueado_t* crearNodoInterbloqueado(char id);
char *personajeDesbloqueado_serializer(personaje_desbloqueado_t *self,
		int16_t *length);
personaje_desbloqueado_t *personajeDesbloqueado_deserializer(char *data);
char *listaPersonajeDesbloqueado_serializer(t_list *self, int16_t *length);
t_list *listaPersonajeDesbloqueado_deserializer(char *data, int16_t length);

#endif /* SOCKETS_H_ */
