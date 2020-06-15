///////////////////////////////////////////////////////////////////
// @file session_data_manager.h
// @date 2020/06/21
///////////////////////////////////////////////////////////////////

#include "session_cmd_manager.h"

static struct sockaddr_in	s_servSockAddr;				//server internet socket address
//static unsigned short		s_servPort;					//server port number
int							g_servSock;			//server socket descriptor
static int					s_need_end_thread_cnt = 0;

typedef enum st_session_data_state
{
	WAIT_INIT = 0x0000 ,
	WAIT_START_IDLE ,
	IDLE ,
	WAIT_END ,
	WAIT_STOPPER ,
}st_session_data_state;

static st_session_data_state s_state = WAIT_INIT;

// FUNCTION PROT
static int32_t RecvStartIDLE(char* e_message);
static int32_t RecvAliveCheck(char* e_message);
static int32_t RecvNtfWakeDoneChild(char* e_message);
static int32_t RecvReqAcceptChild(char* e_message);
static int32_t RecvNtfFinishChild(char* e_message);
static int32_t RecvReqQuitChild(char* e_message);
static int32_t RecvResResetChild(char* e_message);
static int32_t RecvReqEndSession(char* e_message);
static int32_t RecvResEndSessionChild(char* e_message);

static st_function_msg_list s_function_list [] =
{
		//	COMMAND							,FUNC
	{	FTP_MSG_NTF_START_IDLE				,RecvStartIDLE			},
	{	FTP_MSG_REQ_ALIVE_CHECK				,RecvAliveCheck			},
		//  子プロセス
	{	FTP_MSG_NTF_WAKE_DONE_CHILD			,RecvNtfWakeDoneChild	},
	{	FTP_MSG_REQ_ACCEPT_CHILD			,RecvReqAcceptChild		},
	{	FTP_MSG_NTF_FINISH_CHILD			,RecvNtfFinishChild		},
	{	FTP_MSG_REQ_QUIT_CHILD				,RecvReqQuitChild		},
	{	FTP_MSG_RES_RESET_CHILD				,RecvResResetChild		},
	{	FTP_MSG_REQ_END_ALL_SESSION			,RecvReqEndSession		},
	{	FTP_MSG_REQ_END_ALL_SESSION_CHILD	,RecvResEndSessionChild },
	{	0xFFFF								,NULL}
};

static int32_t InitSessioncmdMng_thread(void);
static int32_t SendEndSessionMsgChild(void);

void* SessionDataManager(void* argv)
{
	trc("[%s: %d] CMD SESSION START" , __FILE__ , __LINE__);

	int32_t a_return = InitSessioncmdMng_thread( );
	if ( ERROR_RETURN == a_return )
	{
		return NULL;
	}

	char a_msg [10240];

	struct timespec a_sleep;
	memset(&a_sleep , 0 , sizeof(a_sleep));
	a_sleep.tv_nsec = 500000000;

	for ( ;;)
	{
		nanosleep(&a_sleep , NULL);

		memset(&a_msg , 0 , sizeof(a_msg));
		a_return = RecvMQ(CMP_NO_SESSION_COMMAND_ID , a_msg , sizeof(a_msg));
		int a_index = 0;
		st_mq_header* a_header = ( st_mq_header*) a_msg;
		while ( 1)
		{
			if ( s_function_list [a_index].m_commandcode == a_header->m_commandcode )
			{
				a_return = s_function_list[a_index].func(a_msg);
				if ( ERROR_RETURN == a_return )
				{
					trc("[%s: %d] func error" , __FILE__ , __LINE__);
				}
				else if ( END_RETURN == a_return )
				{
					trc("[%s: %d] CMD Session Manager END" , __FILE__ , __LINE__);
					CloseMQ(CMP_NO_SESSION_COMMAND_ID , 0 , 0);
					return NULL;
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

static int32_t InitSessioncmdMng_thread(void)
{
	trc("[%s: %d] InitSessioncmdMng_threadN START" , __FILE__ , __LINE__);

	// TCPソケット生成
	g_servSock = socket(AF_INET , SOCK_STREAM , 0);

	if ( g_servSock < 0 )
	{
		trc("[%s: %d] socket error" , __FILE__ , __LINE__);
		return ERROR_RETURN;
	}

	memset(&s_servSockAddr , 0 , sizeof(s_servSockAddr));
	s_servSockAddr.sin_family = AF_INET;
	s_servSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	s_servSockAddr.sin_port = htons(60000);

	// メッセージキュー初期化
	int a_return = OpenMQ(CMP_NO_SESSION_COMMAND_ID);
	if ( ERROR_RETURN == a_return )
	{
		return ERROR_RETURN;
	}
	trc("[%s: %d] Init DONE" , __FILE__ , __LINE__);

	// ソケットにポート紐づけ
	a_return = bind(g_servSock , (struct sockaddr*) & s_servSockAddr , sizeof(s_servSockAddr));

	if ( a_return != 0 )
	{
		st_msg_ntf_wake_done a_msg_wakedone;
		a_msg_wakedone.m_mq_header.m_commandcode = FTP_MSG_NTF_WAKE_DONE;
		a_msg_wakedone.m_result = RESULT_NG;
		a_return = SendMQ(CMP_NO_INIT_MANAGER_ID , CMP_NO_SESSION_COMMAND_ID , &a_msg_wakedone , sizeof(a_msg_wakedone));
		if ( ERROR_RETURN == a_return )
		{
			return ERROR_RETURN;
		};

		trc("[%s: %d] bind error" , __FILE__ , __LINE__);
		close(g_servSock);
		return ERROR_RETURN;
	}

	st_msg_ntf_wake_done a_msg_wakedone;
	a_msg_wakedone.m_mq_header.m_commandcode = FTP_MSG_NTF_WAKE_DONE;
	a_msg_wakedone.m_result = RESULT_OK;
	a_return = SendMQ(CMP_NO_INIT_MANAGER_ID , CMP_NO_SESSION_COMMAND_ID , &a_msg_wakedone , sizeof(a_msg_wakedone));
	if ( ERROR_RETURN == a_return )
	{
		return ERROR_RETURN;
	};

	s_state = WAIT_START_IDLE;

	return NORMAL_RETURN;
}

int32_t SendEndSessionMsgChild(void)
{
	int a_session_id = GetUsedSessionID( );
	static int s_wait_end_thread;
	int a_return = 0;
	st_msg_req_end_all_session_child a_msg;
	memset(&a_msg , 0 , sizeof(a_msg));
	a_msg.m_mq_header.m_commandcode = FTP_MSG_REQ_END_ALL_SESSION_CHILD;

	trc("[%s: %d] a_session_id = %d" , __FILE__ , __LINE__, a_session_id);

	if ( 0 == a_session_id )
	{
		a_return = s_wait_end_thread;
		s_wait_end_thread = 0;
		return a_return;
	}
	else
	{
		a_return = SendMQ_CHILD(CMP_NO_CHILD , a_session_id , CHILD_CMD , &a_msg , sizeof(a_msg));
		if ( ERROR_RETURN == a_return )
		{
			return ERROR_RETURN;
		}
	}

	return SendEndSessionMsgChild( );
}

static int32_t RecvStartIDLE(char* e_message)
{
	trc("[%s: %d] start idle" , __FILE__ , __LINE__);
	if ( WAIT_START_IDLE != s_state )
	{
		return NORMAL_RETURN;
	}

	s_state = IDLE;
	trc("[%s: %d] CMD Manager Proc start" , __FILE__ , __LINE__);

	st_session_data* a_session_data= CreateSession( );

	#if 1 // 202006012 clone関数でthread生成に変更
	pthread_attr_t a_attr;
	pthread_attr_init(&a_attr);
	pthread_attr_setdetachstate(&a_attr , PTHREAD_CREATE_DETACHED);
	// コマンドスレッド起動
	int a_return = pthread_create(&a_session_data->m_command_thread_id , &a_attr , Cmdthread , (void*)&(a_session_data->m_session_id));
	#else
	int a_return = Create_Cmp(CLONE_NEWNS , Cmdthread , (void*) & (a_session_data->m_session_id));
	#endif
	if ( -1 == a_return )
	{
		trc("[%s: %d] Create_Cmp error" , __FILE__ , __LINE__);
		return ERROR_RETURN;
	}
	trc("[%s: %d] First cmd thread start" , __FILE__ , __LINE__);
	return NORMAL_RETURN;
}

static int32_t RecvAliveCheck(char* e_message)
{
	trc("[%s: %d] RecvAliveCheck" , __FILE__ , __LINE__);
	st_msg_res_alive_check a_msg;
	memset(&a_msg , 0 , sizeof(a_msg));

	a_msg.m_mq_header.m_commandcode = FTP_MSG_RES_ALIVE_CHECK;
	a_msg.m_result = RESULT_OK;

	int a_return = SendMQ(CMP_NO_INIT_MANAGER_ID , CMP_NO_SESSION_COMMAND_ID , &a_msg , sizeof(a_msg));
	if ( ERROR_RETURN == a_return )
	{
		return ERROR_RETURN;
	};
	trc("[%s: %d] RecvAliveCheck end" , __FILE__ , __LINE__);
	return NORMAL_RETURN;
}

static int32_t RecvNtfWakeDoneChild(char* e_message)
{
	st_msg_ntf_wake_done_child* a_recv_msg = ( st_msg_ntf_wake_done_child*) e_message;

	if ( RESULT_NG != a_recv_msg->m_result )
	{
		st_msg_ntf_start_idle_child a_msg;
		memset(&a_msg , 0 , sizeof(a_msg));

		//セッションID設定
		a_msg.m_mq_header.m_session_id = a_recv_msg->m_mq_header.m_session_id;
		a_msg.m_mq_header.m_commandcode = FTP_MSG_NTF_START_IDLE_CHILD;

		int a_return = SendMQ_CHILD(CMP_NO_CHILD, a_recv_msg->m_mq_header.m_session_id, CHILD_CMD , &a_msg , sizeof(a_msg));
		if ( ERROR_RETURN == a_return )
		{
			return ERROR_RETURN;
		}
	}
	else
	{
		st_msg_res_reset_child a_msg;
		memset(&a_msg , 0 , sizeof(a_msg));
		//セッションID設定
		a_msg.m_mq_header.m_session_id = a_recv_msg->m_mq_header.m_session_id;
		a_msg.m_mq_header.m_commandcode = FTP_MSG_REQ_RESET_CHILD;

		int a_return = SendMQ_CHILD(CMP_NO_CHILD , a_recv_msg->m_mq_header.m_session_id , CHILD_CMD , &a_msg , sizeof(a_msg));
		if ( ERROR_RETURN == a_return )
		{
			return ERROR_RETURN;
		}
	}

	return NORMAL_RETURN;
}

static int32_t RecvReqAcceptChild(char* e_message)
{
	st_msg_req_accept_done_child* a_recv_msg = ( st_msg_req_accept_done_child*) e_message;
	st_msg_res_accept_done_child a_msg;
	memset(&a_msg , 0 , sizeof(a_msg));

	// セッションID設定
	a_msg.m_mq_header.m_session_id = a_recv_msg->m_mq_header.m_session_id;
	a_msg.m_mq_header.m_commandcode = FTP_MSG_RES_ACCEPT_CHILD;

	int a_return = SendMQ_CHILD(CMP_NO_CHILD , a_recv_msg->m_mq_header.m_session_id , CHILD_CMD , &a_msg , sizeof(a_msg));
	if ( ERROR_RETURN == a_return )
	{
		return ERROR_RETURN;
	}

	st_session_data* a_session_data = CreateSession( );

	#if 1 // 202006012 clone関数でthread生成に変更
	pthread_attr_t a_attr;
	pthread_attr_init(&a_attr);
	pthread_attr_setdetachstate(&a_attr , PTHREAD_CREATE_DETACHED);
	// コマンドスレッド起動
	a_return = pthread_create(&a_session_data->m_command_thread_id , &a_attr , Cmdthread , (void*) & (a_session_data->m_session_id));
	if ( 0 != a_return )
	{
		return ERROR_RETURN;
	}
	#else
	a_return = Create_Cmp(CLONE_NEWNS , &Cmdthread , (void*) & (a_session_data->m_session_id));
	if ( -1 == a_return )
	{
		trc("[%s: %d] Create thread fail" , __FILE__ , __LINE__);
	}
	else
	{
		a_session_data->m_command_thread_id = a_return;
	}
	#endif
	trc("[%s: %d] Next cmd thread start" , __FILE__ , __LINE__);

	return NORMAL_RETURN;
}

static int32_t RecvReqQuitChild(char* e_message)
{
	st_msg_req_recvquit_child* a_recv_msg = ( st_msg_req_recvquit_child*) e_message;
	st_msg_res_recvquit_child a_msg;
	memset(&a_msg , 0 , sizeof(a_msg));

	// セッションID設定
	a_msg.m_mq_header.m_session_id = a_recv_msg->m_mq_header.m_session_id;
	a_msg.m_mq_header.m_commandcode = FTP_MSG_RES_QUIT_CHILD;

	int a_return = SendMQ_CHILD(CMP_NO_CHILD , a_recv_msg->m_mq_header.m_session_id , CHILD_CMD , &a_msg , sizeof(a_msg));
	if ( ERROR_RETURN == a_return )
	{
		return ERROR_RETURN;
	};

	return NORMAL_RETURN;
}

static int32_t RecvNtfFinishChild(char* e_message)
{
	st_msg_req_recvquit_child* a_recv_msg = ( st_msg_req_recvquit_child*) e_message;
	st_msg_res_recvquit_child a_msg;
	memset(&a_msg , 0 , sizeof(a_msg));

	int a_return = ClearSession(a_recv_msg->m_mq_header.m_session_id , CHILD_CMD);
	if ( ERROR_RETURN == a_return )
	{
		return ERROR_RETURN;
	};

	return NORMAL_RETURN;
}

static int32_t RecvResResetChild(char* e_message)
{
	st_msg_res_reset_child* a_msg = ( st_msg_res_reset_child*) e_message;

	struct timespec a_sleep;
	memset(&a_sleep , 0 , sizeof(a_sleep));
	a_sleep.tv_nsec = 500000000;
	
	nanosleep(&a_sleep , NULL);

	st_session_data* a_session_ptr = SearchSession(a_msg->m_mq_header.m_session_id);

	// コマンドスレッド起動
	#if 0
	pthread_attr_init(&a_session_ptr->m_command_attr);
	pthread_attr_setdetachstate(&a_session_ptr->m_command_attr , PTHREAD_CREATE_DETACHED);

	int a_return = pthread_create(&a_session_ptr->m_command_thread_id , &a_session_ptr->m_command_attr , Cmdthread , (void*) & (a_session_ptr->m_session_id));
	#endif
	int a_return = Create_Cmp(CLONE_NEWNS , &Cmdthread , ((void*) & (a_session_ptr->m_session_id)));
	if ( -1 == a_return )
	{
		return ERROR_RETURN;
	}
	trc("[%s: %d] Reset cmd thread start" , __FILE__ , __LINE__);

	return NORMAL_RETURN;
}

int32_t RecvReqEndSession(char* e_message)
{
	s_need_end_thread_cnt =  SendEndSessionMsgChild( );

	if ( ERROR_RETURN == s_need_end_thread_cnt )
	{
		return END_RETURN;
	}

	return NORMAL_RETURN;
}

int32_t RecvResEndSessionChild(char* e_message)
{
	static int s_end_thread = 0;

	s_end_thread++;

	if ( s_end_thread == s_need_end_thread_cnt )
	{
		st_msg_res_end_all_session a_msg;
		memset(&a_msg , 0 , sizeof(a_msg));
		a_msg.m_mq_header.m_commandcode = FTP_MSG_RES_END_ALL_SESSION;

		SendMQ(CMP_NO_INIT_MANAGER_ID , CMP_NO_SESSION_COMMAND_ID , &a_msg , sizeof(a_msg));
	}
	return END_RETURN;
}
