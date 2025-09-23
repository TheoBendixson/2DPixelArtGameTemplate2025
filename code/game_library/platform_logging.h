
#define PLATFORM_LOG_MESSAGE(name) void name(char *Message) 
typedef PLATFORM_LOG_MESSAGE(platform_log_message);

#define PLATFORM_LOG_MESSAGE_U32(name) void name(char *Message, u32 Param) 
typedef PLATFORM_LOG_MESSAGE(platform_log_message_u32);
