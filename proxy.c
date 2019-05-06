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

	int maxsd = 10;  //TODO: define largest file descriptor number used for select 
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

  	/* TODO: initialize all the fd_sets and largest fd numbers */
	FD_ZERO(&livesdset);
	FD_ZERO(&servsdset);
	FD_ZERO(&workingsdset);
	FD_SET(servsock, &servsdset);
	FD_SET(maxsd, &livesdset);
	FD_SET(maxsd, &servsdset);
	FD_SET(maxcombinedsd, &workingsdset);


	while (1) {
		int frsock = 0;
    	/* TODO: combine livesdset and servsdset 
     	* use the combined fd_set for select */
		for(int i = 0; i < maxsd; i++){
			if(FD_ISSET(i, &livesdset) || (FD_ISSET(i, &servsdset))){
				printf("Set Socks: %d\n", i);
				FD_SET(i, &workingsdset);
			}
		}
		error = select(maxcombinedsd, &workingsdset, NULL, NULL, NULL);
		if(error == -1) {
			printf("Error selecting file descriptor: %s\n", strerror(errno));
			continue;
		}

		for (frsock = 0; frsock < maxsd; frsock++) {
			if (frsock == servsock) continue;
			if(FD_ISSET(frsock, &livesdset)) {
	    		/* forward the request */
				int newsd = sendrequest(frsock);
				printf("Returned newsd: %d\n", newsd);
				if (!newsd) {
					printf("admin: disconnect from client\n");
					/*TODO: clear frsock from fd_set(s) */
					FD_CLR(frsock, &servsdset);
					FD_CLR(frsock, &livesdset);
				} else {
					/* TODO: insert newsd into fd_set(s) */
					insertpair(table, newsd, frsock);
					FD_SET(newsd, &workingsdset);
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
		if(FD_ISSET(servsock, &workingsdset)) {
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
