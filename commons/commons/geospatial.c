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
	distY = obtenerDistanciaEnX(inicio, fin);
	return distX + distY;
}

/**
 * @NAME: coordenadaMovimientoAlternado
 * @DESC: Retorna la coordenada de la proxima posicion a moverse
 */
coordenada_t* coordenadaMovimientoAlternado(coordenada_t* posicionActual,
		coordenada_t* posicionObjetivio) {
	int flag = posicionActual->ejeX % 2;
	coordenada_t* posicionSiguiente = posicionActual;

	if (obtenerDistancia(posicionActual, posicionObjetivio) != 0) { //Validacion en el objetivo
		return posicionActual;
	}

	if (obtenerDistanciaEnX(posicionActual, posicionObjetivio) == 0) { //Validacion Caso solo por Y
		posicionSiguiente = desplazarEnY(posicionActual, posicionObjetivio);
		return posicionSiguiente;
	}

	if (obtenerDistanciaEnY(posicionActual, posicionObjetivio) == 0) { //Validacion Caso solo por x
		posicionSiguiente = desplazarEnX(posicionActual, posicionObjetivio);
		return posicionSiguiente;
	}

	if (flag) {
		posicionSiguiente = desplazarEnY(posicionActual, posicionObjetivio);
	} else {
		posicionSiguiente = desplazarEnX(posicionActual, posicionObjetivio);
	}
	return posicionSiguiente;

}

/**
 * @NAME: indicacionMovimientoAlternado
 * @DESC: Retorna la indicacion de la proxima posicion a moverse
 */

indicacion_t* indicacionMovimientoAlternado(coordenada_t* posicionActual,
		coordenada_t* posicionObjetivio) {
	indicacion_t* indicacion = malloc(sizeof(indicacion_t));
	coordenada_t* posicionSiguiente = coordenadaMovimientoAlternado(
			posicionActual, posicionObjetivio);
	if (coordenadasIguales(posicionSiguiente, posicionActual)) {
		return indicacion;
	}

	if (posicionSiguiente->ejeX == posicionActual->ejeX) {
		indicacion->eje = "EjeY";
		indicacion->sentido =
				posicionSiguiente->ejeY > posicionActual->ejeY ? "+" : "-";
	} else {
		indicacion->eje = "EjeX";
		indicacion->sentido =
				posicionSiguiente->ejeX > posicionActual->ejeX ? "+" : "-";
	}

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
 * @NAME: coordenadaEvasion
 * @DESC: Retorna la proxima coordenada en funcion de las posicion a evadir y del
 * objetivo.
 */
coordenada_t* coordenadaEvasion(coordenada_t* obstaculo, coordenada_t* objetivo,
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

	return posActual;
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
 * @NAME: movimientoL
 * @DESC: retorna un lista con las 3 posiciones de un movimiento en "L" al azar de las
 * 18 posibles.
 */
t_list* movimientoL(coordenada_t* cActual) {
	int orden, direccion, sentido;
	coordenada_t* c1, *c2, *c3;
	t_list* ret = list_create();

	c1 = malloc(sizeof(coordenada_t));
	c2 = malloc(sizeof(coordenada_t));
	c3 = malloc(sizeof(coordenada_t));

	orden = randomNumber(0, 1);
	direccion = randomNumber(0, 3);
	sentido = randomNumber(0, 1);

	switch (direccion) {
	case 0:
		c1 = desplazarEnYPositivo(cActual);
		if (sentido) { //Positivo
			if (orden) { // 2 1
				c2 = desplazarEnYPositivo(c1);
				c3 = desplazarEnXPositivo(c2);
			} else { // 1 2
				c2 = desplazarEnXPositivo(c1);
				c3 = desplazarEnXPositivo(c2);
			}
		} else {
			if (orden) { // 2 1
				c2 = desplazarEnYPositivo(c1);
				c3 = desplazarEnXNegativo(c2);
			} else { // 1 2
				c2 = desplazarEnXNegativo(c1);
				c3 = desplazarEnXNegativo(c2);
			}
		}
		break;
	case 1:
		c1 = desplazarEnXPositivo(cActual);
		if (sentido) { //Positivo
			if (orden) { // 2 1
				c2 = desplazarEnXPositivo(c1);
				c3 = desplazarEnYPositivo(c2);
			} else { // 1 2
				c2 = desplazarEnYPositivo(c1);
				c3 = desplazarEnYPositivo(c2);
			}
		} else {
			if (orden) { // 2 1
				c2 = desplazarEnXPositivo(c1);
				c3 = desplazarEnYNegativo(c2);
			} else { // 1 2
				c2 = desplazarEnYNegativo(c1);
				c3 = desplazarEnYNegativo(c2);
			}
		}
		break;
	case 2:
		c1 = desplazarEnYNegativo(cActual);
		if (sentido) { //Positivo
			if (orden) { // 2 1
				c2 = desplazarEnYNegativo(c1);
				c3 = desplazarEnXPositivo(c2);
			} else { // 1 2
				c2 = desplazarEnXPositivo(c1);
				c3 = desplazarEnXPositivo(c2);
			}
		} else {
			if (orden) { // 2 1
				c2 = desplazarEnYNegativo(c1);
				c3 = desplazarEnXNegativo(c2);
			} else { // 1 2
				c2 = desplazarEnXNegativo(c1);
				c3 = desplazarEnXNegativo(c2);
			}
		}
		break;
	case 3:
		c1 = desplazarEnXNegativo(cActual);
		if (sentido) { //Positivo
			if (orden) { // 2 1
				c2 = desplazarEnXNegativo(c1);
				c3 = desplazarEnYPositivo(c2);
			} else { // 1 2
				c2 = desplazarEnYPositivo(c1);
				c3 = desplazarEnYPositivo(c2);
			}
		} else {
			if (orden) { // 2 1
				c2 = desplazarEnXNegativo(c1);
				c3 = desplazarEnYNegativo(c2);
			} else { // 1 2
				c2 = desplazarEnYNegativo(c1);
				c3 = desplazarEnYNegativo(c2);
			}
		}
		break;
	}
	list_add_in_index(ret, 0, c1);
	list_add_in_index(ret, 1, c2);
	list_add_in_index(ret, 2, c3);

	return ret ;
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
	srand(time(NULL ));
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

coordenada_t* desplazarEnY(coordenada_t* posicionActual,
		coordenada_t* posicionObjetivio) {
	coordenada_t* posicionSiguiente;

	if (posicionActual->ejeY > posicionObjetivio->ejeY) {
		posicionSiguiente = desplazarEnYNegativo(posicionActual);
	} else {
		posicionSiguiente = desplazarEnYPositivo(posicionActual);
	}
	return posicionSiguiente;
}

coordenada_t* desplazarEnX(coordenada_t* posicionActual,
		coordenada_t* posicionObjetivio) {
	coordenada_t* posicionSiguiente;
	if (posicionActual->ejeX > posicionObjetivio->ejeX) {
		posicionSiguiente = desplazarEnXNegativo(posicionActual);
	} else {
		posicionSiguiente = desplazarEnXPositivo(posicionActual);
	}
	return posicionSiguiente;
}

coordenada_t* desplazarEnXPositivo(coordenada_t* coordenada) {
	coordenada_t* ret = malloc(sizeof(coordenada_t));
	ret->ejeX = coordenada->ejeX++;
	ret->ejeY = coordenada->ejeY;
	return ret;
}

coordenada_t* desplazarEnXNegativo(coordenada_t* coordenada) {
	coordenada_t* ret = malloc(sizeof(coordenada_t));
	ret->ejeX = coordenada->ejeX--;
	ret->ejeY = coordenada->ejeY;
	return ret;
}

coordenada_t* desplazarEnYPositivo(coordenada_t* coordenada) {
	coordenada_t* ret = malloc(sizeof(coordenada_t));
	ret->ejeX = coordenada->ejeX;
	ret->ejeY = coordenada->ejeY++;
	return coordenada;
}

coordenada_t* desplazarEnYNegativo(coordenada_t* coordenada) {
	coordenada_t* ret = malloc(sizeof(coordenada_t));
	ret->ejeX = coordenada->ejeX;
	ret->ejeY = coordenada->ejeY--;
	return coordenada;
}

