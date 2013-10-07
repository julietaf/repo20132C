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
#include "commons/deadlock_detection.h"
#include "commons/collections/list.h"
#include "commons/tad_items.h"
#include "commons/config.h"
#include "commons/log.h"
#include "commons/string.h"
#include "commons/sockets.h"

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
} NIVEL_CONF;

//-----------------------------------------GLOBALES-----------------------------------------------------

t_log *logFile;
int orquestadorSockfd;
int sockEscucha;
int fil = 100, col = 100;
ITEM_NIVEL* listaRecursos = NULL;
ITEM_NIVEL* listaPersonajes = NULL;
ITEM_NIVEL* listaEnemigos = NULL;
NIVEL_CONF *configObj;



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

#endif /* NIVEL_H_ */
