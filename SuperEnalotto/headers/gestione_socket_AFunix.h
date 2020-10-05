
/*
 *  Fonte : https://www.man7.org/linux/man-pages/man7/unix.7.html
 *  Dopo avere letto l'intera documentazione, ho preso spunto per implementare la seguente libreria
 */

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define BUF_SIZE 256

/*  Struttura dati con cui i processi figli dialogano con il processo padre per
 *  comunicare tutti i cambiamenti provocati dalla gestione delle richieste del
 *  client. Modifica liste connessioni, modifica stati ecc ecc.
 */
struct message_son{
  char text1[BUF_SIZE]; // Informazioni generiche testuali 1
  char text2[BUF_SIZE]; // Informazioni generiche testuali 2
  int comand_code;  // Informazione codice del comando per far capire al processo padre cosa vuole comunicargli il processo figlio
  int sock_for_father;  //  Informazione socket figlio
  int what_do;  // Informazioni riguardanti cosa deve fare il processo padre per gestire i dati inviati dal processo figlio
};

/***********************************************************************************************/
/********************************** DICHIARAZIONE METODI ***************************************/
/***********************************************************************************************/

int make_socket(struct sockaddr_un* addr);
void make_sockaddr_un(struct sockaddr_un* channel, char* name);

/***********************************************************************************************/
/******************************* FINE DICHIARAZIONE METODI *************************************/
/***********************************************************************************************/

/*  Funzione per creare il socket.
 *  Grazie al parametro formale possiamo decidere se creare un normalissimo socket (AF_UNIX)
 *  oppure un socket con il quale fare la bind con un file
 *  Questo possiamo farlo o
 *      -> ponendo NULL come parametro attuale creando un semplice socket AF_UNIX
 *  oppure
 *      -> ponendo l'indirizzo creato (struct sockaddr_un) con la funzione
 *             void make_sockaddr_un(struct sockaddr_un* channel, char* name);
 *         In questo caso viene fatta la bind con il canale di comunicazione creato
 *         "channel" associato al file "name"
 */
int make_socket(struct sockaddr_un* addr){
    int sfd;
    ssize_t res;

    sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(sfd == -1){
        perror("Socket AF_UNIX\n");
        return -1;
    }

    /*  Per quando non mi serve fare la bind
     *  Trattandosi di processi locali, non mi serve fare la bind sui socket
     *  che vogliono interagire con il socket principale con cui faccio la bind
     *  per associarli un canale di comunicazione che in questo caso è un semplice
     *  file: addr->sun_path (struttura dati sockaddr_un)
     */
    if(addr == NULL)
        return sfd;

    /*  Se il file a cui voglio associare il socket con la bind è gia stato associato
     *  ad un altro socket lo dissocio usando la unlink(file);
     */
    unlink(addr->sun_path);

    /*  Faccio la bind con un file con il quale il processo figlio potra poi comunicare
     *  con il processo padre
     */
    res = bind(sfd, (struct sockaddr*)addr, sizeof(struct sockaddr_un));
    if(res == -1){
        perror("Bind AF_UNIX\n");
        return -1;
    }

    return sfd;
}

void make_sockaddr_un(struct sockaddr_un* channel, char* name){

    /* Creo la struttura sockaddr_un per i socket del dominio AF_UNIX/LOCAL */

    if(strlen(name) > sizeof(channel->sun_path) - 1){
        printf("Server socket path too long: %s\n", name);
        exit(1);
    }

    /* Creo il canale di comunicazione sfruttando il file con path/name */
    memset(channel, 0, sizeof(struct sockaddr_un));
    channel->sun_family = AF_UNIX;
    strncpy(channel->sun_path, name, sizeof(channel->sun_path) - 1);

}
