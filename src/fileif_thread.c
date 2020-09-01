///////////////////////////////////////////////////////////////////
// @file trans_thread.c
// @date 2020/06/21
///////////////////////////////////////////////////////////////////

#include "trans_thread.h"
#include "fileif_thread.h"

extern __thread st_session_data* g_my_session_ptr;
static __thread char* s_data_buf;

static int32_t InitFileifThread(int e_session_id);
static int32_t CreateList(char* e_data_ptr);

static int32_t NtfStartIdleFileif(char* e_message);
static int32_t RecvRead(char* e_message);
static int32_t RecvWrite(char* e_message);
static int32_t RecvEndThread(char* e_message);

static st_function_msg_list s_function_list [] =
{
	//	COMMAND								,FUNC
	{	FTP_MSG_NTF_START_IDLE_FILEIF		,NtfStartIdleFileif			},
	{	FTP_MSG_REQ_FILE_READ_CHILD			,RecvRead					},
	{	FTP_MSG_REQ_FILE_WRITE_CHILD		,RecvWrite					},
	{	FTP_MSG_REQ_END_THREAD_FILEIF		,RecvEndThread				},
	{	0xFFFF								,NULL						}
};

void* FileifThread(void* argv)
{
	char a_msg [10240];
	static __thread int a_epoll_serv_fd;

	struct timespec a_sleep;
	memset(&a_sleep , 0 , sizeof(a_sleep));
	a_sleep.tv_nsec = 500000000;

	int a_session_id = *(( int*) argv);
	trc("[%s: %d][session: %d] Trans thread start" , __FILE__ , __LINE__ , a_session_id);
	InitFileifThread(a_session_id);

	while ( 1 )
	{
		// メッセージ受信
		//------------------------------------------------------------------------------------------------------------
		memset(&a_msg , 0 , sizeof(a_msg));
		int a_return = RecvMQtimeout(CMP_NO_SESSION_COMMAND_ID , a_msg , sizeof(a_msg) , 500000000);
		if ( NORMAL_RETURN == a_return )
		{
			int a_index = 0;
			st_mq_header* a_header = ( st_mq_header*) a_msg;
			trc("[%s: %d][session: %d]  recv = %08x" , __FILE__ , __LINE__ , g_my_session_ptr->m_session_id , a_header->m_commandcode);
			while ( 1 )
			{
				if ( s_function_list [a_index].m_commandcode == a_header->m_commandcode )
				{
					a_return = s_function_list [a_index].func(a_msg);
					if ( ERROR_RETURN == a_return )
					{
						trc("[%s: %d][session: %d]  func error" , __FILE__ , __LINE__ , g_my_session_ptr->m_session_id);
					}

					// 終了処理
					if ( END_RETURN == a_return )
					{
						// 終了通知送信
						st_msg_ntf_finish_child a_send_msg;
						memset(&a_send_msg , 0 , sizeof(a_send_msg));
						a_send_msg.m_mq_header.m_commandcode = FTP_MSG_NTF_FINISH_CHILD;

						a_return = SendMQ_CHILD(CMP_NO_SESSION_COMMAND_ID , g_my_session_ptr->m_session_id , CHILD_CMD , &a_send_msg , sizeof(a_send_msg));
						return NULL;
					}
					// リセットの場合、同じセッションでスレッドを再立ち上げしているので終了通知を送らない
					else if ( RESET_RETURN == a_return )
					{

						CloseMQ(CMP_NO_CHILD , g_my_session_ptr->m_session_id , CHILD_TRANS);
						return NULL;
					}
					break;
				}
				else if ( s_function_list [a_index].m_commandcode == 0xFFFF )
				{
					trc("[%s: %d][session: %d]  MSG unknown" , __FILE__ , __LINE__ , g_my_session_ptr->m_session_id);
					break;
				}
				a_index++;
			}
		}
		//------------------------------------------------------------------------------------------------------------


		switch ( g_my_session_ptr->m_Fileif_state )
		{
			case CHILD_FILEIF_STATE_TRANSING_LIST:
			case CHILD_FILEIF_STATE_TRANSING_NLIST:
			case CHILD_FILEIF_STATE_TRANSING:
			case CHILD_FILEIF_STATE_RECIVING:
			case CHILD_FILEIF_STATE_WAIT_END:
			case CHILD_FILEIF_STATE_WAIT_MSG:
			default:
				nanosleep(&a_sleep , NULL);
				continue;
		}
	}
}

static int32_t InitFileifThread(int e_session_id)
{
	int32_t a_return = OpenMQ_CHILD(g_my_session_ptr->m_session_id , CHILD_TRANS);
	if ( ERROR_RETURN == a_return )
	{
		return ERROR_RETURN;
	}

	st_msg_ntf_wake_done_fileif a_msg;
	memset(&a_msg , 0 , sizeof(a_msg));

	a_msg.m_mq_header.m_commandcode = FTP_MSG_NTF_WAKE_DONE_FILEIF;
	a_msg.m_result = RESULT_OK;

	return NORMAL_RETURN;
	int a_return = SendMQ_CHILD(CMP_NO_CHILD , g_my_session_ptr->m_session_id , CHILD_TRANS , &a_msg , sizeof(a_msg));
	if ( ERROR_RETURN == a_return )
	{
		return ERROR_RETURN;
	}

	s_data_buf = malloc(1 * 1000 * 1000);
	if ( NULL == s_data_buf )
	{
		return ERROR_RETURN;
	}

	return NORMAL_RETURN;
}

#define NOT_END_LIST
#define END_LIST

static int32_t CreateList(char* e_data_ptr)
{
	char a_path [PATH_MAX];
	memset(a_path , 0 , sizeof(char));
	realpath(g_my_session_ptr->m_use_filepath , a_path);
	nftw(a_path , CreateBlock , 1 , FTW_ACTIONRETVAL);

	return NORMAL_RETURN;
}

int CreateBlock(const char* e_filename , const struct stat* e_status , int e_flag , struct FTW* e_info)
{
	static s_data_len = 0;
	char a_temp_buf [PATH_MAX];



}

static int32_t NtfStartIdleFileif(char*  e_message)
{
	g_my_session_ptr->m_Fileif_state = CHILD_FILEIF_STATE_WAIT_MSG;
	return NORMAL_RETURN;
}

static int32_t RecvRead(char* e_message)
{
	g_my_session_ptr->m_Fileif_state = CHILD_FILEIF_STATE_TRANSING;
	return NORMAL_RETURN;
}

static int32_t RecvWrite(char* e_message)
{
	g_my_session_ptr->m_Fileif_state = CHILD_FILEIF_STATE_RECIVING;
	return NORMAL_RETURN;
}

static int32_t RecvEndThread(char* e_message)
{
	g_my_session_ptr->m_Fileif_state = CHILD_FILEIF_STATE_WAIT_END;
	return NORMAL_RETURN;
}
