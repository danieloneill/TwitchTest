#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
//#include <netinet/ip_var.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
//#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <QDebug>

#define	DEFDATALEN	(64-8)	/* default data length */
#define	MAXIPLEN	60
#define	MAXICMPLEN	76
#define	MAXPACKET	(65536 - 60 - 8)/* max packet size */

// Shamelessly stolen from: http://www.linuxforums.org/forum/members/allomeen.html
// Literally found this code (except in_checksum) on linuxforums because it didn't work...
// I just fixed it until it did.
// Also, in_checksum shamelessly stolen from: https://stackoverflow.com/users/2813589/%e3%82%a2%e3%83%ac%e3%83%83%e3%82%af%e3%82%b9

unsigned short in_checksum(unsigned short *ptr, int n_bytes)
{
    register long sum = 0;
    u_short odd_byte;
    register u_short ret_checksum;

    while (n_bytes > 1)
    {
        sum += *ptr++;
        n_bytes -= 2;
    }

    if (n_bytes == 1)
    {
        odd_byte = 0;
        *((u_char *) & odd_byte) = * (u_char *) ptr;
        sum += odd_byte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    ret_checksum = ~sum;

    return ret_checksum;
}

double ping(const char *target)
{
    int s, i, cc, packlen, datalen = DEFDATALEN;
    struct hostent *hp;
    struct sockaddr_in to, from;
    struct protoent	*proto;
    //struct ip *ip = NULL;
    u_char *packet, outpack[MAXPACKET];
    char hnamebuf[MAXHOSTNAMELEN];
    const char *hostname;
    struct icmp *icp;
    int ret, fromlen, hlen;
    fd_set rfds;
    struct timeval tv;
    int retval;
    struct timeval start, end;
    double end_t;
    char cont = 1;

    memset(&to, 0, sizeof(struct sockaddr_in));
    memset(&from, 0, sizeof(struct sockaddr_in));

    to.sin_family = AF_INET;

    // try to convert as dotted decimal address, else if that fails assume it's a hostname
    to.sin_addr.s_addr = inet_addr(target);
    if (to.sin_addr.s_addr != (u_int)-1)
        hostname = target;
    else
    {
        hp = gethostbyname(target);
        if (!hp)
        {
            fprintf(stderr, "Unknown host: %s\n", target);
            return -1;
        }
        to.sin_family = hp->h_addrtype;
        bcopy(hp->h_addr, (caddr_t)&to.sin_addr, hp->h_length);
        strncpy(hnamebuf, hp->h_name, sizeof(hnamebuf) - 1);
        hostname = hnamebuf;
    }
    packlen = datalen + MAXIPLEN + MAXICMPLEN;
    packet = (u_char *)malloc((u_int)packlen);
    if( !packet )
    {
        fprintf(stderr, "Malloc error.\n");
        return -1;
    }

    if ( (proto = getprotobyname("icmp")) == NULL)
    {
        fprintf(stderr, "Unknown protocol ICMP. Are you using a computer?\n");
        goto errout;
    }

    if ( (s = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0)
    {
        perror("socket");	/* probably not running as superuser */
        goto errout;
    }

    icp = (struct icmp *)outpack;
    memset( icp, 0, sizeof(struct icmp) );
    icp->icmp_type = ICMP_ECHO;
    icp->icmp_code = 0;
    icp->icmp_seq = 12345;	/* seq and id must be reflected */
    icp->icmp_id = getpid();
    icp->icmp_cksum = in_checksum( (unsigned short*)icp, sizeof(struct icmp) );

    //cc = datalen + 8;
    cc = sizeof(struct icmp);

    gettimeofday(&start, NULL);

    i = sendto(s, (char *)outpack, cc, 0, (struct sockaddr*)&to, (socklen_t)sizeof(struct sockaddr_in));
    if (i < 0 || i != cc)
    {
        if (i < 0)
            perror("sendto error");
        printf("Wrote %d chars to %s, ret=%d\n", cc, hostname, i);
    }

    // Watch stdin (fd 0) to see when it has input.
    FD_ZERO(&rfds);
    FD_SET(s, &rfds);
    // Wait up to one seconds.
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    while(cont == 1)
    {
        retval = select(s+1, &rfds, NULL, NULL, &tv);
        if (retval == -1)
        {
            perror("select()");
            goto errout;
        }
        else if (retval)
        {
            fromlen = sizeof(struct sockaddr_in);
            if ( (ret = recvfrom(s, (char *)packet, packlen, 0,(struct sockaddr *)&from, (socklen_t*)&fromlen)) < 0)
            {
                perror("recvfrom error");
                goto errout;
            }

            // Check the IP header
            //ip = (struct ip *)((char*)packet);
            hlen = sizeof( struct ip );
            if (ret < (hlen + ICMP_MINLEN))
            {
                fprintf(stderr, "Packet too short: (%d bytes) %s\n", ret, hostname);
                goto errout;
            }

            // Now the ICMP part
            icp = (struct icmp *)(packet + hlen);
            if (icp->icmp_type == ICMP_ECHOREPLY)
            {
                //printf("Recv: echo reply\n");
                if (icp->icmp_seq != 12345)
                {
                    printf("Received seq #%d\n", icp->icmp_seq);
                    continue;
                }
                if (icp->icmp_id != getpid())
                {
                    printf("Received id #%d\n", icp->icmp_id);
                    continue;
                }
                cont = 0;
            }
            else
            {
                printf("Received a non-echo reply\n");
                continue;
            }

            gettimeofday(&end, NULL);
            end_t = 1000.0 * (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_usec - start.tv_usec);

            if(end_t < 1)
                end_t = 1;

            //printf("Elapsed time = %.3fusec\n", end_t);
            free((void*)packet);
            return end_t;
        }
        else
        {
            printf("No data within 1 second.\n");
            goto errout;
        }
    }

    return 0;

errout:
    free((void*)packet);
    return -1;
}

#ifdef TESTPING
int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("Usage: ping <hostname>\n");
        exit(1);
    }
    double result = ping(argv[1]);
    printf("Result: %.2f\n", result);
    return 0;
}
#endif

