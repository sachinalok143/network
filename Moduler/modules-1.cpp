/*
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <bits/stdc++.h>
#include <pthread.h>
#include <semaphore.h>


using namespace std;

#define DATA 1
#define ACK 0

static int alarm_fired = 0;
struct timeval tv;
struct itimerval Time;

timer_t timerid;
struct sigevent sev;
struct itimerspec its;

int sockfd, portno;
int serverlen, otherend_len;
struct sockaddr_in serveraddr, otherend_addr;
struct hostent *otherend;
char *hostname, *hostaddrp;
pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER,lock1=PTHREAD_MUTEX_INITIALIZER ,lock2=PTHREAD_MUTEX_INITIALIZER ;

#define BUFSIZE 1024
#define HEAD 3*sizeof(int)
#define sendBufferSize 1000*(BUFSIZE-HEAD)
#define recvBufferSize 1000*(BUFSIZE-HEAD)

int sendBufferPos, sendBackIdx, sendBufferBackIdx, sendBufferAvlSpace;
int recvBufferPos, recvBackIdx, recvBufferBackIdx, recvBufferFilledSpace;
char recvBuffer[recvBufferSize], sendBuffer[sendBufferSize] ;
int cwnd,rwnd=1, ssth, seqToSend, max_ack_recvd, max_ack_sent, dup_ack_count,sendcnt,rcvcnt;
float fraction_wnd;
int pSend[2];
int pRecv[2];



void create_packet(int num, int size, int type, char *data)
{
	((int*)data)[0]=num;
	((int*)data)[1]=size;
	((int*)data)[2]=type;
    //memcpy(packet+HEAD, data, size);
    //memcpy(data, packet, BUFSIZE);
}




void mysig(int sig)
{
	if (sig == SIGALRM)
	{
		printf("Timeout %d %d\n",max_ack_recvd,cwnd);
		alarm_fired = 1;
	}
	signal(SIGALRM, mysig);

}


void error(char *msg) {
	perror(msg);
	exit(0);
}


int appSend(char * buf, int size, int *pSend)
{

	int n=write(pSend[1],buf,size);
    //cout<<n<<"\n";
	return n;
}

int appRecv(char * buf, int size , int *pRecv)
{
	return read(pRecv[0],buf,size);
    /*int t=0,n;
    while(t<size && n)
    {
        n=read(pRecv[0],buf,size-t);
        t+=n;
    }
    return t;*/
}

void *sendBufferHandler(void* V)
{
    sendBufferPos=0;    // used in this process
    sendBufferBackIdx=0;    // used in this process
    sendBufferAvlSpace=sendBufferSize; // used in this process
    int n;
    while(1)
    {
        //cout<<sendcnt<<" sendcnt\n";
    	n=sendBufferSize-sendBufferBackIdx;
    	if(n>sendBufferAvlSpace)n=sendBufferAvlSpace;

        //cout<<sendBufferAvlSpace<<" "<<sendBackIdx<<" "<<sendBufferPos<<"\n";
    	if((n=read(pSend[0],sendBuffer+sendBufferBackIdx,BUFSIZE-HEAD>n?n:BUFSIZE-HEAD))>0)
    	{
    		pthread_mutex_lock(&lock);
    		sendBufferBackIdx=(sendBufferBackIdx+n)%(sendBufferSize);
    		sendcnt+=n;
            //cout<<n<<" "<<sendBufferBackIdx<<" "<<sendcnt<<" sendidx\n";
    		sendBufferAvlSpace-=n;
    		pthread_mutex_unlock(&lock);
    		
    	}
    	
    }
}



void *recvBufferHandler(void* V)
{
    recvBufferPos=0;    // used in process
    recvBufferBackIdx=0;      // used in process
    recvBufferFilledSpace=0; // used in this process
    int n;
    while(1)
    {
        //cout<<recvBufferFilledSpace<<" receiver\n";
    	if(recvBufferFilledSpace > (BUFSIZE-HEAD))
    		n=BUFSIZE-HEAD;
    	else
    		n=recvBufferFilledSpace;

    	if(recvBufferSize-recvBufferPos<n)n= recvBufferSize-recvBufferPos;
    	if(n>0 && (n=write(pRecv[1],recvBuffer+recvBufferPos,n))>0)
    	{
        //cout<<"rB data "<<recvBuffer+recvBufferPos<<endl;
    		pthread_mutex_lock(&lock1);
    		recvBufferPos=(recvBufferPos+n)%(recvBufferSize);
    		recvBufferFilledSpace-=n;
    		pthread_mutex_unlock(&lock1);
    	}
    }
}



int udp_send(int sockfd, char *buf, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
	int n = ((int*)buf)[1];
    //cout<<buf+HEAD<<"send\n";
	n = sendto(sockfd,buf,n+HEAD,flags,dest_addr,addrlen);
    //cout<<((int*)buf)[0]<<"send\n";
	return n;
}



int udp_recv(int sockfd, char *buf, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
	bzero(buf,BUFSIZE);
	
	int ret;
    /*
    if((ret=recvfrom(sockfd,buf,BUFSIZE,flags,src_addr,addrlen))<=0)return ret;
    int n=((int*)buf)[1];
  */
	if((ret=recvfrom(sockfd,buf,BUFSIZE,flags,src_addr,addrlen))<=0)return -1;
    //cout<<ret<<" ret"<<endl;

	int n = ((int*)buf)[1];
    //cout<<buf+HEAD;
//cout<<((int*)buf)[0]<<" "<<((int*)buf)[1]<<" "<<((int*)buf)[2]<<" "<<recvBufferFilledSpace<<" recvd"<<endl;
    //if((ret=recvfrom(sockfd,buf+HEAD,n,flags,src_addr,addrlen))<=0)cout<<ret;
    //cout<<"2nd ret "<<ret<<" "<<strlen(buf+HEAD)<<endl;
    //cout<<buf+HEAD<<endl;
////cout<<((int*)buf)[0]<<" "<<((int*)buf)[1]<<" "<<((int*)buf)[2]<<" "<<buf+HEAD<<endl;
   // n=11; while(n++<BUFSIZE)//cout<<buf[n];
    ////cout<<buf+HEAD<<endl;
	

	
    //int n=recvfrom(sockfd,buf,BUFSIZE,flags,src_addr,addrlen);*/
	return ret;
}



int send_ack(int ackNum, int advertisedWindowSize, int sockfd, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
	char data[BUFSIZE];
	((int*)data)[3]=advertisedWindowSize;

	create_packet(ackNum,sizeof(int),ACK,data);

	int n = udp_send(sockfd,data,0,dest_addr,addrlen);
    //cout<<((int*)data)[0]<<" "<<((int*)data)[1]<<" "<<((int*)data)[2]<<" "<<((int*)data)[3]<<endl;

	return n;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void update_window(char *buf)
{
	int ack;
	if(buf){
		ack= ((int*)buf)[0];
    //cout<<ack<<"\n";
		rwnd=((int*)buf)[3]/(BUFSIZE);
		if(rwnd<1)rwnd=1;
    //cout<<"rwnd = "<<rwnd;
		if(ack==max_ack_recvd)
		{
			dup_ack_count++;
			if(dup_ack_count>=3)
			{
				ssth=cwnd/2;
				cwnd=ssth;
				seqToSend=max_ack_recvd;
				cout<<"dup_ack count 3\n";
				pthread_mutex_lock(&lock);
				sendBufferPos=(sendBufferBackIdx+sendBufferAvlSpace)%(sendBufferSize);
				sendcnt=sendBufferSize-sendBufferAvlSpace;
				pthread_mutex_unlock(&lock);
			}
		}   
		else if(ack>max_ack_recvd)
		{
        //cout<<ack<<" ack\n";
			pthread_mutex_lock(&lock2);
			its.it_value.tv_sec = 0;
			if (timer_settime(timerid, 0, &its, NULL) == -1)
				printf("timer_settime\n");
			pthread_mutex_unlock(&lock2);
			dup_ack_count=0;
			pthread_mutex_lock(&lock);
			sendBufferAvlSpace+=(ack-max_ack_recvd);
			pthread_mutex_unlock(&lock);
			if(cwnd>=ssth)
			{
				fraction_wnd+=(float)(ack-max_ack_recvd)/(BUFSIZE-HEAD);
				while(fraction_wnd>=cwnd)
				{
					fraction_wnd-=cwnd;
					cwnd++;
				}
			}
			else
			{
				fraction_wnd=0;
				if(cwnd+(ack-max_ack_recvd)/(BUFSIZE-HEAD)>=ssth)cwnd=ssth;
				else cwnd +=(ack-max_ack_recvd)/(BUFSIZE-HEAD);
			}
			max_ack_recvd=ack;
		}
	}

	else
	{
		ssth=cwnd/2;
		cwnd=1;
		seqToSend=max_ack_recvd;
		pthread_mutex_lock(&lock);
		sendBufferPos=(sendBufferBackIdx+sendBufferAvlSpace)%(sendBufferSize);
		sendcnt=sendBufferSize-sendBufferAvlSpace;
		pthread_mutex_unlock(&lock);
	}
    //if(cwnd>1)
    //cout<<cwnd<<"\n";
    //cout<<fraction_wnd;
}

void *rate_control(void* V)
{
	cwnd=1;
	char data[BUFSIZE];
	seqToSend=0;
	max_ack_recvd=0;
	int avlData=0;
	int i, chunks, size, n;
    //signal(SIGALRM, mysig);

	while(1)
	{
		while((seqToSend-max_ack_recvd)/(BUFSIZE-HEAD)<(cwnd>rwnd?rwnd:cwnd))
		{
            //cout<<seqToSend<<" "<<max_ack_recvd<<" "<<avlData<<"\n";
			avlData=sendcnt;
			if(avlData>0)
			{
                ////cout<<sendBufferPos<<" "<<sendBufferBackIdx<<" "<<sendBufferSize<<"ab\n";
                //cout<<avlData<<"\n";
				if(avlData>BUFSIZE-HEAD)n=BUFSIZE-HEAD;
				else n=avlData;
                //cout<<n<<"\n";
				if(n>sendBufferSize-sendBufferPos)
				{
					memcpy(data+HEAD,sendBuffer+sendBufferPos,sendBufferSize-sendBufferPos);
					memcpy(data+HEAD+sendBufferSize-sendBufferPos,sendBuffer,n-(sendBufferSize-sendBufferPos));
				}
				else  memcpy(data+HEAD,sendBuffer+sendBufferPos,n);
                ////cout<<data<<"\n";

                ////cout<<"data available to write "<<seqToSend<<" "<<max_ack_recvd<<" "<<cwnd<<" "<<sendBufferPos<<endl;

                /*if(seqToSend==0)
                {
                    for(i=0;data[i]!='\0';++i);
                    for(;data[i]!=' ';--i);
                    chunks=atoi(data+i+1);
                    for(--i;data[i]!=' ';--i);
                    size=atoi(data+i+1);
                }
                if(seqToSend==chunks)n=size;*/
                ////cout<<n<<"\n";
				create_packet(seqToSend,n,DATA,data);
                //cout<<(data+HEAD)<<endl;
				avlData=((int*)(data))[0];
                //cout<<"here "<<avlData<<endl;
				pthread_mutex_lock(&lock);
				sendBufferPos=(sendBufferPos+n)%(sendBufferSize);
				sendcnt-=n;
				pthread_mutex_unlock(&lock);
				seqToSend+=n;
				
				pthread_mutex_lock(&lock2);
                /*Time.it_value.tv_usec=0;
                Time.it_value.tv_sec=1;
                alarm_fired=0;
                setitimer(ITIMER_REAL,&Time,NULL);*/
				its.it_value.tv_nsec = 100000000;
				if (timer_settime(timerid, 0, &its, NULL) == -1)
					printf("timer_settime\n");
				avlData=udp_send(sockfd, data, 0, (struct sockaddr*)&otherend_addr, otherend_len);
				pthread_mutex_unlock(&lock2);
                //cout<<"packet sent "<<avlData<<endl;
                //sendBufferPos+=(BUFSIZE-HEAD+sendBufferSize);
                //sendBufferPos%=sendBufferSize;
                //seqToSend+=1;

                //alarm start
				
			}
		}

		if(alarm_fired)
		{
            //cout<<"alarmFired"<<endl;
			alarm_fired=0;
			update_window(NULL);
            //sendBufferPos = (sendBufferBackIdx+sendBufferAvlSpace)%(sendBufferSize);
		}
	}
}

void *parse_packets(void* V)
{

	char buf[BUFSIZE];
	int a,i, seq, size, advertisedWindowSize;
	while(1)
	{
		bzero(buf,BUFSIZE);
        //cout<<"Hey"<<endl;
		if((a=udp_recv(sockfd,buf,0,(struct sockaddr *)&otherend_addr,(socklen_t*)&otherend_len))<=0)
		{
			cout<<"error on receive "<<endl;
		}



		i=((int*)buf)[2];
		if(i==DATA)
		{
            //write/send data in recvBuffer

			seq=((int*)buf)[0];
			size=((int*)buf)[1];
            //cout<<seq<<" "<<max_ack_sent<<" "<<size<<" mas\n";

			if(seq==max_ack_sent && size<=(recvBufferSize-recvBufferFilledSpace))
			{
				if((recvBufferSize-recvBufferBackIdx)>=size)memcpy(recvBuffer+recvBufferBackIdx,buf+HEAD,size);
				else
				{
                    //size-=(recvBufferSize-recvBufferBackIdx);
					memcpy(recvBuffer+recvBufferBackIdx,buf+HEAD,recvBufferSize-recvBufferBackIdx);
					memcpy(recvBuffer,buf+HEAD+recvBufferSize-recvBufferBackIdx,size-(recvBufferSize-recvBufferBackIdx));
				}
				pthread_mutex_lock(&lock1);
				recvBufferFilledSpace+=size;
				recvBufferBackIdx=(recvBufferBackIdx+size)%(recvBufferSize);
				pthread_mutex_unlock(&lock1);
				max_ack_sent=seq+size;
				advertisedWindowSize=(recvBufferSize-recvBufferFilledSpace);
				send_ack(seq+size,advertisedWindowSize,sockfd,0,(struct sockaddr*)&otherend_addr,otherend_len);
            //cout<<seq<<" "<<max_ack_sent<<" mas\n";
				
            //send ack

			}

			else if(seq<max_ack_sent)
			{
                //cout<<seq<<" "<<max_ack_sent<<" mas\n";
				
				advertisedWindowSize=(recvBufferSize-recvBufferFilledSpace);
				send_ack(seq+size,advertisedWindowSize,sockfd,0,(struct sockaddr*)&otherend_addr,otherend_len);
			}

			

		}
		else if(i==ACK)
		{
            //cout<<"ACK\n";
			update_window(buf);
		}
	}
}


int main(int argc, char **argv) {

	int status=0;
	pipe(pSend);
	pipe(pRecv);
	if (pthread_mutex_init(&lock, NULL) != 0)
	{
		printf("\n mutex init has failed\n");
		return 1;
	}
	if(fork())
	{
        //THIS IS APPLICATION LAYER

		char buf[BUFSIZE],buf1[BUFSIZE];
		int i,m,n,optval;

		if(argc==3)
		{

            /* check command line arguments */
			if (argc != 3) {
				fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
				exit(0);
			}


            /* get file from user */
			bzero(buf, BUFSIZE);
			puts("Enter fileName you want to transfer");
			fgets(buf, BUFSIZE, stdin);

			for(i=0;buf[i]!='\n' && buf[i]!='\0';++i);
				buf[i]='\0';

            /* finding size of file  */
			FILE *fp=fopen(buf,"rb");
			if(fp<=0)
			{
				cout<<"error opening file\n";
			}
			fseek(fp, 0L, SEEK_END);
			int x= ftell(fp);
			fclose(fp);

            // y is #chunks
			int y=(x%(BUFSIZE-HEAD)?x/(BUFSIZE-HEAD)+1:x/(BUFSIZE-HEAD));

            /* opening file for transfer */
			char buf2[BUFSIZE];
			strcpy(buf2,buf);
			int fr=open(buf,O_RDONLY);
			buf[i]=' ';
			sprintf(buf+i+1,"%d",x);
			for(;buf[i]!='\n' && buf[i]!='\0';++i);
				buf[i]=' ';++i;
			sprintf(buf+i,"%d",y);
            printf("fileName, size, #chunks are -> %s\n",buf);   // buf contains -> [fileName<space>size<space>chunks]

            /*while(1){
                   n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr*)&serveraddr, serverlen);
                   printf("%d\n",n);
                    n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr*)&serveraddr, &serverlen);
                if(n>0)break;
            }*/
            appSend(buf,strlen(buf), pSend);
            appRecv(buf,BUFSIZE-HEAD,pRecv);
            cout<<"server says "<<buf<<"\n";
            /* transfering file */
            i=0;
            while(n=read(fr,buf,BUFSIZE-HEAD))
            {
            	i+=appSend(buf,n, pSend);
                //cout<<i<<" sent\n";

            }

            printf("File/Data sent from APPLICATION LAYER\n");
        }
        else if(argc==2)
        {


        	appRecv(buf,BUFSIZE-HEAD, pRecv);
              printf("Receiving File/Data from %s\n", hostaddrp);  //////////////////////////////////////////////////////////
              printf("file name , size , number of chunks = %s\n",buf);

            /*
              creating new file
            */
              for(i=0;buf[i]!='\0';++i);
              	for(;buf[i]!=' ';--i);
              		int chunks=atoi(buf+i+1);
              	for(--i;buf[i]!=' ';--i);
              		int size=atoi(buf+i+1);
              	buf[i]='\0';
              	int file=open(buf,O_CREAT | O_RDWR,S_IRUSR | S_IWUSR);
              	strcpy(buf,"OK\0");
              	appSend(buf,3,pSend);
                //receiving file
              	printf("receiving file..\n");
              	int t=0;
              	while(t<size)
              	{
              		bzero(buf,BUFSIZE);
              		if((n=appRecv(buf,BUFSIZE, pRecv))>0)
              			write(file,buf,n);
              		t+=n;
                    //cout<<"received "<<t<<"\n";
              	}
              	printf("File/Data received\n");
              	close(file);
              }
              else
              {
              	fprintf(stderr, "usage for receiving file: %s <listening_port>\n", argv[0]);
              	fprintf(stderr, "usage for sending   file: %s <dest_hostname> <dest_port>\n", argv[0]);
              	exit(0);
              }
          }
          else
          {
        //THIS IS TRANSPORT LAYER
          	signal(SIGALRM, mysig);
          	sev.sigev_notify = SIGEV_SIGNAL;
          	sev.sigev_signo = SIGALRM;
          	sev.sigev_value.sival_ptr = &timerid;
          	its.it_value.tv_sec = 0;
          	its.it_value.tv_nsec = 0;
          	its.it_interval.tv_sec = 0;
          	its.it_interval.tv_nsec = 0;

          	if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1)cout<<"timer erro\n";
        /* socket: create the socket */
          	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
          	if (sockfd < 0)
          		error("ERROR opening socket");

          	if(argc==3)
          	{
          		hostname = argv[1];
          		portno = atoi(argv[2]);

            /* gethostbyname: get the otherend's DNS entry */
          		otherend = gethostbyname(hostname);
          		if (otherend == NULL) {
          			fprintf(stderr,"ERROR, no such host as %s\n", hostname);
          			exit(0);
          		}

            /* build the otherend's Internet address */
          		bzero((char *) &otherend_addr, sizeof(otherend_addr));
          		otherend_addr.sin_family = AF_INET;
          		bcopy((char *)otherend->h_addr,
          			(char *)&otherend_addr.sin_addr.s_addr, otherend->h_length);
          		otherend_addr.sin_port = htons(portno);
          		otherend_len = sizeof(otherend_addr);

          		int optval = 1;
          		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int)); 
          	}
          	else if(argc==2)
          	{

          		portno = atoi(argv[1]);

                          /* setsockopt: Handy debugging trick that lets
                           * us rerun the server immediately after we kill it;
                           * otherwise we have to wait about 20 secs.
                           * Eliminates "ERROR on binding: Address already in use" error.
                           */
          		int optval = 1;
          		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

                          /*
                           * build the server's Internet address
                           */
          		bzero((char *) &serveraddr, sizeof(serveraddr));
          		serveraddr.sin_family = AF_INET;
          		serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
          		serveraddr.sin_port = htons((unsigned short)portno);

                          /*
                           * bind: associate the parent socket with a port
                           */
          		if (bind(sockfd, (struct sockaddr *) &serveraddr,
          			sizeof(serveraddr)) < 0)
          			error("ERROR on binding");

          		otherend_len = sizeof(otherend_addr);
                /*char buf[1024];
                recvfrom(sockfd,buf,12,0,(struct sockaddr *)&otherend_addr,&otherend_len);//cout<<"a\n";
                printf("%s\n",buf);
                hostaddrp=inet_ntoa(otherend_addr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server received datagram from %s\n", hostaddrp);
                int n=recvfrom(sockfd,buf+12,18,0,(struct sockaddr *)&otherend_addr,&otherend_len);//cout<<"b\n";
                printf("%d\n",n);*/


          	}


          	pthread_t thread1, thread2, thread3, thread4;

          	int iret1, iret2, iret3, iret4;

          	iret1 = pthread_create( &thread1, NULL, sendBufferHandler, (void*)NULL);
          	iret2 = pthread_create( &thread2, NULL, recvBufferHandler, (void*)NULL);
          	iret3 = pthread_create( &thread3, NULL, rate_control, (void*)NULL);
          	iret4 = pthread_create( &thread4, NULL, parse_packets, (void*)NULL);

          	pthread_join(thread1,NULL);
          	pthread_join(thread2,NULL);
          	pthread_join(thread3,NULL);
          	pthread_join(thread4,NULL);
        //cout<<"Bye"<<endl;
          	exit(0);


          }

          while(wait(&status));


          return 0;
      }
