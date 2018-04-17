
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include "myRing.h"

int main(void)
{
    struct sockaddr_in myaddr, remaddr;
    int fd;
    socklen_t slen = (socklen_t)sizeof(remaddr);
    char *server = "test.ring.com";	/* change this to use a different server */
    char buf[3] = {'\0', '\0', '\0'};
    struct hostent *hp;
    int32_t port = SERVICE_PORT;

    /* create a socket */

    if ((fd=socket(AF_INET, SOCK_DGRAM, 0))==-1)
        printf("socket created\n");

    /* bind it to all local addresses and pick any port number */

    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(0);

    if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("bind failed");
        return 0;
    }

    /* now define remaddr, the address to whom we want to send messages */
    /* For convenience, the host address is expressed as a numeric IP address */
    /* that we will convert to a binary format via inet_aton */

    memset((char*)&remaddr, 0, sizeof(remaddr));
    remaddr.sin_family = AF_INET;
    remaddr.sin_port = htons(port);

    /* look up the address of the server given its name */
    hp = gethostbyname(server);
    if (!hp) {
        fprintf(stderr, "could not obtain address of %s\n", server);
        return 0;
    }

    /* put the host's address into the server address structure */
    memcpy((void *)&remaddr.sin_addr, hp->h_addr_list[0], hp->h_length);


    /* now let's send the messages */

    while( 1 )
    {
        if( recvfrom(fd, buf, 2, 0, (struct sockaddr *)&remaddr, &slen) == 2)
        {
            DBG_MSG("received message:%s\n", buf );
            sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, slen);
            DBG_MSG("Sending packet %s port %d\n",server, SERVICE_PORT);
        }
    }

    close(fd);
    return 0;
}
