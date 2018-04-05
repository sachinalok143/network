/* 
 * sachin kumar -15CS30025
 * Pankaj Dhurve-15CS10031
 * usage: tcpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h> 


#include <sys/stat.h>
#include <fcntl.h>

#define BUFSIZE 1024
#define CHUNK_SIZE 1024
#define MAX_FILE_SIZE 999999

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

char *removeNewL(char * buf){
   int l =strlen(buf);
   char *buf1=(char*)malloc(l*sizeof(char));
    int i;
    for (i = 0; i < strlen(buf)-1; ++i)
    {
        buf1[i]=buf[i];
    }
    buf1[i]='\0';
    return buf1;
}




int main(int argc, char **argv) {
    int sockfd, portno, n;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];

    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error( COLOR_RED "ERROR opening socket" COLOR_RESET);

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
      (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* connect: create a connection with the server */
    if (connect(sockfd, &serveraddr, sizeof(serveraddr)) < 0) 
      error( COLOR_RED "ERROR connecting" COLOR_RESET);

    /* get message line from the user */
    printf("Please enter file name: ");
    bzero(buf, BUFSIZE);
    fgets(buf, BUFSIZE, stdin);
    strcpy(buf,removeNewL(buf));
    // printf("%s\n",buf);
    char file[BUFSIZE];
    strcpy(file,buf);

    FILE *fp;
    fp=fopen(buf,"r");
    
    int fsize=0;
    char text;
    char *data=(char*)malloc(MAX_FILE_SIZE*sizeof(char));
    if(fp==NULL){error( COLOR_RED "ERROR file dosen't exist." COLOR_RESET);exit(0);}
    while((fscanf(fp,"%c",&text)==1))fsize++;
    fclose(fp);
    
    char fileSize[5];
    sprintf(fileSize,"%d",fsize);

    char filename[15] =" ";
    strcat(filename, buf);
    strcat(filename, "\n");
    strcat(filename, fileSize);
    strcat(filename, " bytes\n");
    // printf("%s\n",filename );

    /* send the message line to the server */
    n = write(sockfd, filename, strlen(filename));
    if (n < 0)error( COLOR_RED "ERROR writing to socket" COLOR_RESET);

    char info[BUFSIZE];
    n = read(sockfd, info, BUFSIZE);
    if (n < 0) 
      error( COLOR_RED "ERROR reading from socket" COLOR_RESET);
    printf("%s"COLOR_RESET, info);
   
    char temp[1];
    bzero(temp,10);
    if(read(sockfd,temp,10)<0)error( COLOR_RED "ERROR" COLOR_RESET);
    // printf("%s\n", temp);
    if(atoi(&temp[0]))
        {
            error( COLOR_RED "File Exist On Server" COLOR_RESET);
            exit(0);
        }




    char *END_FLAG = "\n================END";

    char * fContent=(char*)malloc(BUFSIZE*sizeof(char));
    int fd;
    strcpy(buf,strtok(buf, "\n"));
    fd = open(buf, O_RDONLY);
    printf("%s\n",buf);
    if(fd<0)error( COLOR_RED "ERROR,file doesn't exist:" COLOR_RESET);
    
    // printf("%d\n",n );
    int chunk_no=0,size=0;
        bzero(fContent,BUFSIZE);
    while ((n = read(fd, fContent, BUFSIZE))>0) {
        write(sockfd, fContent, BUFSIZE);
        size=size+n;
        if((chunk_no%200)==0)
        printf("Sending %dth chunk of size:%dbytes.....................%f Mb\n",chunk_no,n,(float)size/(1024.0*1024.0) );
        chunk_no++;
        // printf("%s\n",fContent ) ;
        bzero(fContent,BUFSIZE);
    }
    write(sockfd, END_FLAG, BUFSIZE);
    // printf("%s:sent\n",END_FLAG );
    close(fd);

    // printf("\n");
    char md5[1000];
    bzero(md5, BUFSIZE);
    // printf("sachin\n");
    n = read(sockfd, md5, BUFSIZE);
    // printf("sk\n" );
    if (n < 0) 
      error( COLOR_RED "ERROR reading from socket" COLOR_RESET);
  
    
    n=strlen(md5);
    char *name=(char*)malloc(n*sizeof(char));
    name = strtok(NULL, " ");
    name = strtok(md5, " ");
    
    char md5checksum[8]="md5sum ";
    strcat(md5checksum,file);
    strcat(md5checksum," > 7879797");
    system(md5checksum);
    char MD5[1000];
    // char text;
    int i=0;
    FILE *md=fopen("7879797","r");
    while((fscanf(md,"%c",&text))==1)MD5[i++]=text;
    fclose(md);
    n=strlen(MD5);
    char *name1=(char*)malloc(n*sizeof(char));
    name1 = strtok(NULL, " ");
    name1 = strtok(md5, " ");
    // printf("%s\n",name );
    // printf("%s\n",name1 );
    remove("7879797");
    if(strcmp(name1,name))printf("MD5 Not Mathed..........\n");
    else printf("MD5 Mathed............\n");
    // printf("%d\n", );
    close(sockfd);
    return 0;
}