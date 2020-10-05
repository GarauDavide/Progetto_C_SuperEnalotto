
/*  La struttura del server è formata sia Multiprocesso che Multiplexing
 *
 *  La STRUTTURA MULTIPROCESSO mi serve per la gestione delle richieste da parte
 *  dei client. In questo modo il processo padre si occupa solo di accettare le
 *  le richieste in coda. Accettata la richiesta la "scarica" al processo figlio.
 *
 *  La STRUTTURA MULTIPLEXING viene utilizzata per gestire la comunicazione tra
 *  processi figli e processo padre creando un canale di comunicazione locale.
 *  I processi figli utilizzano i socket di famiglia AF_UNIX/LOCAL per comunicare
 *  con il processo padre. Le primitive chiamate sono di tipo UDP:
 *      sendto() e recvfrom()
 *  Il canale di comunicazione è associato al socket -> sockForked
 *  In questo modo il multiplexing gestisce le richieste che arrivano al descrittore
 *  di socket listen e al descrittore di socket sockForked.
 *  Quindi quando arriva una richiesta dal client, la select vede e notifica questo,
 *  gestendo la richiesta del client.
 *  Quando arriva un messaggio al descrittore di socket da parte dei processi figli,
 *  per il processo padre, la select mette nello stato di pronto, in quanto ci sono
 *  nuovi dati nel buffer di ricezione del socket sockForked.
 *
 *  Implementazione nel file -> "gestione_socket_AFunix.h"
 *
 *
 *  NotaBene: il timer del BAN è settato a 60 secondi.
 *              Per poterlo cambiare, andare nel file:
 *                   -> condivision_data.h
 *                           cambiare il valore BAN_TIME_SECONDS
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>

#include "./headers/gestione_socket_AFunix.h"
#include "./headers/gestione_server_dati.h"
#include "./headers/lotto_gestione_dati_server.h"
#include "./headers/condivision_data.h"

#define MAX_PORT 65535
#define MIN_PORT 1024

#define DELETE_USER_FOR_PROBABLY_DISCONNECTION 11223344

int main(int argc, char** argv){

    struct sockaddr_in sv_addr; // Indirizzo di server
    struct sockaddr_in clientAddr; // Indirizzo client

    int BLOCKED_ANY_REQUEST = 0;

    fd_set master;  // Set principale
    fd_set read_fds;    // Set di lettura
    int fd_max; // Numero max di descrittori

    int listener;   // Socket per l'ascolto
    int sockForked;    // Socket AF_UNIX per la comunicazione che con i processi figli
    int clientSocket;

    struct sockaddr_un channel;
    socklen_t len;
    ssize_t res;

    int addrLenClient;
    pid_t pid;
    int i;
    int ret;
    char ip_client[INET_ADDRSTRLEN];

    char* buffer;

    int parallel_proces = 1;    //  (PER PRECAUZIONE!) -> Variabile per gestire una sola chiamata del processo estrazioni

    /* Strutture per la gestione degli utenti bannati e utenti connessi al server */
    struct management_banned_ip* management_banned_ip_list;
    struct connections_list* conn_list;

    /* Variabili per la gestione della porta del server nel giusto formato */
    int server_port_terminal;
    uint16_t server_port_terminal_uint16;

    int time_extraction_minutes = 5;    // Minuti estrazione di default


    /**************************************************************************************************************************************/
    /* FACCIO I DOVUTI CONTROLLI RIGUARDO ALLA PORTA DI ASCOLTO DEL PROCESSO SERVER E AL MINUTAGGIO DELLE ESTRAZIONI PASSATI DA TERMINALE */

    if((argc > 1) && (argc <= 3)){
        server_port_terminal = atoi(argv[1]);

        if((server_port_terminal >= MIN_PORT) && (server_port_terminal < MAX_PORT))
            server_port_terminal_uint16 = (uint16_t)server_port_terminal;
        else{
            printf("Errore nella scelta della porta. Porta non valida.\n");
            return 0;
        }

        if(argc == 3)
            time_extraction_minutes = atoi(argv[2]);

    }else{
        printf("Parametri per avviare il server non validi.\nRicorda: ./lotto_server <porta> <minuti processo estrazione (OPZIONALE)>\n");
        return 0;
    }

    /* FINE CONTROLLI RIGUARDO I PARAMETRI PASSATI DA TERMINALE */
    /**************************************************************************************************************************************/

    /* Inizializzo le due liste per la gestione delle connessioni e gestione utenti bannati */
    management_banned_ip_list = 0;
    conn_list = 0;

    /* CREO IL CANALE DI COMUNICAZIONE PER IL SOCKET sockForked. */
    /* Spiegazione file "gestione_socket_AFunix.h" */
    /* Faccio la bind tra il sockForked e il file definito con ID */
    make_sockaddr_un(&channel, ID);
    sockForked = make_socket(&channel);

    /* Azzero i set */
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    listener = socket(AF_INET, SOCK_STREAM, 0);

    sv_addr.sin_family = AF_INET;
    /* Mi metto in ascolto su tutte le interfacce */
    sv_addr.sin_addr.s_addr = INADDR_ANY;
    sv_addr.sin_port = htons(server_port_terminal_uint16);
    bind(listener, (struct sockaddr*)&sv_addr, sizeof(sv_addr));
    listen(listener, 10);

	printf("Server avviato..\nPorta: %u\nTimer estrazione: %d minuti\n", server_port_terminal_uint16, time_extraction_minutes);

	FD_SET(sockForked, &master);
    FD_SET(listener, &master);
    fd_max = listener;


    /******************************************************************************************/
    /************************** PARTE CHE SI OCCUPA DELLE ESTRAZIONI **************************/
    /******************************************************************************************/

    /* Definizione start_parallel_process_son_for_make_extractions(..) nel file "lotto_gestione_dati_server.h" */
    if(parallel_proces){
        parallel_proces = 0;
        start_parallel_process_son_for_make_extractions(time_extraction_minutes);
    }

    /******************************************************************************************/
    /************************ FINE PARTE CHE SI OCCUPA DELLE ESTRAZIONI ***********************/
    /******************************************************************************************/

    for(;;){
        read_fds = master;
        select(fd_max + 1, &read_fds, NULL, NULL, NULL);
        for(i = 0; i <= fd_max; i ++){
            if(FD_ISSET(i, &read_fds)){
                if(i == listener){
                    struct message_info msg_info_ban;

                    addrLenClient = sizeof(clientAddr);
                    clientSocket = accept(listener, (struct sockaddr*)&clientAddr, &addrLenClient);

                    printf("Nuova connessione di un client accettata con successo..\n");

                    /**********************************************************************************************/
                    /* FACCIO TUTTI I CONTROLLI PRIMA DI FAR GESTIRE LA RICHIESTA DAL SERVER **********************/
                    /**********************************************************************************************/

                    /*  Controllo se l'utente è stato bannato
                     *  Grazie alla lista di appoggio:
                     *  "management_banned_ip_list"
                     *  posso vedere se un utente ha provato ad eseguire dei login sbagliando.
                     *  Se sbaglia per piu volte (in questo caso 3), verra bannato. Gli sbagli vengono
                     *  conteggiati nella variabile appartenente alla struttura "how_many_times"
                     *  L'utente rimarrà bannato per:
                     *      BAN_TIME_SECONDS settabile nel file "condivision_data.h"
                     */

                    inet_ntop(AF_INET, &(clientAddr.sin_addr), ip_client, INET_ADDRSTRLEN);
                    if(get_me_how_many_times(&management_banned_ip_list, ip_client) >= 3){
                        time_t sec = get_me_time_ban(&management_banned_ip_list, ip_client);
                        int diff = (int)(time(NULL) - sec);

                        if(diff <= BAN_TIME_SECONDS){
                            msg_info_ban.result = BAN_STATE;
                            msg_info_ban.info = diff;

                            BLOCKED_ANY_REQUEST = 1;

                            printf("Utente con IP: %s ha tentato l'accesso, ma e' stato bannato\n", ip_client);

                        }else{
                            remove_ip_at_ban_list(&management_banned_ip_list, ip_client);
                            msg_info_ban.result = 1;
                            msg_info_ban.info = 0;
                            BLOCKED_ANY_REQUEST = 0;
                        }

                    }else{
                        msg_info_ban.result = 1;
                        msg_info_ban.info = 0;
                        BLOCKED_ANY_REQUEST = 0;
                    }

                    /* Do l'OK al client di mandarmi i dati o meno */
                    ret = send(clientSocket, (void*)&msg_info_ban, sizeof(msg_info_ban), 0);
                    if(ret < sizeof(msg_info_ban)){
                        printf("Errore invio stato ban al client\n");
                        exit(1);
                    }

                    /**********************************************************************************************/
                    /* FINE CONTROLLI SUI CLIENT CHE SI CONNETTONO ************************************************/
                    /**********************************************************************************************/

                    pid = fork();
                    if(pid == -1){
                        printf("Errore creazione del processo figlio (Client)\n");
                        exit(1);
                    }

/******************************************************************************************************************/

                    if(pid == 0){
                        char* buffer_client;
                        char* buffer;
                        char** usr_psw;
                        struct message_info msg_info_client;
                        int res_client;

                        char id_client_global[11] = "default_id";

                        int my_sfd;
                        struct message_son msg_to_father;

                        close(listener);
                        close(sockForked);

                        do{
                            /**********************************************************************************************/
                            /* Sto ricevendo il buffer dal client con i vari parametri ************************************/
                            /**********************************************************************************************/
                            if(!BLOCKED_ANY_REQUEST){

                                /* Ricevo lunghezza dal client della struttura del messaggio */
                                res_client = recv(clientSocket, &msg_info_client, sizeof(msg_info_client), 0);
                                if(res_client < sizeof(msg_info_client)){
                                    printf("Errore ricezione struttura dati client, probabile disconnessione inaspettata.\n");
                                    /*  Avvio la procedura per togliere l'utente dalla lista delle connessioni
                                     *  Probabile disconnessione da parte del client
                                     */

                                    /* CREO UN SOCKET AF_UNIX/LOCAL PER COMUNICARE CON IL PROCESSO PADRE */
                                    /**********************************************/
                                    /* MESSAGGIO PER IL PROCESSO PADRE */
                                    my_sfd = make_socket(NULL);

                                    msg_to_father.comand_code = DELETE_USER_FOR_PROBABLY_DISCONNECTION;
                                    msg_to_father.what_do = 1;
                                    strcpy(msg_to_father.text1, id_client_global);

                                    len = sizeof(channel);

                                    res = sendto(my_sfd, &msg_to_father, sizeof(msg_to_father), 0, (struct sockaddr *)&channel, len);
                                    if(res < 0){
                                        perror("SendTo\n");
                                    }

                                    close(my_sfd);
                                    /* FINE MESSAGGIO PER IL PROCESSO PADRE */
                                    /**********************************************/

                                    /*  Avendo padre e figlio le stesse strutture dati, devo cancellarlo anche dal
                                     *  dalla struttura dati del processo figlio (cioè questo processo)
                                     */
                                    delete_session_of_a_specific_id(&conn_list, id_client_global);

                                    exit(1);
                                }

                                buffer_client = malloc(msg_info_client.nByte);

                                res_client = recv(clientSocket, buffer_client, msg_info_client.nByte, 0);
                                if(res_client < msg_info_client.nByte){
                                    perror("Errore ricezione buffer dati client: \n");
                                    exit(1);
                                }

                            }

                            /**********************************************************************************************/
                            /* Ho ricevuto il buffer dal client con i vari parametri **************************************/
                            /**********************************************************************************************/

                            /* Se l'utente è bannato viro la sua richiesta nella sezione BAN_STATE */
                            if(BLOCKED_ANY_REQUEST)
                                msg_info_client.comand_code = BAN_STATE;

                            switch(msg_info_client.comand_code){

/**************************************************************************************************************************/
                                case BAN_STATE :

                                    msg_info_client.comand_code = ESCI;
                                    close(clientSocket);

                                    break;

/**************************************************************************************************************************/
                                case SIGNUP :

                                    msg_info_client.result = 0;

                                    do{
                                        if(msg_info_client.result){
                                            res_client = recv(clientSocket, &msg_info_client, sizeof(msg_info_client), 0);
                                            if(res_client < sizeof(msg_info_client)){
                                                printf("Errore nella ricezione della lunghezza del comando\n");
                                                exit(1);
                                            }

                                            buffer_client = malloc(msg_info_client.nByte);

                                            res_client = recv(clientSocket, buffer_client, msg_info_client.nByte, 0);
                                            if(res_client < msg_info_client.nByte){
                                                printf("Errore nella ricezione della lunghezza del comando\n");
                                                exit(1);
                                            }
                                        }

                                        // Provo a crare il file personale per l'utente
                                        usr_psw = getMe_two_parameters_separated_at_space(buffer_client, strlen(buffer_client));

                                        printf("%s\n", buffer_client);

                                        if(tellMe_if_user_is_registered(usr_psw[0])){
                                            /* Utente gia registrato */
                                            printf("Utente %s gia registrato\n", usr_psw[0]);
                                            msg_info_client.result = 1;
                                        }else{
                                            /* Utente non ancora registrato */
                                            signup_new_user(usr_psw[0], usr_psw[1]);
                                            printf("Nuovo utente (%s : %s) registrato con successo..\n", usr_psw[0], usr_psw[1]);
                                        }

                                        res_client = send(clientSocket, &msg_info_client, sizeof(msg_info_client), 0);
                                        if(res_client < sizeof(msg_info_client)){
                                            printf("Errore risposta al client per l'esito registrazione utente..\n");
                                            exit(1);
                                        }

                                    }while(msg_info_client.result);

                                    close(clientSocket);

                                    msg_info_client.comand_code = ESCI;

                                    break;

/**************************************************************************************************************************/
                                case LOGIN :

                                    usr_psw = getMe_two_parameters_separated_at_space(buffer_client, msg_info_client.nByte);

                                    if(!tellMe_if_user_is_already_connected(&conn_list, usr_psw[0])){

                                        if(tellMe_if_user_is_registered(usr_psw[0])){
                                            char* password_file;

                                            password_file = read_user_password_from_the_associate_file(usr_psw[0]);

                                            if(!strcmp(password_file, usr_psw[1])){
                                                char created_id[11];

                                                strcpy(created_id, getMe_one_user_id(10));

                                                strcpy(id_client_global, created_id);

                                                msg_info_client.result = 1;
                                                strcpy(msg_info_client.id, created_id);

                                                /* CREO UN SOCKET AF_UNIX/LOCAL PER COMUNICARE CON IL PROCESSO PADRE */
                                                /**********************************************/
                                                /* MESSAGGIO PER IL PROCESSO PADRE */
                                                my_sfd = make_socket(NULL);

                                                strcpy(msg_to_father.text1, usr_psw[0]);
                                                strcpy(msg_to_father.text2, created_id);
                                                msg_to_father.comand_code = LOGIN;
                                                msg_to_father.what_do = 1;
                                                msg_to_father.sock_for_father = clientSocket;

                                                len = sizeof(channel);

                                                res = sendto(my_sfd, &msg_to_father, sizeof(msg_to_father), 0, (struct sockaddr *)&channel, len);
                                                if(res < 0){
                                                    perror("SendTo\n");
                                                }

                                                close(my_sfd);
                                                /* FINE MESSAGGIO PER IL PROCESSO PADRE */
                                                /**********************************************/

                                                /* Essendo processo figlio, ha le stesse strutture del padre, ma nel suo spazio del processo */
                                                /* Inserisco l'utente nella lista connessioni sia del figlio che del padre (inviata tramite socket AF_UNIX) */
                                                add_new_user_at_the_connections_list(&conn_list, usr_psw[0], created_id, clientSocket);

                                            }

                                        }

                                    }else{
                                        /* Un utente sta cercando di collegarsi da un altro terminale, lo blocco */
                                        msg_info_client.info = 5;
                                    }

                                    if(msg_info_client.result == 0){
                                        inet_ntop(AF_INET, &(clientAddr.sin_addr), ip_client, INET_ADDRSTRLEN);

                                        /* CREO UN SOCKET AF_UNIX/LOCAL PER COMUNICARE CON IL PROCESSO PADRE */
                                        /**********************************************/
                                        /* MESSAGGIO PER IL PROCESSO PADRE */
                                        my_sfd = make_socket(NULL);

                                        strcpy(msg_to_father.text1, ip_client);
                                        msg_to_father.comand_code = LOGIN;
                                        msg_to_father.what_do = 2;

                                        len = sizeof(channel);

                                        res = sendto(my_sfd, &msg_to_father, sizeof(msg_to_father), 0, (struct sockaddr *)&channel, len);
                                        if(res < 0){
                                            perror("SendTo\n");
                                        }

                                        close(my_sfd);
                                        /* FINE MESSAGGIO PER IL PROCESSO PADRE */
                                        /**********************************************/

                                        if(get_me_how_many_times(&management_banned_ip_list, ip_client) == 2){
                                            char* msg_log;

                                            msg_info_client.result = 2;    // Avviso l utente lato client che è stato bannato dal server

                                            printf("L'ip %s è stato bannato.. segnalazione nel file di Logs\n", ip_client);

                                            msg_log = create_message_log(ip_client);
                                            write_in_a_specific_file_with_lock(msg_log, "./Logs/logs.txt");

                                        }else
                                            msg_info_client.result = 0;

                                    }

                                    /* Rispondo al client con esito negativo, lasciando il valore di msg_info.result */
                                    ret = send(clientSocket, &msg_info_client, sizeof(msg_info_client), 0);
                                    if(ret < sizeof(msg_info_client)){
                                        printf("Errore risposta al client riguardo esito login..\n");
                                        exit(1);
                                    }

                                    if(msg_info_client.result != 1){
                                        msg_info_client.comand_code = ESCI;
                                        close(clientSocket);
                                    }

                                    break;


/**************************************************************************************************************************/
                                case INVIA_GIOCATA :

                                    /*
                                     *  Controllo se l'utente ha effettuato il login e ne guardo il suo
                                     *  ID per vedere se ha i permessi per effettuare la giocata
                                     */
                                     if(tellMe_if_the_user_id_is_valid(&conn_list, msg_info_client.id)){
                                         struct game_card* gc;
                                         char* usr;
                                         char id[11];

                                         strcpy(id, msg_info_client.id);

                                         gc = create_game_card(buffer_client, strlen(buffer_client));
                                         usr = getMe_username_for_a_specific_id(&conn_list, id);
                                         save_game_card_at_personal_file(gc, usr);

                                         printf("L'utente %s ha effettuato una giocata\n", usr);

                                         msg_info_client.result = 1;

                                     }else{
                                         /* Utente deve effettuare il login o il suo id è errato */
                                         printf("Utente ha fallito una giocata. ID non valido..\n");
                                         msg_info_client.result = 0;
                                     }

                                     ret = send(clientSocket, &msg_info_client, sizeof(msg_info_client), 0);
                                     if(ret < sizeof(msg_info_client)){
                                         printf("Errore risposta al client riguardo esito invio giocata..\n");
                                         exit(1);
                                     }

                                    break;

/**************************************************************************************************************************/
                                case VEDI_GIOCATE :

                                    /*
                                     *  Controllo se l'utente ha effettuato il login e ne guardo il suo
                                     *  ID per vedere se ha i permessi per effettuare la giocata
                                     */
                                    if(tellMe_if_the_user_id_is_valid(&conn_list, msg_info_client.id)){
                                        char* usr;
                                        char id[11];

                                        strcpy(id, msg_info_client.id);

                                        usr = getMe_username_for_a_specific_id(&conn_list, id);
                                        buffer = get_game_cards_at_personal_file(usr, atoi(buffer_client));

                                        if(buffer == NULL){
                                            buffer = "Non ci sono schedine con questo parametro";
                                            msg_info_client.nByte = strlen(buffer) + 1;
                                            msg_info_client.result = 5;
                                        }else{

                                            msg_info_client.nByte = strlen(buffer) + 1;
                                            msg_info_client.result = 1;
                                        }

                                        printf("Utente %s sta ricevendo le giocate effettuate\n", usr);

                                    }else{
                                        /* Utente deve effettuare il login o il suo id è errato */
                                        printf("Utente non ha i privilegi. ID non valido..\n");
                                        msg_info_client.result = 0;
                                    }

                                    ret = send(clientSocket, &msg_info_client, sizeof(msg_info_client), 0);
                                    if(ret < sizeof(msg_info_client)){
                                        printf("Errore risposta al client riguardo esito invio giocate effettuata..\n");
                                        exit(1);
                                    }

                                    if(msg_info_client.result){
                                        ret = send(clientSocket, buffer, msg_info_client.nByte, 0);
                                        if(ret < msg_info_client.nByte){
                                            printf("Errore risposta al client riguardo esito invio giocate effettuata..\n");
                                            exit(1);
                                        }
                                    }

                                    break;

/**************************************************************************************************************************/
                                case VEDI_ESTRAZIONE :

                                    /*
                                     *  Controllo se l'utente ha effettuato il login e ne guardo il suo
                                     *  ID per vedere se ha i permessi per effettuare la giocata
                                     */
                                    if(tellMe_if_the_user_id_is_valid(&conn_list, msg_info_client.id)){
                                        char* usr = getMe_username_for_a_specific_id(&conn_list, msg_info_client.id);

                                        if(strlen(buffer_client) > 1){
                                            char** n_r = getMe_two_parameters_separated_at_space(buffer_client, strlen(buffer_client));
                                            int verify_ruota_pointer = -1;

                                            verify_ruota_pointer = getMe_RUOTE_associate_pointer(n_r[1], 0);

                                            if(verify_ruota_pointer == 11){
                                                buffer = getMe_n_extractions(atoi(n_r[0]), NULL);
                                                strcpy(msg_info_client.info_char, "ruota_no");
                                            }else{
                                                buffer = getMe_n_extractions(atoi(n_r[0]), n_r[1]);
                                                strcpy(msg_info_client.info_char, "ruota_ok");
                                            }

                                        }else
                                            buffer = getMe_n_extractions(atoi(buffer_client), NULL);

                                        if(buffer == NULL){
                                            buffer = malloc(sizeof(char) * 5);
                                            strcpy(buffer, "null");
                                        }

                                        msg_info_client.nByte = strlen(buffer) + 1;
                                        if(strlen(buffer) == 0)
                                            msg_info_client.result = 4;
                                        else
                                            msg_info_client.result = 1;

                                        printf("Utente %s ha richiesto di vedere le estrazioni\n", usr);

                                    }else{
                                        /* Utente deve effettuare il login o il suo id è errato */
                                        printf("Utente non ha i privilegi per ricevere le estrazioni. ID non valido..\n");
                                        msg_info_client.result = 0;
                                    }

                                    ret = send(clientSocket, &msg_info_client, sizeof(msg_info_client), 0);
                                    if(ret < sizeof(msg_info_client)){
                                        printf("Errore risposta al client riguardo esito invio estrazioni..\n");
                                        exit(1);
                                    }

                                    if(msg_info_client.result){

                                        ret = send(clientSocket, buffer, msg_info_client.nByte, 0);
                                        if(ret < msg_info_client.nByte){
                                            printf("Errore risposta al client riguardo esito invio estrazioni..\n");
                                            exit(1);
                                        }

                                    }

                                    break;

/**************************************************************************************************************************/
                                case VEDI_VINCITE :
                                    /*
                                     *  Controllo se l'utente ha effettuato il login e ne guardo il suo
                                     *  ID per vedere se ha i permessi per effettuare la giocata
                                     */
                                    if(tellMe_if_the_user_id_is_valid(&conn_list, msg_info_client.id)){
                                        char* user;

                                        user = getMe_username_for_a_specific_id(&conn_list, msg_info_client.id);
                                        buffer = get_wins(user);

                                        if(buffer == NULL){
                                            buffer = "Non ci sono schedine revisionate";
                                            msg_info_client.nByte = strlen(buffer) + 1;
                                            msg_info_client.result = 3;
                                        }else{
                                            msg_info_client.nByte = strlen(buffer) + 1;
                                            msg_info_client.result = 1;
                                        }

                                        printf("Utente %s ha richiesto di vedere le sue vincite\n", user);

                                    }else{
                                        /* Utente deve effettuare il login o il suo id è errato */
                                        printf("Utente non ha i privilegi per ricevere le vincite. ID non valido..\n");
                                        msg_info_client.result = 0;
                                    }

                                    ret = send(clientSocket, &msg_info_client, sizeof(msg_info_client), 0);
                                    if(ret < sizeof(msg_info_client)){
                                        printf("Errore risposta al client riguardo esito invio vincite..\n");
                                        exit(1);
                                    }

                                    if(msg_info_client.result){

                                        ret = send(clientSocket, buffer, msg_info_client.nByte, 0);
                                        if(ret < msg_info_client.nByte){
                                            printf("Errore risposta al client riguardo esito invio vincite..\n");
                                            exit(1);
                                        }

                                    }

                                    break;

/**************************************************************************************************************************/
                                case ESCI :

                                    /*
                                     *  Controllo se l'utente ha effettuato il login e ne guardo il suo
                                     *  ID per vedere se ha i permessi per effettuare la giocata
                                     */
                                    if(tellMe_if_the_user_id_is_valid(&conn_list, msg_info_client.id)){
                                        char* user = getMe_username_for_a_specific_id(&conn_list, msg_info_client.id);

                                        /* CREO UN SOCKET AF_UNIX/LOCAL PER COMUNICARE CON IL PROCESSO PADRE */
                                        /**********************************************/
                                        /* MESSAGGIO PER IL PROCESSO PADRE */
                                        my_sfd = make_socket(NULL);

                                        msg_to_father.comand_code = ESCI;
                                        msg_to_father.what_do = 1;
                                        strcpy(msg_to_father.text1, msg_info_client.id);

                                        len = sizeof(channel);

                                        res = sendto(my_sfd, &msg_to_father, sizeof(msg_to_father), 0, (struct sockaddr *)&channel, len);
                                        if(res < 0){
                                            perror("SendTo\n");
                                        }

                                        close(my_sfd);
                                        /* FINE MESSAGGIO PER IL PROCESSO PADRE */
                                        /**********************************************/

                                        printf("Disconnetto utente %s con id %s\n", user, msg_info_client.id);

                                        msg_info_client.result = 1;

                                    }else{
                                        /* Utente deve effettuare il login o il suo id è errato */
                                        printf("Utente non ha i privilegi per questo comando. ID non valido..\n");
                                        msg_info_client.result = 0;
                                    }

                                    ret = send(clientSocket, &msg_info_client, sizeof(msg_info_client), 0);
                                    if(ret < sizeof(msg_info_client)){
                                        printf("Errore risposta al client riguardo esito Disconnessione client..\n");
                                        exit(1);
                                    }

                                    if(msg_info_client.result == 1)
                                        close(clientSocket);

                                    break;

/**************************************************************************************************************************/
                                default :
                                    /* Non faccio nulla */
                                    break;

                            }   /* FINE SWITCH */

                        }while(msg_info_client.comand_code != ESCI);

                        exit(1);

                    }   /* FINE PROCESSO FIGLIO */

                    // Chiudo socket nel processo padre. Lo sta gestendo un processo figlio
                    close(clientSocket);

                }else{
                    /*  In questo "else" ci entro quando i processi figli hanno comunicato
                     *  con il processo padre attraverso i socket AF_UNIX/LOCAL
                     *  Il descrittore di socket sockForked è stato associato con la bind
                     *  ad un file creando un channel (canale di comunicazione locale)
                     *  Ogni volta che un processo figlio vuole comunicare con il padre
                     *  può scrivere/inviare dati dentro quel file attraverso le primitive
                     *  sendto e recvfrom
                     */

                    struct message_son mssg_son;
                    int mssg_son_len;

                    struct message_info mssg_info;

                    len = sizeof(struct sockaddr_un);
                    mssg_son_len = sizeof(mssg_son);

                    res = recvfrom(sockForked, &mssg_son, mssg_son_len, 0, (struct sockaddr *)&channel, &len);
                    if(res < 0){
                        perror("RecvFrom\n");
                        return -1;
                    }

                    /*  All'interno dello switch scelgo cosa fare in base al comand_code e i dati
                     *  che sono arrivati dal processo figlio attraverso l'apposita struttura "message_son"
                     */
                    switch (mssg_son.comand_code) {
                        case LOGIN :
                            if(mssg_son.what_do == 1){
                                /* Aggiungo l'utente alla lista delle connessioni del processo padre */
                                add_new_user_at_the_connections_list(&conn_list, mssg_son.text1, mssg_son.text2, mssg_son.sock_for_father);
                                stamp_connections_list(&conn_list);
                            }else{
                                /* Aggiungo ip alla lista management_banned_ip_list */
                                add_or_management_IP_at_going_to_banned_list(&management_banned_ip_list, mssg_son.text1);
                                stamp_going_to_banned_list(&management_banned_ip_list);
                            }

                            break;

                        case DELETE_USER_FOR_PROBABLY_DISCONNECTION :
                            /*  Sfrutto il socket che mi ero salvato nell inserimento del client
                             *  nella lista delle connessioni attive. In questo modo, se si disconnette
                             *  improvvisamente senza fare il comando ESCI, lo tolgo dalle connessioni
                             *  Questo controllo viene fatto nell'attesa del comando da parte del client
                             *  all inizio del do{}while per la gestione dei client da parte dei processi figli.
                             */
                            delete_session_of_a_specific_id(&conn_list, mssg_son.text1);
                            stamp_connections_list(&conn_list);

                            break;

                        case ESCI :
                            if(mssg_son.what_do){

                                delete_session_of_a_specific_id(&conn_list, mssg_son.text1);
                                stamp_connections_list(&conn_list);
                            }

                            break;

                        default :
                            break;

                    }

                }   /* FINE ELSE DESCRITTORE FORKED */

            }   /* FINE FD_ISSET */

        }   /* FINE FOR CONTROLLO DESRITTORI DI SOCKET */

    }   /* FINE FOR SEMPRE */

    return (EXIT_SUCCESS);
}
