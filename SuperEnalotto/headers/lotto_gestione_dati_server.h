#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define N_RUOTE 11
#define N_NUMERI 10
#define N_IMPORTI 5
#define N_BETS 5
#define TIME_EXTRAXTION 158158158

/* File con cui fa la BIND il socketForked definito nel sorgente lotto_server.c */
/* Questo file mi permette di comunicare con il processo padre attraverso i socket AF_UNIX/LOCAL */
#define ID "./sockForked"

/* Strutture dati per la gestione delle ruote */
char all_RUOTE[N_RUOTE][9] = {"bari","cagliari","firenze","genova","milano","napoli","palermo","roma","torino","venezia","tutte"};
char all_BETS[N_BETS][9] = {"estratto","ambo","terno","quaterna","cinquina"};
float all_WINNINGS[5] = {11.23, 250, 4500, 120000, 6000000};

struct played_or_extracted_date{
    int minute;
    int hour;
    int day;
    int month;
    int year;
};

/* Struttura dati schedina */
struct game_card{
    int r[11];
    int n[10];
    float i[5];
    int extracted;
    struct played_or_extracted_date played_date;
    struct played_or_extracted_date extracted_date;
    struct game_card* next;
};

/***********************************************************************************************/
/********************************** DICHIARAZIONE METODI ***************************************/
/***********************************************************************************************/

int getMe_RUOTE_associate_pointer(char* r, int n_r);
int* getMe_RUOTE(char* params, int n_params);
int* getMe_NUMERI(char* params, int n_params);
float* getMe_IMPORTI_scommessi(char* params, int n_params);
struct game_card* create_game_card(char* params, int n_params);
void stamp_card(struct game_card* gc);
void save_game_card_at_personal_file(struct game_card* gc, char* user);
char* get_game_cards_at_personal_file(char* user, int cards_extracted);
struct game_card* decode_game_cards_with_saved_format(char* card_buffer);
char* get_me_substring(char* buffer, int end, int start);
void stamp_game_cards_with_saved_format(char* buffer);
char* make_numbers_extractions();
char* getMe_n_extractions(int n, char* ruota);
void add_new_game_card_at_list(struct game_card** l, struct game_card* gc);
struct game_card* getMe_list_of_games_card(char* buffer);
int getMe_user_subscribers_list_length();
char** getMe_user_subscribers_list();
void save_numbers_extraction(char* extracted);
int calcola_fattoriale(int fat);
void match_game_card_with_extraction(char* extr_n, struct game_card* gc, char* user, int start);
void change_state_games_card(char* user);
void start_process();
float get_tot_of_specific_bets(char* buffer, char* bet_type);
char* get_wins(char* user);

/******************* METODO CHE SI OCCUPA DELLE ESTRAZIONI *********************/
    void start_parallel_process_son_for_make_extractions(int minutes);
/*******************************************************************************/

/***********************************************************************************************/
/******************************* FINE DICHIARAZIONE METODI *************************************/
/***********************************************************************************************/

/*  Questa funzione, data una stringa facente parte della matrice all_RUOTE,
 *  mi restituisci l'indice dell'array associato alla ruota mandata come parametro stringa
 *
 *  Se la ruota non viene trovata perche sbagliato il nome della ruota o per altri motivi,
 *  il metodo restituisce -1
 */
int getMe_RUOTE_associate_pointer(char* r, int n_r){
    int i;

    for(i = 0; (i < N_RUOTE) && (strcmp(r, all_RUOTE[i])); i ++){ ; }

    return i;
}

/*  Questa funzione analizza la stringa mandata dal client e ritorna solo le ruote
 *  NB: se un utente scrive male una ruota, non viene riconosciuta e quindi nella schedina
 *  non verrà salvata, mentre tutte altre scritte in maniera corretta si.
 */
int* getMe_RUOTE(char* params, int n_params){
    int i, j, z, pointer;
    int* ruote;
    char tmp_r[9];

    ruote = malloc(sizeof(int) * N_RUOTE);

    for(i = 0; i < N_RUOTE; i ++)
        ruote[i] = 0;

    for(i = 0; i < n_params; i ++){
        if(params[i] == '-'){
            i += 1;
            if(params[i] == 'r'){
                i += 2;
                z = 0;
                for(j = i; (j < n_params) && (params[j] != '-'); j ++){
                    if(params[j] != ' '){
                        tmp_r[z] = params[j];
                        z += 1;
                    }else{
                        tmp_r[z] = '\0';
                        pointer = getMe_RUOTE_associate_pointer(tmp_r, strlen(tmp_r));
                        if(pointer != N_RUOTE)
                            ruote[pointer] = 1;
                        z = 0;
                    }
                }
                i = j;
            }
        }
    }

    return ruote;
}

/*  Questa funzione analizza la stringa mandata dal client e ritorna solo i
 *  numeri scommessi
 */
int* getMe_NUMERI(char* params, int n_params){
    int i, j, z, pointer;
    int* numbers;
    char tmp_n[10];

    numbers = malloc(sizeof(int) * N_NUMERI);

    for(i = 0; i < N_NUMERI; i ++)
        numbers[i] = 0;

    for(i = 0; i < n_params; i ++){
        if(params[i] == '-'){
            i += 1;
            if(params[i] == 'n'){
                i += 2;
                z = 0;
                pointer = 0;
                for(j = i; (j < n_params) && (params[j] != '-'); j ++){
                    if(params[j] != ' '){
                        tmp_n[z] = params[j];
                        z += 1;
                    }else{
                        tmp_n[z] = '\0';
                        numbers[pointer] = atoi(tmp_n);
                        z = 0;
                        pointer += 1;
                        if(pointer >= N_NUMERI)
                            return numbers;
                    }
                }
                i = j;
            }
        }
    }

    return numbers;
}

/*  Questa funzione analizza la stringa mandata dal client e ritorna solo gli
 *  importi scommessi
 */
float* getMe_IMPORTI_scommessi(char* params, int n_params){
    int i, j, z, pointer;
    float* importi;
    char tmp_i[10];

    importi = malloc(sizeof(float) * N_IMPORTI);

    for(i = 0; i < N_IMPORTI; i ++)
        importi[i] = 0;

    for(i = 0; i < n_params; i ++){
        if(params[i] == '-'){
            i += 1;
            if(params[i] == 'i'){
                i += 2;
                z = 0;
                pointer = 0;
                for(j = i; (j < n_params) && (params[j] != '-'); j ++){
                    if(params[j] != ' '){
                        tmp_i[z] = params[j];
                        if((j + 1) == n_params){
                            z += 1;
                            tmp_i[z] = params[j + 1];
                            z += 1;
                            tmp_i[z] = '\0';
                            importi[pointer] = atof(tmp_i);
                        }
                        z += 1;
                    }else{
                        tmp_i[z] = '\0';
                        importi[pointer] = atof(tmp_i);
                        z = 0;
                        pointer += 1;
                    }
                    if(pointer >= N_IMPORTI)
                        return importi;
                }
                i = j;
            }
        }
    }

    return importi;
}

/*  Questa funzione crea una schedina, passati in ingresso i SOLI PARAMETRI inviati
 *  dal client. Solo i parametri, senza il comando.
 */
struct game_card* create_game_card(char* params, int n_params){
    int* r, * n, j;
    float* i;
    struct game_card* gc;
    time_t t;
    struct tm* date_time;

    gc = malloc(sizeof(struct game_card));

    r = getMe_RUOTE(params, strlen(params));
    n = getMe_NUMERI(params, strlen(params));
    i = getMe_IMPORTI_scommessi(params, strlen(params));

    for(j = 0; j < N_RUOTE; j ++)
        gc->r[j] = r[j];

    for(j = 0; j < N_NUMERI; j ++)
        gc->n[j] = n[j];

    for(j = 0; j < N_IMPORTI; j ++)
        gc->i[j] = i[j];

    gc->extracted = 0;

    //Recupero la data odierna e locale
    time(&t);
    date_time = localtime(&t);

    gc->played_date.minute = date_time->tm_min;
    gc->played_date.hour = date_time->tm_hour;
    gc->played_date.day = date_time->tm_mday;
    gc->played_date.month = date_time->tm_mon + 1;
    gc->played_date.year = date_time->tm_year + 1900;

    gc->extracted_date.minute = 0;
    gc->extracted_date.hour = 0;
    gc->extracted_date.day = 0;
    gc->extracted_date.month = 0;
    gc->extracted_date.year = 0;

    return gc;
}

/*  Questa funzione stampa una schedina in formato struttura
 */
void stamp_card(struct game_card* gc){
    int j;

    for(j = 0; j < N_RUOTE; j ++)
        if(gc->r[j] != 0)
            printf("%s ", all_RUOTE[j]);

    for(j = 0; (j < N_NUMERI) && (gc->n[j] != 0); j ++)
        printf("%d ", gc->n[j]);

    for(j = (N_IMPORTI - 1); (j >= 0); j --)
        if(gc->i[j] != 0)
            printf("* %0.2f %s ", gc->i[j], all_BETS[j]);

    printf("\n");
}

/*  Questa funzione salva una schedina giocata nel file personale dell'utente
 */
void save_game_card_at_personal_file(struct game_card* gc, char* user){
    int i, j;
    char game_card[512];
    char tmp_buf[256];
    char* path;

    path = create_path_for_user_files(user, "./Utenti/");

    sprintf(tmp_buf, "&%d\n", gc->extracted);
    strcpy(game_card, tmp_buf);

    sprintf(tmp_buf, "%d %d %d %d %d\n", gc->played_date.year, gc->played_date.month, gc->played_date.day, gc->played_date.hour, gc->played_date.minute);
    strcat(game_card, tmp_buf);

    sprintf(tmp_buf, "%d %d %d %d %d\n", gc->extracted_date.year, gc->extracted_date.month, gc->extracted_date.day, gc->extracted_date.hour, gc->extracted_date.minute);
    strcat(game_card, tmp_buf);

    for(i = 0; i < N_RUOTE - 1; i ++){
        sprintf(tmp_buf, "%d ", gc->r[i]);
        strcat(game_card, tmp_buf);
    }
    sprintf(tmp_buf, "%d", gc->r[i]);
    strcat(game_card, tmp_buf);
    sprintf(tmp_buf, "\n");
    strcat(game_card, tmp_buf);

    for(i = 0; i < N_NUMERI - 1; i ++){
        sprintf(tmp_buf, "%d ", gc->n[i]);
        strcat(game_card, tmp_buf);
    }
    sprintf(tmp_buf, "%d", gc->n[i]);
    strcat(game_card, tmp_buf);
    sprintf(tmp_buf, "\n");
    strcat(game_card, tmp_buf);

    for(j = 0; j < N_IMPORTI - 1; j ++){
        sprintf(tmp_buf, "%0.2f ", gc->i[j]);
        strcat(game_card, tmp_buf);
    }
    sprintf(tmp_buf, "%0.2f", gc->i[j]);
    strcat(game_card, tmp_buf);
    sprintf(tmp_buf, "\n");
    strcat(game_card, tmp_buf);

    write_in_a_specific_file_with_lock(game_card, path);

}

/*  Questa funzione ritorna un buffer di caratteri con tutte le schedine verificate
 *  o non verificate dell'utente passato come parametro. Il parametro cards_extracted
 *  permette di scegliere quale tipo di schedina prendere in considerazione. Quelle
 *  gia verificate dal processo astrazione oppure quelle giocate, ma non ancora verificate
 *  -> cards_extracted = 1 : allora ritorna quelle verificate
 *  -> cards_extracted = 0 : allora ritorna le schedine giocate, ma non ancora verificate
 *                           dal processo estrazione
 */
char* get_game_cards_at_personal_file(char* user, int cards_extracted){
    char* buffer, crd_extr[2];
    char* game_cards_extracted, * game_cards_not_extracted;
    int i, k, j, x, extracted_length, not_extracted_length;
    int file_length, cards_work, start;
    char* path;

    path = create_path_for_user_files(user, "./Utenti/");

    buffer = read_in_a_specific_file_with_lock(path);

    file_length = strlen(buffer);

    for(i = 0; (i < file_length) && (buffer[i] != '\n'); i ++){ ; }

    if((i + 1) >= file_length)
        return NULL;
    else{
        start = i += 1;
        extracted_length = 0;
        not_extracted_length = 0;

        for(i = start; i < file_length; i ++){
            if(buffer[i] == '&'){
                crd_extr[0] = buffer[i + 1];
                crd_extr[1] = '\0';
                if(atoi(crd_extr) == 1){
                    extracted_length += 1;
                    cards_work = 1;
                }else{
                    not_extracted_length += 1;
                    cards_work = 0;
                }
            }else{
                if(cards_work)
                    extracted_length += 1;
                else
                    not_extracted_length += 1;
            }
        }

        game_cards_extracted = malloc(extracted_length + 1);
        game_cards_not_extracted = malloc(not_extracted_length + 1);

        k = j = 0;

        for(i = start; i < file_length; i ++){
            if(buffer[i] == '&'){
                crd_extr[0] = buffer[i + 1];
                crd_extr[1] = '\0';
                if(atoi(crd_extr) == 1){
                    game_cards_extracted[k] = buffer[i];
                    k += 1;
                    cards_work = 1;
                }else{
                    game_cards_not_extracted[j] = buffer[i];
                    j += 1;
                    cards_work = 0;
                }
            }else{
                if(cards_work){
                    game_cards_extracted[k] = buffer[i];
                    k += 1;
                }else{
                    game_cards_not_extracted[j] = buffer[i];
                    j += 1;
                }
            }
        }

        game_cards_extracted[k] = '\0';
        game_cards_not_extracted[j] = '\0';

        if(cards_extracted){
            free(game_cards_not_extracted);
            return game_cards_extracted;
        }else{
            free(game_cards_extracted);
            return game_cards_not_extracted;
        }
    }
}

/*  Questa funzione serve per decodificare una schedina salvata su file e trasformarla
 *  in una schedina vera e propria nel suo formato struttura.
 *  Quando si va a leggere una certa schedina da file si utilizza questa funzione
 *  per renderla leggibile ed accessibile in tutti i suoi campi.
 */
struct game_card* decode_game_cards_with_saved_format(char* card_buffer){
    int i, j, k, x, s, y, card_buffer_len, carry_line;
    char tmp_num_char[10];
    struct game_card* gc;

    int val;

    gc = malloc(sizeof(struct game_card));

    card_buffer_len = strlen(card_buffer);

    carry_line = 0;

    for(i = 0; (i < card_buffer_len) && (carry_line < 3); i ++)
        if(card_buffer[i] == '\n')
            carry_line += 1;

    j = 0;
    // Recupero le ruote
    for(; (i < card_buffer_len) && (card_buffer[i] != '\n'); i ++){
        if(card_buffer[i] != ' '){
            tmp_num_char[0] = card_buffer[i];
            tmp_num_char[1] = '\0';
            gc->r[j] = atoi(tmp_num_char);
            j += 1;
        }
    }

    i += 1; j = 0; x = 0;
    //Recupero i numeri
    for(; (i < card_buffer_len) && (card_buffer[i] != '\n'); i ++){
        if(card_buffer[i] != ' '){
            tmp_num_char[x] = card_buffer[i];
            x += 1;
            if(card_buffer[i + 1] == '\n'){
                tmp_num_char[x] = '\0';
                gc->n[j] = atoi(tmp_num_char);
            }
        }else{
            tmp_num_char[x] = '\0';
            gc->n[j] = atoi(tmp_num_char);
            x = 0;
            j += 1;
        }
    }

    i += 1; j = 0; x = 0;

    y = i;
    //Recupero gli importi
    for(; y < strlen(card_buffer); y ++){
        if(card_buffer[y] != ' '){
            tmp_num_char[x] = card_buffer[y];
            x += 1;
            if(card_buffer[y + 1] == '\n'){
                tmp_num_char[x] = '\0';
                gc->i[j] = atof(tmp_num_char);
                j += 1;
                x = 0;
            }
        }else{
            tmp_num_char[x] = '\0';
            gc->i[j] = atof(tmp_num_char);
            j += 1;
            x = 0;
        }
    }

    return gc;
}

/*  Questa funzione, passato un buffer e due indici facenti parte del dominio del buffer
 *  mi restituisce una sottostringa del buffer stesso che parte dall indice start e
 *  finisce all'indice end
 */
char* get_me_substring(char* buffer, int end, int start){
    int buffer_len, i, j;
    char* sub_buffer;

    buffer_len = strlen(buffer);

    sub_buffer = malloc((end - start) + 1);

    j = 0;
    for(i = start; (i <= end) && (i < buffer_len); i ++){
        sub_buffer[j] = buffer[i];
        j += 1;
    }

    sub_buffer[j] = '\0';

    return sub_buffer;
}

/*  Questa funzione stampa piu schedine passate come stringa, cioè nel formato in
 *  in cui sono state salvate. Senze decodificarle, leggo il file e ne prendo il contenuto,
 *  ne passo il buffer a questa funzione e fa tutto lei, stampandole.
 */
void stamp_game_cards_with_saved_format(char* buffer){
    int i, start, buffer_len, how_many_cards;
    char* tmp_buf_card;
    struct game_card* gc;

    if(buffer == NULL)
        return;

    buffer_len = strlen(buffer);

    how_many_cards = 0;

    for(i = 0; i < buffer_len; i ++){
        if(buffer[i] == '&')
            how_many_cards += 1;
    }

    start = 0;

    if(how_many_cards > 1){

        for(i = 0; i < buffer_len; i ++){
            if((buffer[i] == '&') && (i != 0)){
                tmp_buf_card = get_me_substring(buffer, (i - 1), start);
                start = i;
                gc = decode_game_cards_with_saved_format(tmp_buf_card);
                stamp_card(gc);
            }
            if(i == buffer_len - 1){
                tmp_buf_card = get_me_substring(buffer, i, start);
                gc = decode_game_cards_with_saved_format(tmp_buf_card);
                stamp_card(gc);
            }
        }
    }else{
            tmp_buf_card = get_me_substring(buffer, buffer_len, start);
            gc = decode_game_cards_with_saved_format(tmp_buf_card);
            stamp_card(gc);
    }
}

/*  Questa funzione crea l'estrazione e la restituisce sottoforma di buffer di caratteri
 *  L'estrazione è composta dalla data e dall ora, e da tutti i numeri di tutte le varie ruote
 */
char* make_numbers_extractions(){
    int** num;
    char* num_buffer;
    char buf[15];
    int i, j, k, tmp_num;
    time_t t, t2;
    struct tm* date_time;

    srand((unsigned) time(&t));

    num = (int**) malloc (11 * sizeof(int*));

    for(i = 0; i < 11; i ++)
        num[i] = (int*) malloc (5 * sizeof(int));

    num_buffer = (char*) malloc (12 * 15 * sizeof(char) + 2); // fine stringa piu un a capo finale per uno spazio vuoto

    //Recupero la data odierna e locale
    time(&t2);
    date_time = localtime(&t2);

    sprintf(buf, "%d %d %d %d %d\n", date_time->tm_year + 1900 - 2000, date_time->tm_mon + 1, date_time->tm_mday, date_time->tm_hour, date_time->tm_min);
    strcpy(num_buffer, buf);

    for(i = 0; i < 11; i ++){
        for(j = 0; j < 5; j ++){
            if(j == 0)
                num[i][j] = ((rand() % 90) + 1);

            else{
                tmp_num = ((rand() % 90) + 1);

                for(k = 0; (k < j) && (num[i][k] != tmp_num); k ++){ ; }

                if(k < j)
                    j -= 1;
                else
                    num[i][j] = tmp_num;
            }
        }

        sprintf(buf, "%d %d %d %d %d\n", num[i][0], num[i][1], num[i][2], num[i][3], num[i][4]);
        strcat(num_buffer, buf);
    }

    strcat(num_buffer, "\n");

    return num_buffer;
}

/*  Questa funzione restituisce le n estrazioni della ruota passata per argomento.
 *  Se la ruota non è passata per argomento, restituisce tutte le n estrazioni di
 *  tutte le ruote
 */
char* getMe_n_extractions(int n, char* ruota){
    char* buffer, * buf_extractions, * buf_decode;
    int i, carry_line, extractions, data_pointer;

    buffer = read_in_a_specific_file_with_lock("./Estrazioni/Estrazioni.txt");

    if(strlen(buffer) == 0 || buffer == NULL)
        return NULL;

    extractions = 0;
    carry_line = 0;
    for(i = 0; (i < strlen(buffer)) && (extractions < n); i ++){
        if(buffer[i] == '\n')
            carry_line += 1;
        else
            carry_line = 0;

        if(carry_line == 2){
            extractions += 1;
        }
    }

    buf_extractions = get_me_substring(buffer, (i - 1), 0);

    if(ruota != NULL){
        int j = 0, data_pointer = 0, k = 0, x, ruota_pointer = 0, take_ruota = 0, num_ext = 0;
        carry_line = 0;
        char tmp_ruota[3];

        buf_decode = malloc((n * 32) + 2);

        for(i = 0; i < strlen(buf_extractions); i ++){

            if(buffer[i] == '\n')
                carry_line += 1;
            else
                carry_line = 0;

            if((take_ruota == getMe_RUOTE_associate_pointer(ruota, 0)) && (buf_extractions[i] != '\n') && (data_pointer == 1)){
                buf_decode[j] = buf_extractions[i];
                j += 1;
            }

            if((take_ruota == getMe_RUOTE_associate_pointer(ruota, 0)) && (buf_extractions[i] == '\n') && (data_pointer == 1)){
                buf_decode[j] = '\n';
                j += 1;
            }

            if((ruota_pointer == 1) && (buf_extractions[i] == '\n')){
                take_ruota += 1;
            }

            if(data_pointer == 0 && buf_extractions[i] == '\n'){
                data_pointer = 1;
                ruota_pointer = 1;
                buf_decode[j] = '\n';
                j += 1;
            }

            if((data_pointer == 0) && (buf_extractions[i] != '\n')){
                buf_decode[j] = buf_extractions[i];
                j += 1;
            }

            if(carry_line == 2){
                data_pointer = 0;
                take_ruota = 0;
                ruota_pointer = 0;
                num_ext += 1;
            }

        }

        buf_decode[j - 1] = '\0';

        return buf_decode;

    }else
        return buf_extractions;
}

/*  Questa funzione aggiunge un elemento di tipo schedina in una lista di schedine
 *  Le game_cards sono schedine
 */
void add_new_game_card_at_list(struct game_card** l, struct game_card* gc){
    struct game_card* p;

    if(!(*l)){
        gc->next = (*l);
        (*l) = gc;
    }else{
        for(p = (*l); p->next != 0; p = p->next){ ; }

        p->next = gc;
        gc->next = 0;
    }
}

void delete_geme_card_list(struct game_card** l){
    struct game_card* q, * p;

    if(!(*l))
        return;
    else{
        while((*l) != 0){
            for(q = (*l); q->next != 0; q = q->next){
                p = q;
            }

            if(q == (*l)){
                (*l) = 0;
                free(q);
            }else{
                p->next = 0;
                free(q);
            }
        }
    }

}

/*  Questa funzione prende in ingresso le schedine nel formato in cui sono state salvate,
 *  ovvero un buffer di caratteri. Ne crea una lista decodificandole e portandole nel formato
 *  struttura game_card. Creata la lista di schedine, la restituisce al chiamante sottoforma
 *  di lista. Restituisce il puntatore alla testa della lista creata.
 */
struct game_card* getMe_list_of_games_card(char* buffer){
    int i, start, buffer_len, how_many_cards, point;
    char* tmp_buf_card;
    struct game_card* gc_list;
    struct game_card* gc, * tmp;

    gc_list = 0;

    if(buffer == NULL)
        return gc_list;

    buffer_len = strlen(buffer);

    if(buffer_len != 0){
        how_many_cards = 0;

        for(i = 0; i < buffer_len; i ++){
            if(buffer[i] == '&')
                how_many_cards += 1;
        }

        start = 0;

        if(how_many_cards > 1){

            for(i = 0; i < buffer_len; i ++){
                if((buffer[i] == '&') && (i != 0)){
                    tmp_buf_card = get_me_substring(buffer, (i - 1), start);
                    start = i;
                    gc = decode_game_cards_with_saved_format(tmp_buf_card);
                    add_new_game_card_at_list(&gc_list, gc);
                }
                if(i == buffer_len - 1){
                    tmp_buf_card = get_me_substring(buffer, i, start);
                    gc = decode_game_cards_with_saved_format(tmp_buf_card);
                    add_new_game_card_at_list(&gc_list, gc);
                }
            }
        }else{
            tmp_buf_card = get_me_substring(buffer, buffer_len, start);
            gc = decode_game_cards_with_saved_format(tmp_buf_card);
            add_new_game_card_at_list(&gc_list, gc);
        }

    }else
        gc_list = 0;

    return gc_list;
}

/*  Questa funzione legge nel file "every_user_list.txt", dove sono presenti tutti
 *  tutti gli iscritti al server. Ne restituisce il numero di persone che sono iscritte
 */
int getMe_user_subscribers_list_length(){
    int list_length, i;
    char* file_name_list;

    file_name_list = read_in_a_specific_file_with_lock("./every_user_list.txt");

    list_length = 0;

    for(i = 0; i < strlen(file_name_list); i ++){
        if(file_name_list[i] == '\n')
            list_length += 1;
    }

    return list_length;
}

/*  Questa funzione legge nel file "every_user_list.txt", dove sono presenti tutti
 *  tutti gli iscritti al server. Ne restituisce una matrice di caratteri o array di
 *  stringhe dove sono presenti tutti i nomi degli iscritti al server.
 */
char** getMe_user_subscribers_list(){
    char* file_name_list;
    char** list;
    int i, list_length, j, len_list;

    file_name_list = read_in_a_specific_file_with_lock("./every_user_list.txt");

    list_length = 0;

    for(i = 0; i < strlen(file_name_list); i ++){
        if(file_name_list[i] == '\n')
            list_length += 1;
    }

    list = (char**)malloc(list_length * sizeof(char*));

    for(i = 0; i < list_length; i ++)
        list[i] = (char*)malloc(40 * sizeof(char));

    len_list = list_length;
    j = 0;
    list_length = 0;
    for(i = 0; (i < strlen(file_name_list)) && (list_length < len_list); i ++){
        if(file_name_list[i] == '\n'){
            list[list_length][j] = '\0';
            list_length += 1;
            j = 0;
        }else{
            list[list_length][j] = file_name_list[i];
            j += 1;
        }
    }

    return list;

}

/*  Questa funzione salva nel rispettivo file "Estrazioni.txt" l'estrazione avvenuta.
 *  Sempre in formato array di caratteri.
 */
void save_numbers_extraction(char* extracted){
    char* buffer;
    char* all_buffer;

    buffer = read_in_a_specific_file_with_lock("./Estrazioni/Estrazioni.txt");

    all_buffer = malloc(strlen(buffer) + strlen(extracted) + 1);

    strcpy(all_buffer, extracted);
    strcat(all_buffer, buffer);

    replace_all_content_in_a_specific_file_with_lock(all_buffer, "./Estrazioni/Estrazioni.txt");

    free(buffer);
    free(all_buffer);

}

/*  Questa funzione calcola il fattoriale di un numero passato come argomento
 */
int calcola_fattoriale(int fat){
    int ris;
    ris = fat;

    if((fat == 0) || (fat == 1))
        return 1;

    while(fat > 1){
        fat -= 1;
        ris *= fat;
    }

    return ris;
}

/*  Questa funzione è molto importante.
 *  Passata una sola schedina in formato struttura e non in formato di salvataggio,
 *  viene comparato il contenuto della schedina e il contenuto dell estrazione in
 *  modo da vedere se il contenuto della schedina contiene una vittoria o meno.
 *  Quindi ci passo l'estrazione in formato buffer di caratteri, una schedina, lo
 *  username del proprietario della schedina ed infine una variabile che se vale 1
 *  comprende anche la data nel buffer finale che viene creato.
 *  Questo buffer creato viene salvato nel file personale nella cartella Vincite.
 *  L'utente in questo modo potrà richidere un consuntivo delle sue vincite inerenti
 *  alle giocate fatte.
 */
void match_game_card_with_extraction(char* extr_n, struct game_card* gc, char* user, int start){
    int z, j, i, k, x;
    int totale_indovinati, num_giocati, num_ruote, alternate;
    int winnings[5] = {0, 0, 0, 0, 0};
    char buf_tmp[5][14] = {"Estrazione 20", "-", "-", " ", ":"};
    char buffer_date[100];
    char buffer[1024];
    char tmp_num[25];
    char* path_user;
    int extracted[11][5];
    int extracted_n, pointer;
    char num[6];
    int indovinati_singola_ruota[11];

    /* Inizializzo il vettore per contenere i numeri indovinati in ogni singola ruota */
    for(i = 0; i < 11; i ++)
        indovinati_singola_ruota[i] = 0;


    extracted_n = strlen(extr_n);

    /* Inizializzo la matrice che conterrà i numeri estratti in ogni ruota */
    for(i = 0; i < 11; i ++){
        for(j = 0; j < 5; j ++)
            extracted[i][j] = 0;
    }

    /* Avvio la procedura di decodifica del buffer che contiene l'estrazione */
    /* Alla fine di questa procedura avrò una matrice di numeri contenenti le estrazioni */
    for(i = 0; (i < extracted_n) && (extr_n[i] != '\n'); i ++){ ; }

    i += 1;
    pointer = 0;
    x = 0;
    j = 0;

    for(; (i < extracted_n) && (pointer < 11); i ++){
        //printf("pointer: %d\n", pointer);
        if((extr_n[i] != ' ') && (extr_n[i] != '\n')){
            num[x] = extr_n[i];
            x += 1;
        }else{
            if(extr_n[i] != '\n'){
                num[x] = '\0';
                extracted[pointer][j] = atoi(num);
                j += 1;
                x = 0;
            }else{
                num[x] = '\0';
                x = 0;
                extracted[pointer][j] = atoi(num);
                pointer += 1;
                j = 0;
            }
        }
    }

    /*  Inizio procedura per il calcolo delle vincite contenute nella schedina
     *  passata come argomento.
     */
    totale_indovinati = 0;
    num_giocati = 0;
    num_ruote = 0;

    /* Recupero i numeri totali giocati nella schedina */
    for(i = 0; (i < N_NUMERI) && (gc->n[i] != 0); i ++)
        num_giocati += 1;

    /* Recupero il numero delle ruote contenute nella schedina */
    /* Recupero i numeri totali indovinati in tutte le ruote della schedina */
    /* Recupero i numeri indovinati in una singola ruota e lo salvo in un array
     *  dove l'indice dell array rappresenta la ruota stessa. L'ordine è lo stesso
     *  del vettore contenente tutte le ruote "all_RUOTE"
     */
    for(z = 0; z < 11; z ++){

        if(gc->r[z] == 1){
            num_ruote += 1;

            for(j = 0; j < 5; j ++){

                for(k = 0; k < N_NUMERI; k ++){

                    if(extracted[z][j] == gc->n[k]){

                        indovinati_singola_ruota[z] += 1;
                        totale_indovinati += 1;

                    }

                }

            }
        }
    }

    /*  Se è presente la giocata Nazionale su tutte le ruote, allora in automatico
     *  il numero di ruote sarà 11, cioè tutte le ruote
     */
    if(gc->r[10] != 0)
        num_ruote = 11;

    /*  Se questa variabile vale 1 viene creata anche la data
     *  Questa funzione viene utilizzata all interno di un ciclo dove vengono
     *  passate tutte le schedine. Se è la prima schedina passata, stampa prima
     *  la data prima del contenuto o vincita della schedina.
     */
    if(start == 1){

        j = 0;
        alternate = 1;
        k = 0;
        x = 1;

        for(i = 0; i < strlen(buf_tmp[0]); i ++){
            buffer[j] = buf_tmp[0][i];
            j += 1;
        }

        for(i = 0; (i < strlen(extr_n)) && (extr_n[i] != '\n'); i ++){
            if(extr_n[i] != ' '){
                buffer[j] = extr_n[i];
                j += 1;
            }else{
                buffer[j] = buf_tmp[x][0];
                x += 1;
                j += 1;
            }
        }

        buffer[j] = '\n';
        j += 1;
        buffer[j] = '\0';

    }else
        strcpy(buffer, "");

    /* Recupero ruote, numeri e importi della schedina per il salvataggio */
    for(j = 0; j < N_RUOTE; j ++){
        if(gc->r[j] !=  0){
            strcat(buffer, all_RUOTE[j]);
            strcat(buffer, " ");
        }
    }

    for(j = 0; (j < N_NUMERI); j ++){
        if(gc->n[j] != 0){
            sprintf(tmp_num, "%d ", gc->n[j]);
            strcat(buffer, tmp_num);
        }
    }

    strcat(buffer, ">> ");

    /* In questa parte calcolo le vincite della schedina */
    if(totale_indovinati == 0){
        for(j = (N_IMPORTI - 1); (j >= 0); j --){
            if(gc->i[j] != 0){
                sprintf(tmp_num, "%s 0.00 ", all_BETS[j]);
                strcat(buffer, tmp_num);
            }
        }
    }else{
        /* Mi serve sapere per ogni scommessa e per ogni ruota, quindi mi serve un array */
        float vincita[5] = {0, 0, 0, 0, 0};

        /* Scorro tutte le vincite in ogni ruota */
        for(j = 0; j < 11; j ++){

            if(indovinati_singola_ruota[j] != 0){

                /*  Per ogni ruota guardo i numeri indovinati. Scorro le scommesse al
                 *  contrario in modo da fermarmi sulla maggiore scommessa indovinata
                 */
                for(z = (N_IMPORTI - 1); z >= 0; z --){

                    if(gc->i[z] != 0){

                        if((z + 1) <= indovinati_singola_ruota[j]){
                            float tmp_vinc = 0;

                            /* Qui calcolo la vittoria della schedina */
                            /* Calcolo le combinazioni semplici per quando mi trovo davanti a piu combinazioni */
                            int combinazione = (calcola_fattoriale(num_giocati)/(calcola_fattoriale((z + 1)) * calcola_fattoriale(num_giocati - (z + 1))));
                            tmp_vinc += gc->i[z] * (all_WINNINGS[z]/(num_ruote * combinazione));

                            /* Se il tipo di scommessa è piu di uno in una ruota */
                            /* Esempio: scommetto un estratto e gioco due numeri. Indovino i due numeri e quindi sono due estratti nella stessa ruota */
                            if(((indovinati_singola_ruota[j] % (z + 1)) == 0) && (indovinati_singola_ruota[j] > (z + 1)))
                                tmp_vinc *= (indovinati_singola_ruota[j] / (z + 1));

                            vincita[z] += tmp_vinc;

                            break;

                        }
                    }

                }

            }

        }
        /* Metto in ordine le vincite calcolate per il formato di salvataggio */
        for(j = (N_IMPORTI - 1); (j >= 0); j --){
            if(gc->i[j] != 0){
                sprintf(tmp_num, "%s %0.2f ", all_BETS[j], vincita[j]);
                strcat(buffer, tmp_num);
            }
        }

    }


    strcat(buffer, "\n");

    /* Salvo nel rispettivo file la schedina */
    path_user = create_path_for_user_files(user, "./Vincite/");
    write_in_a_specific_file_with_lock(buffer, path_user);
}

/*  Questa funzione, per ogni schedina verificata dal processo di estrazione, ne
 *  cambia il valore da 0 a 1, in modo da non verificarle piu nella estrazione
 *  successiva. Legge carattere per carattere, quando trova trova l'intervallo di
 *  interesse, cioè il carattere di inizio schedina &, cambia il carattere subito
 *  successivo che rappresenta lo stato della schedina stessa.
 */
void change_state_games_card(char* user){
    char* path_user;
    char* content_file;
    int i;

    path_user = create_path_for_user_files(user, "./Utenti/");

    content_file = read_in_a_specific_file_with_lock(path_user);

    for(i = 0; i < strlen(content_file); i ++){
        if(content_file[i] == '&'){
            i += 1;
            if(content_file[i] == '0')
                content_file[i] = '1';
        }
    }

    replace_all_content_in_a_specific_file_with_lock(content_file, path_user);

}

/* PROCESSO CHE RACCHIUDE TUTTI I PASSAGGI INERENTI ALL'ESTRAZIONE */
void start_process(){
    int i, j, subscribers_list_length, point;
    char* extraction_numbers;
    int** extr_numbers;
    char** subscribers_list;
    char* every_geme_card_of_user_not_verify;
    char* line = "******************************************************************\n";
    int stamp_date;
    struct game_card* game_card_user_list;

    /* EFFETTUO L'ESTRAZIONE */
    extraction_numbers = make_numbers_extractions();

    /* PRENDO TUTTE LE PERSONE CHE SONO ISCRITTE AL SERVER */
    subscribers_list = getMe_user_subscribers_list();
    subscribers_list_length = getMe_user_subscribers_list_length();

    /* Inizializzo la lista che conterrà le schedine di ogni singolo utente */
    game_card_user_list = 0;

    /*  Questa variabile definita sotto "stamp_date" mi serve per identificare la prima schedina
     *  della lista di ogni utente.
     *  In questo modo, se è la prima schedina della lista, metto prima la data di estrazione.
     *  Grazie a questa variabile creao il formato di salvataggio per il file Vincite->utente.txt
     *  Per ogni estrazione il formato sarà:
     *      DATA ESTRAZIONE
     *          SCHEDINA GIOCATA/VERIFICATA
     *          SCHEDINA GIOCATA/VERIFICATA
     *            ....... .....
     *      *****************************************************
     */
    stamp_date = 0; // se vale 1 creo la stringa della data da salvare una sola volta. Se vale 0 non la creo.
    i = 0;
    while(i < subscribers_list_length){
        /* Recupero le schedine non ancora verificate dal processo di estrazione (GIOCATE, MA NON ANCORA VERIFICATE) */
        /* LE SCHEDINE RIGUARDANO SOLO L'UTENTE SPECIFICATO DAL VALORE CORRENTE DELLA VARIABILE subscribers_list[i] */
        /* CHE SCORRE TUTTI GLI UTENTI REGISTRATI AL SERVER */
        every_geme_card_of_user_not_verify = get_game_cards_at_personal_file(subscribers_list[i], 0);

        /*  Se nel file dell utente ci sono schedine non verificate, allora ci vengono ritornate sottoforma
         *  di lista di schedine. Se il buffer delle schedine ritornato è vuoto allora non ci sono schedine
         *  giocate dall utente non ancora verificate.
         */
        game_card_user_list = getMe_list_of_games_card(every_geme_card_of_user_not_verify);

        stamp_date = 1;

        /*  Scorro le schedine di un utente specifico per volta e una ad una ne confronto il contenuto
         *  con l'estrazione grazie alla funzione match_game_card_with_extraction()
         *  Dopo fatto questo ne salvo il contenuto sul file delle vincite del giocatore specifico.
         */
        if(game_card_user_list != 0){
            int number_game_cards;
            struct game_card* tmp;

            number_game_cards = 0;
            for(tmp = game_card_user_list; tmp != 0; tmp = tmp->next){
                match_game_card_with_extraction(extraction_numbers, tmp, subscribers_list[i], stamp_date);

                number_game_cards += 1;

                stamp_date = 0;
                if(tmp->next == 0){
                    /* Mi fermo all'ultima schedina della lista in modo da stampare la linea "line" */
                    char* user_path;

                    user_path = create_path_for_user_files(subscribers_list[i], "./Vincite/");

                    write_in_a_specific_file_with_lock(line, user_path);
                }
            }

            /* Cambio lo stato di tutte le schedine dell utente specifico, da non verificato a verificato */
            /* Nella prossima estrazione non vengono piu verificate */
            change_state_games_card(subscribers_list[i]);

            printf("Verificate %d schedine dell'utente %s\n", number_game_cards, subscribers_list[i]);

        }

        /* Svuoto l'intera lista delle schedine dell'utente specifico che sono state verificare */
        delete_geme_card_list(&game_card_user_list);

        free(every_geme_card_of_user_not_verify);

        i += 1;
    }

    free(subscribers_list);

    /* Salvo l'estrazione nel relativo file "Estrazioni.txt" */
    save_numbers_extraction(extraction_numbers);

    free(extraction_numbers);
}

/*  Questa funzione mi torna le vincite di un rispettivo tipo (estratto, ambo ecc ecc)
 *  relativo ad uno specifico file di vincite (di un rispettivo utente).
 */
float get_tot_of_specific_bets(char* buffer, char* bet_type){
    float bet;
    char* token;
    int found_it;

    token = strtok(buffer, " ");
    found_it = 0;
    bet = 0;

    while (token != NULL) {
        if(found_it == 1){
            bet += atof(token);
            found_it = 0;
        }

        if(!strcmp(token, bet_type))
            found_it = 1;
        token = strtok(NULL, " ");
    }

    return bet;
}

/*  Questa funzione ritorna un consuntivo di tutte le vincite delle schedine giocate
 *  da un preciso utente
 */
char* get_wins(char* user){
    char* buffer_wins;
    char* buffer_wins2;
    char bets[256];
    char* all_buffer_wins;
    char* path;
    int buffer_wins_length;
    float e, a, t, q, c;


    e = a = t = q = c = 0;

    path = create_path_for_user_files(user, "./Vincite/");

    buffer_wins = read_in_a_specific_file_with_lock(path);

    buffer_wins_length = strlen(buffer_wins);

    if(buffer_wins_length == 0)
        return NULL;

    /*  Per ogni tipo di scommessa recupero il valori dal file vincite del
     *  rispettivo utente e poi li sommo, per ottenere un consuntivo sulle vincite
     */

    /* Estratti */
    e = get_tot_of_specific_bets(buffer_wins, "estratto");
    free(buffer_wins);

    /* Ambi */
    buffer_wins = read_in_a_specific_file_with_lock(path);
    a = get_tot_of_specific_bets(buffer_wins, "ambo");
    free(buffer_wins);

    /* Terni */
    buffer_wins = read_in_a_specific_file_with_lock(path);
    t = get_tot_of_specific_bets(buffer_wins, "terno");
    free(buffer_wins);

    /* Quaterne */
    buffer_wins = read_in_a_specific_file_with_lock(path);
    q = get_tot_of_specific_bets(buffer_wins, "quaterna");
    free(buffer_wins);

    /* Cinquine */
    buffer_wins = read_in_a_specific_file_with_lock(path);
    c = get_tot_of_specific_bets(buffer_wins, "cinquina");
    free(buffer_wins);

    sprintf(bets, "Vincite su ESTRATTO: %0.2f\nVincite su AMBO: %0.2f\nVincite su TERNO: %0.2f\nVincite su QUATERNA: %0.2f\nVincite su CINQUINA: %0.2f\n\0", e, a, t, q, c);

    buffer_wins2 = read_in_a_specific_file_with_lock(path);

    all_buffer_wins = malloc(strlen(bets) + strlen(buffer_wins2) + 1);

    strcpy(all_buffer_wins, buffer_wins2);
    strcat(all_buffer_wins, bets);

    return all_buffer_wins;
}

/******************************************************************************************/
/*  PROCESSO CHE SI OCCUPA DELL'ESTRAZIONE                                                */
/******************************************************************************************/

void start_parallel_process_son_for_make_extractions(int minutes){
    pid_t pid;

    pid = fork();
    if(pid == -1){
        printf("Errore durante la creazione processo figlio (Estrazioni)\n");
        exit(1);
    }

    if(pid == 0){
        int i;
        int NUMBER_EXTRACTIONS_SERVER = 10;    // Inizializzo e limito il numero di estrazioni

        printf("Processo che si occupa delle estrazioni avviato\n");

        i = 0;

        /* Pongo un limite alle estrazioni del server */
        while(i < NUMBER_EXTRACTIONS_SERVER){

            sleep((minutes * 60));

            printf("\n");
            printf("************************************************\n");
            printf("Avvio la procedura di estrazione\n");


            /* FACCIO PARTIRE IL PROCESSO DI ESTRAZIONE */
        /**********************************************************/
        /**********************************************************/
            printf("... Inizio eventi estrazione ...\n");
            /*  Definizione sopra, controllare la dichiarazione dei metodi all'inizio del file
             *  per trovarne la definizione
             */
            start_process();
            printf("... Fine eventi estrazione ...\n");
        /**********************************************************/
        /**********************************************************/


            printf("Fine procedura estrazione\n");
            printf("************************************************\n\n");

            i += 1;
        }

        exit(1);
    }

}

/******************************************************************************************/
/*  FINE PROCESSO CHE SI OCCUPA DELL'ESTRAZIONE                                           */
/******************************************************************************************/
