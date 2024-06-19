#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Estructura para representar un pixel como RGB
typedef struct {
    unsigned char r, g, b;
} Pixel;

// Prototipos para la organizacion del programa
Pixel** cargar_imagen(const char *nombre, int *ancho, int *alto);
void guardar_imagen(const char *nombre, Pixel **imagen, int ancho, int alto);
void crossfade(Pixel **imagen_origen, Pixel **imagen_destino, Pixel **imagen_frame, int ancho, int alto, float P);

int main(int argc, char **argv) {
    int ancho, alto, ancho2, alto2;
    // Defino la cantidad de frames que se desean, caso actual: se quieren 4 segundos a 24 frames por segundo
    int frames = (24 * 4) - 1;
    // Inicio MPI
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Carga de imagenes (para todos los procesos)
    Pixel **imagen_origen = cargar_imagen("tt1.bmp", &ancho, &alto);
    Pixel **imagen_destino = cargar_imagen("tt2.bmp", &ancho2, &alto2);

    // Validacion de dimensiones
    if (ancho != ancho2 || alto != alto2) {
        if (rank == 0) { // Proceso principal se encarga de hacer el Abort
            printf("Error de dimensiones de las imagenes\n");
            printf("Imagen 1: %d x %d\n", ancho, alto);
            printf("Imagen 2: %d x %d\n", ancho2, alto2);
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Division de P para asignar a cada proceso
    float *P_valores = (float *)malloc(frames * sizeof(float));
    for (int i = frames; i >= 0; i--) {
        P_valores[frames - i] = (float)i / (float)frames;
    }

    int frames_por_proceso = (frames + 1) / size;
    float *local_P_valores = (float *)malloc(frames_por_proceso * sizeof(float));

    // Disperso/Distribuyo ("Scatter") P a los procesos
    MPI_Scatter(P_valores, frames_por_proceso, MPI_FLOAT,
                local_P_valores, frames_por_proceso, MPI_FLOAT,
                0, MPI_COMM_WORLD);

    // Definicion de local_frame, donde se guardara cada iteracion
    Pixel **local_frame = (Pixel **)malloc(alto * sizeof(Pixel *));
    for (int y = 0; y < alto; y++) {
        local_frame[y] = (Pixel *)malloc(ancho * sizeof(Pixel));
    }

    /** Inicio de la ejecucion */
    // Captura del tiempo de sistema (Linux) para calcular el tiempo de ejecucion.
    struct timespec tiempo_comienzo, tiempo_fin;
    double tiempo_diferencia;
    clock_gettime(CLOCK_REALTIME, &tiempo_comienzo);

    for (int i = 0; i < frames_por_proceso; i++) {
        float P = local_P_valores[i]; //Recupero el P que corresponde
        crossfade(imagen_origen, imagen_destino, local_frame, ancho, alto, P);
        char filename[256];
        // Formateo el nombre del frame actual a ser guardado para guardar local_frame
        sprintf(filename, "frame_%04d_%d.bmp", i + rank * frames_por_proceso, rank);
        guardar_imagen(filename, local_frame, ancho, alto);
    }

    // Captura del tiempo de sistema.
    clock_gettime(CLOCK_REALTIME, &tiempo_fin);
    // Calculo del tiempo de ejecucion
    tiempo_diferencia = (tiempo_fin.tv_sec - tiempo_comienzo.tv_sec) +
                        (tiempo_fin.tv_nsec - tiempo_comienzo.tv_nsec) / 1e9;
    printf("Proceso %d: Se ejecuto en: %.10f segundos\n", rank, tiempo_diferencia);
    /** Fin de la ejecucion */

    // Limpieza de memoria
    for (int y = 0; y < alto; y++) {
        free(local_frame[y]);
        free(imagen_origen[y]);
        free(imagen_destino[y]);
    }
    free(local_frame);
    free(local_P_valores);
    free(P_valores);
    free(imagen_origen);
    free(imagen_destino);
    MPI_Finalize();
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
    Pixel **imagen_configurada = (Pixel **)malloc(*alto * sizeof(Pixel *));
    for (int y = 0; y < *alto; y++) {
        imagen_configurada[y] = (Pixel *)malloc(*ancho * sizeof(Pixel));
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
    unsigned char *data = (unsigned char *)malloc(ancho * alto * 3);
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