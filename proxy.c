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

#include "utils.h"

extern int     sendrequest(int sd);
extern char *  readresponse(int sd);
extern void    forwardresponse(int sd, char *msg);
extern int     startserver();

main(int argc, char *argv[])
{
  int    servsock;    /* server socket descriptor */

  fd_set livesdset, servsdset;   /* set of live client sockets and set of live http server sockets */
  /* TODO: define largest file descriptor number used for select */
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
  
  while (1) {
    int frsock;

    /* TODO: combine livesdset and servsdset 
     * use the combined fd_set for select */

    if(/* TODO: select from the combined fd_set */) {
        fprintf(stderr, "Can't select.\n");
        continue;
    }

    for (/* TODO: iterate over file descriptors */) {
        if (frsock == servsock) continue;

	if(/* TODO: input from existing client? */) {
	    /* forward the request */
	    int newsd = sendrequest(frsock);
            if (!newsd) {
	        printf("admin: disconnect from client\n");
		/*TODO: clear frsock from fd_set(s) */
	    } else {
	        insertpair(table, newsd, frsock);
		/* TODO: insert newsd into fd_set(s) */
	    }
	} 
	if(/* TODO: input from the http server? */) {
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
        }
    }

    /* input from new client*/
    if(FD_ISSET(servsock, &currset)) {
	struct sockaddr_in caddr;
      	socklen_t clen = sizeof(caddr);
      	int csd = accept(servsock, (struct sockaddr*)&caddr, &clen);

	if (csd != -1) {
	    /* TODO: put csd into fd_set(s) */
	} else {
	    perror("accept");
            exit(0);
	}
    }
  }
}
