/* 
 * tcpserver.c - A simple TCP echo server 
 * usage: tcpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <sys/stat.h>
#include <fcntl.h>

#define BUFSIZE 1024
#define CHUNK 1024
#define MAX_FILE_SIZE 999999



#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

char OK[1]="0";
char ERROR[1]="1";
/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

char* getfileName(char* buf){
  /*int n=strlen(buf);
  char *name=(char*)malloc(n*sizeof(char));
  name = strtok(buf, "\n");
  int k=0;
  for (int i = 0; i < strlen(name); ++i)
  {
    if(name[i]==' ' || name[i]=='\n')i++;
    name[k]=name[i];
    k++;
  }
  name[k]='\0';
  return name;*/
  int i = 0;
 
  int l=strlen(buf);
  int count=0;
  for (; i < l; ++i)
  {
    if(buf[i]=='/')count++;
  }
  if(count<1)return buf;
  strtok(buf,"/");
  if(count<2)return strtok(NULL,"/");

  for (int i = 0; i < count-1; ++i)
  {
    strtok(NULL,"/");
  }
  //while(buf[i]=' ')i++;
  return strtok(NULL,"/");
}


char *removeSpace(char * fileName){
  int l=strlen(fileName);
  int j,i=0;
  // printf("s:%s\n",fileName );
  while(fileName[i]!='\0'){
    if (fileName[i]==' ')
    {
      i++;
    }
    else
      break;
  }
  // printf("%d\n",i );
  char* out=(char*)malloc(l*sizeof(char));
  for (j=0; fileName[i]!='\0'; ++j)
  {
    out[j]=fileName[i++];
    // if(fileName[i]='\0')break;
  }
  out[j]='\0';
  char folder[25]="Received_data/";
  strcat(folder,out);
  bzero(out,l);
  strcat(out,folder);

  // printf("%s\n",out );
  return out;
}


char bytefromtext(char* text) 
{
  char result=0;
  for(int i=0;i<8;i++)
  {
    if(text[i]=='1')
    {
      result |= (1<< (7-i) );
    }
  }
  return result;
}



int main(int argc, char **argv) {
  int parentfd; /* parent socket */
  int childfd; /* child socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buffer */
  char data[MAX_FILE_SIZE]; 
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */

  /* 
   * check command line arguments 
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* 
   * socket: create the parent socket 
   */
  parentfd = socket(AF_INET, SOCK_STREAM, 0);
  if (parentfd < 0) 
    error( COLOR_RED "ERROR opening socket" COLOR_RESET);

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));

  /* this is an Internet address */
  serveraddr.sin_family = AF_INET;

  /* let the system figure out our IP address */
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

  /* this is the port we will listen on */
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(parentfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error( COLOR_RED "ERROR on binding" COLOR_RESET);

  printf("Server Running ....\n");
  /* 
   * listen: make this socket ready to accept connection requests 
   */
  if (listen(parentfd, 5) < 0) /* allow 5 requests to queue up */ 
    error( COLOR_RED "ERROR on listen" COLOR_RESET);
  /* 
   * main loop: wait for a connection request, echo input line, 
   * then close connection.
   */
  clientlen = sizeof(clientaddr);
  while (1) {
  	printf(COLOR_CYAN "Waiting for client..................\n" COLOR_RESET);
    /* 
     * accept: wait for a connection request 
     */
    childfd = accept(parentfd, (struct sockaddr *) &clientaddr, &clientlen);
    if (childfd < 0) 
      error( COLOR_RED "ERROR on accept" COLOR_RESET);
    else
    {
      printf("Client ( ");
      printf(COLOR_CYAN "%d.%d.%d.%d"COLOR_RESET,(int)(clientaddr.sin_addr.s_addr&0xFF),(int)((clientaddr.sin_addr.s_addr&0xFF00)>>8),
                  (int)((clientaddr.sin_addr.s_addr&0xFF0000)>>16),(int)((clientaddr.sin_addr.s_addr&0xFF000000)>>24));
      printf(" ) is connected..... \n");
    }

    /* 
     * read: read input string from the client
     */
    bzero(buf, BUFSIZE);
    n = read(childfd, buf, BUFSIZE);
    if (n < 0) 
      error( COLOR_RED "ERROR reading from socket" COLOR_RESET);
    printf("server received %d bytes: %s\n", n, buf);

    /* 
     * write: echo the input string back to the client 
     */
    int m = write(childfd, COLOR_CYAN "Connection OK.....\nSending data.....\n", strlen("Connection OK.....\nSending data.....\n"));
    // printf(""COLOR_RESET);
    if (m < 0)error( COLOR_RED "ERROR writing to socket" COLOR_RESET);
    
  // for (int i = 0; i <= (n/CHUNK); ++i)
    // {
    if(n>0){
    bzero(data, MAX_FILE_SIZE);
/*    m = read(childfd, data, MAX_FILE_SIZE);
    if (m < 0) 
      error( COLOR_RED "ERROR reading from socket");*/
    // printf("%s\n",fileName );
    char *fileName=(char*)malloc(n*sizeof(char));
    strcpy(buf,strtok(buf,"\n"));
    strcpy(fileName,getfileName(buf));
        // strcpy(fileName,getfileName(buf));
    strcpy(fileName,removeSpace(fileName));
    FILE *f;
    if ((f = fopen(fileName, "r")))
    {
        fclose(f);
        write(childfd, ERROR, 1); //info about existance of file
        // printf("%s\n",ERROR );
        printf( COLOR_RED "file already exist.\n" COLOR_RESET);
        continue;
    }
    else{
      // printf("%s\n",OK);
    // printf("%s\n",fileName );
    	write(childfd, OK, 1);  //info about existance of file
	    char *END_FLAG = "\n================END";
	    int fd = open(fileName, O_RDWR | O_CREAT, 0666);
	    int chunk_no=0,size=0;
	    bzero(buf,BUFSIZE);
	    // printf("%s\n",buf );
	    while ((n = read(childfd, buf, BUFSIZE))>0){
	        buf[n] = 0;
	        if (!(strcmp(buf,END_FLAG)))break;
	        size=size+n;
	        if((chunk_no%200)==0)
	        printf("Receiving %dth chunk of size:%dbytes....................%f Mb\n",chunk_no,n,(float)size/(1024.0*1024.0) );
	        chunk_no++;
	       	// printf("%s\n",buf );
	        write(fd, buf, n);
	        bzero(buf,BUFSIZE);
      	}
	    close(fd);
	    char md5checksum[8]="md5sum ";
	    strcat(md5checksum,fileName);
	    strcat(md5checksum,"> 7879797");
	    // printf("%s\n",md5checksum );
	    unsigned int md5;
	    // printf("sachin\n");
	    md5 = system(md5checksum);
	    char MD5[1000];
	    char text;
	    int i=0;
	    FILE *md=fopen("7879797","r");
	    while((fscanf(md,"%c",&text))==1)MD5[i++]=text;
	    fclose(md);
	    remove("7879797");
	    m = write(childfd, MD5, strlen(MD5));
	    if (m < 0)error( COLOR_RED "ERROR writing to socket" COLOR_RESET);
    }
    close(childfd);
  }
}

}
