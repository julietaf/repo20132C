/*
 * Nivel.h
 *
 *  Created on: 24/09/2013
 *      Author: utnso
 */

#ifndef NIVEL_H_
#define NIVEL_H_

//-----------------------------------------INCLUDEs---------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "commons/deadlock_detection.h"
#include "commons/collections/list.h"
#include "commons/tad_items.h"
#include "commons/config.h"
#include "commons/log.h"
#include "commons/string.h"
#include "commons/sockets.h"
#include "commons/geospatial.h"

//----------------------------------------CONSTANTES--------------------------------------------------
#define CONFIG_PATH "./nivelconfig.txt"
#define LOG_PATH "./nivellog.txt"
#define POS "+"
#define NEG "-"
#define EJE_X "ejeX"
#define EJE_Y "ejeY"

//------------------------------------------TYPES-----------------------------------------------------
typedef struct nivel_conf_t {
        char nombre[10];
        char simbolo[2];
        char *orquestadoraddr;
        char *orquestadorport;
        char *localhostaddr;
        char *localhostport;
        char *logLevel;
        int deadlockTime;
        int recovery;
        int enemigos;
        int sleepEnemigos;
} NIVEL_CONF;

//-----------------------------------------GLOBALES-----------------------------------------------------

t_log *logFile;
int orquestadorSockfd;
int sockEscucha;
int fil = 100, col = 100;
//int fil, col;
ITEM_NIVEL* listaRecursos = NULL;
ITEM_NIVEL* listaPersonajes = NULL;
ITEM_NIVEL* listaEnemigos = NULL;
NIVEL_CONF *configObj;
planificador_ini_t* configPlanificador;

//--------------------------------------------FIRMAS--------------------------------------------------
NIVEL_CONF* inicializarCongiuracionNivel();
void separador_log(char* head);
void inicializarInterfazGrafica();
void inicializarConfiguracionCajas();
void crearCajasConVector(char** array);
void inicializarSockEscucha();
void inicializarConexionPlataforma();
int conectarOrquestador();
void hacerHandshake(int sockfdReceptor);
void enviarDatosConexion();
void recibirDatosPlanificador();
void atenderMensajePlanificador(int sockfd);
int validarRecive(int sockfd, header_t* h);
void tratarNuevoPersonaje(char* data);
void tratarMovimiento(char* data);
void tratarSolicitudRecurso(char* data);
void tratarFinalizacionPersonaje(char* data);
void notificarMuertePersonaje(char id);
void crearHiloEnemigo ();
void dibujar();
//-----------------------Hilo DeadLock-------------------------------
void deadLock();
//-----------------------Hilo Enemigo--------------------------------
void enemigo(int idEnemigo);
void agregarEnemigo(int idEnemigo, coordenada_t* posicion);
void entrarAlNivel(coordenada_t* posicion );
void cazarPersonajes(t_list* bufferMovimiento, coordenada_t* posicion);
int hayPersonajes();
void perseguirPersonaje(coordenada_t* posicion);
void movimientoDeEspera(t_list* bufferMovimiento, coordenada_t* posicion);
int validarPosicionesEnemigo(t_list* bufferMovimiento);
int validarPosicionEnemigo(coordenada_t* posicion);
#endif /* NIVEL_H_ */
