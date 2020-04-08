#include <stdio.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <stdlib.h>

#define PROCS_DIR "/proc"

int main(int argc, char *argv[]) 
{
    /* the identifier for the shared memory segment */
	int segment_id;
	/* a pointer to the shared memory segment */
	char* shared_memory;

    if(argc != 2) {
        printf("Usage: ./process_manager <segment_id>");
        exit(-1);
    }

	segment_id = atoi(argv[1]);
    //printf("\nSegment ID in process manager : %d\n", segment_id);

	/** attach the shared memory segment */
	shared_memory = (char *) shmat(segment_id, NULL, 0);
	//printf("shared memory segment %d attached at address %p\n", segment_id, shared_memory);

    while(1) 
    {   
        // pegar informações summary no /proc/stat 
        // normal_process_user %s
        // niced_process_user %s
        // process_kernel %s
        // procs_running %s
        // 
        // rodar todos os processos
        // abrir um por um
        // /proc/%pid/stat:
        // pid %d
        // command %s (palemoon)
        // state %c Verificar state válido? Incrementa contador e addiciona processo à lista : break;
        // Remover o lixo do caminho %d%d%d%d%d%u%lu%lu%lu%lu
        // Ler todos os status de TIME e somá-los %lu+%lu+%lu+%lu
        // Ler Priority %ld
        // Ler Nice     %ld
        // 
    }
		
	/** now detach the shared memory segment */ 
	if ( shmdt(shared_memory) == -1) {
		fprintf(stderr, "Unable to detach\n");
	}

	return 0;
}
