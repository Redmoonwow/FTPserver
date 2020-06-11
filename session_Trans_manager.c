///////////////////////////////////////////////////////////////////
// @file session_Trans_manager.c
// @date 2020/06/21
///////////////////////////////////////////////////////////////////

#include "session_Trans_manager.h"


typedef enum st_session_trans_state
{
	WAIT_INIT		= 0x0000,
	WAIT_START_IDLE,
	IDLE,
	WAIT_END,
	WAIT_STOPPER,
}st_session_trans_state;

static st_session_trans_state s_state = WAIT_INIT;

// FUNCTION PROT
static int32_t StartIDLE(char* e_message);
static int32_t RecvAliveCheck(char* e_message);

static st_function_msg_list s_function_list[]=
{
	//	COMMAND						,FUNC
{	FTP_MSG_NTF_START_IDLE			,StartIDLE},
{	FTP_MSG_REQ_ALIVE_CHECK		,RecvAliveCheck},
{	0xFFFF						,NULL}
};

static int32_t InitSessionMng_thread(void);

void* session_mng_thread(void* argv)
{
	trc("[%s: %d] TRANS SESSION START" , __FILE__ , __LINE__);

	int32_t a_return = InitSessionMng_thread( );

	st_msg_ntf_wake_done a_msg_wakedone;
	a_msg_wakedone.m_mq_header.m_commandcode = FTP_MSG_NTF_WAKE_DONE;
	a_msg_wakedone.m_result = RESULT_OK;
	a_return = SendMQ(CMP_NO_INIT_MANAGER_ID , CMP_NO_SESSION_TRANS_ID , &a_msg_wakedone , sizeof(a_msg_wakedone));
	if ( ERROR_RETURN == a_return )
	{
		return NULL;
	}
	s_state = WAIT_START_IDLE;

	char a_msg [10240];

	struct timespec a_sleep;
	memset(&a_sleep , 0 , sizeof(a_sleep));
	a_sleep.tv_nsec = 500000000;

	for ( ;;)
	{
		nanosleep(&a_sleep , NULL);

		memset(&a_msg , 0 , sizeof(a_msg));
		a_return = RecvMQ(CMP_NO_SESSION_TRANS_ID , a_msg , sizeof(a_msg));
		int a_index = 0;
		st_mq_header* a_header = ( st_mq_header*) a_msg;
		while ( 1 )
		{
			// ƒRƒ}ƒ“ƒhŒŸõ
			if ( s_function_list [a_index].m_commandcode == a_header->m_commandcode )
			{
				a_return = s_function_list[a_index].func(a_msg);
				if ( ERROR_RETURN == a_return )
				{
					trc("[%s: %d] func error" , __FILE__ , __LINE__);
				}
				break;
			}
			else if ( s_function_list [a_index].m_commandcode == 0xFFFF )
			{
				trc("[%s: %d] MSG unknown" , __FILE__ , __LINE__);
				break;
			}
			a_index++;
		}
	}
	return NULL;
}

static int32_t InitSessionMng_thread(void)
{
	trc("[%s: %d] InitSessionMng_thread START" , __FILE__ , __LINE__);
	int32_t a_return = OpenMQ(CMP_NO_SESSION_TRANS_ID);
	if ( ERROR_RETURN == a_return )
	{
		return ERROR_RETURN;
	}
	trc("[%s: %d] Init DONE" , __FILE__ , __LINE__);
	return NORMAL_RETURN;
}


static int32_t StartIDLE(char* e_message)
{
	trc("[%s: %d] start idle" , __FILE__ , __LINE__);
	if ( WAIT_START_IDLE != s_state )
	{
		return NORMAL_RETURN;
	}

	s_state = IDLE;
	trc("[%s: %d] Trans Manager Proc start" , __FILE__ , __LINE__);
	return NORMAL_RETURN;
}

static int32_t RecvAliveCheck(char* e_message)
{
	st_msg_res_alive_check a_msg;
	memset(&a_msg , 0 , sizeof(a_msg));

	a_msg.m_mq_header.m_commandcode = FTP_MSG_RES_ALIVE_CHECK;
	a_msg.m_result = RESULT_OK;

	int a_return = SendMQ(CMP_NO_INIT_MANAGER_ID , CMP_NO_SESSION_TRANS_ID , &a_msg , sizeof(a_msg));
	if ( ERROR_RETURN == a_return )
	{
		return ERROR_RETURN;
	};

	return NORMAL_RETURN;
}