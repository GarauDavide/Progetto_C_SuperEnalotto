
/*
 * Fonte : https://www.ict.griffith.edu.au/teaching/2501ICT/archive/guide/ipc/flock.html
 * Dopo avere letto l'intera documentazione, ho preso spunto per implementare la seguente libreria
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

/***********************************************************************************************/
/********************************** DICHIARAZIONE METODI ***************************************/
/***********************************************************************************************/

char* create_message_log(char* ip);
void write_in_a_specific_file_with_lock(char* buffer, char* path);
void replace_all_content_in_a_specific_file_with_lock(char* buffer, char* path);
char* read_in_a_specific_file_with_lock(char* path);

/***********************************************************************************************/
/******************************* FINE DICHIARAZIONE METODI *************************************/
/***********************************************************************************************/

char* create_message_log(char* ip){
    time_t t;
    struct tm* date_time;
    char msg_log_tmp[150];
    char* msg_log;

    //Recupero la data odierna e locale
    time(&t);
    date_time = localtime(&t);

    sprintf(msg_log_tmp, "L'ip %s e' stato bannato dal server: %d-%d-%d %d:%d\n", ip,date_time->tm_year + 1900, date_time->tm_mon + 1, date_time->tm_mday, date_time->tm_hour, date_time->tm_min);

    msg_log = malloc(strlen(msg_log_tmp) + 1);

    strcpy(msg_log, msg_log_tmp);

    return msg_log;
}

void write_in_a_specific_file_with_lock(char* buffer, char* path){
    /* l_type   l_whence  l_start  l_len  l_pid   */
    struct flock fl = { F_WRLCK, SEEK_SET, 0,       0,     0 };
    int fd;
    int ret;

    fl.l_pid = getpid();

    fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0777); // 0777 -> Permessi se dovesse venir creato il file

    if(fd == -1){
        perror("open");
        exit(1);
    }

    // Qui i processi si bloccano in attesa di acquisire il locka al file

    if(fcntl(fd, F_SETLKW, &fl) == -1){
        perror("fcntl");
        exit(1);
    }

    /* HO ACQUISITO IL LOCK AL FILE */

    ret = write(fd, (void*)buffer, strlen(buffer));
    if(ret == -1){
        perror("write");
        exit(1);
    }

    /* TENTO DI LASCIARE IL LOCK */

    fl.l_type = F_UNLCK;  /* set to unlock same region */

    if(fcntl(fd, F_SETLKW, &fl) == -1){
        perror("fcntl");
        exit(1);
    }

    //Il lock è stato lasciato. Chiudo il file

    close(fd);
}

void replace_all_content_in_a_specific_file_with_lock(char* buffer, char* path){
    /* l_type   l_whence  l_start  l_len  l_pid   */
    struct flock fl = { F_WRLCK, SEEK_SET, 0,       0,     0 };
    int fd;
    int ret;

    fl.l_pid = getpid();

    fd = open(path, O_WRONLY | O_CREAT, 0777); // 0777 -> Permessi se dovesse venir creato il file

    if(fd == -1){
        perror("open");
        exit(1);
    }

    // Qui i processi si bloccano in attesa di acquisire il locka al file

    if(fcntl(fd, F_SETLKW, &fl) == -1){
        perror("fcntl");
        exit(1);
    }

    /* HO ACQUISITO IL LOCK AL FILE */

    ret = write(fd, (void*)buffer, strlen(buffer));
    if(ret == -1){
        perror("write");
        exit(1);
    }

    /* TENTO DI LASCIARE IL LOCK */

    fl.l_type = F_UNLCK;  /* set to unlock same region */

    if(fcntl(fd, F_SETLKW, &fl) == -1){
        perror("fcntl");
        exit(1);
    }

    //Il lock è stato lasciato. Chiudo il file

    close(fd);

}

char* read_in_a_specific_file_with_lock(char* path){
    /* l_type   l_whence  l_start  l_len  l_pid   */
    struct flock fl = { F_WRLCK, SEEK_SET, 0,       0,     0 };
    int fd;
    int ret;
    char* buffer;
    int buf_length;
    off_t fsize;


    fl.l_pid = getpid();

    fd = open(path, O_RDWR);

    if(fd == -1){
        perror("open");
        exit(1);
    }

    // Qui i processi si bloccano in attesa di acquisire il locka al file

    if(fcntl(fd, F_SETLKW, &fl) == -1){
        perror("fcntl");
        exit(1);
    }

    /* HO ACQUISITO IL LOCK AL FILE */

    fsize = lseek(fd, 0, SEEK_END);

    lseek(fd, 0, SEEK_SET);

    buffer = malloc((int)fsize + 1);

    ret = read(fd, (void*)buffer, (size_t)fsize);
    if(ret == -1){
        perror("write");
        exit(1);
    }

    buffer[fsize] = '\0';

    /* TENTO DI LASCIARE IL LOCK */

    fl.l_type = F_UNLCK;  /* set to unlock same region */

    if(fcntl(fd, F_SETLKW, &fl) == -1){
        perror("fcntl");
        exit(1);
    }

    //Il lock è stato lasciato. Chiudo il file

    close(fd);

    return buffer;
}
