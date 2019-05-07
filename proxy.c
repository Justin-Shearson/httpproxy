#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

#include "utils.h"

extern int     sendrequest(int sd);
extern char *  readresponse(int sd);
extern void    forwardresponse(int sd, char *msg);
extern int     startserver();

int main(int argc, char *argv[])
{
	int    servsock;    /* server socket descriptor */
	struct timeval timeout;
	timeout.tv_sec = 2;
	int error;

	fd_set livesdset, servsdset, workingsdset;   /* set of live client sockets and set of live http server sockets */

	// Largest file descriptor for the servsdset and livesdset
	int maxsd = 10;
	//Largest file descriptor for the workingfdset
	int maxcombinedsd = (2 * maxsd);
	struct pair * table = malloc(sizeof(struct pair)); /* table to keep client<->server pairs */

	char *msg;

  	/* check usage */
	if (argc != 1) {
		fprintf(stderr, "usage : %s\n", argv[0]);
		exit(1);
	}

 	/* get ready to receive requests */
	servsock = startserver();
	if (servsock == -1) {
		exit(1);
	}

	table->next = NULL;

  	// initialize all the fd_sets and largest fd numbers
	FD_ZERO(&livesdset);
	FD_ZERO(&servsdset);
	FD_ZERO(&workingsdset);
	FD_SET(servsock, &servsdset); //add server socket to the server fd_set
	FD_SET(maxsd, &livesdset);
	FD_SET(maxsd, &servsdset);
	FD_SET(maxcombinedsd, &workingsdset);


	while (1) {
		int frsock = 0;
    	/*
    	  Combines the two fd_sets into one working fd_set. This working fd_set is then used in the select function
    	    to check if there are any active sockets.
    	*/
		for(int i = 0; i < maxsd; i++){
			if(FD_ISSET(i, &livesdset) || (FD_ISSET(i, &servsdset))){
				printf("Set Socks: %d\n", i);
				FD_SET(i, &workingsdset);
			}
		}

		/*
		  Selects from the workingsdset and monitors the set until a request is made to the server.
		*/
		if(select(maxcombinedsd, &workingsdset, NULL, NULL, NULL)== -1) {
			printf("Error selecting file descriptor: %s\n", strerror(errno));
			continue;
		}
		/*
		  Checks to find what file descriptors in the livesdset are active.
			If found file descriptor points to the socket for the server, that iteration is skipped.
		*/
		for (frsock = 0; frsock < maxsd; frsock++) {
			if (frsock == servsock) continue;
			
			/*
			  If the socket is set, the server has found a client wanting to make a request through the server
			    The server then parses the message being requested by the client and then forwards that
			    request to the website passed by the client.
			*/
			if(FD_ISSET(frsock, &livesdset)) {
	    		/* forward the request */
				int newsd = sendrequest(frsock);
				printf("Returned newsd: %d\n", newsd);
				if (!newsd) {
					printf("admin: disconnect from client\n");
					// Inserts that socket into the server and live sd sets
					FD_CLR(frsock, &servsdset);
					FD_CLR(frsock, &livesdset);
				} else {
					/* TODO: insert newsd into fd_set(s) */
					insertpair(table, newsd, frsock);
					FD_SET(newsd, &workingsdset);
					FD_SET(newsd, &servsdset);
				}
			} 
			if(FD_ISSET(frsock, &servsdset)) {
				char *msg;
				struct pair *entry=NULL;	
				struct pair *delentry;
				printf("frsock: %d\n", frsock);
				msg = readresponse(frsock);
				printf("Returned message: %s\n", msg);

				if (!msg) {
					fprintf(stderr, "error: server died\n");
					exit(1);
				}

				/* forward response to client */
				entry = searchpair(table, frsock);
				if(!entry) {
					fprintf(stderr, "error: could not find matching clent sd\n");
					exit(1);
				}

				forwardresponse(entry->clientsd, msg);
				delentry = deletepair(table, entry->serversd);

				/* TODO: clear the client and server sockets used for 
				 * this http connection from the fd_set(s) */
				FD_CLR(frsock, &livesdset);
				FD_CLR(frsock, &servsdset);
			}
		}

    	/* input from new client*/
		if(FD_ISSET(frsock, &livesdset)) {
			struct sockaddr_in caddr;
			socklen_t clen = sizeof(caddr);
			int csd = accept(servsock, (struct sockaddr*)&caddr, &clen);

			if (csd != -1) {
	    		/* TODO: put csd into fd_set(s) */
				FD_SET(csd, &livesdset);
				FD_SET(csd, &servsdset);
			} else {
				perror("accept");
				exit(0);
			}
		}
	}
	return 1;
}
