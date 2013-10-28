/*
 * Personaje.h
 *
 *  Created on: 24/09/2013
 *      Author: utnso
 */

#ifndef PERSONAJE_H_
#define PERSONAJE_H_

#include <stdlib.h>
#include <string.h>
#include <commons/config.h>
#include <commons/sockets.h>
#include <pthread.h>

#define LOG_PATH "./txt/log.txt"
#define CONFIG_PATH "./txt/config.txt"

typedef struct {
	char *nombre;
	char simbolo;
	int vidas;
	char *ipOrquestador;
	char *puertoOrquestador;
} configuracion_personaje_t;

typedef struct {
	pthread_t *hilo;
	char *nivel;
	char **objetivos;
	int vidas;
	char *ipOrquestador;
	char *puertoOrquestador;
} hilo_personaje_t;

t_config *configFile;
configuracion_personaje_t *config;

void getConfiguracion(void);
void hiloPersonaje(hilo_personaje_t *datos);
hilo_personaje_t *crearDatosPersonaje(int index);
void enviarHandshakePersonaje(int sockfd);
char *getObjetivoKey(char *nombreNivel);

#endif /* PERSONAJE_H_ */
