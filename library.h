#include<stdint.h>

typedef uint8_t flex_act;
typedef uint8_t flex_opt;

#define FLX_ACT_WATCH                   0x00
#define FLX_ACT_QUIT                    0x01
#define FLX_ACT_NOTIFY                  0x02
#define FLX_ACT_REPLY                   0x03
#define FLX_ACT_STATUS                  0x04
#define FLX_ACT_UNSET                   0xFF

#define FLX_WATCH_ADD                   0x00
#define FLX_WATCH_REM                   0x01

#define FLX_QUIT_USER                   0x00
#define FLX_QUIT_ERROR                  0x01

#define FLX_NOTIFY_CREATE               0x00
#define FLX_NOTIFY_DELETE               0x01
#define FLX_NOTIFY_ACCESS               0x02
#define FLX_NOTIFY_CLOSE                0x03
#define FLX_NOTIFY_MODIFY               0x04
#define FLX_NOTIFY_MOVE                 0x05

#define FLX_REPLY_VALID                 0x00
#define FLX_REPLY_BAD_SIZE              0x01
#define FLX_REPLY_BAD_ACTION            0x02
#define FLX_REPLY_BAD_OPTION            0x03
#define FLX_REPLY_BAD_PATH              0x04
#define FLX_REPLY_INVALID_DATA          0x05
#define FLX_REPLY_UNSET                 0xFF

#define FLX_STATUS_SUCCESS              0x00
#define FLX_STATUS_ERR_INIT_INOTIFY     0x01
#define FLX_STATUS_ERR_ADD_WATCH        0x02
#define FLX_STATUS_ERR_READ_INOTIFY     0x03

#define FLX_UNSET_UNSET                 0x00

#define FLX_PKT_MAXIMUM_SIZE            255
#define FLX_PKT_MINIMUM_SIZE            3

#define FLX_DLEN_WATCH                  1
#define FLX_DLEN_QUIT                   0
#define FLX_DLEN_NOTIFY                 2
#define FLX_DLEN_REPLY                  0
#define FLX_DLEN_STATUS                 0
#define FLX_DLEN_UNSET                  0

struct flex_msg{
    flex_act action;
    flex_opt option;
    uint8_t size;

    char** data;
    int dataLen;
};

struct serialize_result{
    int size;
    flex_opt reply;
};         


void flex_msg_factory(struct flex_msg* message);
void flex_msg_reset(struct flex_msg* message);
void serialize_result_factory(struct serialize_result* result);

void deserialize(uint8_t buffer[FLX_PKT_MAXIMUM_SIZE],struct flex_msg* msg,struct serialize_result* result);
void serialize(uint8_t buffer[FLX_PKT_MAXIMUM_SIZE],struct flex_msg* msg,struct serialize_result* result);




