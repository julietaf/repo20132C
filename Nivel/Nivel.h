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
#include <sys/types.h>
#include <sys/inotify.h>

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

//---- Inotify
#define BUFF_SIZE 1024
#define MAXDATASIZE 100
#define MAXCONN 5
#define direccion INADDR_ANY
#define EVENT_SIZE  ( sizeof (struct inotify_event) + 24 )
#define BUF_LEN     ( 1024 * EVENT_SIZE )

//------------------------------------------TYPES-----------------------------------------------------
typedef struct nivel_conf_t {
        char* nombre;
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
int plataformaSockfd;
int sockEscucha;
int fil = 100, col = 100;
//int fil, col;
int orden = 0;
t_list* listaRecursos = NULL;
t_list* listaPersonajes = NULL;
t_list* listaEnemigos = NULL;
NIVEL_CONF *configObj;
informacion_planificacion_t* configPlanificador;

pthread_mutex_t *mutexDibujables;

//--------------------------------------------FIRMAS--------------------------------------------------
NIVEL_CONF* inicializarCongiuracionNivel();
void separador_log(char* head);
void inicializarInterfazGrafica();
void inicializarConfiguracionCajas();
void crearCajasConVector(char** array);
void inicializarSockEscucha();
void inicializarConexionPlataforma();
int conectarOrquestador();
int hacerHandshake(int sockfdReceptor);
void enviarDatosAlgoritmo();
void recibirDatosPlanificador();
int atenderMensajePlanificador(int sockfd);
int validarRecive(int sockfd, header_t* h);
void obtenerDatosAlgorimo(informacion_planificacion_t* datosAlgoritmo);
void tratarNuevoPersonaje(char* data);
void tratarSolicitudUbicacionCaja(char* data);
void tratarMovimiento(char* data);
void tratarSolicitudRecurso(char* data);
void tratarFinalizacionPersonaje(char* data);
void notificarMuertePersonaje(char id, int causa);
void notificacionDarRecurso(char id);
void notificacionBloqueo(char id);
void crearHiloEnemigo ();
void crearHiloDeadLock();
void dibujar();
int personajeEnCaja(char pId, char rId);
void mandarRecursosLiberados(t_list* recursosLiberados);
t_list* esperarRecursosAsignados();
void actualizarEstado(t_list* asignados);
int getNotifyFileDescriptor();
void tratarModificacionAlgoritmo(int file_descriptor);
void inicializarLog();
//-----------------------Hilo DeadLock-------------------------------
void deadLock();
void gestionarDeadLock();
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
