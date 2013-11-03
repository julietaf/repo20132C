/*
 * geospatial.c
 *
 *  Created on: 30/09/2013
 *      Author: utnso
 */
#include "geospatial.h"

//-----------------------------FUNCIONES PUBLICAS---------------------------------------

/**
 * @NAME: obtenerDistancia
 * @DESC: Retrona la distancia entre 2 puntos del tipo cooredenada_t.
 */
int obtenerDistancia(coordenada_t* inicio, coordenada_t* fin) {
	int distX;
	int distY;
	distX = obtenerDistanciaEnX(inicio, fin);
	distY = obtenerDistanciaEnY(inicio, fin);
	return distX + distY;
}

/**
 * @NAME: coordenadaMovimientoAlternado
 * @DESC: Retorna la coordenada de la proxima posicion a moverse
 */
void coordenadaMovimientoAlternado(coordenada_t* posicionActual,
		coordenada_t* posicionObjetivio) {

	if (obtenerDistancia(posicionActual, posicionObjetivio) == 0) { //Validacion en el objetivo
		return;
	}

	if (obtenerDistanciaEnX(posicionActual, posicionObjetivio) == 0) { //Validacion Caso solo por Y
		desplazarEnY(posicionActual, posicionObjetivio);
		return;

	}

	if (obtenerDistanciaEnY(posicionActual, posicionObjetivio) == 0) { //Validacion Caso solo por x
		desplazarEnX(posicionActual, posicionObjetivio);
		return;
	}

	if (esPar(posicionActual->ejeX) && esPar(posicionActual->ejeY)) {
		desplazarEnX(posicionActual, posicionObjetivio);
		return;
	}
	if (!esPar(posicionActual->ejeX) && esPar(posicionActual->ejeY)) {
		desplazarEnY(posicionActual, posicionObjetivio);
		return;
	}
	if (!esPar(posicionActual->ejeX) && !esPar(posicionActual->ejeY)) {
		desplazarEnX(posicionActual, posicionObjetivio);
		return;
	}
	if (esPar(posicionActual->ejeX) && !esPar(posicionActual->ejeY)) {
		desplazarEnY(posicionActual, posicionObjetivio);
		return;
	}
	return;

}

/**
 * @NAME: indicacionMovimientoAlternado
 * @DESC: Retorna la indicacion de la proxima posicion a moverse
 */

indicacion_t* indicacionMovimientoAlternado(coordenada_t* posicionActual,
		coordenada_t* posicionObjetivio) {
	indicacion_t* indicacion = malloc(sizeof(indicacion_t));
//	coordenada_t* posicionSiguiente = coordenadaMovimientoAlternado(
//			posicionActual, posicionObjetivio);
//	if (coordenadasIguales(posicionSiguiente, posicionActual)) {
//		return indicacion;
//	}
//
//	if (posicionSiguiente->ejeX == posicionActual->ejeX) {
//		indicacion->eje = "EjeY";
//		indicacion->sentido =
//				posicionSiguiente->ejeY > posicionActual->ejeY ? "+" : "-";
//	} else {
//		indicacion->eje = "EjeX";
//		indicacion->sentido =
//				posicionSiguiente->ejeX > posicionActual->ejeX ? "+" : "-";
//	}
//
	return indicacion;
}

/**
 * @NAME: coordenadasIguales
 * @DESC: Retorna 1 si ambas coordenadas son iguales y 0 en csaso contrario
 */
int coordenadasIguales(coordenada_t* c1, coordenada_t* c2) {
	return (c1->ejeX == c2->ejeX) && (c1->ejeY == c2->ejeY);
}

/**
 * @NAME: coordenadasIguales
 * @DESC: Retorna 1 si a las componentes de las coordenadas son iguales a
 * los enteros pasados y 0 en csaso contrario
 */
int coordenadasIgualesInt(coordenada_t* c1, int x, int y) {
	return (c1->ejeX == x) && (c1->ejeY == y);
}

/**
 * @NAME: coordenadaEvasion
 * @DESC: Retorna la proxima coordenada en funcion de las posicion a evadir y del
 * objetivo.
 */
void coordenadaEvasion(coordenada_t* obstaculo, coordenada_t* objetivo,
		coordenada_t* posActual, coordenada_t* cMaxima) {

	if (obstaculo->ejeX == posActual->ejeX) {
		if (cMaxima->ejeX == posActual->ejeX) { //Validacion para no salirme de los limites
			return desplazarEnXNegativo(posActual);
		}
		return desplazarEnX(posActual, objetivo);
	}
	if (obstaculo->ejeY == posActual->ejeY) {
		if (cMaxima->ejeY == posActual->ejeY) { //Validacion para no salirme de los limites
			return desplazarEnYNegativo(posActual);
		}
		return desplazarEnY(posActual, objetivo);
	}
}

/**
 * @NAME: coordenadaRandom
 * @DESC: Retorna una coordenada al azar entre 0,0 y la cMaxima
 */
coordenada_t* coordenadaRandom(coordenada_t* cMaxima) {
	int x = randomNumber(0, cMaxima->ejeX);
	int y = randomNumber(0, cMaxima->ejeY);
	coordenada_t* ret = malloc(sizeof(coordenada_t));
	ret->ejeX = x;
	ret->ejeY = y;
	return ret;
}

/**
 * @NAME: coordenadaRandomEjes
 * @DESC: Retorna una coordenada al azar entre 0,0 y los ejes maximos
 */
void coordenadaRandomEjes(coordenada_t* ret, int xMax, int yMax) {
	int x = randomNumber(0, xMax);
	int y = randomNumber(0, yMax);
	modificarCoordenada(ret, x, y);
}

/**
 * @NAME: movimientoLRandom
 * @DESC: Retorna una lista con los 3 movimientos de una L al azar
 */
void movimientoLRandom(coordenada_t* cActual, t_list* buffer) {
	int orden, direccion, sentido;

	orden = randomNumber(0, 1);
	direccion = randomNumber(0, 3);
	sentido = randomNumber(0, 1);

	return movimientoL(cActual, buffer, orden, direccion, sentido);
}

/**
 * @NAME: movimientoL
 * @DESC: retorna un lista con las 3 posiciones de un movimiento en "L" pasando por paremetro
 * orden, direccion y sentido.
 * @PARAM: Direccion 0 arriba, 1 derecha, 2 abajo, 3 izquierda
 * 		   Orden 1 = primero 2 despues 1, 0 viceversa
 * 		   Sentido = 1 dobla en positivo, 0 en negativo
 */
void movimientoL(coordenada_t* cActual,t_list* buffer, int orden, int direccion,
		int sentido) {

	coordenada_t* c1, *c2, *c3;
//	t_list* ret = list_create();

	c1 = malloc(sizeof(coordenada_t));
	c2 = malloc(sizeof(coordenada_t));
	c3 = malloc(sizeof(coordenada_t));

	switch (direccion) {
	case 0:
		modificarCoordenada(c1, cActual->ejeX, cActual->ejeY+1);
		if (sentido) { //Positivo
			if (orden) { // 2 1
				modificarCoordenada(c2, c1->ejeX, c1->ejeY + 1);
				modificarCoordenada(c3, c2->ejeX +1, c2->ejeY);
			} else { // 1 2
				modificarCoordenada(c2, c1->ejeX +1, c1->ejeY);
				modificarCoordenada(c3, c2->ejeX +1, c2->ejeY);
			}
		} else {
			if (orden) { // 2 1
				modificarCoordenada(c2, c1->ejeX, c1->ejeY + 1);
				modificarCoordenada(c3, c2->ejeX -1, c2->ejeY);
			} else { // 1 2
				modificarCoordenada(c2, c1->ejeX -1, c1->ejeY);
				modificarCoordenada(c3, c2->ejeX -1, c2->ejeY);
			}
		}
		break;
	case 1:
		modificarCoordenada(c1, cActual->ejeX+1, cActual->ejeY);
		if (sentido) { //Positivo
			if (orden) { // 2 1
				modificarCoordenada(c2, c1->ejeX +1, c1->ejeY);
				modificarCoordenada(c3, c2->ejeX, c2->ejeY +1);
			} else { // 1 2
				modificarCoordenada(c2, c1->ejeX, c1->ejeY + 1);
				modificarCoordenada(c3, c2->ejeX, c2->ejeY + 1);
			}
		} else {
			if (orden) { // 2 1
				modificarCoordenada(c2, c1->ejeX + 1, c1->ejeY);
				modificarCoordenada(c3, c2->ejeX, c2->ejeY -1);
			} else { // 1 2
				modificarCoordenada(c2, c1->ejeX, c1->ejeY - 1);
				modificarCoordenada(c3, c2->ejeX, c2->ejeY - 1);
			}
		}
		break;
	case 2:
		modificarCoordenada(c1, cActual->ejeX, cActual->ejeY - 1);
		if (sentido) { //Positivo
			if (orden) { // 2 1
				modificarCoordenada(c2, c1->ejeX, c1->ejeY - 1);
				modificarCoordenada(c3, c2->ejeX + 1, c2->ejeY );
			} else { // 1 2
				modificarCoordenada(c2, c1->ejeX +1, c1->ejeY);
				modificarCoordenada(c3, c2->ejeX +1, c2->ejeY);
			}
		} else {
			if (orden) { // 2 1
				modificarCoordenada(c2, c1->ejeX, c1->ejeY - 1);
				modificarCoordenada(c3, c2->ejeX -1, c2->ejeY);
			} else { // 1 2
				modificarCoordenada(c2, c1->ejeX -1, c1->ejeY);
				modificarCoordenada(c3, c2->ejeX -1, c2->ejeY);
			}
		}
		break;
	case 3:
		modificarCoordenada(c1, cActual->ejeX -1 , cActual->ejeY);
		if (sentido) { //Positivo
			if (orden) { // 2 1
				modificarCoordenada(c2, c1->ejeX -1, c1->ejeY);
				modificarCoordenada(c3, c2->ejeX , c2->ejeY+1);
			} else { // 1 2
				modificarCoordenada(c2, c1->ejeX, c1->ejeY +1);
				modificarCoordenada(c3, c2->ejeX, c2->ejeY +1);
			}
		} else {
			if (orden) { // 2 1
				modificarCoordenada(c2, c1->ejeX -1, c1->ejeY);
				modificarCoordenada(c3, c2->ejeX , c2->ejeY -1);
			} else { // 1 2
				modificarCoordenada(c2, c1->ejeX, c1->ejeY  -1);
				modificarCoordenada(c3, c2->ejeX, c2->ejeY  -1);
			}
		}
		break;
	}
	list_add_in_index(buffer, 0, c1);
	list_add_in_index(buffer, 1, c2);
	list_add_in_index(buffer, 2, c3);

//	return ret;
}

/**
 * @NAME: modificarCoordenada
 * @DESC: Modifica el valor de una coordenada ya creada (no hace malloc)
 */
void modificarCoordenada(coordenada_t* coordenada, int x, int y) {
	coordenada->ejeX = x;
	coordenada->ejeY = y;

}


//-----------------------------FUNCIONES PRIVADAS---------------------------------------

int randomNumber(int min_num, int max_num) {
	int result = 0, low_num = 0, hi_num = 0;
	if (min_num < max_num) {
		low_num = min_num;
		hi_num = max_num + 1;
	} else {
		low_num = max_num + 1;
		hi_num = min_num;
	}
	srand(time(NULL));
	result = (rand() % (hi_num - low_num)) + low_num;
	return result;
}

int obtenerDistanciaEnX(coordenada_t* inicio, coordenada_t* fin) {
	int distX = abs(inicio->ejeX - fin->ejeX);
	return distX;
}

int obtenerDistanciaEnY(coordenada_t* inicio, coordenada_t* fin) {
	int distY = abs(inicio->ejeY - fin->ejeY);
	return distY;
}

void desplazarEnY(coordenada_t* posicionActual, coordenada_t* posicionObjetivio) {

	if (posicionActual->ejeY > posicionObjetivio->ejeY) {
		desplazarEnYNegativo(posicionActual);
	} else {
		desplazarEnYPositivo(posicionActual);
	}
	return;
}

void desplazarEnX(coordenada_t* posicionActual, coordenada_t* posicionObjetivio) {

	if (posicionActual->ejeX > posicionObjetivio->ejeX) {
		desplazarEnXNegativo(posicionActual);
	} else {
		desplazarEnXPositivo(posicionActual);
	}
	return;
}



void desplazarEnXPositivo(coordenada_t* coordenada) {
	coordenada->ejeX++;
}



void desplazarEnXNegativo(coordenada_t* coordenada) {
	coordenada->ejeX--;
}



void desplazarEnYPositivo(coordenada_t* coordenada) {
	coordenada->ejeY++;
}



void desplazarEnYNegativo(coordenada_t* coordenada) {
	coordenada->ejeY--;
}

int esPar(int a) {
	return (a % 2 == 0);
}
