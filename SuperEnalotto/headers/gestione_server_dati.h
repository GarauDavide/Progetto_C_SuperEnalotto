#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>

#include "./gestione_file_mutua_esclusione.h"

struct connections_list{
	char* user;
	char user_id[11];
	int user_socket;
	time_t seconds;
	struct connections_list* next;
};

struct management_banned_ip{
	char ip[INET_ADDRSTRLEN];
	int how_many_times;
	time_t seconds;
	struct management_banned_ip* next;
};

#define TIME_TO_LOGOUT 120

/***********************************************************************************************/
/********************************** DICHIARAZIONE METODI ***************************************/
/***********************************************************************************************/

char* create_path_for_user_files(char* user, char* prefix);
void stamp_connections_list(struct connections_list** l);
char* getMe_one_user_id(int length_id);
void add_new_user_at_the_connections_list(struct connections_list** l, char* usr, char* id, int socket);
void verify_time_to_logout_for_every_connection(struct connections_list** l);
char** getMe_two_parameters_separated_at_space(char* params, int n_params);
int tellMe_if_user_is_registered(char* user);
void signup_new_user(char* usr, char* psw);
char* read_user_password_from_the_associate_file(char* user);
char* getMe_session_id_of_a_specific_user(struct connections_list** l, char* user);
int tellMe_if_user_is_connected(struct connections_list** l, char* user);
char* getMe_username_for_a_specific_id(struct connections_list** l, char* user_id);
int tellMe_if_the_user_id_is_valid(struct connections_list** l, char* user_id);
void delete_session_of_a_specific_id(struct connections_list** l, char* id);
void delete_session_of_a_specific_socket(struct connections_list** l, int socket);
void add_or_management_IP_at_going_to_banned_list(struct management_banned_ip** l, char* ip);
void stamp_going_to_banned_list(struct management_banned_ip** l);
int get_me_how_many_times(struct management_banned_ip** l, char* ip);
time_t get_me_time_ban(struct management_banned_ip** l, char* ip);
void remove_ip_at_ban_list(struct management_banned_ip** l, char* ip);

/***********************************************************************************************/
/******************************* FINE DICHIARAZIONE METODI *************************************/
/***********************************************************************************************/

/*	Questa funzione, dato uno username e un path prefissato, ne ritorna il path
 *	relativo specifico all'interno del dominio del progetto.
 */
char* create_path_for_user_files(char* user, char* prefix){
    char* all_path;
    char* path1 = prefix;
    char* path2 = ".txt";
    int all_path_length;

    all_path_length = strlen(path1) + strlen(path2) + strlen(user) + 1;

    all_path = malloc(all_path_length);

    strcpy(all_path, path1);
    strcat(all_path, user);
    strcat(all_path, path2);

    return all_path;
}

/*	Stampa la lista delle connessioni presenti nel server
 */
void stamp_connections_list(struct connections_list** l){
	struct connections_list* tmp;

	if(!(*l)){
		printf("Nessun utente connesso..\n");
		return;
	}

	printf("Lista utenti connessi al server:\n");

	for(tmp = (*l); tmp != 0; tmp = tmp->next)
		printf("	User: %s - id: %s\n", tmp->user, tmp->user_id);
}

/*	Questa funzione ritorna un ID nuovo per gli utenti che effettuano il login ad
 * 	ogni chiamata. L'ID è composto da 10 caratteri ciascuno.
 */
char* getMe_one_user_id(int length_id){
	const char* range = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	int i, length_range;
	time_t t;
	char* id;

	id = malloc(11);

	length_range = strlen(range);
	srand((unsigned) time(&t));

	for(i = 0; i < length_id; i ++)
		id[i] = range[rand() % length_range];

	id[i] = '\0';

	return id;
}

/*	Aggiunge un nuovo utente alla lista delle connessioni presenti nel server.
 */
void add_new_user_at_the_connections_list(struct connections_list** l, char* usr, char* id, int socket){
	struct connections_list* tmp, * tmp1;
	char tmp_user_id[11];
	time_t sec;

	/* Sono sicuro della dimensione e non faccio controlli sulla lunghezza dell id */
	strcpy(tmp_user_id, id);

	/*	Se l utente esiste gia gli rinnovo l id.
	 *	Altrimenti lo ne creo uno nuovo e lo inserisco nella lista connessioni
	 */

	tmp1 = (*l);

	for(; (tmp1 != 0) && (strcmp(tmp1->user, usr) != 0); tmp1 = tmp1->next){ ; }

	if(!tmp1){
		tmp = malloc(sizeof(struct connections_list));

		tmp->user = malloc(strlen(usr) + 1);
		strcpy(tmp->user, usr);

		strcpy(tmp->user_id, tmp_user_id);

		tmp->user_socket = socket;

		tmp->seconds = time(NULL);

		tmp->next = (*l);
		(*l) = tmp;

	}else{
		strcpy(tmp1->user_id, tmp_user_id);
		tmp1->seconds = time(NULL);
	}
}

/*	Funzione che viene chiamata per cancellare una connessione nel caso un utente
 *	non interagisce per un tempo X con il server. Ne invalida l'ID e di conseguenza
 *	l'utente deve rieffettuare il login con il server per certi comandi
 */
void verify_time_to_logout_for_every_connection(struct connections_list** l){
	struct connections_list* tmp1, * tmp2;
	time_t tmp_time;

	if(!(*l))
		return;

	tmp_time = time(NULL);
	tmp2 = NULL;

	for(tmp1 = (*l); (tmp1->next != 0); tmp1 = tmp1->next){
		if(((int)tmp_time - (int)tmp1->seconds) > TIME_TO_LOGOUT){
			if(tmp1 == (*l))
				(*l) = tmp1->next;
			else
				tmp2->next = tmp1->next;

			free(tmp1);
		}
		tmp2 = tmp1;
	}
}

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

/*	Questo metodo mi dice se un utente è gia registrato con un preciso username
 *	Restituisce 1 se è gia registrato, 0 altrimenti.
 */
int tellMe_if_user_is_registered(char* user){
	char* username_list;
	char* token;

	username_list = read_in_a_specific_file_with_lock("./every_user_list.txt");

	if(strlen(username_list) == 0 || username_list == NULL)
		return 0;

	token = strtok(username_list, "\n");

	while (token != NULL){
        if(!strcmp(token, user))
			return 1;
        token = strtok(NULL, "\n");
    }

	return 0;

}

/*	Questa funzione registra un utente al server creando tutti i relativi file per
 *	la gestione dell utente stesso. Dati , giocate e vincite.
 */
void signup_new_user(char* usr, char* psw){
	char initializzation[256];
	char* path = create_path_for_user_files(usr, "./Utenti/");
	char* path2 = create_path_for_user_files(usr, "./Vincite/");
	char user[strlen(usr) + 2];

	strcpy(user, usr);
	strcat(user, "\n");
	write_in_a_specific_file_with_lock(user, "./every_user_list.txt");

	strcpy(initializzation, psw);
	strcat(initializzation, "\n");
	write_in_a_specific_file_with_lock(initializzation, path);

	write_in_a_specific_file_with_lock("", path2);

}

/*	Questa funzione ritorna la password di uno specifico utente.
 *	La password viene recuperata dal file personale che si trova nella cartella
 *	Utenti nel dominio del progetto.
 *	La password viene salvata nella prima riga di quel file dell'utente specifico.
 */
char* read_user_password_from_the_associate_file(char* user){
	char* buffer;
	char* psw;
	int i;
	char* path;

	path = create_path_for_user_files(user, "./Utenti/");

	buffer = read_in_a_specific_file_with_lock(path);

	for(i = 0; (i < strlen(buffer)) && (buffer[i] != '\n'); i ++){ ; }

	psw = malloc(i + 1);

	for(i = 0; (i < strlen(buffer)) && (buffer[i] != '\n'); i ++)
		psw[i] = buffer[i];
	psw[i] = '\0';

	free(buffer);

	return psw;
}

/*	Questa funzione ritorna l'ID di un utente connesso al server.
 *	Legge la lista delle connessioni e ritorna il valore dell'ID.
 *	Se non c'e significa che l'utente non è connesso al server oppure l'ID
 *	che ha salvato l'utente lato client non è piu valido.
 */
char* getMe_session_id_of_a_specific_user(struct connections_list** l, char* user){
	struct connections_list* tmp;

	tmp = (*l);

	for(; (tmp != 0) && (strcmp(tmp->user, user) != 0); tmp = tmp->next){ ; }

	if(!tmp)
		return NULL;

	return tmp->user_id;
}

/*	Questa funzione controlla se un utente è gia connesso al server leggendo se
 *	fa parte delle lista connessioni.
 */
int tellMe_if_user_is_already_connected(struct connections_list** l, char* user){
	struct connections_list* tmp;

	tmp = (*l);

	for(; (tmp != 0) && (strcmp(tmp->user, user) != 0); tmp = tmp->next){ ; }

	if(!tmp)
		return 0;

	return 1;
}

/*	Questa funzione, passato un ID, mi ritorna lo username con quello specifico ID
 *	Questo possiamo farlo grazie alla lista delle connessioni che tiene traccia
 *	delle informazioni degli utenti connessi.
 */
char* getMe_username_for_a_specific_id(struct connections_list** l, char* user_id){
	struct connections_list* tmp;

	if(!tmp)
		return NULL;

	for(tmp = (*l); (tmp != 0) && (strcmp(tmp->user_id, user_id) != 0); tmp = tmp->next){ ; }

	return tmp->user;
}

/*	Questa funzione mi dice se l'ID di un utente è valido.
 *	Scorre la lista delle connessioni e vede se l'ID dell'utente è presente nella lista
 *	delle connessioni. Allora è stato accettato correttamente come utente connesso.
 *	Quindi, se valido, lo trovo nella lista di connessioni valide accettate dal server.
 */
int tellMe_if_the_user_id_is_valid(struct connections_list** l, char* user_id){
	struct connections_list* tmp;

	if(!(*l))
		return 0;

	for(tmp = (*l); (tmp != 0) && (strcmp(tmp->user_id, user_id) != 0); tmp = tmp->next){ ; }

	if(!tmp)
		return 0;

	return 1;
}

/*	Questa funzione elimina la sessione di connessione di uno specifico user con
 *	uno specifico ID. Quando un utente si disconnette oppure stacca la connessione
 *	viene cancellato dalla lista della connessioni.
 */
void delete_session_of_a_specific_id(struct connections_list** l, char* id){
	struct connections_list* tmp, * tmp2;

	if(!(*l))
		return;

	for(tmp = (*l); (tmp->next != 0) && (strcmp(tmp->user_id, id) != 0); tmp = tmp->next){
		tmp2 = tmp;
	}

	if(!strcmp(tmp->user_id, id)){
		if(tmp == (*l))
			(*l) = tmp->next;
		else
			tmp2->next = tmp->next;
		free(tmp);
	}
}

/*	Questa funzione elimina la sessione di connessione di uno specifico user con
 *	uno specifico socket. Quando un utente si disconnette oppure stacca la connessione
 *	viene cancellato dalla lista della connessioni.
 */
void delete_session_of_a_specific_socket(struct connections_list** l, int socket){
	struct connections_list* tmp, * tmp2;

	if(!(*l))
		return;

	for(tmp = (*l); (tmp->next != 0) && (tmp->user_socket != socket); tmp = tmp->next){
		tmp2 = tmp;
	}

	if(tmp->user_socket == socket){
		if(tmp == (*l))
			(*l) = tmp->next;
		else
			tmp2->next = tmp->next;
		free(tmp);
	}
}

/*	Questa lista gestisce gli utente bannati dal server con un certo indirizzo IP
 *	Ogni volta che un utente viene bannato viene aggiunto a questa lista.
 *	Ad ogni connessione viene controllata la lista in modo da vedere se l'host connesso
 *	è stato bannato in precedenza, rifitando tutte le richieste da esso fatte.
 *	Si può settare un tempo di BAN con alcune variabili globali.
 *		BAN_TIME_SECONDS
 *	presente nel file libreria:
 *		condivision_data.h
 */
void add_or_management_IP_at_going_to_banned_list(struct management_banned_ip** l, char* ip){
	struct management_banned_ip* tmp;
	time_t sec;

	tmp = malloc(sizeof(struct management_banned_ip));

	// Se la lista è vuota lo inserisco in testa subito

	if(!(*l)){
		tmp->how_many_times = 1;
		strcpy(tmp->ip, ip);

		tmp->next = (*l);
		(*l) = tmp;

		return;
	}

	for(tmp = (*l); (tmp->next != 0) && (strcmp(tmp->ip, ip)); tmp = tmp->next){ ; }

	/*	Scorro la lista. Se è vuota o non lo trovo, creo e aggiungo in testa
	 *	Altrimenti l'ho trovato e aumento how_many_times
	 */
	if(!strcmp(tmp->ip, ip)){

	 	tmp->how_many_times += 1;

		if(tmp->how_many_times == 3)
			tmp->seconds = time(NULL);
	}else{
		tmp->how_many_times = 1;
		strcpy(tmp->ip, ip);

		tmp->next = (*l);
		(*l) = tmp;
	}

}

/*	Stampa la lista degli utenti bannati dal server
 */
void stamp_going_to_banned_list(struct management_banned_ip** l){
    struct management_banned_ip* tmp;

	if(!(*l)){
		printf("Lista ban vuota..\n");
		return;
	}

	printf("Lista utenti presi di mira:\n");
	for(tmp = (*l); tmp->next != 0; tmp = tmp->next)
		printf("%s times: %d - ", tmp->ip, tmp->how_many_times);
	printf("%s times: %d\n", tmp->ip, tmp->how_many_times);
}

/*	Questa funzione ritorna quante volte il client ha fatto errori nella fase di
 *	login.
 */
int get_me_how_many_times(struct management_banned_ip** l, char* ip){
	struct management_banned_ip* tmp;

	if(!(*l))
		return 0;

	for(tmp = (*l); (tmp->next != 0) && (strcmp(tmp->ip, ip)); tmp = tmp->next){ ; }

	if(!strcmp(tmp->ip, ip))
	 	return tmp->how_many_times;
	else
		return 0;
}

/*	Questa funzione mi ritorna da quanto tempo l'utente è stato bannato in modo
 *	che passato un tempo BAN_TIME_SECONDS (presente nel file condivision_data.h)
 *	viene cancellato dalla lista utenti bannati e gli rende di nuovo disponibili
 *	le richieste da parte di quel client (host -> IP);
 */
time_t get_me_time_ban(struct management_banned_ip** l, char* ip){
	struct management_banned_ip* tmp;
	time_t sec;

	if(!(*l))
		return -1;

	for(tmp = (*l); (tmp->next != 0) && (strcmp(tmp->ip, ip)); tmp = tmp->next){ ; }

	if(!strcmp(tmp->ip, ip))
	 	return tmp->seconds;
	else
		return -1;
}

/*	Questa funzione rimuove un IP dalla lista utenti bannati rendendo di nuovo
 *	disponibili tutte le richieste effettuate dall'IP stesso.
 */
void remove_ip_at_ban_list(struct management_banned_ip** l, char* ip){
	struct management_banned_ip* tmp, * tmp2;

	if(!(*l))
		return;

	for(tmp = (*l); (tmp->next != 0) && (strcmp(tmp->ip, ip)); tmp = tmp->next){
		tmp2 = tmp;
	}

	if(!strcmp(tmp->ip, ip)){
		if(tmp == (*l))
			(*l) = tmp->next;
		else
			tmp2->next = tmp->next;
		free(tmp);
	}
}
