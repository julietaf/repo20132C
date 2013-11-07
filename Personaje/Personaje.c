/*
 * Personaje.c
 *
 *  Created on: 24/09/2013
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/geospatial.h>
#include "Personaje.h"

//TODO: Poner loggers, hay muchas cosas mucho muy importantes
int main(void) {
	getConfiguracion();
	void sigterm_handler(int signum) {
		perderVida("SIGTERM");
	}
	void sigusr1_handler(int signum) {
		config->vidas++;
		}

	struct sigaction sigterm_action;

	sigterm_action.sa_handler = sigterm_handler;
	sigemptyset(&sigterm_action.sa_mask);
	if (sigaction(SIGTERM, &sigterm_action, NULL) == -1) {
		return EXIT_FAILURE;
	}

	struct sigaction sigusr1_action;

	sigusr1_action.sa_handler = sigusr1_handler;
	sigusr1_action.sa_flags = SA_RESTART;
	sigemptyset(&sigusr1_action.sa_mask);
	if (sigaction(SIGUSR1, &sigusr1_action, NULL) == -1) {
		return EXIT_FAILURE;
	}

	int i;

	for (i = 0; config_get_array_value(configFile, "planDeNiveles")[i] != NULL ;
			i++) {
		hilo_personaje_t *dataHilo = crearDatosPersonaje(i);
		pthread_create(dataHilo->hilo, NULL, (void *) hiloPersonaje,
				(void *) dataHilo);
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
	dataHilo->objetivo_actual = string_duplicate(dataHilo->objetivos[0]);
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

	header_t header;
	recv(sockfdOrquestador, &header, sizeof(header), MSG_WAITALL);


}

void enviarHandshakePersonaje(int sockfd) {
	header_t head;
	head.type = HANDSHAKE_PERSONAJE;
	head.length = 0;

	sockets_send(sockfd, &head, '\0');
}

//Catcheo si es por sigterm,
void perderVida(hilo_personaje_t* personaje, char* motivo){
	char banderaSignal = strcmp(motivo, "SIGTERM");
	config->vidas--;
	if(!banderaSignal && (config->vidas >0)){
		reiniciarNivel(personaje, personaje->nivel);
	}else if((config->vidas <= 0)){
		ofrecerContinuar();
	}

}

void reiniciarNivel(hilo_personaje_t* personaje, int nivelAReiniciar){
	int pivot = config_get_int_value(configFile, personaje->nivel);
	liberarHilo(personaje);
	crearDatosPersonaje(pivot);

}

void liberarHilo(hilo_personaje_t* personaje){
	free(personaje->hilo);
	free(personaje->objetivos);
	free(personaje->puertoOrquestador);
	free(personaje->objetivo_actual);
	free(personaje);
}

void ofrecerContinuar(t_config configuracion){
	//Cerra todo.

	liberarPersonajes();
	//Promptear por pantalla si quiere continuar
	printf("Presiona Y para seguir intentando");
	char banderita;
	scanf("%c", banderita);
	if(banderita == 'Y'){
		for (int i = 0; config_get_array_value(configuracion, "planDeNiveles")[i] != NULL ;
				i++) {
			hilo_personaje_t *dataHilo = crearDatosPersonaje(i);
			pthread_create(dataHilo->hilo, NULL, (void *) hiloPersonaje,
					(void *) dataHilo);
		}
	}else{
		//Manda un SIGKILL sobre si mismo
		printf("Nos vemos en otro momento");
		sleep(5000);
		raise(9);
	}
}



void liberarPersonajes(){
	//TODO: For each hilo creado, llama a liberarHilo.
}


