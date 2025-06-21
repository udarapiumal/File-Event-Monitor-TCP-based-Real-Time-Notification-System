#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <libnotify/notify.h>
#include <signal.h>


#include <arpa/inet.h>
#include <sys/socket.h>
#include "library.h"

#define EXIT_ERR_SUCCESS  0
#define EXIT_ERR_TOO_FEW_ARGS 1
#define EXIT_ERR_INOTIFY 2
#define EXIT_ERR_ADD_WATCH 3
#define EXIT_ERR_BASE_PATH_NULL 4
#define EXIT_ERR_READ_INOTIFY 5
#define EXIT_ERR_INIT_LIBNOTIFY 6
#define PORT 5521
#define EXIT_ERR_CONNECTION 7
#define EXIT_ERR_CONSTRUCTION 8
#define EXIT_ERR_SOCKET_READ 9

void signal_handler(int signal){
	
	printf("Signal recieved Cleaning up....");
	notify_uninit();
	exit(EXIT_ERR_SUCCESS);
}


int main(int argc,char** argv){

    
    uint8_t socketBuffer[FLX_PKT_MAXIMUM_SIZE];
    
    char* notificationMessage=NULL;
    int bytesRead=-1;
    char* ProgramTitle="daemon.c";
    
    bool libnotifyInitStatus=false;
    NotifyNotification* notifyHandle;
    int socketFd=-1;

    
    struct sockaddr_in serverAddress;
    struct flex_msg* readMsg,* sendMsg;
    struct serialize_result* result;

    if(argc < 3){
        fprintf(stderr,"USAGE:HOST PATH\n");
        exit(EXIT_ERR_TOO_FEW_ARGS);
    }
  
    libnotifyInitStatus=notify_init(ProgramTitle);
    if( libnotifyInitStatus==false){
        fprintf(stderr,"Error initializing libnotify\n");
        exit(EXIT_ERR_INIT_LIBNOTIFY);
    }


    signal(SIGABRT,signal_handler);
    signal(SIGINT,signal_handler);
    signal(SIGTERM,signal_handler);

    socketFd=socket(AF_INET,SOCK_STREAM,0);
    serverAddress.sin_family=AF_INET;
    serverAddress.sin_addr.s_addr=inet_addr(argv[1]);
    serverAddress.sin_port=htons(PORT);


    if(connect(socketFd, (struct sockaddr *)&serverAddress,sizeof(serverAddress))==-1){
        fprintf(stderr,"Error connecting to tcp with params %s %d!\n",argv[1],PORT);
        exit(EXIT_ERR_CONNECTION);
    }

    readMsg=(struct flex_msg *)malloc(sizeof(struct flex_msg));
    flex_msg_factory(readMsg);
    sendMsg=(struct flex_msg *)malloc(sizeof(struct flex_msg));
    flex_msg_factory(sendMsg);
    result=(struct serialize_result *)malloc(sizeof(struct serialize_result));
    serialize_result_factory(result);
    
    sendMsg->action=FLX_ACT_WATCH;
    sendMsg->option=FLX_WATCH_ADD;

    sendMsg->dataLen=FLX_DLEN_WATCH;
    sendMsg->data=(char **)malloc(sizeof(char *)*FLX_DLEN_WATCH);
    sendMsg->data[0]=argv[2];
    serialize(socketBuffer,sendMsg,result);
    if(result->reply!=FLX_REPLY_VALID){
        fprintf(stderr,"Error %x connecting init packet\n",result->reply);
        exit(EXIT_ERR_CONSTRUCTION);
    }


write(socketFd,socketBuffer,sizeof(socketBuffer));
free(sendMsg->data);
flex_msg_factory(sendMsg);

    while(true){
        bytesRead=read(socketFd,socketBuffer,sizeof(socketBuffer));
        if(bytesRead==0 || bytesRead==-1){
            fprintf(stderr,"Error from socket\n");
            exit(EXIT_ERR_SOCKET_READ);
        }

        deserialize(socketBuffer,readMsg,result);
        if(result->reply!=FLX_REPLY_VALID){
            fprintf(stderr,"Received %x from client!\n",result->reply);
            sendMsg->action=FLX_ACT_REPLY;
            sendMsg->option=result->reply;
            sendMsg->size=0;

            bzero(socketBuffer,sizeof(socketBuffer));

            serialize(socketBuffer,sendMsg,result);

            write(socketFd,socketBuffer,sizeof(socketBuffer));
            flex_msg_factory(sendMsg);
            continue;
        }




        if(readMsg->action!=FLX_ACT_NOTIFY){
            continue;
        }

        notificationMessage=NULL;
        switch (readMsg->option)
        {
        case FLX_NOTIFY_CREATE:
            notificationMessage="file created\n";
            break;
         case FLX_NOTIFY_DELETE:
            notificationMessage="file deleted\n";
            break;
         case FLX_NOTIFY_ACCESS:
            notificationMessage="file accessed\n";
            break;
        case FLX_NOTIFY_CLOSE:
            notificationMessage="file writtern and closed\n";
            break;
        case FLX_NOTIFY_MODIFY:
            notificationMessage="file modified\n";
            break;
        case FLX_NOTIFY_MOVE:
            notificationMessage="file moved\n";
            break;
        default:
            break;
        }

       
            notifyHandle=notify_notification_new(readMsg->data[0],notificationMessage,"dialog-information");
            if(notifyHandle==NULL){
                fprintf(stderr,"Failed to create notification\n");
                continue;
        }
        notify_notification_show(notifyHandle,NULL);
    

}
}
