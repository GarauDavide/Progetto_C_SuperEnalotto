#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./condivision_data.h"

/***********************************************************************************************/
/********************************** DICHIARAZIONE METODI ***************************************/
/***********************************************************************************************/

char* getMe_all_buffer_at_the_stdin(char* cmd, int n_cmd);
char* getMe_chosen_comand(char* cmd, int n_cmd);
char* getMe_chosen_comand_params(char* cmd, int n_cmd);
int getMe_how_many_params(char* cmd, int n_cmd);
int getMe_code_number_associate_at_the_comand(char* cmd, int n_cmd);
int getMe_code_number_associate_at_the_params_comand(char* cmd, int n_cmd);
void getMe_comand_descriptions(int code);
void show_the_menu();

/***********************************************************************************************/
/******************************* FINE DICHIARAZIONE METODI *************************************/
/***********************************************************************************************/

/*
 *  Questo metodo riceve in ingresso un array di caratteri (Buffer)
 *  assieme alla sua dimensione.
 *  Questo metodo viene utilizzato per estrapolare, dal Buffer dello standard
 *  input, l'intero comando digitato da tastiera compreso di tutti i suoi
 *  parametri, escluso tutto il resto.
 *
 *  IMPORTANTE:
 *      tutti gli spazi in piu vengono eliminati, in modo da avere un maggior
 *      controllo sul Buffer contenente l'intero comandi digitato.
 *
 *  Dato un Buffer indefinito in ingresso, ritorna un Buffer con solo
 *  l'intero comando digitato compreso di parametri, esclusi spazi in eccesso.
 */
char* getMe_all_buffer_at_the_stdin(char* cmd, int n_cmd){
    int i, n_space, temp_buff_counter;
    char* buffer_without_spaces;   // Buffer finele che alla fine viene restituito
    char temp_buff[n_cmd];  // Buffer di appoggio

    for(i = 0; (cmd[i] == ' ') && (cmd[i] != '\n') && (i < n_cmd); i ++){ ; }

    if(i == n_cmd)
        return NULL;

    temp_buff_counter = n_space = 0;

    for(; (cmd[i] != '\n') && (i < n_cmd); i ++){
        if(cmd[i] != ' '){
            temp_buff[temp_buff_counter] = cmd[i];
            temp_buff_counter += 1;
            if(n_space > 0)
                n_space = 0;
        }else{
            n_space += 1;
            if(n_space <= 1){
                temp_buff[temp_buff_counter] = cmd[i];
                temp_buff_counter += 1;
            }
        }
    }

    if(temp_buff[temp_buff_counter - 1] == ' ')
        temp_buff_counter -= 1;

    temp_buff[temp_buff_counter] = '\0';

    buffer_without_spaces = malloc(temp_buff_counter);

    for(i = 0; i <= temp_buff_counter; i ++)
        buffer_without_spaces[i] = temp_buff[i];

    return buffer_without_spaces;
}

/*
 *  Questo metodo riceve in ingresso un array di caratteri e la sua dimensione.
 *  Dato un Buffer di caratteri dove risiede l'intero comando compreso di
 *  parametri, viene restituito solo il comando
 *  Esempio:
 *      !help login
 *  Viene restituito:
 *      !help
 */
char* getMe_chosen_comand(char* cmd, int n_cmd){
    int i, temp_size;
    char* temp_cmd;

    for(i = 0; (i < n_cmd) && (cmd[i] != ' '); i ++){ ; }

    temp_size = i;
    temp_cmd = malloc(temp_size + 1);

    for(i = 0; i < temp_size; i ++)
        temp_cmd[i] = cmd[i];
    temp_cmd[temp_size] = '\0';

    return temp_cmd;
}

/*
 *  Questo metodo riceve in ingresso un array di caratteri e la sua dimensione.
 *  Dato un Buffer di caratteri dove risiede l'intero comando compreso di
 *  parametri, vengono restituiti solo i parametri
 *  Esempio:
 *      !help login
 *  Viene restituito:
 *      login
 */
char* getMe_chosen_comand_params(char* cmd, int n_cmd){
    int i, j, temp_size;
    char* temp_params;

    if(cmd[0] != '!')
        return NULL;

    for(i = 0; (i < n_cmd) && (cmd[i] != ' '); i ++){ ; }

    if(cmd[i] == ' ')
        i += 1;

    if(i < n_cmd){
        j = i;
        temp_size = 0;

        for(; i < n_cmd; i ++)
            temp_size += 1;

        temp_params = malloc(temp_size + 1);

        for(i = 0; i < temp_size; i ++){
            temp_params[i] = cmd[j];
            j += 1;
        }
        temp_params[temp_size] = '\0';
    }else
        temp_params = NULL;

    return temp_params;
}

/*
 *  Questo metodo riceve in ingreasso un comando. Se non riceve esattamente
 *  un comando ritorna -1, altrimenti il numero dei comandi
 */
int getMe_how_many_params(char* cmd, int n_cmd){
    int i, params_number;
    char* params;

    if(cmd[0] != '!')
        return -1;

    params = getMe_chosen_comand_params(cmd, n_cmd);
    if(params == NULL)
        return 0;

    if(strlen(params) == 0)
        return 0;

    params_number = 0;

    if(strlen(params) > 0){
        for(i = 0; i < strlen(params); i ++){
            if(params[i] == ' '){
                i += 1;
                params_number += 1;
            }
        }

        params_number += 1;
    }

    return params_number;
}

/*
 *  Questo metodo riceve in ingresso un array di caratteri e la sua dimensione.
 *  Viene utilizzato per restituire il codice del comando associato.
 *  Restituisce un intero calcolato facendo la SOMMA del valore ASCII della lettera
 *	moltiplicata per la sua posizione allinterno della stringa:
 *	ESEMPIO:
 *		!help -> (h * 1) + (e * 2) .... dove le lettere sono il valore ASCII associato
 *	In questo modo il comando deve essere preciso. Se non viene fatto questo passaggio
 *	potrebbe essere valido anche una sequenza disordinata delle lettere di cui è composto
 *	il comando, senza accorgersene.
 *
 *  Questo metodo viene utilizzato quando la stringa del comando contiene
 *  anche il carattere ! iniziale. Quest ultimo viene scartato per calcolare
 *  la somma totale dei caratteri.
 */
int getMe_code_number_associate_at_the_comand(char* cmd, int n_cmd){
    int i, sum_characters = 0;

    if(cmd[0] != '!')
        return sum_characters;

    for(i = 1; i < n_cmd; i ++)
        sum_characters += (int)cmd[i] * i;

    return sum_characters;
}

/*
 *  Questo metodo riceve in ingresso un array di caratteri e la sua dimensione.
 *	Viene utilizzato quando il comando è un parametro di un altro comando.
 *  Viene utilizzato per restituire il codice del comando associato.
 *  Restituisce un intero calcolato facendo la SOMMA del valore ASCII della lettera
 *	moltiplicata per la sua posizione allinterno della stringa:
 *	ESEMPIO:
 *		!help -> (h * 1) + (e * 2) .... dove le lettere sono il valore ASCII associato
 *	In questo modo il comando deve essere preciso. Se non viene fatto questo passaggio
 *	potrebbe essere valido anche una sequenza disordinata delle lettere di cui è composto
 *	il comando, senza accorgersene.
 *
 *	IMPORTANTE
 *  Questo metodo viene utilizzato quando la stringa del comando NON contiene
 *  il carattere ! iniziale. Piu' precisamente viene utilizzato quando il nome
 *  del comando viene passato come parametro al comando stesso
 *      esempio:
 *          !help login
 *  Per il comando login (parametro) chiamo la funzione seguente.
 */
int getMe_code_number_associate_at_the_params_comand(char* cmd, int n_cmd){
    int i, sum_characters = 0;

    for(i = 0; i < n_cmd; i ++)
            sum_characters += (int)cmd[i] * (i + 1);

    return sum_characters;
}

/*
 *  Questo metodo restituisce una stringa dove viene spiegato il funzionamento
 *  del comando che gli viene passato come parametro.
 *  Il parametro formale e' il codice associato al comando
 *  Questo metodo viene chiamato sia quando viene chiamato il comando !help
 *  sia quando vengono sbagliati i comandi, proponendo un esempio del loro
 *  corretto utilizzo.
 */
void getMe_comand_descriptions(int code){
    printf("DESCRIZIONE COMANDI:\n");
    switch(code){
        case HELP :
            printf("!help <comando> --> mostra i dettagli di un comando\n");
            break;
        case ESCI :
            printf("!esci --> termina il client\n");
            break;
        case SIGNUP :
            printf("!signup <username> <password> --> crea un nuovo utente\n");
            break;
        case LOGIN :
            printf("!login <username> <password> --> autentica un utente\n");
            break;
        case INVIA_GIOCATA :
            printf("!invia_giocata g --> invia una giocata g al server\n");
            break;
        case VEDI_GIOCATE :
            printf("!vedi_giocate tipo --> visualizza le giocate precedenti dove tipo = {0,1} e permette di visualizzare le giocate passate ‘0’ oppure le giocate attive ‘1’ (ancora non estratte)\n");
            break;
        case VEDI_ESTRAZIONE :
            printf("!vedi_estrazione <n> <ruota> --> mostra i numeri delle ultime n estrazioni sulla ruota specificata\n");
            break;
        case VEDI_VINCITE :
            printf("!vedi_vincite --> richiede al server tutte le vincite del client, l’estrazione in cui sono state realizzate e un consuntivo per tipologia d giocata.\n");
            break;
        default :
            printf("!help <comando> --> mostra i dettagli di un comando\n");
            printf("!esci --> termina il client\n");
            printf("!signup <username> <password> --> crea un nuovo utente\n");
            printf("!login <username> <password> --> autentica un utente\n");
            printf("!invia_giocata g --> invia una giocata g al server\n");
            printf("!vedi_giocate tipo --> visualizza le giocate precedenti dove tipo = {0,1} e permette di visualizzare le giocate passate ‘0’ oppure le giocate attive ‘1’ (ancora non estratte)\n");
            printf("!vedi_estrazione <n> <ruota> --> mostra i numeri delle ultime n estrazioni sulla ruota specificata\n");
            printf("!vedi_vincite --> richiede al server tutte le vincite del client, l’estrazione in cui sono state realizzate e un consuntivo per tipologia d giocata.\n");
            break;
    }
}

void show_the_menu(){
    printf(">> BENVENUTO AL GIOCO LOTTO <<\n");
    printf("I comandi disponibili per interagire sono:\n");
    printf("    !help <comando>\n");
    printf("    !signup <username> <password>\n");
    printf("    !login <username> <password>\n");
    printf("    !invia_giocata <schedina>\n");
    printf("    !vedi_giocate <tipo>\n");
    printf("    !vedi_estrazione <n> <ruota>\n");
    printf("    !vedi_vincite\n");
    printf("    !esci\n\n");
}
