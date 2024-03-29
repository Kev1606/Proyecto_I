#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <libgen.h>

// Estructuras y definiciones
#define MAX_PROCESS 10  // Número máximo de procesos en el pool
#define MSG_SIZE 256    // Tamaño del mensaje

// Estructura para mensajes
struct msgbuf {
    long mtype;             // Tipo de mensaje
    char mtext[MSG_SIZE];   // Contenido del mensaje
};
void copy_file(const char *src_path, const char *dst_path) {
	FILE *src = fopen(src_path, "rb");
	if (src == NULL) {
		perror("fopen src");
		return;
	}
	struct stat st;
	FILE *dst = fopen(dst_path, "wb");
	if (dst == NULL) {
		perror("fopen dst");
		fclose(src);
		return;
	}

	char buffer[1024];
	size_t bytes;
	//Imprimir el nombre del archivo y su tamaño
	stat(src_path, &st);
	printf("copiando %s (%ld bytes)\n", src_path, st.st_size);
	while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
  	fwrite(buffer, 1, bytes, dst);
	}

	fclose(src);
	fclose(dst);
}

void child_process(const char *src_dir, const char *dst_dir, int msgid) {
	struct msgbuf msg;
	char src_path[MSG_SIZE], dst_path[MSG_SIZE];

	while (1) {
		if (msgrcv(msgid, &msg, MSG_SIZE, 0, 0) == -1) {
			perror("msgrcv");
    	continue;
		}

		if (strcmp(msg.mtext, "FIN") == 0) {
			break;  // Finalizar proceso
		}

		strcpy(src_path, msg.mtext);
		snprintf(dst_path, MSG_SIZE, "%s/%s", dst_dir, basename(src_path));

		copy_file(src_path, dst_path);
	}
}

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
	pid_t pids[MAX_PROCESS];
  for (int i = 0; i < MAX_PROCESS; ++i) {
		pids[i] = fork();
		if (pids[i] < 0) {
		  perror("fork");
		  exit(EXIT_FAILURE);
		}

		if (pids[i] == 0) { // Proceso hijo
			child_process(src_dir, dst_dir, msgid);
			exit(EXIT_SUCCESS);
		}
  }
  
	struct dirent *dp;
	struct msgbuf msg;
	int current_process = 0;

	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_type == DT_REG) {
		  snprintf(msg.mtext, MSG_SIZE, "%s/%s", src_dir, dp->d_name);
		  msg.mtype = current_process + 1;  // Asignar a un proceso hijo

		  if (msgsnd(msgid, &msg, strlen(msg.mtext) + 1, 0) == -1) {
		    perror("msgsnd");
		    exit(EXIT_FAILURE);
		  }

		  current_process = (current_process + 1) % MAX_PROCESS;
		}
	}

	// Enviar mensaje de finalización a cada proceso hijo
	for (int i = 0; i < MAX_PROCESS; ++i) {
		msg.mtype = i + 1;
		strcpy(msg.mtext, "FIN");
		msgsnd(msgid, &msg, strlen(msg.mtext) + 1, 0);
	}
	
	for (int i = 0; i < MAX_PROCESS; ++i) {
 		wait(NULL);
	}
	
  // Analizar rendimiento
  
  // Cerrar directorio y cola de mensajes
	closedir(dirp);
	msgctl(msgid, IPC_RMID, NULL);
  return 0;   
}
