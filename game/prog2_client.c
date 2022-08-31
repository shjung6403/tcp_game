#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>

int main(int argc, char **argv){

	/* Check Number of Arguments */
	if (argc != 3){
		fprintf(stderr, "Error: Wrong number of arguments\n");
		fprintf(stderr, "usage:\n");
		fprintf(stderr, "./client server_address server_port\n");
		exit(EXIT_FAILURE);
	}

	/* Parse and test port number */
	int port = atoi(argv[2]); /* convert to binary */

	if (port <= 0){
		fprintf(stderr, "Error: bad port number %s\n", argv[2]);
		exit(EXIT_FAILURE);
	}
	struct sockaddr_in sad;
	sad.sin_family = AF_INET;
	sad.sin_port = htons((u_short)port);
	char *host = argv[1];
	struct hostent *ptrh;
	ptrh = gethostbyname(host);
	if (ptrh == NULL){
		fprintf(stderr, "Error: Invalid host: %s\n", host);
		exit(EXIT_FAILURE);
	}
	memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);
	struct protoent *ptrp;
	ptrp = getprotobyname("tcp");
	if (((long int)ptrp) == 0){
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}
	int mySocket = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (mySocket < 0){
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}
	int connectResult = connect(mySocket, (struct sockaddr *)&sad, sizeof(sad));
	if (connectResult < 0){
		fprintf(stderr, "connect failed\n");
		exit(EXIT_FAILURE);
	}

	char playerNumber, isActive;
	uint8_t playerScore, round, myScore, opponentScore, boardSize, turnTime, correct, wordSize;
	myScore = 0;
	opponentScore = 0;

	recv(mySocket, &playerNumber, sizeof(char), 0);
	printf("You are player %c... ", playerNumber);
	if (playerNumber == '1'){
		printf("the game will begin when Player 2 joins...\n");
	}
	else{
		printf("\n");
	}
	recv(mySocket, &boardSize, sizeof(boardSize), 0);
	printf("Board size: %d\n", boardSize);
	recv(mySocket, &turnTime, sizeof(turnTime), 0);
	printf("Seconds per turn: %d\n", turnTime);
	char board[boardSize];

	while (1){
		recv(mySocket, &round, sizeof(round), 0);
		printf("Round %d...\n", round);
		recv(mySocket, &myScore, sizeof(myScore), 0);
		printf("Score is %d-%d\n", myScore, opponentScore);
		recv(mySocket, &board, sizeof(board), 0);
		board[boardSize] = '\0';
		printf("Board: ");
		for (int i = 0; i < boardSize; i++){
			printf("%c ", board[i]);
		}
		printf("\n");
		int turn = 1;
		while (turn){
			recv(mySocket, &isActive, sizeof(isActive), 0);
			if (isActive == 'Y'){
				char guess[255];
				printf("Your turn, enter word: ");
				time_t start, end;
				double dif;
				time(&start);
				fgets(guess, 255, stdin);
				time(&end);
				dif = difftime(end, start);
				uint8_t guessSize = strlen(guess);

				if (dif > (double)turnTime){
					printf("You ran out of time!\n");
					memset(guess, 0, sizeof(char) * guessSize);
					guessSize = 1;
				}
				send(mySocket, &guessSize, sizeof(guessSize), 0);
				send(mySocket, guess, guessSize, 0);
				recv(mySocket, &correct, sizeof(correct), 0);
				if (correct){
					printf("Valid word!\n");
				}
				else{
					printf("Invalid word!\n");
					opponentScore++;
					if (opponentScore == 3)
					{
						printf("You lost!\n");
						close(mySocket);
						exit(EXIT_SUCCESS);
					}
					turn = 0;
				}
			}
			else{
				printf("Please wait for opponent to enter word...\n");
				recv(mySocket, &correct, sizeof(correct), 0);
				if (correct){
					recv(mySocket, &wordSize, sizeof(wordSize), 0);
					char word[wordSize];
					recv(mySocket, word, wordSize, 0);
					printf("Your opponent guessed \"%s\"\n", word);
				}
				else{
					printf("Your opponent lost the round!\n");
					myScore++;
					if (myScore == 3){
						printf("You won!\n");
						close(mySocket);
						exit(EXIT_SUCCESS);
					}
					turn = 0;
				}
			}
		}
	}

	close(mySocket);
	exit(EXIT_SUCCESS);
}
