#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <malloc.h>
#include "omp.h"
#include <string.h>


#define MAGIC_VALUE    0X4D42 

#define BITS_PER_PIXEL 24
#define NUM_PLANE      1
#define COMPRESSION    0
#define BITS_PER_BYTE  8

#define NUM_THREADS 200

#pragma pack(1)


typedef struct{
  uint16_t type;
  uint32_t size;
  uint16_t reserved1;
  uint16_t reserved2;
  uint32_t offset;
  uint32_t header_size;
  uint32_t width;
  uint32_t height;
  uint16_t planes;
  uint16_t bits;
  uint32_t compression;
  uint32_t imagesize;
  uint32_t xresolution;
  uint32_t yresolution;
  uint32_t importantcolours;
}BMP_Header;

typedef struct{
  BMP_Header header;
  unsigned int pixel_size;
  unsigned int width;
  unsigned int height;
  unsigned int bytes_per_pixel;
  unsigned char * pixel; 
}BMP_Image;


int checkHeader(BMP_Header *);
BMP_Image* cleanUp(FILE *, BMP_Image *);
BMP_Image* BMP_open(const char *);
int BMP_save(const BMP_Image *img, const char *filename);
void BMP_destroy(BMP_Image *img);


int checkHeader(BMP_Header *hdr){
  if((hdr -> type) != MAGIC_VALUE)           {printf("No es un bmp\n"); return 0;}
  if((hdr -> bits) != BITS_PER_PIXEL)        {printf("Revisa bit depth\n"); return 0;}
  if((hdr -> planes) != NUM_PLANE)           {printf("Array de diferente dimensiones\n"); return 0;}
  if((hdr -> compression) != COMPRESSION)    {printf("Hay compresion\n"); return 0;}
  return 1;
}

BMP_Image * cleanUp(FILE * fptr, BMP_Image * img)
{
  if (fptr != NULL)
    {
      fclose (fptr);
    }
  if (img != NULL)
    {
      if (img -> pixel != NULL)
	{
	  free (img -> pixel);
	}
      free (img);
    }
  return NULL;
}

BMP_Image* BMP_open(const char *filename){
  FILE *fptr = NULL;        // Puntero a FILE para manejar el archivo BMP
  BMP_Image *img = NULL;   // Puntero a estructura BMP_Image para almacenar la imagen BMP

  fptr = fopen(filename, "rb"); // Abrir el archivo en modo lectura binaria
  if(fptr == NULL){ // Verificar si el archivo se abrió correctamente
    printf("Archivo no existe\n"); // Imprimir mensaje de error
    return cleanUp(fptr,img); // Llamar a la función de limpieza y retornar NULL
  }

  img = malloc(sizeof(BMP_Image)); // Asignar memoria para la estructura BMP_Image
  if(img == NULL){ // Verificar si la asignación de memoria fue exitosa
    return cleanUp(fptr,img); // Llamar a la función de limpieza y retornar NULL
  }

  if(fread(&(img -> header), sizeof(BMP_Header),1,fptr) != 1) {
    // Leer el encabezado de la imagen BMP
    // Verificar si se pudo leer correctamente
    printf("Header no disponible\n"); // Imprimir mensaje de error
    return cleanUp(fptr,img); // Llamar a la función de limpieza y retornar NULL
  }

  if(checkHeader(&(img -> header)) == 0) {
    // Verificar si el encabezado cumple con el estándar de BMP
    printf("Header fuera del estandar\n"); // Imprimir mensaje de error
    return cleanUp(fptr,img); // Llamar a la función de limpieza y retornar NULL
  }

  img -> pixel_size = (img -> header).size - sizeof(BMP_Header);
  // Calcular el tamaño de los pixeles de la imagen BMP
  img -> width = (img -> header).width; // Obtener el ancho de la imagen BMP
  img -> height = (img -> header).height; // Obtener el alto de la imagen BMP
  img -> bytes_per_pixel = (img -> header).bits/BITS_PER_BYTE;
  // Calcular el número de bytes por pixel de la imagen BMP
  img -> pixel = malloc(sizeof(unsigned char) * (img -> pixel_size));
  // Asignar memoria para el arreglo de pixeles de la imagen BMP
  if((img -> pixel) == NULL){ // Verificar si la asignación de memoria fue exitosa
    printf("Imagen vacia\n"); // Imprimir mensaje de error
    return cleanUp(fptr,img); // Llamar a la función de limpieza y retornar NULL
  }

  if(fread(img->pixel, sizeof(char), img -> pixel_size,fptr) != (img -> pixel_size)){
    // Leer el contenido de los pixeles de la imagen BMP
    // Verificar si se pudo leer correctamente
    printf("Imagen con contenido irregular\n"); // Imprimir mensaje de error
    return cleanUp(fptr,img); // Llamar a la función de limpieza y retornar NULL
  }

  char onebyte;
  if(fread(&onebyte,sizeof(char),1,fptr) != 0) {
    // Verificar si hay bytes residuales en el archivo BMP
    printf("Hay pixeles residuales\n"); // Imprimir mensaje de error
    return cleanUp(fptr,img); // Llamar a la función de limpieza y retornar NULL
  }
  fclose(fptr);
  return img;
}

int BMP_save(const BMP_Image *img, const char *filename){
  FILE *fptr = NULL; // Puntero a archivo
  fptr = fopen(filename, "wb"); // Abrir archivo en modo escritura binaria
  if(fptr == NULL) {return 0;} // Si no se pudo abrir el archivo, retornar 0 indicando error
  if(fwrite(&(img -> header), sizeof(BMP_Header),1,fptr) != 1) {fclose(fptr); return 0;} // Escribir el encabezado en el archivo, si no se pudo escribir, cerrar el archivo y retornar 0 indicando error
  if(fwrite(img->pixel, sizeof(char), img -> pixel_size, fptr) != (img -> pixel_size)) {fclose(fptr); return 0;} // Escribir los datos de píxeles en el archivo, si no se pudo escribir todo, cerrar el archivo y retornar 0 indicando error
  fclose(fptr); // Cerrar el archivo
  return 1; // Retornar 1 indicando éxito en la escritura
}

//libera memoria
void BMP_destroy(BMP_Image *img){
  free (img -> pixel); 
  free (img);
}

//imprime las especificaciones de la miagen
void specs(BMP_Image* img){
  printf("Image width: %i\n", img->width);
  printf("Image height: %i\n", abs(img->height));
  printf("Image BPP: %i\n",  img->bytes_per_pixel);
  printf("Image size: %i\n",  img->pixel_size);
}

float ** kernel(unsigned int size){
  unsigned int height = size; // Altura de la matriz
  unsigned int width = size*3; // Ancho de la matriz, se multiplica por 3 para tener espacio para los valores de los canales RGB
  float ** matrix  = malloc(sizeof(float*)*height); // Reservar memoria para el arreglo de punteros a filas de la matriz
  for(int i = 0; i < height; i++){
    matrix[i] = malloc(sizeof(float)*width); // Reservar memoria para cada fila de la matriz
  }
  
  for(int i = 0; i<height; i++){
    for(int j = 0; j<width; j++){
      matrix[i][j] = (float)1.0/(size*size); // Inicializar todos los valores de la matriz con el valor 1/(size*size)
    }
  }
  return matrix; // Retornar el puntero a la matriz de convolución (kernel)
}

char ** pixelMat(BMP_Image * img){
  unsigned int height = img->height; // Altura de la imagen
  unsigned int width = img->width*3; // Ancho de la imagen, se multiplica por 3 para tener en cuenta los canales RGB
  char** mat = malloc(sizeof(char*) * (height)); // Reservar memoria para el arreglo de punteros a filas de la matriz
  for(int i = 0; i < height; i++){
    mat[i] = malloc(sizeof(char)*(width)); // Reservar memoria para cada fila de la matriz
  }
  
  // Inicio de una región paralela usando OpenMP
  #pragma omp parallel
  {
    // Bucle paralelo con programación dinámica y especificación de número de hilos
    #pragma omp for schedule(dynamic, NUM_THREADS) collapse(2)
    for(int i=0; i < height; i++){
      for(int j=0; j< width; j++){
        mat[i][j] = img->pixel[i*width+j]; // Asignar el valor del píxel correspondiente de la imagen a la matriz de píxeles
      }
    }
  }

  return mat; // Retornar el puntero a la matriz de píxeles
}

void BMP_blur(char* open, unsigned int size){
  BMP_Image * img = BMP_open(open); // Abrir la imagen BMP
  char** out_buffer = pixelMat(img); // Obtener la matriz de píxeles de la imagen
  float** kerSn = kernel(size); // Obtener el kernel de desenfoque

  unsigned int height = img->height; // Altura de la imagen
  unsigned int width = img->width * 3; // Ancho de la imagen, se multiplica por 3 para tener en cuenta los canales RGB

  int M = (size-1)/2; // Calculo del tamaño del margen del kernel

  omp_set_num_threads(NUM_THREADS); // Establecer el número de hilos a usar para la región paralela
  const double startTime = omp_get_wtime(); // Obtener el tiempo de inicio de la ejecución

  // Inicio de una región paralela usando OpenMP
  #pragma omp parallel
  {
    // Bucle paralelo con programación dinámica y especificación de número de hilos
    #pragma omp for schedule(dynamic, NUM_THREADS) collapse(1)
    for(int x=M;x<height-M;x++)
    {
      for(int y=M;y<width-M;y++)
      {
        float sum= 0.0;
        for(int i=-M;i<=M;++i)
        {
          for(int j=-M;j<=M;++j)
          {
            sum+=(float)kerSn[i+M][j+M]*img->pixel[(x+i)*width+(y+j)]; // Aplicar el filtro de desenfoque multiplicando el valor del píxel por el valor correspondiente del kernel
          }
        }
        out_buffer[x][y]=(char)sum; // Almacenar el resultado en el buffer de salida
      }
    }

    // Bucle paralelo para copiar el resultado del buffer de salida a la imagen original, excluyendo los márgenes
    #pragma omp for schedule(dynamic, NUM_THREADS)
    for(int i = 1; i<height-1; i++){
      for(int j = 1; j<width-1; j++){
        img->pixel[i*width+j] = out_buffer[i][j];
      }
    }
  }

  char* name;
  // Generar el nombre del archivo de salida dependiendo del archivo de entrada
  if(strcmp(open,"HorizontalRot.bmp") == 0){
    char filename[] = "Rblur0X.bmp";
    if(size < 10) filename[6]=size+'0';
    else{
      filename[5]= (size/10)+'0';
      filename[6]= (size%10)+'0';
    }
    name = filename; 
  }
  else{
    char filename[] = "Blur0X.bmp";
    if(size < 10) filename[5]=size+'0';
    else{
      filename[4]= (size/10)+'0';
      filename[5]= (size%10)+'0';
    }
    name = filename; 
  }

  // Guardar la imagen procesada en un archivo de salida
  if (BMP_save(img, name) == 0)
  {
    printf("Output file invalid!\n");
    BMP_destroy(img);
    free(kerSn);
    free(out_buffer);
  }
 
  BMP_destroy(img);
  free(kerSn);
  free(out_buffer);
  const double endTime = omp_get_wtime();
  printf("%s terminado en (%lf) segundos\n",name, (endTime - startTime)); //imprimir el tiempo estimado del proceso
}
  
int main(){
  omp_set_num_threads(NUM_THREADS);
  const double startTime = omp_get_wtime();

  #pragma omp sections
  {
    #pragma omp section
    BMP_blur("GrayScale.bmp",3);
    #pragma omp section
    BMP_blur("GrayScale.bmp",5);
    #pragma omp section
    BMP_blur("GrayScale.bmp",7);
    #pragma omp section
    BMP_blur("GrayScale.bmp",9);
    #pragma omp section
    BMP_blur("GrayScale.bmp",11);
    #pragma omp section
    BMP_blur("GrayScale.bmp",13);
    #pragma omp section
    BMP_blur("GrayScale.bmp",15);
    #pragma omp section
    BMP_blur("GrayScale.bmp",17);
    #pragma omp section
    BMP_blur("GrayScale.bmp",19);
    #pragma omp section
    BMP_blur("GrayScale.bmp",21);
    #pragma omp section
    BMP_blur("GrayScale.bmp",23);
    #pragma omp section
    BMP_blur("GrayScale.bmp",25);
    #pragma omp section
    BMP_blur("GrayScale.bmp",27);
    #pragma omp section
    BMP_blur("GrayScale.bmp",29);
    #pragma omp section
    BMP_blur("GrayScale.bmp",31);
    #pragma omp section
    BMP_blur("GrayScale.bmp",33);
    #pragma omp section
    BMP_blur("GrayScale.bmp",35);
    #pragma omp section
    BMP_blur("GrayScale.bmp",37);
    #pragma omp section
    BMP_blur("GrayScale.bmp",39);
    #pragma omp section
    BMP_blur("GrayScale.bmp",41);
    #pragma omp section
    BMP_blur("GrayScale.bmp",43);
    #pragma omp section
    BMP_blur("GrayScale.bmp",45);
    #pragma omp section
    BMP_blur("GrayScale.bmp",47);
    #pragma omp section
    BMP_blur("GrayScale.bmp",49);
    #pragma omp section
    BMP_blur("GrayScale.bmp",51);
    #pragma omp section
    BMP_blur("GrayScale.bmp",53);
    #pragma omp section
    BMP_blur("GrayScale.bmp",55);
    #pragma omp section
    BMP_blur("GrayScale.bmp",57);
    #pragma omp section
    BMP_blur("GrayScale.bmp",59);
    #pragma omp section
    BMP_blur("GrayScale.bmp",61);
    #pragma omp section
    BMP_blur("GrayScale.bmp",63);
    #pragma omp section
    BMP_blur("GrayScale.bmp",65);
    #pragma omp section
    BMP_blur("GrayScale.bmp",67);
    #pragma omp section
    BMP_blur("GrayScale.bmp",69);
    #pragma omp section
    BMP_blur("GrayScale.bmp",71);
    #pragma omp section
    BMP_blur("GrayScale.bmp",73);
    #pragma omp section
    BMP_blur("GrayScale.bmp",75);
    #pragma omp section
    BMP_blur("GrayScale.bmp",77);
    #pragma omp section
    BMP_blur("GrayScale.bmp",79);
    #pragma omp section
    BMP_blur("GrayScale.bmp",81);
  }
  return 0;
}