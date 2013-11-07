/*
 * Personaje.c
 *
 *  Created on: 24/09/2013
 *      Author: utnso
 */

#include "Personaje.h"

int main(void) {
	getConfiguracion();
	int i;
	t_list *hilos = list_create();
	hilo_personaje_t *dataHilo;

	for (i = 0; config_get_array_value(configFile, "planDeNiveles")[i] != NULL ;
			i++) {
		dataHilo = crearDatosPersonaje(i);
		pthread_create(dataHilo->hilo, NULL, (void *) hiloPersonaje,
				(void *) dataHilo);
		list_add(hilos, dataHilo);
	}

	for (i = 0; list_get(hilos, i) != NULL ; i++) {
		dataHilo = list_get(hilos, i);
		pthread_join(*dataHilo->hilo, (void **) NULL );
	}

	return EXIT_SUCCESS;
}

hilo_personaje_t *crearDatosPersonaje(int index) {
	hilo_personaje_t *dataHilo = malloc(sizeof(hilo_personaje_t));
	dataHilo->ipOrquestador = config->ipOrquestador;
	dataHilo->puertoOrquestador = config->puertoOrquestador;
	dataHilo->nivel = malloc(
			strlen(config_get_array_value(configFile, "planDeNiveles")[index])
					+ 1);
	strcpy(dataHilo->nivel,
			config_get_array_value(configFile, "planDeNiveles")[index]);
	char *key = getObjetivoKey(dataHilo->nivel);
	dataHilo->objetivos = config_get_array_value(configFile, key);
	dataHilo->hilo = malloc(sizeof(pthread_t));
	dataHilo->cantObjetivos = 0;
	free(key);

	return dataHilo;
}

char *getObjetivoKey(char *nombreNivel) {
	char *key = malloc(strlen("obj[") + strlen(nombreNivel) + strlen("]") + 1);
	strcat(key, "obj[");
	strcat(key, nombreNivel);
	strcat(key, "]");

	return key;
}

void getConfiguracion(void) {
	configFile = config_create(CONFIG_PATH);
	config = malloc(sizeof(configuracion_personaje_t));
	config->nombre = malloc(
			strlen(config_get_string_value(configFile, "nombre")) + 1);
	strcpy(config->nombre, config_get_string_value(configFile, "nombre"));
	config->simbolo = *config_get_string_value(configFile, "simbolo");
	config->vidas = config_get_int_value(configFile, "vidas");
	char *tok = ":";
	char *orqaddr = strdup(config_get_string_value(configFile, "orquestador"));
	config->ipOrquestador = strtok(orqaddr, tok);
	config->puertoOrquestador = strtok(NULL, tok);
}

void hiloPersonaje(hilo_personaje_t *datos) {
	fd_set bagMaster, bagEscucha;
	int sockfdOrquestador = sockets_createClient(datos->ipOrquestador,
			datos->puertoOrquestador);
	enviarHandshakePersonaje(sockfdOrquestador);
	FD_ZERO(&bagMaster);
	FD_ZERO(&bagEscucha);
	FD_SET(sockfdOrquestador, &bagMaster);
	int sockfd, sockfdMax = sockfdOrquestador;

	while (1) {
		bagEscucha = bagMaster;

		select(sockfdMax + 1, &bagEscucha, NULL, NULL, NULL );

		for (sockfd = 0; sockfd < sockfdMax + 1; sockfd++) {
			if (FD_ISSET(sockfd,&bagEscucha)) {

			}
		}
	}
}

void enviarHandshakePersonaje(int sockfd) {
	header_t head;
	head.type = HANDSHAKE_PERSONAJE;
	head.length = 0;

	sockets_send(sockfd, &head, '\0');
}
