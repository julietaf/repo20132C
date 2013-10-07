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
} configuracion_plataforma_t;

typedef struct {
	pthread_t *hilo;
	char *nombre;
	int sockfdNivel;
	t_queue *personajesListos;
	t_queue *personajesBloqueados;
} datos_planificador_t;

configuracion_plataforma_t *configuracion;
t_log *logFile;
t_dictionary *planificadores;

void orquestador(void);
void agregarSockfd(fd_set *bagMaster, int *sockfdMax, int sockfd);
void removerSockfd(fd_set *bagMaster, int sockfd);
void aceptarNuevaConexion(int sockfd, fd_set *bagMaster, int *sockfdMax);
configuracion_plataforma_t *getConfiguracion(void);
int atenderPedido(int sockfd);
int enviarHandshakeOrquestador(int sockfd);
void crearNuevoHiloPlanificador(int sockfd);
datos_planificador_t *crearDatosPlanificador(char *nombre);
void delegarAlHiloplanificador(int sockfd);

#endif /* ORQUESTADOR_H_ */
