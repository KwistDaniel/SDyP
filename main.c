#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Estructura para representar un pixel como RGB
typedef struct {
    unsigned char r, g, b;
} Pixel;

//Prototipos para la organizacion del programa
Pixel** cargar_imagen(const char *nombre, int *ancho, int *alto);
void guardar_imagen(const char *nombre, Pixel **imagen, int ancho, int alto);
void crossfade(Pixel **imagen_origen, Pixel **imagen_destino, Pixel **imagen_frame, int ancho, int alto, float P);

int main() {
    int ancho, alto, ancho2, alto2;
    // Defino la cantidad de frames que se desean, caso actual: se quieren 4 segundos a 24 frames por segundo
    int frames = (24 * 4) - 1;

    // Carga de imagenes
    Pixel **imagen_origen = cargar_imagen("tt1.bmp", &ancho, &alto);
    Pixel **imagen_destino = cargar_imagen("tt2.bmp", &ancho2, &alto2);

    // Validacion de dimensiones
    if(ancho !=ancho2 || alto != alto2 ){
        printf("Error de dimensiones de las imagenes\n");
        printf("Imagen 1: %d x %d\n",ancho,alto);
        printf("Imagen 2: %d x %d\n",ancho2,alto2);
        exit(1);
    }
    // Definicion de imagen_frame, donde se guardara cada iteracion
    Pixel **imagen_frame = (Pixel**)malloc(alto * sizeof(Pixel*));
    for (int y = 0; y < alto; y++) {
        imagen_frame[y] = (Pixel*)malloc(ancho * sizeof(Pixel));
    }

    /** Inicio de la ejecucion */
    // Captura del tiempo de sistema (Linux) para calcular el tiempo de ejecucion.
    struct timespec tiempo_comienzo, tiempo_fin;
    double tiempo_diferencia;
    clock_gettime(CLOCK_REALTIME, &tiempo_comienzo);

    for (int i = frames; i >= 0; i--) {
        float P = (float)i / (float)frames; //P que especifica el fade
        crossfade(imagen_origen, imagen_destino, imagen_frame, ancho, alto, P);
        char filename[256];
        // Formateo el nombre del frame actual a ser guardado para guardar imagen_frame localmente
        sprintf(filename, "frame_%04d.bmp", (frames - i));
        guardar_imagen(filename, imagen_frame, ancho, alto);
    }

    // Captura del tiempo de sistema.
    clock_gettime(CLOCK_REALTIME, &tiempo_fin);
    // Calculo del tiempo de ejecucion
    tiempo_diferencia = (tiempo_fin.tv_sec - tiempo_comienzo.tv_sec) +
                (tiempo_fin.tv_nsec - tiempo_comienzo.tv_nsec) / 1e9;
    printf("La ejecucion se completo en: %.10f segundos\n", tiempo_diferencia);
    /** Fin de la ejecucion */

    // Limpieza de memoria
    for (int y = 0; y < alto; y++) {
        free(imagen_origen[y]);
        free(imagen_destino[y]);
        free(imagen_frame[y]);
    }
    free(imagen_origen);
    free(imagen_destino);
    free(imagen_frame);
    return 0;
}

/** Funciones de cargado y guardado **/
Pixel** cargar_imagen(const char *nombre, int *ancho, int *alto) {
    int canales;
    unsigned char *imagen_fuente = stbi_load(nombre, ancho, alto, &canales, 3);
    if (!imagen_fuente) {
        printf("No se pudo cargar la imagen: %s\n", nombre);
        exit(1);
    }
    Pixel **imagen_configurada = (Pixel**)malloc(*alto * sizeof(Pixel*));
    for (int y = 0; y < *alto; y++) {
        imagen_configurada[y] = (Pixel*)malloc(*ancho * sizeof(Pixel));
        for (int x = 0; x < *ancho; x++) {
            int indice_canal = (y * *ancho + x) * 3;
            imagen_configurada[y][x].r = imagen_fuente[indice_canal];
            imagen_configurada[y][x].g = imagen_fuente[indice_canal + 1];
            imagen_configurada[y][x].b = imagen_fuente[indice_canal + 2];
        }
    }
    stbi_image_free(imagen_fuente);
    return imagen_configurada;
}

void guardar_imagen(const char *nombre, Pixel **imagen, int ancho, int alto) {
    unsigned char *data = (unsigned char*)malloc(ancho * alto * 3);
    for (int y = 0; y < alto; y++) {
        for (int x = 0; x < ancho; x++) {
            int indice_canal = (y * ancho + x) * 3;
            data[indice_canal] = imagen[y][x].r;
            data[indice_canal + 1] = imagen[y][x].g;
            data[indice_canal + 2] = imagen[y][x].b;
        }
    }
    stbi_write_png(nombre, ancho, alto, 3, data, ancho * 3);
    free(data);
}

void crossfade(Pixel **imagen_origen, Pixel **imagen_destino, Pixel **imagen_frame, int ancho, int alto, float P) {
    for (int y = 0; y < alto; y++) {
        for (int x = 0; x < ancho; x++) {
            // Recorriendo la matriz de la imagen, en cada posicion hago el fade del P correspondiente para los 3 canales (RGB) entre las dos imagenes.
            imagen_frame[y][x].r = (unsigned char)(imagen_origen[y][x].r * P + imagen_destino[y][x].r * (1 - P));
            imagen_frame[y][x].g = (unsigned char)(imagen_origen[y][x].g * P + imagen_destino[y][x].g * (1 - P));
            imagen_frame[y][x].b = (unsigned char)(imagen_origen[y][x].b * P + imagen_destino[y][x].b * (1 - P));
        }
    }
}