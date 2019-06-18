#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/wait.h>
#include "common.h"

#define WRITERS_COUNT  2
#define READERS_COUNT  3
#define WRITE_MUTEX    "/write_mutex"
#define READ_MUTEX     "/read_mutex"
#define MSG_COUNT 12
#define MSG_ELEMS (64 * PIPE_BUF)

int pipefd[2];

int write_to_pipe(int fd, const void *data, size_t data_len) {

    /**
     * COMPLETARE QUI
     *
     * Obiettivi:
     * - scrivere sul descrittore 'fd' in input i primi 'data_len' bytes
     *   contenuti in 'data'
     * - gestire eventuali interruzioni ed errori
     * - assicurarsi che tutti i 'data_len' byte siano stati scritti
     * - restituire il numero di bytes scritti
     **/

    int written_bytes = 0, ret;

    while (written_bytes < data_len) {
        ret = write(fd, data + written_bytes, data_len - written_bytes);
        if (ret == -1 && errno == EINTR) continue;
        if (ret == -1) handle_error("error writing to pipe");
        written_bytes += ret;
    }
    return written_bytes;
}

int read_from_pipe(int fd, void *data, size_t data_len) {

    /**
     * COMPLETARE QUI
     *
     * Obiettivi:
     * - leggere dal descrittore 'fd' in input 'data_len' bytes e
     *   memorizzarli in 'data'
     * - gestire eventuali interruzioni ed errori
     * - in caso di chiusura inaspettata della pipe terminare il
     *   programma con un errore
     * - assicurarsi che tutti i 'data_len' bytes siano stati letti
     * - restituire il numero di bytes letti
     **/

    int read_bytes = 0, ret;
    while (read_bytes < data_len) {
        ret = read(fd, data + read_bytes, data_len - read_bytes);
        if (ret == -1 && errno == EINTR) continue;
        if (ret == -1) handle_error("error reading from pipe");
        if (ret ==  0) handle_error("unexpected close of the pipe");
        read_bytes += ret;
    }
    return read_bytes;
}

/**
 * Questa funzione scrive nell'array di interi 'data' un numero pari a
 * 'elem_count' di interi, tutti aventi lo stesso valore 'value'.
 **/
void create_msg(int *data, int elem_count, int value) {
    int i;
    for (i = 0; i < elem_count; i++) {
        data[i] = value;
    }
}

/**
 * Questa funzione verifica che un messaggio sia integro controllando
 * che tutti gli 'elem_count' valori interi contenuti nell'array 'data'
 * abbiano lo stesso valore.
 * In caso positivo, restituisce 1, altrimenti 0.
 **/
int is_msg_ok(const int *data, int elem_count) {
    int i;
    for (i = 0; i < elem_count; i++)
        if (data[0] != data[i])
            return 0;
    return 1;
}

void reader(int reader_id, sem_t* read_mutex) {
    int data[MSG_ELEMS];
    printf("[READER_%d] processo reader creato.\n", reader_id);

    /**
     * COMPLETARE QUI
     *
     * Obiettivi:
     * - chiudere i descrittori non necessari
     * - gestire eventuali errori
     **/
     
    int ret = close(pipefd[1]);
    if(ret) handle_error("error closing pipe");
    
    int i = 0;
    for ( ; i < MSG_COUNT/READERS_COUNT; i++) {

        ret = sem_wait(read_mutex);
        if(ret) handle_error("error waiting on read mutex");
        
        read_from_pipe(pipefd[0], data, sizeof(data)); // sizeof(data) == MSG_ELEMS * sizeof(int)

        ret = sem_post(read_mutex);
        if(ret) handle_error("error posting on read mutex");

        printf("[READER_%d] Letto msg #%d con valore %d\n", reader_id, i, data[0]);
        if (!is_msg_ok(data, MSG_ELEMS))
            printf("corrupted message!!!\n");
    }

    ret = sem_close(read_mutex);
    if(ret) handle_error("error closing read mutex");

    /**
     * COMPLETARE QUI
     *
     * Obiettivi:
     * - chiudere i descrittori rimanenti
     * - gestire eventuali errori
     **/
     
    ret = close(pipefd[0]);
    if(ret) handle_error("error closing pipe");

}

void writer(int writer_id, sem_t* write_mutex) {
    int data[MSG_ELEMS];
    int i,ret;
    printf("[WRITER_%d] processo writer creato.\n", writer_id);
    
    /**
     * COMPLETARE QUI
     *
     * Obiettivi:
     * - chiudere i descrittori non necessari
     * - gestire eventuali errori
     **/
     
    ret = close(pipefd[0]);
    if(ret) handle_error("error closing pipe");
    
    for (i = 0 ; i < MSG_COUNT/WRITERS_COUNT; i++) {
        create_msg(data, MSG_ELEMS, i);
        ret = sem_wait(write_mutex);
        if(ret) handle_error("error waiting on write mutex");
        
        write_to_pipe(pipefd[1], data, sizeof(data)); // sizeof(data) == MSG_ELEMS * sizeof(int)
        
        ret = sem_post(write_mutex);
        if(ret) handle_error("error posting on write mutex");
        
        printf("[WRITER_%d] Inviato il msg #%d\n", writer_id, i);
     }

    ret = sem_close(write_mutex);
    if(ret) handle_error("error closing write mutex");

    /**
     * COMPLETARE QUI
     *
     * Obiettivi:
     * - chiudere i descrittori rimanenti
     * - gestire eventuali errori
     **/
     
    ret = close(pipefd[1]);
    if(ret) handle_error("error closing pipe");


}

int main(int argc, char* argv[]) {
    int ret,i;
    pid_t pid;

    sem_unlink(READ_MUTEX);
    sem_t *read_mutex = sem_open(READ_MUTEX, O_CREAT|O_EXCL, 0600, 1);
    if(read_mutex == SEM_FAILED) handle_error("Error Creating Read Mutex");
    
    sem_unlink(WRITE_MUTEX);
    sem_t *write_mutex = sem_open(WRITE_MUTEX, O_CREAT|O_EXCL, 0600, 1);
    if(write_mutex == SEM_FAILED) handle_error("Error Creating Write Mutex");
    
    /**
     * COMPLETARE QUI
     *
     * Obiettivi:
     * - creare una pipe nella variabile globale 'pipefd'
     * - gestire eventuali errori
     **/

    ret = pipe(pipefd);
    if(ret) handle_error("error creating the pipe");

    for (i = 0; i < READERS_COUNT; i++) {
        pid = fork();
        if (pid == -1) handle_error("Error creating reader");
        if (pid == 0) {
            reader(i,read_mutex);
            _exit(0);
        }
    }

    ret = sem_close(read_mutex);
    if(ret) handle_error("Error closing read mutex");

    for (i = 0; i < WRITERS_COUNT; i++) {
        pid = fork();
        if (pid == -1) handle_error("error creating reader");
        if (pid == 0) {
            writer(i,write_mutex);
            _exit(0);
        }
    }
    
    ret = sem_close(write_mutex);
    if(ret) handle_error("Error closing write mutex");

    // NOTA: per pulizia chiudiamo i descrittori anche nel main process
    ret = close(pipefd[0]);
    if(ret) handle_error("error closing pipe");
    ret = close(pipefd[1]);
    if(ret) handle_error("error closing pipe");

    // shutdown phase
    for (i = 0 ; i < READERS_COUNT + WRITERS_COUNT; i++) {
        int status;
        ret = wait(&status);
        if(ret == -1) handle_error("error waiting for a child to terminate");
        if (WEXITSTATUS(status)) {
            handle_error("ERROR: child process died unexpectedly");
        }
    }

    printf("[PARENT] processi figlio terminati.\n");

    ret = sem_unlink(READ_MUTEX);
    if(ret) handle_error("error removing read mutex");

    ret = sem_unlink(WRITE_MUTEX);
    if(ret) handle_error("error removing write mutex");

    return 0;
}
