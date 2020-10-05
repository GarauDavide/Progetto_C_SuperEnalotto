#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define N_RUOTE 11
#define N_NUMERI 10
#define N_IMPORTI 5
#define N_BETS 5

char all_RUOTE[N_RUOTE][9] = {"bari","cagliari","firenze","genova","milano","napoli","palermo","roma","torino","venezia","tutte"};
char all_BETS[N_BETS][9] = {"estratto","ambo","terno","quaterna","cinquina"};

struct played_or_extracted_date{
    int minute;
    int hour;
    int day;
    int month;
    int year;
};

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

char** getMe_two_parameters_separated_at_space(char* params, int n_params);
void stamp_card(struct game_card* gc);
struct game_card* decode_game_cards_with_saved_format(char* card_buffer);
char* get_me_substring(char* buffer, int end, int start);
void stamp_game_cards_with_saved_format(char* buffer);
void stamp_extractions_with_saved_format(char* buffer, int specific_ruota, char* ruota);

/***********************************************************************************************/
/******************************* FINE DICHIARAZIONE METODI *************************************/
/***********************************************************************************************/

/*  Questa funzione mi ritorna una matrice di due parole. Utile per quando dal client
 *  arrivano in un unico buffer nome_utente e password. Con questa funzione ci viene tornato
 *  un array di strighe con nella prima locazione utente e nella seconda password.
 *  Puo essere utilizzata anche per altri motivi.
 */
char** getMe_two_parameters_separated_at_space(char* params, int n_params){
	char** tmp_usr_psw;
	int i, j, tmp_length_usr = 0, tmp_length_psw = 0, what_do = 0;

	tmp_usr_psw = (char**) malloc (2 * sizeof(char*));

	for(i = 0; i < n_params; i ++){
		if(params[i] == ' ' && !what_do){
			i += 1;
			what_do = 1;
		}
		if(!what_do)
			tmp_length_usr += 1;
		else
			tmp_length_psw += 1;
	}

	tmp_usr_psw[0] = (char*) malloc (tmp_length_usr + 1);
	tmp_usr_psw[1] = (char*) malloc (tmp_length_psw + 1);

	for(i = 0; (params[i] != ' ') && (i < n_params); i ++)
		tmp_usr_psw[0][i] = params[i];
	tmp_usr_psw[0][i] = '\0';

	j = 0;
	for(i += 1; i < n_params; i ++){
		tmp_usr_psw[1][j] = params[i];
		j += 1;
	}
	tmp_usr_psw[1][j] = '\0';

	return tmp_usr_psw;
}

/*  Questa funzione stampa la schedina. Deve essere passata nel formato struttura
 */
void stamp_card(struct game_card* gc){
    int j;

    printf("> ");
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

    buffer_len = strlen(buffer);

    if((buffer_len == 0) || (buffer == NULL))
        printf("Non ci sono schedine.\n");
    else{

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
}

/*  Stampo l'estrazione per il client, in modo da mostrare l'estrazione piu
 *  comprensibile rispetto al formato di salvataggio.
 */
void stamp_extractions_with_saved_format(char* buffer, int specific_ruota, char* ruota){
    int i, j, k, buffer_length, data_numbers, alternate, ruot;
    char tmp[5][14] = {"Estrazione 20", "-", "-", " ", ":"};

    buffer_length = strlen(buffer);

    if(specific_ruota){
        data_numbers = 1;
        alternate = 1;
        k = 0;
        ruot = 0;

        for(i = 0; i < buffer_length; i ++){
            if((data_numbers % 2) != 0){
                ruot = 0;
                if((alternate % 2) != 0){
                    printf("%s", tmp[k]);
                    k += 1;
                    alternate += 1;
                    printf("%c", buffer[i]);
                }else{
                    if(buffer[i] != ' ')
                        printf("%c", buffer[i]);
                    else
                        alternate += 1;
                }

            }else{
                if(ruot == 0){
                    printf("%s ", ruota);
                    ruot = 1;
                    printf("%c", buffer[i]);
                }else
                    printf("%c", buffer[i]);
            }

            if(buffer[i] == '\n'){
                data_numbers += 1;
                alternate = 1;
                k = 0;
            }

        }

        printf("\n");

    }else{
        int carry_line = 0, ruote_pointer = -1;
        data_numbers = 1;
        alternate = 1;
        k = 0;
        ruot = 0;

        for(i = 0; i < buffer_length; i ++){
            if((data_numbers % 2) != 0){
                if((alternate % 2) != 0){
                    printf("%s", tmp[k]);
                    k += 1;
                    alternate += 1;
                    printf("%c", buffer[i]);
                }else{
                    if(buffer[i] != ' ')
                        printf("%c", buffer[i]);
                    else
                        alternate += 1;
                }

            }else{
                if(ruot == 0){
                    printf("%s ", all_RUOTE[ruote_pointer]);
                    ruot = 1;
                    printf("%c", buffer[i]);
                }else
                    printf("%c", buffer[i]);
            }

            if(buffer[i] == '\n'){
                data_numbers = 2;
                carry_line += 1;
                ruote_pointer += 1;
                ruot = 0;
            }else
                carry_line = 0;

            if(carry_line == 2){
                data_numbers = 1;
                alternate = 1;
                k = 0;
                ruote_pointer = -1;
            }

        }
    }

}
