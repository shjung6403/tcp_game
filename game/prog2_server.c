#include "trie.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

#define REQUEST_QUEUE_LENGTH 10 /* size of request queue */

/*------------------------------------------------------------------------
* Program: demo_server
*
* Purpose: allocate a socket and then repeatedly execute the following:
* (1) wait for the next connection from a client
* (2) send a short message to the client
* (3) close the connection
* (4) go back to step (1)
*
* Syntax: ./demo_server port
*
* port - protocol port number to use
*
*------------------------------------------------------------------------
*/

//Generates random string
void makeRandomStr(char *dest, size_t size){
	srand(time(0));
	char vowels[] = "aiueo";
	char charset[] = "abcdefghijklmnopqrstuvwxyz";
	while (size-- > 0){
		size_t index = (double)rand() / RAND_MAX * (sizeof charset - 1);
		*dest++ = charset[index];
	}

	//Checks if the generated string contains at least one vowel
	int hasVowel = 1;
	for (int m = 0; m < strlen(charset); m++){
		if (charset[m] == 'a' || charset[m] == 'e' || charset[m] == 'i' || charset[m] == 'o' || charset[m] == 'u'){
			hasVowel = 0;
		}
	}
	//Make a new Random Sting if the current one doesn't contain a vowel.
	if (hasVowel == 1){
		makeRandomStr(dest, size);
	}
	*dest = '\0';
}

void sendInt(uint8_t n, int c1, int c2){
	send(c1, &n, sizeof(n), 0);
	send(c2, &n, sizeof(n), 0);
}

// Checks if guess can be created using board
int isInBoard(char *board, char *guess){
	int alphabet[26] = {0};
	int boardLen;
	int guessLen;
	boardLen = strlen(board);
	guessLen = strlen(guess);
	char lowerGuess[strlen(guess)];
	// To lowercase entire guess
	for (int i = 0; i < strlen(guess); i++){
		lowerGuess[i] = tolower(guess[i]);
	}
	// Add board to alphabet array
	for (int i = 0; i < strlen(board); i++){
		alphabet[board[i] - 97]++;
	}
	for (int i = 0; i < guessLen; i++){
		if (alphabet[lowerGuess[i] - 97] < 1){
			return 0;
		}
		else{
			alphabet[lowerGuess[i] - 97]--;
		}
	}
	return 1;
}

//Checks if a guess has already been made
int hasBeenGuessed(const char guessedWords[255][255], int guessedWordsCount, char *guess){
	if (guessedWordsCount == 0){
		return 0;
	}
	for (int i = 0; i < guessedWordsCount; i++){
		if (strcmp(guessedWords[i], guess) == 0){
			return 1;
		}
	}
	return 0;
}

int main(int argc, char **argv){
	if (argc != 5){
		fprintf(stderr, "Error: Wrong number of arguments\n");
		fprintf(stderr, "usage:\n");
		fprintf(stderr, "./server server_port board_size turn_length dictionary_path\n");
		exit(EXIT_FAILURE);
	}

	int port = atoi(argv[1]); /* protocol port number */
	if (port <= 0){
		fprintf(stderr, "Error: Bad port number %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	uint8_t boardSize = atoi(argv[2]);
	if (boardSize < 1 || isdigit(boardSize) == -1){
		fprintf(stderr, "Error: Board size must be greater than 0.");
		exit(EXIT_FAILURE);
	}
	uint8_t secondsPerTurn = atoi(argv[3]);
	if (secondsPerTurn < 1 || isdigit(secondsPerTurn) == -1){
		fprintf(stderr, "Error: Seconds per turn must be greater than 0.");
		exit(EXIT_FAILURE);
	}
	char *dictionaryPath = argv[4];

	struct sockaddr_in sad; /* structure to hold server's address */
	memset((char *)&sad, 0, sizeof(sad));
	struct sockaddr_in cad1; /* structure to hold client's address */
	struct sockaddr_in cad2;
	int alen = sizeof(cad1); /* length of address */
	sad.sin_family = AF_INET;
	sad.sin_addr.s_addr = INADDR_ANY;
	sad.sin_port = htons((u_short)port);

	struct protoent *ptrp; /* pointer to a protocol table entry */
	ptrp = getprotobyname("tcp");
	if (((long int)ptrp) == 0){
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}
	int listenerSocket = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (listenerSocket < 0){
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}
	int optval = 1; /* boolean value when we set socket option */
	if (setsockopt(listenerSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0){
		fprintf(stderr, "Error Setting socket option failed\n");
		exit(EXIT_FAILURE);
	}
	if (bind(listenerSocket, (struct sockaddr *)&sad, sizeof(sad)) < 0){
		fprintf(stderr, "Error: Bind failed\n");
		exit(EXIT_FAILURE);
	}
	if (listen(listenerSocket, REQUEST_QUEUE_LENGTH) < 0){
		fprintf(stderr, "Error: Listen failed\n");
		exit(EXIT_FAILURE);
	}

	int sd1, sd2; /* socket descriptors */

	// Main Loop: connect clients
	int endGame = 0;
	// Buffer to keep track of game state
	char msg[5] = "012YN";
	uint8_t correct = 1;
	uint8_t incorrect = 0;

	char guessedWords[255][255];
	int guessedWordsCount = 0;
	while (1){
		if ((sd1 = accept(listenerSocket, (struct sockaddr *)&cad1, &alen)) < 0){
			fprintf(stderr, "Error: Accept failed\n");
			exit(EXIT_FAILURE);
		}
		send(sd1, &msg[1], sizeof(char), 0);
		if ((sd2 = accept(listenerSocket, (struct sockaddr *)&cad1, &alen)) < 0){
			fprintf(stderr, "Error: Accept failed\n");
			exit(EXIT_FAILURE);
		}
		send(sd2, &msg[2], sizeof(char), 0);
		if (fork() == 0){
			// Send board size and seconds per turn
			sendInt(boardSize, sd1, sd2);
			sendInt(secondsPerTurn, sd1, sd2);

			uint8_t round = 1;
			uint8_t score1 = 0;
			uint8_t score2 = 0;
			char board[boardSize];
			while (1){
				int active, inactive;
				makeRandomStr(board, boardSize);
				// Send round number, scores, and board
				sendInt(round, sd1, sd2);

				send(sd1, &score1, sizeof(score1), 0);
				send(sd2, &score2, sizeof(score2), 0);

				send(sd1, &board, boardSize, 0);
				send(sd2, &board, boardSize, 0);

				// If round is odd, player 1 is first
				if (round % 2){
					active = sd1;
					inactive = sd2;
				}
				else{
					active = sd2;
					inactive = sd1;
				}

				int turn = 1;
				while (turn){
					send(active, &msg[3], sizeof(char), 0);
					send(inactive, &msg[4], sizeof(char), 0);
					uint8_t wordSize;
					if (recv(active, &wordSize, sizeof(wordSize), 0) == -1){
						close(sd1);
						close(sd2);
						exit(EXIT_SUCCESS);
					}
					char word[wordSize];
					if (recv(active, &word, wordSize, 0) == -1){
						close(sd1);
						close(sd2);
						exit(EXIT_SUCCESS);
					}
					if (wordSize != 0){
						word[wordSize] = '\0';
					}
					trimwhitespace(word);
					if (trie(dictionaryPath, word) == 0 && (isInBoard(board, word)) && !hasBeenGuessed(guessedWords, guessedWordsCount, word)){
						send(active, &correct, sizeof(correct), 0);
						send(inactive, &correct, sizeof(correct), 0);
						send(inactive, &wordSize, sizeof(wordSize), 0);
						send(inactive, word, wordSize, 0);
						strcpy(guessedWords[guessedWordsCount], word);
						guessedWordsCount++;
						// interchange active and inactive
						int temp = active;
						active = inactive;
						inactive = temp;
					}
					else{
						memset(guessedWords, 0, sizeof(guessedWords));
						guessedWordsCount = 0;
						send(active, &incorrect, sizeof(incorrect), 0);
						send(inactive, &incorrect, sizeof(incorrect), 0);
						if (inactive == sd1){
							score1++;
						}
						else{
							score2++;
						}
						if (score1 == 3 || score2 == 3){
							close(sd1);
							close(sd2);
						}
						round++;
						turn = 0;
					}
				}
			}
		}
		else{
			close(sd1);
			close(sd2);
		}
	}
}
