///////////////////////////////////////////////////////////////////
// @file utility.h
// @date 2020/06/21
///////////////////////////////////////////////////////////////////

#ifndef __UTILITY_H_
#define __UTILITY_H_
#include "headers.h"


#define RELEASE		0
#define DEBUG			1
#define ALL				2

void trc(const char* e_string , ...); // トレースログ
int InitUtilitis(void);
int CloseUtilities(void);

typedef struct st_function_msg_list
{
	uint32_t m_commandcode;
	int(*func)(char* e_message);
}st_function_msg_list;

extern void trc(const char* e_string , ...);
extern int InitUtilitis(void);
extern int CloseUtilities(void);

extern pid_t Create_Cmp(int e_flag , void* (*e_start_routine) (void*) , void* e_arg);
extern int CloseCmp(pid_t e_tid);

extern int32_t OpenMQ(int32_t e_mq_name);
extern int32_t CloseMQ(int e_src_mq_id , int e_session_id , int e_thread_type);
extern int32_t SendMQ(int e_dst_mq_id , int e_src_mq_id , void* e_send_msg , int32_t e_msg_size);
extern int32_t RecvMQ(int e_src_mq_id , void* e_recv_msg , int32_t e_msg_size );
extern int32_t RecvMQtimeout(int e_src_mq_id , void* e_recv_msg , int32_t e_msg_size , int32_t e_timeout);
extern int32_t RefMQ(void);

extern int32_t OpenMQ_CHILD(int32_t e_session_id , int e_child_type);
extern int32_t SendMQ_CHILD(int e_dst_mq_id , int e_session_id , int e_child_type , void* e_send_msg , int32_t e_msg_size);

#endif