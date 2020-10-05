#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdint.h>

#include "./headers/gestione_comandi.h"
#include "./headers/lotto_gestione_dati_client.h"

#define MAX_PORT 65535
#define MIN_PORT 1024

// Dimensione del buffer dove verrà trasferito il contenuto dello standard input
#define MAX_COMAND_SIZE_AT_STDIN 256

/*  Dichiaro queste 3 strutture/variabili globali, in modo da gestirle meglio
 *  CLIENT_ID, clientSD e server_addr.
 */
char CLIENT_ID[11] = "default_id";

int clientSD;   // Intero che identifica il descrittore del socket
struct sockaddr_in server_addr; // Struttura per il server - indirizzo e porta

/***********************************************************************************************/
/********************************** DICHIARAZIONE METODI ***************************************/
/***********************************************************************************************/

void create_client_socket();
void close_client_socket();
void connection_with_the_server();
void send_message_to_the_server(char* buffer_cl, int result, int comand_code, int info, char* info_char);
int receive_if_you_ban_from_server();

/* LA DEFINIZIONE DEI METODI E' DOPO IL MAIN ALLA FINE DEL FILE */

/***********************************************************************************************/
/******************************* FINE DICHIARAZIONE METODI *************************************/
/***********************************************************************************************/

int main(int argc, char** argv){

    char* chosen_comand_buffer_stdin;   // Buffer di appoggio allo standard input
    char* comand_with_params;   // Contiene l'intero comando assieme ai parametri
    char* comand;   // Contiene solo il comando inserito
    char* params;   // Contiene solo i parametri del comando inserito
    int comand_len_only;    // Contine la lunghezza del comando
    int comand_with_params_len;    // Contiene la lunghezza del comando compreso di parametri
    int code_associated_a_comand;    // Contiene il codice del comando digitato da tastiera
    int number_of_params;   // Contiene il numero di parametri che sono stati inviati assieme al comando

    int ret;

    int server_port_terminal;   // Porta del server (valore int)
    uint16_t server_port_terminal_uint16;   // Porta server nel suo formato ideale


    /*************************************************************************************************************/
    /* FACCIO I DOVUTI CONTROLLI RIGUARDO I PARAMETRI PASSATI DA TERMINALE PER L'AVVIO DELLA APPLICAZIONE CLIENT */
    /*************************************************************************************************************/

    if(argc != 3){
        printf("Parametri per avviare il client non validi.\nRicorda: ./lotto_client <ip server> <porta server>\n");
        return 0;
    }

    server_port_terminal = atoi(argv[2]);

    if((server_port_terminal >= MIN_PORT) && (server_port_terminal < MAX_PORT))
        server_port_terminal_uint16 = (uint16_t)server_port_terminal;
    else{
        printf("Errore nella scelta della porta. Porta non valida.\n");
        return 0;
    }

    /************************************************************/
    /* FINE CONTROLLI RIGUARDO I PARAMETRI PASSATI DA TERMINALE */
    /************************************************************/


    memset(&server_addr, 0, sizeof(server_addr));   // Pulizia memeoria
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port_terminal_uint16);
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    /* Stampo il menu */
    show_the_menu();

    /* Ciclo DO-WHILE che gestisce il menu con le scelte che può fare l'utente */
    do{
        /* Gestisco e prendo il contenuto del buffer stdin */
        printf("Digita un comando: ");
        chosen_comand_buffer_stdin = malloc(MAX_COMAND_SIZE_AT_STDIN);
        fgets(chosen_comand_buffer_stdin, MAX_COMAND_SIZE_AT_STDIN, stdin);
        fflush(stdin);

        /*  Questa variabile conterrà il comando e i parametri senza errori di spazi
         *    getMe_all_buffer_at_the_stdin(...);
         *  Questa funzione ritorna tutto il buffer (comando_parametri) controllato
         *  in modo che posso lavorarci sapendo cosa c'è e come è strutturato
         */
        comand_with_params = getMe_all_buffer_at_the_stdin(chosen_comand_buffer_stdin, MAX_COMAND_SIZE_AT_STDIN);
        comand_with_params_len = strlen(comand_with_params);

        /*  Mi faccio restituire il comando senza parametri e calcolo il suo codice
         *  per accedere al costrutto SWITCH
         */
        comand = getMe_chosen_comand(comand_with_params, comand_with_params_len);
        comand_len_only = strlen(comand);

        code_associated_a_comand = getMe_code_number_associate_at_the_comand(comand, comand_len_only);

        /* I codici dei comandi si trovano nel file "condivision_data.h" */
        switch(code_associated_a_comand){

            /*  Quasi tutti i comandi sono gestiti in questo modo:
             *     1) invio al server i parametri del comando
             *     2) mi risponde attraverso una struttura standard con il quale gestisco lato client le informazioni
             */

/**************************************************************************************************************************/
            case HELP :
                /* Mi recupero solo i parametri del comando inserito da tastiera dall'utente */
                params = getMe_chosen_comand_params(comand_with_params, comand_with_params_len);
                if(params != NULL){
                    getMe_comand_descriptions(getMe_code_number_associate_at_the_params_comand(params, strlen(params)));
                    if(params != NULL)
                        free(params);
                }else
                    getMe_comand_descriptions(0);

				        break;

/**************************************************************************************************************************/
            case SIGNUP :

                number_of_params = getMe_how_many_params(comand_with_params, strlen(comand_with_params));
                if(number_of_params == 2){
                    struct message_info mess_info;

                    /* Creo il socket e provo a connettermi con il server */
                    create_client_socket();
                    connection_with_the_server();

                    /* Controllo se sono bannato dal server */
                    if(receive_if_you_ban_from_server())
                        break;

                    mess_info.result = 0;

                    /*  Tramite il valore della struttura "result" gestisco il fatto
                     *  di chiedere uno username diverso nel caso quello inserito è gia
                     *  presente nel server. In base alla variabile result entro nelle giuste
                     *  condizioni if e rimango o esco dal ciclo while
                     */
                    do{
                        if(mess_info.result){
                            printf("Digita di nuovo il comando !signup con utente diverso: ");
                            chosen_comand_buffer_stdin = malloc(MAX_COMAND_SIZE_AT_STDIN);
                            fgets(chosen_comand_buffer_stdin, MAX_COMAND_SIZE_AT_STDIN, stdin);
                            fflush(stdin);

                            comand_with_params = getMe_all_buffer_at_the_stdin(chosen_comand_buffer_stdin, MAX_COMAND_SIZE_AT_STDIN);
                            if(chosen_comand_buffer_stdin != NULL || strlen(chosen_comand_buffer_stdin) != 0)
                                free(chosen_comand_buffer_stdin);
                            comand_with_params_len = strlen(comand_with_params);

                            comand = getMe_chosen_comand(comand_with_params, comand_with_params_len);
                            code_associated_a_comand = getMe_code_number_associate_at_the_comand(comand, strlen(comand));

                            number_of_params = getMe_how_many_params(comand_with_params, strlen(comand_with_params));
                            if((number_of_params != 2) || (code_associated_a_comand != SIGNUP)){
                                getMe_comand_descriptions(SIGNUP);
                                continue;
                            }
                        }

                        params = getMe_chosen_comand_params(comand_with_params, comand_with_params_len);

                        send_message_to_the_server(params, 0, SIGNUP, 0, "");

                        ret = recv(clientSD, &mess_info, sizeof(mess_info), 0);
                        if(ret < sizeof(mess_info)){
                            printf("Errore nella ricezione esito registrazione\n");
                            exit(1);
                        }

                    }while(mess_info.result);

                    close(clientSD);

                    printf("Registrazione avvenuta con successo\n");

				}else
                    getMe_comand_descriptions(SIGNUP);

                break;

/**************************************************************************************************************************/
            case LOGIN :

                if(strcmp(CLIENT_ID, "default_id")){
                    printf("Hai gia effettuato il login con il server.\n");
                    break;
                }

                number_of_params = getMe_how_many_params(comand_with_params, strlen(comand_with_params));
                if(number_of_params == 2){
                    struct message_info mess_info;

                    /* Creo il socket e provo a connettermi con il server */
                    create_client_socket();
                    connection_with_the_server();

                    /* Controllo se il server mi risponde se sono stato bannato o meno */
                    if(receive_if_you_ban_from_server())
                        break;

                    params = getMe_chosen_comand_params(comand_with_params, comand_with_params_len);

                    send_message_to_the_server(params, 0, LOGIN, 0, "");

                    ret = recv(clientSD, &mess_info, sizeof(mess_info), 0);
                    if(ret < sizeof(mess_info)){
                        printf("Errore nella ricezione esito registrazione\n");
                        exit(1);
                    }

                    /*  In base ai valori restituiti dal server tramite la struttura dati "mess_info"
                     *  dati gestisco la situazione seguente
                     */

                    if(mess_info.info == 5)
                        printf("Il tuo account e' attualmente gia' connesso in un altro host\n");

                    if(mess_info.result){
                        if(mess_info.result == 1){
                            printf("Login avvenuto con successo\nIl tuo session id: %s\n", mess_info.id);
                            strcpy(CLIENT_ID, mess_info.id);
                        }else{
                            printf("Hai effettuato 3 tentativi sbagliati di login.\nSei stato bannato dal server per %d secondi\n", BAN_TIME_SECONDS);
                            close(clientSD);
                        }
                    }else{
                        printf("Login fallito, riprova..\n");
                        close(clientSD);
                    }

                }else
                    getMe_comand_descriptions(LOGIN);

                break;

/**************************************************************************************************************************/
            case INVIA_GIOCATA :

                if(!strcmp(CLIENT_ID, "default_id")){
                    printf("Non hai effettuato il login con il server.\nNon puoi richiamare questo comando prima del login\n");
                    break;
                }

                number_of_params = getMe_how_many_params(comand_with_params, strlen(comand_with_params));
                if(number_of_params >= 6){
                    struct message_info msg_info;

                    params = getMe_chosen_comand_params(comand_with_params, comand_with_params_len);

                    send_message_to_the_server(params, 0, INVIA_GIOCATA, 0, "");

                    ret = recv(clientSD, &msg_info, sizeof(msg_info), 0);
                    if(ret < sizeof(msg_info)){
                        printf("Errore nella ricezione esito invio giocata\n");
                        exit(1);
                    }

                    if(msg_info.result)
                        printf("Giocata effettuata con successo..\n");
                    else
                        printf("Il tuo corrente ID %s non è valido,\nrieffettua il !login per averne uno valido..", CLIENT_ID);

                }else
                    getMe_comand_descriptions(INVIA_GIOCATA);

                break;

/**************************************************************************************************************************/
            case VEDI_GIOCATE :

                if(!strcmp(CLIENT_ID, "default_id")){
                    printf("Non hai effettuato il login con il server.\nNon puoi richiamare questo comando prima del login\n");
                    break;
                }

                number_of_params = getMe_how_many_params(comand_with_params, comand_with_params_len);
                if(number_of_params == 1){
                    struct message_info msg_info;

                    params = getMe_chosen_comand_params(comand_with_params, comand_with_params_len);

                    send_message_to_the_server(params, 0, VEDI_GIOCATE, 0, "");

                    ret = recv(clientSD, &msg_info, sizeof(msg_info), 0);
                    if(ret < sizeof(msg_info)){
                        printf("Errore nella ricezione esito invio giocata - struttura risposta\n");
                        exit(1);
                    }

                    if(msg_info.result != 0){
                        char* buf;

                        buf = malloc(msg_info.nByte);

                        ret = recv(clientSD, buf, msg_info.nByte, 0);
                        if(ret < msg_info.nByte){
                            printf("Errore nella ricezione esito invio giocata - risposta\n");
                            exit(1);
                        }

                        if(msg_info.result == 1){

                            stamp_game_cards_with_saved_format(buf);

                        }else if(msg_info.result == 5){

                            printf("%s\n", buf);

                        }

                        free(buf);

                    }else
                        printf("Il tuo corrente ID %s non è valido,\nrieffettua il !login per averne uno valido..", CLIENT_ID);

                }else
                    getMe_comand_descriptions(VEDI_GIOCATE);

                break;

/**************************************************************************************************************************/
            case VEDI_ESTRAZIONE :

                if(!strcmp(CLIENT_ID, "default_id")){
                    printf("Non hai effettuato il login con il server.\nNon puoi richiamare questo comando prima del login\n");
                    break;
                }

                number_of_params = getMe_how_many_params(comand_with_params, comand_with_params_len);
                if(number_of_params > 0 && number_of_params <= 2 ){
                    struct message_info msg_info;

                    params = getMe_chosen_comand_params(comand_with_params, comand_with_params_len);

                    send_message_to_the_server(params, 0, VEDI_ESTRAZIONE, 0, "");

                    ret = recv(clientSD, &msg_info, sizeof(msg_info), 0);
                    if(ret < sizeof(msg_info)){
                        printf("Errore nella ricezione esito estrazione\n");
                        exit(1);
                    }

                    if(msg_info.result == 1){
                        char* buf;

                        buf = malloc(msg_info.nByte);

                        ret = recv(clientSD, buf, msg_info.nByte, 0);
                        if(ret < msg_info.nByte){
                            printf("Errore nella ricezione esito estrazione\n");
                            exit(1);
                        }

                        if((strlen(params) > 1) && (strcmp(buf, "null"))){

                            if(!strcmp(msg_info.info_char, "ruota_ok")){
                                char** n_ruota;
                                n_ruota = getMe_two_parameters_separated_at_space(params, strlen(params));
                                stamp_extractions_with_saved_format(buf, 1, n_ruota[1]);

                                free(n_ruota);

                            }else
                                stamp_extractions_with_saved_format(buf, 0, "");

                        }else if(strcmp(buf, "null"))
                                stamp_extractions_with_saved_format(buf, 0, "");
                            else
                                printf("Non ci sono estrazioni\n");

                    }else if(msg_info.result == 4){
                        printf("Non ci sono estrazioni\n");
                    }else
                        printf("Non ci sono estrazioni\n");

                }else
                    getMe_comand_descriptions(VEDI_ESTRAZIONE);

                break;

/**************************************************************************************************************************/
            case VEDI_VINCITE :
                if(!strcmp(CLIENT_ID, "default_id")){
                    printf("Non hai effettuato il login con il server.\nNon puoi richiamare questo comando prima del login\n");
                    break;
                }

                number_of_params = getMe_how_many_params(comand_with_params, comand_with_params_len);

                if(number_of_params == 0){
                    struct message_info msg_info;

                    send_message_to_the_server(params, 0, VEDI_VINCITE, 0, "");

                    ret = recv(clientSD, &msg_info, sizeof(msg_info), 0);
                    if(ret < sizeof(msg_info)){
                        printf("Errore nella ricezione esito vincite\n");
                        exit(1);
                    }

                    if(msg_info.result != 0){
                        char* buf;

                        buf = malloc(msg_info.nByte);

                        ret = recv(clientSD, buf, msg_info.nByte, 0);
                        if(ret < msg_info.nByte){
                            printf("Errore nella ricezione esito ricezione vincite\n");
                            exit(1);
                        }

                        printf("%s\n", buf);

                    }else
                        printf("Non ci sono schedine giocate verificate\n");

                }else
                    getMe_comand_descriptions(VEDI_VINCITE);

                break;

/**************************************************************************************************************************/
            case ESCI :
                /*  Se si inserisce ESCI come comando e si è connessi, ci disconnette solo dal server
                 *  Mentre se non siamo connessi ci fa uscire dalla applicazione
                 */
                if(!strcmp(CLIENT_ID, "default_id")){
                    printf("Uscita dalla applicazione avvenuta con successo\n");
                    break;
                }

                number_of_params = getMe_how_many_params(comand_with_params, comand_with_params_len);

                if(number_of_params == 0){
                    struct message_info msg_info;

                    send_message_to_the_server("", 0, ESCI, 0, "");

                    ret = recv(clientSD, &msg_info, sizeof(msg_info), 0);
                    if(ret < sizeof(msg_info)){
                        printf("Errore nella ricezione esito invio giocata\n");
                        exit(1);
                    }

                    if(msg_info.result == 1){
                        printf("Disconnessione dal server avvenuta con successo.\n");
                        close(clientSD);

                        strcpy(CLIENT_ID, "default_id");
                        code_associated_a_comand = 1;
                    }
                }else
                    getMe_comand_descriptions(ESCI);

                break;

/**************************************************************************************************************************/
            default :
                /*  Se il comando non viene riconosciuto, restituito il menu
                 *  con i suggerimenti.
                 */
                printf("Comando non riconosciuto...\n\n");
                getMe_comand_descriptions(0);

				break;

        }   // FINE SWITCH

        printf("\n");
    }while(code_associated_a_comand != ESCI);

    return (EXIT_SUCCESS);
}


/***********************************************************************************************/
/************************************ DEFINIZIONE METODI ***************************************/
/***********************************************************************************************/

/* Funzione che crea il socket */
void create_client_socket(){
    clientSD = socket(AF_INET, SOCK_STREAM, 0);
}

/* Funzione che chiude il socket */
void close_client_socket(){
    close(clientSD);
}

/*  Questa funzione serve per connettere il socket client al server.
 */
void connection_with_the_server(){
    int ret;

    ret = connect(clientSD, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(ret == -1){
        printf("Connessione con il server non riuscita..\n");
        exit(1);
    }
}

/*  Questa funzione gestisce tutte le richieste al server. In base alla richiesta
 *  vengono settati alcuni parametri a dovere, in modo che client e server si
 *  sincronizzano per dialogare nella maniera giusta.
 */
void send_message_to_the_server(char* buffer_cl, int result, int comand_code, int info, char* info_char){
    struct message_info msg_info;
    int ret;
    char* buf;

    buf = malloc(strlen(buffer_cl) + 1);
    strcpy(buf, buffer_cl);

    msg_info.nByte = (strlen(buf) + 1);
    msg_info.result = result;
    msg_info.comand_code = comand_code;
    msg_info.info = info;
    strcpy(msg_info.info_char, info_char);
    strcpy(msg_info.id, CLIENT_ID);

    ret = send(clientSD, (void*)&msg_info, sizeof(msg_info), 0);
    if(ret < sizeof(msg_info)){
        printf("Errore sull'invio informazioni messaggio al server\n");
        exit(1);
    }

    ret = send(clientSD, (void*)buf, msg_info.nByte, 0);
    if(ret < msg_info.nByte){
        printf("Errore sull'invio dati al server\n");
        exit(1);
    }
}

/*  Funzione che mi ritorna se sono stato bannato dal server.
 *  Avendo un receive al suo interno, bisogna sapere dove richiamarla, perche
 *  sennò diventa bloccante. Viene chiamata quando si tenta di fare il login.
 */
int receive_if_you_ban_from_server(){
    struct message_info msg_info;
    int ret;

    ret = recv(clientSD, &msg_info, sizeof(msg_info), 0);
    if(ret < sizeof(msg_info)){
        printf("Errore nel ricevere lo stato di ban dal server\n");
        exit(1);
    }

    if(msg_info.result == BAN_STATE){
        printf("Sei bannato dal server.\nTempo rimanente: %ld secondi\n", ((long)BAN_TIME_SECONDS - msg_info.info));
        return 1;
    }

    return 0;
}

/***********************************************************************************************/
/********************************* FINE DEFINIZIONE METODI *************************************/
/***********************************************************************************************/
