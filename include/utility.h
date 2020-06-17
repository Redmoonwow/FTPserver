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

extern int32_t OpenMQ(int32_t e_mq_name);
extern int32_t CloseMQ(int e_src_mq_id , int e_session_id , int e_thread_type);
extern int32_t SendMQ(int e_dst_mq_id , int e_src_mq_id , void* e_send_msg , int32_t e_msg_size);
extern int32_t RecvMQ(int e_src_mq_id , void* e_recv_msg , int32_t e_msg_size );
extern int32_t RecvMQtimeout(int e_src_mq_id , void* e_recv_msg , int32_t e_msg_size , int32_t e_timeout);
extern int32_t RefMQ(void);

extern int32_t OpenMQ_CHILD(int32_t e_session_id , int e_child_type);
extern int32_t SendMQ_CHILD(int e_dst_mq_id , int e_session_id , int e_child_type , void* e_send_msg , int32_t e_msg_size);

//queue

typedef struct st_queue_header
{
	char* m_top_ptr;
	char* m_tail_ptr;
	char* m_empty_top_ptr;
	char* m_empty_tail_ptr;
	uint64_t m_data_size;
}st_queue;

#define BYTE_B 1
#define BYTE_KB 2
#define BYTE_MB 3

extern st_queue* QUEUE_init(uint32_t e_size , int e_size_unit , uint32_t e_create_number);
extern uint32_t QUEUE_push(st_queue* e_queue , void* e_data;
extern int32_t QUEUE_pop(st_queue* e_queue , char** e_data_ptr)
extern int32_t QUEUE_isEmpty(st_queue* e_queue);
extern int32_t QUEUE_isFull(st_queue* e_queue);
extern int32_t QUEUE_isAvailable(st_queue* e_queue);
extern int32_t QUEUE_destroy(st_queue* e_queue);

#endif