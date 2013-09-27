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
	PEDIDO_INFO_CONEXION = 1,
	RESPUESTA_PEDIDO_INFO_CONEXION = 1,
	PEDIDO_INFO_ITEM = 2,
	RESPUESTA_INFO_ITEM = 2,
	NOTIFICACION_MOVIDA = 3,
	MOVIMIENTO = 4,
	NOTIFICACION_BLOQUEO = 5,
	NOTIFICACION_MUERTE = 6,
	NOTIFICACION_REINICIAR_NIVEL = 7,
	NOTIFICACION_INFO_CONEXION = 8,
	NOTIFICACION_DATOS_PERSONAJE = 9,
	RESPUESTA_NOTIFICACION_DATOS_PERSONAJE = 9,
	PEDIDO_RECURSO = 10,
	RESPUESTA_PEDIDO_RECURSO = 10,
	NOTIFICACION_REINICIAR_PLAN = 11,
	NOTIFICACION_DATOS_NIVEL = 12,
	RESPUESTA_NOTIFICACION_DATOS_NIVEL = 12,
	HANDSHAKE_PERSONAJE = 13,
	HANDSHAKE_ORQUESTADOR = 14,
	HANDSHAKE_PLANIFICADOR = 15,
	HANDSHAKE_NIVEL = 16,
	NOTIFICACION_DATOS_PLANIFICADOR = 17,
	NOTIFICACION_NIVEL_FINALIZADO = 18,
	NOTIFICACION_RECURSOS_LIBERADOS = 19,
	NOTIFICACION_RECURSOS_ASIGNADOS = 20,
	NOTIFICACION_PERSONAJES_ITERBLOQUEADOS = 21,
	NOTIFICACION_MOVIMIENTO_REALIZADO = 22,
	NOTIFICACION_FIN_PLAN_NIVELES = 23
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
