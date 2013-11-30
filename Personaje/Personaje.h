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
#include <semaphore.h>

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
	pthread_t hilo;
	char *nivel;
	char **objetivos;
	char *ipOrquestador;
	char *puertoOrquestador;
	char simbolo;
	char eje;
	coordenada_t *coordObjetivo;
	coordenada_t *coordPosicion;
	int objetivoActual;
	int sockfdPlanificador;
	int estadoPersonaje;
} hilo_personaje_t;

typedef struct {
	char id;
	int motivo;
} estado_t;

typedef void (*funcPtr)();

enum enum_finalizacion_hilo {
	FIN_REINICIO_PLAN,
	FIN_NIVEL
};

enum finalizacion_proceso{
	FINALIZAR,
	REINICIAR,
	ESPERAR_HILO
};

t_config *configFile;
configuracion_personaje_t *config;
t_log *logFile;
int flagReinicioPlan = 0, contItentos = 0, vidasPlan = 0;
t_list * hilos;
estado_t* estado;
pthread_mutex_t *mutexContadorVidas;
sem_t sHiloTermino;

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
void matarHilos();
int gestionarFinNivel(char id);
void enviarFinNivel(hilo_personaje_t *datos);
int gestionarFinHilo();
void sacarHiloLista(hilo_personaje_t* hiloPersonaje) ;
#endif /* PERSONAJE_H_ */
