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

  fd_set livesdset, servsdset, workingsdset;   /* set of live client sockets and set of live http server sockets */

  int maxsd = 200; /* TODO: define largest file descriptor number used for select */
	int maxcombinedsd = 400;
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
	FD_SET(0, &livesdset);
	FD_SET(0, &servsdset);
	FD_SET(maxsd, &livesdset);
	FD_SET(maxsd, &servsdset);
	FD_SET(maxcombinedsd, &workingsdset);


	while (1) {
		int frsock;
    /* TODO: combine livesdset and servsdset 
     * use the combined fd_set for select */
		for(int i = 0; i < maxsd; i++){
			if(FD_ISSET(i, &livesdset) || (FD_ISSET(i, &servsdset))){
				FD_SET(i, &workingsdset);
			}
		}
		if(select(maxcombinedsd + 1, &workingsdset, NULL, NULL, NULL) == -1) {
			fprintf(stderr, "Can't select.\n");
			continue;
		}

		for (frsock = 0; frsock < maxcombinedsd; frsock++) {
			if (frsock == servsock) continue;

			if(FD_ISSET(frsock, &livesdset)) {
	    /* forward the request */
				int newsd = sendrequest(frsock);
				if (!newsd) {
					printf("admin: disconnect from client\n");
		/*TODO: clear frsock from fd_set(s) */
					FD_CLR(frsock, &workingsdset);
					FD_CLR(frsock, &livesdset);
				} else {
		/* TODO: insert newsd into fd_set(s) */
					insertpair(table, newsd, frsock);
					FD_SET(newsd, &workingsdset);
					FD_SET(newsd, &livesdset);
					FD_SET(newsd, &servsdset);
				}
			} 
			if(FD_ISSET(frsock, &servsdset)) {
				char *msg;
				struct pair *entry=NULL;	
				struct pair *delentry;
				msg = readresponse(frsock);
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
