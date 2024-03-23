#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

// Estructuras y definiciones
#define MAX_PROCESS 10  // Número máximo de procesos en el pool
#define MSG_SIZE 256    // Tamaño del mensaje

// Estructura para mensajes
struct msgbuf {
    long mtype;             // Tipo de mensaje
    char mtext[MSG_SIZE];   // Contenido del mensaje
};

int main(int argc, char *argv[]) {
	// Verificar argumentos
	if (argc != 3) {
		fprintf(stderr, "Uso: %s directorio_origen directorio_destino\n", argv[0]);
		exit(EXIT_FAILURE);
  }

  char *src_dir = argv[1];
  char *dst_dir = argv[2];
  
  
  // Abrir directorio origen
  DIR *dirp = opendir(src_dir);
  if (dirp == NULL) {
      perror("opendir");
      exit(EXIT_FAILURE);
  }
  
  
  // Crear cola de mensajes
	int msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
	if (msgid == -1) {
		  perror("msgget");
		  exit(EXIT_FAILURE);
	}
  // Crear pool de procesos
  // Bucle principal para asignar archivos a procesos
  // Esperar a que todos los procesos hijos terminen
  // Analizar rendimiento
  return 0;   
}
// Funciones adicionales aquí
