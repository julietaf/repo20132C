/*
 * Planificador.h
 *
 *  Created on: 07/10/2013
 *      Author: utnso
 */

#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

#include "Orquestador.h"
#include <commons/geospatial.h>

void planificador(datos_planificador_t *datos);
int gestionarDesbloqueoPersonajes(header_t *header,
		datos_planificador_t *datosPlan);
int atenderReinicioPlan(datos_planificador_t *datosPlan, int sockfdPersonaje);
int informarPersonajeReinicio(datos_planificador_t *datosPlan,
		int sockfdPersonaje);
int informarPersonajeFinalizado(datos_planificador_t *datosPlan,
		int sockfdPersonaje);
int esperarSolicitudRecurso(datos_planificador_t *datosPlan,
		datos_personaje_t *personaje);
int gestionarUbicacionCaja(datos_planificador_t *datosPlan, header_t *header);
int esperarUbicacionCaja(datos_planificador_t *datosPlan,
		datos_personaje_t *unPersonaje);
int notificarReinicioPlan(int sockfd);
int gestionarReinicioPlan(datos_planificador_t *datosPlan, int sockfdPersonaje);
int enviarPersonajeFinalizo(datos_planificador_t *datosPlan, char simbolo);
datos_personaje_t *removerPersonajePorSimbolo(datos_planificador_t *datosPlan,
		char idPersonaje);
datos_personaje_t *removerPersonajePorSockfd(datos_planificador_t *datosPlan,
		int sockfd);
datos_personaje_t *buscarPersonajePorSimbolo(datos_planificador_t *datosPlan,
		char simbolo);
datos_personaje_t *buscarPersonajePorSockfd(datos_planificador_t *datosPlan,
		int sockfdPersonaje);
int solicitarUbicacionRecursos(datos_planificador_t *datosPlan);
int atenderPedidoPlanificador(fd_set *bagEscucha, int sockfdMax,
		datos_planificador_t *datos);
void moverPersonaje(datos_planificador_t *datosPlan);
void seleccionarPorRoundRobin(datos_planificador_t *datosPlan);
void seleccionarPorSRDF(datos_planificador_t *datos);
int enviarTurnoConcedido(datos_personaje_t *personaje);
int atenderPedidoPersonaje(datos_planificador_t *datos, int sockfdPersonaje);
int atenderPedidoNivel(datos_planificador_t *datos);
int reenviarUbicacionCaja(datos_planificador_t *datosPlan, int sockfdPersonaje,
		header_t *header);
int reenviarNotificacionMovimiento(datos_planificador_t *datosPlan,
		int sockfdPersonaje, header_t *header);
datos_personaje_t *seleccionarCaminoMasCorto(datos_planificador_t *datosPlan);
int actualizarAlgoritmo(header_t *header, datos_planificador_t *datos);
int removerPersonaje(header_t *header, datos_planificador_t *datos,
		char *motivo);
t_list *desbloquearPersonajes(t_list *recursosLiberados,
		datos_planificador_t *datos);
int usarRecurso(char idObjetivo, t_list *recursosLiberados, t_list *recUsados,
		char simboloPersonaje);
int informarDesbloqueo(datos_personaje_t *perBloqueado);
void desbloquearPersonaje(datos_personaje_t *perDesbloqueado,
		datos_planificador_t *datos);
int informarRecursosUsados(t_list *recursosUsados, datos_planificador_t *datos);
int reenviarSolicitudRecurso(datos_planificador_t *datosPlan,
		int sockfdPersonaje, header_t *header);
int informarSolicitudRecurso(datos_personaje_t *personaje, int type);
int notificarMuertePersonaje(datos_personaje_t *personajeMuerto,
		datos_planificador_t *datos);
int llamadaSelect(datos_planificador_t *datosPlan, fd_set *bagEscucha);

#endif /* PLANIFICADOR_H_ */
