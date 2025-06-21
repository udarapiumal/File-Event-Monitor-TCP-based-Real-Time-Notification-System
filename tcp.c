#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/inotify.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "library.h"
#include <signal.h>


#define EXIT_ERR_SUCCESS  0
#define EXIT_ERR_TOO_FEW_ARGS 1
#define EXIT_ERR_INOTIFY 2
#define EXIT_ERR_ADD_WATCH 3
#define EXIT_ERR_BASE_PATH_NULL 4
#define EXIT_ERR_READ_INOTIFY 5
#define EXIT_ERR_INIT_LIBNOTIFY 6
#define EXIT_ERR_SOCKET 7
#define PORT 5521
#define EXIT_ERR_BIND 8
#define EXIT_ERR_LISTEN 9
#define EXIT_ERR_CONN 10
#define EXIT_ERR_SOCKET_READ 11
int IeventQueue=-1;
int IeventStatus=-1;


void signal_handler(int signal){
	int close_status=-1;
	printf("Signal recieved Cleaning up....");
	
	close_status=inotify_rm_watch(IeventQueue,IeventStatus);
	if(close_status==-1){
	   fprintf(stderr,"Error Removing File watch\n");
	}
	
	close(IeventQueue);
	exit(EXIT_ERR_SUCCESS);
}


int main(){

    char* basePath=NULL;
    char* token=NULL;
    char buffer[4096];
    uint8_t socketBuffer[FLX_PKT_MAXIMUM_SIZE];
    char* watchPath=NULL;
   
    int readLength;
    

    
    const struct inotify_event* watchEvent;
    
 

    const uint32_t watchMask=IN_CREATE | IN_DELETE | IN_ACCESS | IN_CLOSE_WRITE | IN_MODIFY |IN_MOVE_SELF ;
    struct flex_msg* readMsg, *sendMsg;
    struct serialize_result* result;

    int socketFd,connectionFd=-1;
    int byteRead=-1;

    struct sockaddr_in serverAddress,clientAddress;
    socklen_t addressSize=sizeof(struct sockaddr_in);

    if((socketFd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        fprintf(stderr,"Error creating socket\n");
        exit(EXIT_ERR_SOCKET);
    }
    int opt = 1;
setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    bzero(&serverAddress,sizeof(serverAddress));
    serverAddress.sin_family=AF_INET;
    serverAddress.sin_addr.s_addr=htonl(INADDR_ANY);
    serverAddress.sin_port=htons(PORT);




    if(bind(socketFd,(struct sockaddr *)&serverAddress,sizeof(serverAddress))==-1){
        fprintf(stderr,"Error binding socket\n");
        exit(EXIT_ERR_BIND);
    }
    if(listen(socketFd,1)==-1){
        fprintf(stderr,"Error Listening to the PORT\n");
        exit(EXIT_ERR_LISTEN);
    }
    printf("Server is Listening on %d\n",PORT);
    if((connectionFd=accept(socketFd,(struct sockaddr *)&clientAddress,(socklen_t *)&addressSize))==-1){
        fprintf(stderr,"Error Accepting Connection\n");
        exit(EXIT_ERR_CONN);
    }
    readMsg=(struct flex_msg *)malloc(sizeof(struct flex_msg));
    flex_msg_factory(readMsg);
    sendMsg=(struct flex_msg *)malloc(sizeof(struct flex_msg));
    flex_msg_factory(sendMsg);
    result=(struct serialize_result *)malloc(sizeof(struct serialize_result));
    serialize_result_factory(result);

    while(true){
        byteRead=read(connectionFd,socketBuffer,sizeof(socketBuffer));
        if(byteRead==0 || byteRead==-1){
            fprintf(stderr,"Error reading from socket\n");
            exit(EXIT_ERR_SOCKET_READ);
        }
        deserialize(socketBuffer,readMsg,result);
        if(result->reply!=FLX_REPLY_VALID){
            fprintf(stderr,"Recived erro %x from client !\n",result->reply);
            sendMsg->action=FLX_ACT_REPLY;
            sendMsg->option=result->reply;
            sendMsg->size=0;

            bzero(socketBuffer,FLX_PKT_MAXIMUM_SIZE);
            serialize(socketBuffer,sendMsg,result);
            write(connectionFd,socketBuffer,sizeof(socketBuffer));
            continue;
        }
        if(readMsg->action!=FLX_ACT_WATCH){
            continue;
        }
        break;
    }
    watchPath=(char *)malloc(sizeof(char)*strlen(readMsg->data[0]) +1);
    strcpy(watchPath,readMsg->data[0]);
    
    basePath=watchPath;
 

    token=strtok(basePath,"/");
    while(token !=NULL){
        basePath=token;
        token=strtok(NULL,"/");
    }
    printf("%s\n",basePath);

    if(basePath==NULL){
        fprintf(stderr,"Error getting basepath\n");
        exit(EXIT_ERR_BASE_PATH_NULL);
    }
    
   
    IeventQueue=inotify_init();
    if(IeventQueue==-1){
        fprintf(stderr,"Failed to initialize inotify\n");
        exit(EXIT_ERR_INOTIFY);
    }
    IeventStatus=inotify_add_watch(IeventQueue,watchPath,watchMask);
        if(IeventStatus==-1){
            fprintf(stderr,"Failed to add watch\n");
            exit(EXIT_ERR_ADD_WATCH);
        }
    signal(SIGABRT,signal_handler);
    signal(SIGINT,signal_handler);
    signal(SIGTERM,signal_handler);
    while(true){
        printf("waiting for event....\n");
        
        readLength=read(IeventQueue,buffer,sizeof(buffer));
        if(readLength==-1){
            fprintf(stderr,"Failed to read inotify event\n");
            exit(EXIT_ERR_READ_INOTIFY);
        }
        for(char* bufferPointer=buffer;bufferPointer < buffer + readLength;bufferPointer+=sizeof(struct inotify_event)+watchEvent->len){
            watchEvent=(const struct inotify_event *) bufferPointer;
            flex_msg_factory(sendMsg);
            if (watchEvent->mask & IN_CREATE) {
    sendMsg->option = FLX_NOTIFY_CREATE;
}
else if (watchEvent->mask & IN_DELETE) {
    sendMsg->option = FLX_NOTIFY_DELETE;
}
else if (watchEvent->mask & IN_MODIFY) {
    sendMsg->option = FLX_NOTIFY_MODIFY;
}
else if (watchEvent->mask & IN_CLOSE_WRITE) {
    sendMsg->option = FLX_NOTIFY_CLOSE;
}
else if (watchEvent->mask & IN_ACCESS) {
    sendMsg->option = FLX_NOTIFY_ACCESS;
}
else if (watchEvent->mask & IN_MOVE_SELF) {
    sendMsg->option = FLX_NOTIFY_MOVE;
}

            sendMsg->action=FLX_ACT_NOTIFY;
            sendMsg->option=FLX_DLEN_NOTIFY;
            sendMsg->data=(char **)malloc(sizeof(char *)*FLX_DLEN_NOTIFY);

            sendMsg->data[0]=basePath;
            sendMsg->data[1]=watchPath;

            bzero(socketBuffer,FLX_PKT_MAXIMUM_SIZE);

            serialize(socketBuffer,sendMsg,result);
            if(result->reply!=FLX_REPLY_VALID){
                fprintf(stderr,"Error %x constructing watch notfication\n",result->reply);
                continue;
            }
            printf("Sending a notification to client\n");
            write(connectionFd,socketBuffer,sizeof(socketBuffer));

            free(sendMsg->data);
    }

}
}
