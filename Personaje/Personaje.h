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
#include <signal.h>
#include <pthread.h>

#include <commons/config.h>
#include <commons/sockets.h>
#include <commons/log.h>
#include <commons/geospatial.h>

#define LOG_PATH "./txt/log.txt"
#define CONFIG_PATH "./txt/config.txt"

typedef struct {
	char *nombre;
	char simbolo;
	int vidas;
	char *ipOrquestador;
	char *puertoOrquestador;
	char **planNiveles;
} configuracion_personaje_t;

typedef struct {
	pthread_t *hilo;
	char *nivel;
	char **objetivos;
//	int vidas;
	char *ipOrquestador;
	char *puertoOrquestador;
	char simbolo;
	char eje;
	coordenada_t *coordObjetivo;
	coordenada_t *coordPosicion;
	int objetivoActual;
	int sockfdPlanificador;
} hilo_personaje_t;

typedef void (*funcPtr)();

t_config *configFile;
configuracion_personaje_t *config;
t_log *logFile;
int flagReinicioPlan = 0, contItentos = 0;
t_list * hilos;
pthread_mutex_t *mutexContadorVidas;

void getConfiguracion(void);
void hiloPersonaje(hilo_personaje_t *datos);
hilo_personaje_t *crearDatosPersonaje(int index);
void enviarHandshakePersonaje(int sockfd);
char *getObjetivoKey(char *nombreNivel);
void perderVida(char* motivo);
void reiniciarNivel(hilo_personaje_t* personaje, int nivelAReiniciar);
int atenderOrquestador(hilo_personaje_t *datos);
int enviarDatosPersonaje( hilo_personaje_t *datos);
int realizarMovimiento( hilo_personaje_t *datos);
int solicitarCoordenadasObjetivo(int sockfdOrquestador, char *objetivo);
int recibirCoordenadas( hilo_personaje_t *datos, header_t header);
int enviarNotificacionMovimiento(int sockfdOrquestador,
		coordenada_t * coordenada, char id);
int enviarSolicitudObjetivo( hilo_personaje_t *datos);
void recibirRecurso( hilo_personaje_t *datos);
int esperarDesbloqueo( hilo_personaje_t *datos);
int hiloRutinaMuerte( hilo_personaje_t *datos, char* causa);
void rutinaReinicioNivel( hilo_personaje_t *datos);
void rutinaReinicioPlan();
void reiniciarDatosNivel(hilo_personaje_t *datos);
void rutinaFinalizarNivel( hilo_personaje_t *datos );
void dataHiloDestroy(hilo_personaje_t* datos);
void signalRutinaVidas();
void signalRutinaMuerte();
int rutinaMuerte();
int mostrarContinue();
void enviarSuccessPersonaje();
void crearClientePlanificador(hilo_personaje_t* datos);
void inicializarLog();
#endif /* PERSONAJE_H_ */
