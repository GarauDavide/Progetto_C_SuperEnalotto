/*
 * 	Qui sotto troviamo i codici dei comandi.
 * 	Questi codici sono generati grazie ai metodi:
 *
 *      int getMe_code_number_associate_at_the_comand(char* cmd, int n_cmd)
 *      int getMe_code_number_associate_at_the_params_comand(char* cmd, int n_cmd)
 *
 * 	dichiarati e definiti sotto.
 * 	I codici vengono generati facendo la somma dei numeri interi associati ai
 * 	caratteri del comando stesso (codice ascii) moltiplicati per la posizione della
 *	lettera nel comando.
 *	ESEMPIO:
 *		!help -> (h * 1) + (e * 2) + ...
 */
#define HELP 1078
#define ESCI 1048
#define SIGNUP 2331
#define LOGIN 1609
#define INVIA_GIOCATA 9424
#define VEDI_GIOCATE 8105
#define VEDI_ESTRAZIONE 12950
#define VEDI_VINCITE 8267

/*  Gestione BAN:
 *      -> BAN_TIME_SECONDS : rappresenta il tempo che un utente deve rimanere bannato
 *      -> BAN_STATE : stato in cui entra il client quando è bannato dal server
 */
#define BAN_TIME_SECONDS 60
#define BAN_STATE 11111

/*  Struttura dati con cui si passano le informazioni client e server per l'invio
 *  e la ricezione di dati.
 */
struct message_info{
    int nByte;
    int result;
    int comand_code;
    long info;
    char info_char[15];
    char id[11];
};
