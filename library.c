#include <stdio.h>
#include <stdlib.h>
#include "library.h"
#include <unistd.h>


const uint8_t OptionRanges[5]={
    FLX_WATCH_REM,
    FLX_QUIT_ERROR,
    FLX_NOTIFY_MOVE,
    FLX_REPLY_INVALID_DATA,
    FLX_STATUS_ERR_READ_INOTIFY
};
const int ActionSizes[5]={
    FLX_DLEN_WATCH,
    FLX_DLEN_QUIT,
    FLX_DLEN_NOTIFY,
    FLX_DLEN_REPLY,
    FLX_DLEN_STATUS
};
int LastActionIndex=sizeof(OptionRanges)/sizeof(OptionRanges[0]) -1;

void serialize_result_factory(struct serialize_result* result){
    result->size=-1;
    result->reply=FLX_REPLY_UNSET;
}
void flex_msg_factory(struct flex_msg* msg){
    msg->action=FLX_ACT_UNSET;
    msg->option=FLX_UNSET_UNSET;
    msg->size=0;

    msg->data=NULL;
    msg->dataLen=0;
}
void flex_msg_reset(struct flex_msg* msg){
    if(msg->data==NULL){
        flex_msg_factory(msg);
        return;
    }
    for(int i=0;i<msg->dataLen;i++){
        if(msg->data[i]!=NULL){
            free(msg->data[i]);
        }
    }
    free(msg->data);
    flex_msg_factory(msg);
}





void serialize(uint8_t buffer[FLX_PKT_MAXIMUM_SIZE],struct flex_msg* msg,struct serialize_result* result){
    int validDatalength=-1;
    int dataSize=0;
    int runningDataSize=0;
    serialize_result_factory(result);
    if(msg->action<=LastActionIndex){
        buffer[0]=msg->action;
        validDatalength=ActionSizes[msg->action];
    }else{
        result->reply=FLX_REPLY_BAD_ACTION;
        return;
    }
    if(validDatalength!=msg->dataLen){
        result->reply=FLX_REPLY_BAD_SIZE;
    }
    if(msg->option>OptionRanges[msg->action]){
        result->reply=FLX_REPLY_BAD_OPTION;
        return;
    }
    buffer[1]=msg->option;
if(validDatalength!=0 && msg->data==NULL){
    result->reply=FLX_REPLY_INVALID_DATA;
    return;
}
    for(int i=0;i<msg->dataLen;i++){
        if(msg->data[i]==NULL){
            result->reply=FLX_REPLY_INVALID_DATA;
            return;
        }

        for(int j=0;j<FLX_PKT_MAXIMUM_SIZE-FLX_PKT_MINIMUM_SIZE-dataSize;j++){
            runningDataSize+=1;
            buffer[FLX_PKT_MINIMUM_SIZE+dataSize+j]=msg->data[i][j];
            if(msg->data[i][j]=='\0'){
                break;
            }
        }
        dataSize+=runningDataSize;
        runningDataSize=0;
    }
    buffer[2]=dataSize;
    result->size=FLX_PKT_MINIMUM_SIZE+dataSize;
    result->reply=FLX_REPLY_VALID;
    return;
}



void deserialize(uint8_t buffer[FLX_PKT_MAXIMUM_SIZE],struct flex_msg* msg,struct serialize_result* result){
    int validDatalength=-1;
    int dataSizeIndex=0;
    int dataSize=0;
    int* dataSizes=NULL;
    int dataOffset=FLX_PKT_MINIMUM_SIZE;
    serialize_result_factory(result);
    flex_msg_reset(msg);
    if(buffer[0]<=LastActionIndex){
        msg->action=buffer[0];
        validDatalength=ActionSizes[msg->action];
    }else{
        result->reply=FLX_REPLY_BAD_ACTION;
        return;
    }
    if(buffer[1]>OptionRanges[msg->action]){
        result->reply=FLX_REPLY_BAD_OPTION;
        return;
    }
    msg->option=buffer[1];

    if(validDatalength==0){
        if(buffer[2]!=0){
            result->reply=FLX_REPLY_BAD_SIZE;
        }else{
            result->reply=FLX_REPLY_VALID;
        }
        return;
    }
    msg->size=buffer[2];
    dataSizes=(int*)malloc(sizeof(int)*(validDatalength));

    for(int i=FLX_PKT_MINIMUM_SIZE;i<FLX_PKT_MAXIMUM_SIZE;i++){
         if(i > msg->size + FLX_PKT_MINIMUM_SIZE){
        free(dataSizes);
        result->reply=FLX_REPLY_BAD_OPTION;
        return;
    }

    if(dataSizeIndex==validDatalength){
        if(dataSize==1){
            free(dataSizes);
            result->reply=FLX_REPLY_BAD_SIZE;
        }
        break;
    }

    if(buffer[i]=='\0'){
        dataSizes[dataSizeIndex]=++dataSize;
        dataSizeIndex++;
        dataSize=0;
        continue;
    }
    if(buffer[i]<'!' || buffer[i] > '~'){
        free(dataSizes);
        result->reply=FLX_REPLY_INVALID_DATA;
        return;
    }
    dataSize++;
    }
   msg->data=(char** )malloc(sizeof(char*)*validDatalength);
   msg->dataLen=validDatalength;

    for(int i=0;i<validDatalength;i++){
        msg->data[i]=(char* )malloc(sizeof(char)*dataSizes[i]);
        for(int j=0;j<dataSizes[i];j++){
            msg->data[i][j]=buffer[j+dataOffset];
        }
        dataOffset+=dataSizes[i];
    }
    result->reply=FLX_REPLY_VALID;
    result->size=FLX_PKT_MINIMUM_SIZE+msg->size;
    free(dataSizes);
    return;


}
