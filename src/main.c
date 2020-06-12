///////////////////////////////////////////////////////////////////
// @file main.c
// @date 2020/06/21
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
// TODO;
//			TLSv1.0 - TLSv1.3の対応（SSL)
//


#include "main.h"

/// Prottype
static int32_t StartFtpServer(void);
static int32_t InitFtpServer(void);

// #### メッセージ送信関数
static int32_t SendIdleStart(void);
static int32_t SendAliveCheck(void);

// #### 分割関数
static int32_t CheckAlive(void);
/// Global
static pthread_t			s_session_command_manager = 0;
static pthread_t			s_session_Trans_manager = 0;
static int32_t				s_AliveCheckSuccess = 1;
static int32_t				s_end_flag = 0;

typedef enum st_manager_state
{
	WAIT_INIT = 0x0000 ,
	WAIT_WAKEDONE,
	IDLE ,
	WAIT_END ,
	WAIT_STOPPER ,
}st_manager_state;

static st_manager_state s_state = WAIT_INIT;

// #### メッセージ受信関数
static int32_t RecvAiveCheck(char* e_message);
static int32_t RecvWakeDone(char* e_message);
static int32_t RecvEndAllSession(char* e_message);


static st_function_msg_list s_function_list [] =
{
	//	COMMAND						,FUNC
{	FTP_MSG_NTF_WAKE_DONE			,RecvWakeDone		}, // 起動完了通知
{	FTP_MSG_RES_ALIVE_CHECK			,RecvAiveCheck		}, // アライブチェック応答
{	FTP_MSG_RES_END_ALL_SESSION		,RecvEndAllSession	}, // 全セッション終了応答
{	0xFFFF							,NULL				}
};


// main
int main(int argc , char** argv) // 1引数
{
	InitUtilitis( );
	
	StartFtpServer( );

	int a_return = FreeSession( );
	if ( NORMAL_RETURN != a_return )
	{
		trc("[%s: %d] FreeSession fail" , __FILE__ , __LINE__);
	}

	CloseUtilities( );
	return 0;
}

// FTPサーバー始動
static int32_t StartFtpServer(void)
{
	int a_index = 0;
	char a_msg [10240];

	trc("[%s: %d] start ftpserver" , __FILE__ , __LINE__);

	int a_return = InitFtpServer( );
	if ( a_return != NORMAL_RETURN )
	{
		return ERROR_RETURN;
	}

	trc("[%s: %d] Init Done" , __FILE__ , __LINE__);
	// sleepタイム
	struct timespec s_sleep;
	memset(&s_sleep , 0 , sizeof(s_sleep));
	s_sleep.tv_nsec = 100000000;

	for ( ;; )
	{
		// sleep 100 ms
		nanosleep(&s_sleep , NULL);

		if ( 1 == s_end_flag )
		{
			// 子プロセス全セッション終了要求送信
			st_msg_req_end_all_session a_end_msg;
			memset(&a_msg , 0 , sizeof(a_msg));
			a_end_msg.m_mq_header.m_commandcode = FTP_MSG_REQ_END_ALL_SESSION_CHILD;
			s_state = IDLE;
			int a_return = SendMQ(CMP_NO_SESSION_COMMAND_ID , CMP_NO_INIT_MANAGER_ID , &a_end_msg , sizeof(a_end_msg));
			if ( ERROR_RETURN == a_return )
			{
				return ERROR_RETURN;
			}
			a_return = SendMQ(CMP_NO_SESSION_TRANS_ID , CMP_NO_INIT_MANAGER_ID , &a_end_msg , sizeof(a_end_msg));
			if ( ERROR_RETURN == a_return )
			{
				return ERROR_RETURN;
			}

			s_end_flag = 0;
		}

		memset(a_msg , 0 , sizeof(a_msg));
		a_return = RecvMQtimeout(CMP_NO_INIT_MANAGER_ID , a_msg , sizeof(a_msg),500000000);

		CheckAlive( );

		if ( TIMEOUT_RETURN == a_return )
		{
			continue;
		}

		a_index = 0;
		st_mq_header* a_header = ( st_mq_header*) a_msg;
		while ( 1 )
		{
			if ( s_function_list [a_index].m_commandcode == a_header->m_commandcode )
			{
				a_return = s_function_list[a_index].func(a_msg);
				if ( ERROR_RETURN == a_return )
				{
					trc("[%s: %d] func error" , __FILE__ , __LINE__);
					return 0;
				}
				if ( END_RETURN == a_return )
				{
					trc("[%s: %d] main ended" , __FILE__ , __LINE__);
					return 0;
				}
				break;
			}
			else if ( s_function_list [a_index].m_commandcode == 0xFFFF )
			{
				trc("[%s: %d] command unknown" , __FILE__ , __LINE__);
				break;
			}
			a_index++;
		}

	}
	return NORMAL_RETURN;
}

// FTPサーバー初期化
static int32_t InitFtpServer(void)
{
	/*
	*	コントロール	== "60000"
	*	データ			== "60001" - "60300" 
	*
	*/

	// サーバーポート設定
	int  a_return = 0;

	a_return = OpenMQ(CMP_NO_INIT_MANAGER_ID);
	if ( ERROR_RETURN == a_return )
	{
		return ERROR_RETURN;
	}
;	
	a_return = InitSession( );
	if ( NORMAL_RETURN != a_return )
	{
		trc("[%s: %d] InitSession fail" , __FILE__ , __LINE__);
	}

	pthread_attr_t	a_thread_attr;

	memset(&s_session_command_manager , 0 , sizeof(s_session_command_manager));
	memset(&s_session_Trans_manager , 0 , sizeof(s_session_Trans_manager));

	pthread_attr_init(&a_thread_attr);
	pthread_attr_setdetachstate(&a_thread_attr , PTHREAD_CREATE_DETACHED);

	// コマンドセッションマネージャースレッド起動
	pthread_create(&s_session_command_manager , &a_thread_attr , SessionDataManager , NULL);
	
	// トランスセッションマネージャースレッド起動
	pthread_create(&s_session_Trans_manager , &a_thread_attr , session_mng_thread , NULL);

	s_state = WAIT_WAKEDONE;

	return NORMAL_RETURN;
}


// #### 受信メッセージ関数

static int32_t RecvWakeDone(char* e_message)
{
	trc("[%s: %d] RecvWakeDone" , __FILE__ , __LINE__);

	if ( WAIT_WAKEDONE != s_state )
	{
		return NORMAL_RETURN;
	}
	static int s_command_thread_done = 0;
	static int s_data_thread_done = 0;

	st_msg_ntf_wake_done* a_msg = (st_msg_ntf_wake_done*) e_message;

	if ( a_msg->m_result == RESULT_NG )
	{
		return ERROR_RETURN;
	}

	switch ( a_msg->m_mq_header.m_src_id )
	{
		case CMP_NO_SESSION_COMMAND_ID:
			trc("[%s: %d] RecvWakeDone CMP_NO_SESSION_COMMAND_ID" , __FILE__ , __LINE__);
			s_command_thread_done = 1;
			break;

		case CMP_NO_SESSION_TRANS_ID:
			trc("[%s: %d] RecvWakeDone CMP_NO_SESSION_TRANS_ID" , __FILE__ , __LINE__);
			s_data_thread_done = 1;
			break;

		default:
			trc("[%s: %d] RecvWakeDone unknown" , __FILE__ , __LINE__);
			break;
	}

	if ( 1 == s_command_thread_done &&
		1 == s_data_thread_done )
	{
		int a_return = SendIdleStart( );
		if ( ERROR_RETURN == a_return )
		{
			return ERROR_RETURN;
		}
	}
	trc("[%s: %d] Manager Proc start" , __FILE__ , __LINE__);
	trc("[%s: %d] RecvWakeDone END" , __FILE__ , __LINE__);
	return NORMAL_RETURN;
}

int32_t RecvEndAllSession(char* e_message)
{
	static int s_command_thread_done = 0;
	static int s_data_thread_done = 0;

	st_msg_ntf_wake_done* a_msg = ( st_msg_ntf_wake_done*) e_message;

	if ( a_msg->m_result == RESULT_NG )
	{
		return ERROR_RETURN;
	}

	switch ( a_msg->m_mq_header.m_src_id )
	{
		case CMP_NO_SESSION_COMMAND_ID:
			trc("[%s: %d] RecvEndAllSession CMP_NO_SESSION_COMMAND_ID" , __FILE__ , __LINE__);
			s_command_thread_done = 1;
			break;

		case CMP_NO_SESSION_TRANS_ID:
			trc("[%s: %d] RecvEndAllSession CMP_NO_SESSION_TRANS_ID" , __FILE__ , __LINE__);
			s_data_thread_done = 1;
			break;

		default:
			trc("[%s: %d] RecvEndAllSession unknown" , __FILE__ , __LINE__);
			break;
	}

	if ( 1 == s_command_thread_done &&
		1 == s_data_thread_done )
	{
		return END_RETURN;
	}

	return NORMAL_RETURN;
}

static int32_t  RecvAiveCheck(char* e_message)
{
	st_msg_res_alive_check* a_msg = ( st_msg_res_alive_check*) e_message;

	static int s_command_thread_done = 0;
	static int s_data_thread_done = 0;

	switch ( a_msg->m_mq_header.m_src_id )
	{
		case CMP_NO_SESSION_COMMAND_ID:
			if ( RESULT_OK == a_msg->m_result )
			{
				s_command_thread_done = 1;
			}
			break;
		case CMP_NO_SESSION_TRANS_ID:
			if ( RESULT_OK == a_msg->m_result )
			{
				s_data_thread_done = 1;
			}
			break;

		default:
			trc("[%s: %d] RecvAiveCheck unknown" , __FILE__ , __LINE__);
			break;
	}

	if ( 1 == s_command_thread_done &&
		1 == s_data_thread_done )
	{
		trc("[%s: %d] ALL ALIVE" , __FILE__ , __LINE__);
		s_AliveCheckSuccess = 1;
	}

	return NORMAL_RETURN;
}

// #### メッセージ送信関数

// 運用開始通知
static int32_t SendIdleStart(void)
{
	st_msg_ntf_start_idle a_msg;
	memset(&a_msg , 0 , sizeof(a_msg));
	a_msg.m_mq_header.m_commandcode = FTP_MSG_NTF_START_IDLE;
	s_state = IDLE;
	int a_return = SendMQ(CMP_NO_SESSION_COMMAND_ID , CMP_NO_INIT_MANAGER_ID , &a_msg , sizeof(a_msg));
	if ( ERROR_RETURN == a_return )
	{
		return ERROR_RETURN;
	}
	a_return = SendMQ(CMP_NO_SESSION_TRANS_ID , CMP_NO_INIT_MANAGER_ID , &a_msg , sizeof(a_msg));
	if ( ERROR_RETURN == a_return )
	{
		return ERROR_RETURN;
	}

	return NORMAL_RETURN;
}

// アライブチェック要求
static int32_t SendAliveCheck(void)
{
	st_msg_req_alive_check a_msg;
	memset(&a_msg , 0 , sizeof(a_msg));
	a_msg.m_mq_header.m_commandcode = FTP_MSG_REQ_ALIVE_CHECK;

	int a_return = SendMQ(CMP_NO_SESSION_COMMAND_ID , CMP_NO_INIT_MANAGER_ID , &a_msg , sizeof(a_msg));
	if ( ERROR_RETURN == a_return )
	{
		return ERROR_RETURN;
	}
	a_return = SendMQ(CMP_NO_SESSION_TRANS_ID , CMP_NO_INIT_MANAGER_ID , &a_msg , sizeof(a_msg));
	if ( ERROR_RETURN == a_return )
	{
		return ERROR_RETURN;
	}

	return NORMAL_RETURN;
}


// #### 分割関数

// アライブチェック
static int32_t CheckAlive(void)
{
	static int32_t s_cnt = 0;
	if ( IDLE < s_state )
	{
		return NORMAL_RETURN;
	}

	s_cnt++;
	// 100ms * 6000 = 600000ms = 600sec = 10min にアライブチェックを実行する
	if ( 6000 <= s_cnt )
	{
		if ( 1 == s_AliveCheckSuccess )
		{
			SendAliveCheck( );
			s_AliveCheckSuccess = 0;
			s_cnt = 0;
		}
		else
		{
			return ERROR_RETURN;
		}
	}

	return NORMAL_RETURN;
}

void* SignalHandler(void* argv)
{
	static sigset_t s_sigset;
	sigemptyset(&s_sigset);
	sigaddset(&s_sigset , SIGUSR1);

	int a_signal = 0;

	while ( 0 == sigwait(&s_sigset , &a_signal))
	{
		switch ( a_signal )
		{
			case SIGUSR1:
				s_end_flag = 1;
				break;

			default:
				break;
		}

		if ( 1 == s_end_flag )
		{
			break;
		}
		a_signal = 0;
	}

	return NULL;
}

