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
#include <sys/time.h>
#include <sys/resource.h>

// Estructuras y definiciones
#define MAX_PROCESS 20  // Número máximo de procesos en el pool
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

void child_process(const char *src_dir, const char *dst_dir, int msgid, pid_t pid) {
	struct msgbuf msg;
	char src_path[MSG_SIZE], dst_path[MSG_SIZE];
	struct timeval start, end;
	double file_time;
	int files_processed = 0;

	while (1) {
		if (msgrcv(msgid, &msg, MSG_SIZE, 0, 0) == -1) {
			perror("msgrcv");
    	continue;
		}

		if (strcmp(msg.mtext, "FIN") == 0) {
			printf("Proceso %d: Procesados %d archivos\n", pid, files_processed);
			break;  // Finalizar proceso
		}

		strcpy(src_path, msg.mtext);
		snprintf(dst_path, MSG_SIZE, "%s/%s", dst_dir, basename(src_path));

		gettimeofday(&start, NULL);
		copy_file(src_path, dst_path);
		gettimeofday(&end, NULL);
		file_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    // Escribir en el archivo de bitácora
    FILE *logfile = fopen("logfile.csv", "a");
    if (logfile == NULL) {
      perror("fopen logfile");
      exit(EXIT_FAILURE);
    }
    fprintf(logfile, "%s,%d,%.6f\n", basename(src_path), pid, file_time);
    fflush(logfile);
    fclose(logfile);

		printf("Proceso %d: Copiado %s en %.6f segundos\n", pid, src_path, file_time);
		files_processed++;
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

  // Escribir encabezado en el archivo de bitácora
  FILE *logfile = fopen("logfile.csv", "w");
  if (logfile == NULL) {
    perror("fopen logfile");
    exit(EXIT_FAILURE);
  }
  fprintf(logfile, "Archivo,Subproceso,Tiempo\n");
  fclose(logfile);
  
	// Crear pool de procesos
	pid_t pids[MAX_PROCESS];
  for (int i = 0; i < MAX_PROCESS; ++i) {
		pids[i] = fork();
		if (pids[i] < 0) {
		  perror("fork");
		  exit(EXIT_FAILURE);
		}

		if (pids[i] == 0) { // Proceso hijo
			child_process(src_dir, dst_dir, msgid, getpid());
			exit(EXIT_SUCCESS);
		}
  }
  
	struct dirent *dp;
	struct msgbuf msg;
	struct timeval start, end;
	struct rusage usage;
	double elapsed;
	int current_process = 0;

  gettimeofday(&start, NULL);

  while ((dp = readdir(dirp)) != NULL) {
  	if (dp->d_type == DT_REG) {
  		snprintf(msg.mtext, MSG_SIZE, "%s/%s", src_dir, dp->d_name);
  		msg.mtype = current_process + 1; // Asignar a un proceso hijo
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

	gettimeofday(&end, NULL);
	elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

	getrusage(RUSAGE_CHILDREN, &usage);

	// Analizar rendimiento
	printf("Análisis de rendimiento:\n");
	printf("- Tiempo de ejecución: %.6f segundos\n", elapsed);
	printf("- Tiempo de CPU utilizado: %ld.%06ld segundos\n", usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
	printf("- Grado de paralelismo: %d procesos\n", MAX_PROCESS);
  
  // Cerrar directorio y cola de mensajes
	closedir(dirp);
	msgctl(msgid, IPC_RMID, NULL);
  return 0;   
}
