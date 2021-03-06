/*
 * Orquestador.h
 *
 *  Created on: 04/10/2013
 *      Author: utnso
 */

#ifndef ORQUESTADOR_H_
#define ORQUESTADOR_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/wait.h>
#include <commons/sockets.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/queue.h>
#include <commons/collections/dictionary.h>

#define LOG_PATH "./txt/log.txt"
#define CONFIG_PATH "./txt/config.txt"

typedef struct {
	char *direccionIp;
	char *puerto;
	int backlog;
	t_log_level detalleLog;
	char *koopaPath;
	char *montajePath;
	char *scriptPath;
} configuracion_plataforma_t;

typedef struct {
	char simbolo;
	int sockfd;
	char objetivo;
	coordenada_t *coordObjetivo;
	coordenada_t *ubicacionActual;
} datos_personaje_t;

typedef struct {
	pthread_t *hilo;
	char *nombre;
	int sockfdNivel;
	int retardo;
	int algoritmo;
	int quatum;
	fd_set *bagMaster;
	int sockfdMax;
	t_queue *personajesListos;
	t_queue *personajesBloqueados;
	pthread_mutex_t *mutexColas;
	datos_personaje_t *personajeEnMovimiento;
	int quantumCorriente;
} datos_planificador_t;

typedef struct {
	char *nombreNivel;
	datos_personaje_t *personaje;
} personaje_espera_t;

typedef struct {
	char simbolo;
	int finalizoPlan;
} estado_personaje_t;

configuracion_plataforma_t *configuracion;
t_log *logFile;
t_list *listaPlanificadores, *listaEspera, *globalPersonajes;

void orquestador(void);
void agregarPersonajeAEspera(char *nombreNivel, datos_personaje_t *personaje);
void informarPersonajesEspera(datos_planificador_t *datosPlanificador);
void personajeEspera_destroy(personaje_espera_t *self);
void agregarSockfd(fd_set *bagMaster, int *sockfdMax, int sockfd);
void removerSockfd(fd_set *bagMaster, int sockfd);
void aceptarNuevaConexion(int sockfd, fd_set *bagMaster, int *sockfdMax);
configuracion_plataforma_t *getConfiguracion(void);
int atenderPedido(int sockfd);
int enviarHandshakeOrquestador(int sockfd);
datos_planificador_t *crearNuevoHiloPlanificador(int sockfd);
datos_planificador_t *crearDatosPlanificador(
		informacion_planificacion_t *infoPlan, int sockfdNivel);
void delegarAlPlanificador(header_t *header, int sockfd);
datos_personaje_t *crearDatosPersonaje(char simbolo, int sockfdPersonaje);
void agregarPersonajeAListos(datos_personaje_t *datosPersonaje,
		char *nombreNivel);
void datosPersonaje_destroy(datos_personaje_t *self);
int notificarNivel(int sockfdNivel, char simbolo);
void chequearUltimoPersonaje(void);
int tienePersonajesActivos(datos_planificador_t *unPlanificador);
void atenderNuevoPersonaje(int sockfd);
datos_planificador_t *buscarPlanificador(char *nombre);
void logguearFinPlan(header_t *header, int sockfd);
datos_planificador_t *removerPlanificador(char *nombre);
int respuestaValida(char respuesta);
estado_personaje_t *buscarEstadoPersonaje(char simbolo);
estado_personaje_t *agregarPersonajeAGlobal(char simbolo);
estado_personaje_t *buscarNoFinalizado(void);
void llamarAKoopa(void);

#endif /* ORQUESTADOR_H_ */
