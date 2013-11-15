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
}__attribute__((__packed__)) coordenada_t;

typedef struct {
	char *eje;
	char *sentido;
}__attribute__((__packed__)) indicacion_t;

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
}__attribute__((__packed__)) personaje_recurso_t;

typedef struct {
	char simbolo;
	char *nombreNivel;
} notificacion_datos_personaje_t;

typedef struct {
	char *nombreNivel;
	int algoritmo;
	int quantum;
	int retardo;
}__attribute__((__packed__)) informacion_planificacion_t;

enum enum_protocolo {
	//	Handshakes
	HANDSHAKE_PERSONAJE,
	HANDSHAKE_PLANIFICADOR,
	HANDSHAKE_ORQUESTADOR,
	HANDSHAKE_NIVEL,
// Hilo Personaje >>>> Planificador
	NOTIFICAR_DATOS_PERSONAJE,
	UBICACION_CAJA,
	NOTIFICACION_MOVIMIENTO,
	SOLICITAR_RECURSO,
	FINALIZAR_NIVEL,
// Hilo Personaje <<<< Planificador
	NIVEL_INEXISTENTE,
	TURNO_CONCEDIDO,
	COORDENADA_CAJA,
	OTORGAR_RECURSO,
	NEGAR_RECURSO,
	NOTIFICAR_MUERTE,
// Planificador >>>> Nivel
// IDEM Hilo Personaje >>>> Planificador
	NOTIFICAR_DATOS_PLANIFICADOR,
	PERSONAJE_FINALIZO,
	NOTIFICACION_RECURSOS_ASIGNADOS,
	NOTIFICAR_REINICIO_PLAN,
// Planificador <<<< Nivel
// IDEM Hilo Personaje <<<< Planificador
	NOTIFICAR_DATOS_NIVEL,
	NOTIFICAR_ALGORITMO_PLANIFICACION,
	VICTIMA_DEADLOCK,
	VICTIMA_ENEMIGO,
	NOTIFICACION_RECURSOS_LIBERADOS
};

enum enum_algoritmoPlanificador {
	ROUND_ROBIN, SRDF
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
char *coordenadas_serializer(coordenada_t *self, int16_t *length);
coordenada_t *coordenadas_deserializer(char *serialized);
void coordenadas_destroy(coordenada_t *self);
char *indicaciones_serializer(indicacion_t *self, int16_t *length);
indicacion_t *indicaciones_deserializer(char *serialized);
void indicaciones_destroy(indicacion_t *self);
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
char *personajeRecurso_serializer(personaje_recurso_t *self,
		int16_t *length);
personaje_recurso_t *personajeRecurso_deserializer(char *data);
char *notificacionDatosPersonaje_serializer(
		notificacion_datos_personaje_t *datos, int16_t *length);
notificacion_datos_personaje_t *notificacionDatosPersonaje_deserializer(
		char *serialized);
void notificacionDatosPersonaje_destroy(notificacion_datos_personaje_t *self);
char *informacionPlanificacion_serializer(informacion_planificacion_t *self, int16_t *length);
informacion_planificacion_t *informacionPlanificacion_deserializer(char *serialized);
void informacionPlanificacion_destroy(informacion_planificacion_t*self);

#endif /* SOCKETS_H_ */
