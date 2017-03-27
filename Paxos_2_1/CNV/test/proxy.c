#include	"Converter.h"
#include	<stdio.h>

struct sockaddr_in CoC;
struct in_addr target;


char *Msg_BadRequest ="HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
char *Msg_NotFound ="HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
char *Msg_InternalError = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
char *Msg_ConnectOK = "HTTP/1.1 200 Connection Established\r\n\r\n";

char *Getline(int fd,char *buf,int size,char **ppRead, char **pLineHead,int *pReadEnd)
{
	char *p;
	int len;
	printf("Getline enter\n");
	while(*ppRead < (buf + size -1) && !*pReadEnd) {
		if ((len = recv(fd,*ppRead,size-(*ppRead - buf)-1,0)) <= 0 ) {
			break;
		}
		*ppRead += len;
		if ( index( *ppRead - len, '\n') ) {
			break;
		}
	}
	if ( len == 0 ) {
		*pReadEnd = 1;
		*ppRead += len;
	}
	p = *pLineHead;
	while ( p < *ppRead && *p != '\n' ) {
		p++;
	}
	if ( *p == '\n' ) {
		char *q = *pLineHead;
		*pLineHead = p +1;
		return q;
	}
	return NULL;
}

void *
recvThread(void *pArg)
{
	int *arg = (int *)pArg;
	int fd = arg[0];
	int so = arg[1];
	char buf[4096];
	char *pRead;
	int len;

	free(pArg);
	memset(buf,'\0',sizeof(buf));
	while ( (len = recv(so,buf,sizeof(buf), 0)) > 0 ) {
		printf("recv from target\n");
		pRead = buf + len;
		while ( len > 0) {
			printf("send to client(len=%d)[%s]\n",len,buf+((pRead - buf) - len));
			int l= send( fd, buf+((pRead - buf) - len), len, 0 );
			if ( l <= 0 ) {
				fprintf(stderr,"send(3) err(%d,%s)\n",errno,strerror(errno));
				goto err;
			}
			len -= l;
		}
		memset(buf,'\0',sizeof(buf));
	}
 out:
	close(so);
	close(fd);
	pthread_exit( 0 );
	return( NULL );
 err:
	send(fd,Msg_InternalError,strlen(Msg_InternalError),0);
	goto out;
}

int
check_method_line(int fd, char *buf, int size,int *pSo, int *pIsConnect) 
{
	struct addrinfo hints;
	struct addrinfo *result;
	int 	s;
	struct in_addr	pin_addr;
	struct sockaddr_in   To;
	char host_name[NAME_MAX];
	char *pPort;
	char *pRead= buf;
	char *lineHead = buf;
	int readEnd = 0;
	char *line;
	char *p;

	*pSo = 0;


 	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family 	= AF_INET;
	hints.ai_socktype 	= SOCK_STREAM;
	hints.ai_flags 		= 0;
	hints.ai_protocol 	= 0;
	hints.ai_canonname 	= NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;



// Request Line
	memset(buf,'\0',sizeof(buf));
	line = Getline(fd,buf,size,&pRead,&lineHead,&readEnd);
	if ( line == NULL ) {
		fprintf(stderr,"line null\n");
		goto out;
	}
// check isConected
	if ( *pIsConnect ) {
		goto out;
	}
// search URL
	// skip method
	p = line;
	while( *p != ' ' && *p != '\n' ) p++;
	if ( *p == '\n' ) {
		fprintf(stderr,"line not blank[%s]\n",line);
		goto out;
	}
	p++;
	printf("%s",p);
	if ( strncasecmp(p,"http://",7) == 0  ) {
		char *ix;
		p +=7;
		if ( ( ix = index(p,'/' ) ) == NULL ) {
 			fprintf(stderr,"line not slash[%s]\n",p);
			goto err;
		}
		memset(host_name,'\0',sizeof(host_name));
		memcpy(host_name,p,ix - p);
	} else if ( strncasecmp(line,"CONNECT ",8) == 0 ) {
// connect
		char *ix;
		if ( ( ix = index(p,' ' ) ) == NULL ) {
 			fprintf(stderr,"line not blank[%s]\n",p);
			goto err;
		}
		memset(host_name,'\0',sizeof(host_name));
		memcpy(host_name,p,ix - p);
		*pIsConnect = 1;
	} else {
// Host Line
		while ( 1 ) {
			line = Getline(fd,buf,sizeof(buf),&pRead,&lineHead,&readEnd);
			if ( line == NULL ) {
				fprintf(stderr,"host line not found\n");
				goto out;
			}
			if ( strncasecmp(line, "Host: ",6) != 0 ) {
				continue;
			}
		}
		char *ix;
		p += 6;
		if ( ( ix = index(p,'\n' ) ) == NULL ) {
 			fprintf(stderr,"host line `nl' not found[%s]\n",line);
			goto err;
		}
		memset(host_name,'\0',sizeof(host_name));
		memcpy(host_name,p,ix - p);
	}
	printf("host name[%s]\n",host_name);
	pPort = index(host_name,':');
	if ( pPort ) {
		*pPort = '\0';
	}
   	s = getaddrinfo( host_name, NULL, &hints, &result);
	if( s < 0 )  {
		fprintf(stderr,"target(%s) address not found\n",host_name);
		goto err;
	}

	pin_addr	= ((struct sockaddr_in*)result->ai_addr)->sin_addr;
	freeaddrinfo( result );

	if ( memcmp(&target,&pin_addr,sizeof(pin_addr)) == 0 ) {
		To = CoC;
	} else {
		int port = 80;
		if ( pPort ) {
			port = atoi(pPort+1);
			if ( port <= 0 || port >= 0x10000 ) {
				goto err;
			}
		}
		printf("addr[%s],port = %d\n",inet_ntoa(pin_addr),port);
		memset( &To, 0, sizeof(To) );
		To.sin_family	= AF_INET;
		To.sin_addr	= pin_addr;
		To.sin_port	= htons((unsigned short)port);
	}
// connect traget
	*pSo = socket(PF_INET,SOCK_STREAM,0);
	if ( *pSo < 0 ) {
		fprintf(stderr,"socket err(%d,%s)\n",errno,strerror(errno));
		goto err;;
	}
	if ( connect(*pSo,(struct sockaddr *)&To,sizeof(To)) < 0 ) {
 		fprintf(stderr,"connetct err(%d,%s)\n",errno,strerror(errno));
		close(*pSo);
		goto err;
	}
 out:
	return pRead - buf;
 err:
	pRead = buf -1;
	*pSo = 0;
	goto out;
}
void*
Thread( void* pArg )
{
	int	fd = PNT_INT32(pArg);
	int so = 0;
	char buf[4096];
	char *pRead = buf;
	int readEnd = 0;
	pthread_attr_t	attr;
	pthread_t	th;
	int *fds;
	int len;
	int isConnect = 0;


// exec reavThread
	while ( readEnd == 0 ) {
		int s;
		int c = isConnect;
		len = check_method_line(fd,buf,sizeof(buf),&s,&c); 
		if ( len < 0 ) {
			fprintf(stderr,"recv err(%d,%s)\n",errno,strerror(errno));
			goto err;
		}
		if ( s > 0 ) {
			if ( so > 0 ) {
				close(so);
			}
			so = s;
			pthread_attr_init( &attr );
			pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
			fds = malloc(sizeof(int)*2);
			fds[0] = fd;
			fds[1] = so;
			pthread_create( &th, &attr, recvThread, fds );
		}
		if ( !isConnect && c ) {
			isConnect = 1;
			send(fd,Msg_ConnectOK,strlen(Msg_ConnectOK),0);
			continue;
		}
		if ( len == 0 ) {
			readEnd = 1;
			continue;
		}
		pRead = buf + len;
		while ( len > 0) {
			printf("send to target(len=%d)[%s]\n",len,buf+((pRead - buf) - len));
			int l= send( so, buf+((pRead - buf) - len), len, 0 );
			if ( l <= 0 ) {
				fprintf(stderr,"send(2) err(%d,%s)\n",errno,strerror(errno));
				goto err;
			}
			len -= l;
		}
	}

 out:
	if ( so > 0 ) close(so);
	close(fd);
	pthread_exit( 0 );
	return( NULL );
 err:
#if 0
	send(fd,Msg_BadRequest,strlen(Msg_BadRequest),0);
	goto out;
 err2:
#endif
	send(fd,Msg_InternalError,strlen(Msg_InternalError),0);
	goto out;
#if 0
 err3:
	send(fd,Msg_NotFound,strlen(Msg_NotFound),0);
	goto out;
#endif
}

void
usage()
{
	printf("HTTPProxy proxy_port CoCAddr CoCport target_host\n");
}


int
main( int ac, char* av[] )
{
	char	host_name[128];

	struct addrinfo hints;
	struct addrinfo *result;
	int 	s;
	struct in_addr	pin_addr;
	struct sockaddr_in	Port, From;
	pthread_attr_t	attr;
	int	ListenFd, len, fd;
	pthread_t	th;

	

	if( ac < 5 ) {
		usage();
		exit( -1 );
	}

 	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family 	= AF_INET;
	hints.ai_socktype 	= SOCK_STREAM;
	hints.ai_flags 		= 0;
	hints.ai_protocol 	= 0;
	hints.ai_canonname 	= NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

/*
 *	Client Accept Address
 */
	gethostname( host_name, sizeof(host_name) );
   	s = getaddrinfo( host_name, NULL, &hints, &result);
	if( s < 0 )	exit(-1);

	pin_addr	= ((struct sockaddr_in*)result->ai_addr)->sin_addr;
	freeaddrinfo( result );

	memset( &Port, 0, sizeof(Port) );
	Port.sin_family	= AF_INET;
	Port.sin_addr	= pin_addr;
	Port.sin_port	= htons( atoi(av[1]) );

	if( (ListenFd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
		perror("socket");
		exit( -1 );
	}
	if( bind( ListenFd, (struct sockaddr*)&Port, sizeof(Port) ) < 0 ) {
		perror("bind");
		exit( -1 );
	}
	listen( ListenFd, 5 );

   	s = getaddrinfo(av[2], NULL, &hints, &result);
	if( s < 0 )	exit(-1);

	pin_addr	= ((struct sockaddr_in*)result->ai_addr)->sin_addr;
	freeaddrinfo( result );
	memset( &CoC, 0, sizeof(CoC) );
	CoC.sin_family	= AF_INET;
	CoC.sin_addr	= pin_addr;
	CoC.sin_port	= htons( atoi(av[3]) );
	

   	s = getaddrinfo( av[3], NULL, &hints, &result);
	if( s < 0 )	exit(-1);

	target	= ((struct sockaddr_in*)result->ai_addr)->sin_addr;
	freeaddrinfo( result );

	pthread_attr_init( &attr );
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

	
	printf("listen\n");
	signal(SIGPIPE,SIG_IGN);
	while( TRUE ) {
		printf("wait for accept\n");
		len	= sizeof(From);
		fd	= accept( ListenFd, (struct sockaddr*)&From, (socklen_t *)&len );
		if( fd < 0 ) {
			perror("accept");
			exit( -1 );
		}
		printf("accept\n");
		pthread_create( &th, &attr, Thread, INT32_PNT(fd) );
	}
}
