///////////////////////////////////////////////////////////////////
// @file trans_thread.c
// @date 2020/06/21
///////////////////////////////////////////////////////////////////

#include "trans_thread.h"
#include "fileif_thread.h"

extern __thread st_session_data* g_my_session_ptr;

static int32_t InitTransThread(int e_session_id);


static __thread int					s_data_sock = 0;
static __thread int					s_data_cliant_sock = 0;
static __thread struct sockaddr_in	s_dataSockAddr;				//data internet socket address

static int32_t ReqConnectPort(char* e_message);
static int32_t NtfWakeDoneFileif(char* e_message);
static int32_t NtfStartIdleTrans(char* e_message);

static st_function_msg_list s_function_list [] =
{
		//	COMMAND						,FUNC
	{	FTP_MSG_RES_CONNECT_PORT_CHILD		,ReqConnectPort				},
	{	FTP_MSG_NTF_WAKE_DONE_FILEIF		,NtfWakeDoneFileif			},
	{	FTP_MSG_NTF_START_IDLE_TRANS		,NtfStartIdleTrans			},
	{	0xFFFF								,NULL						}
};

void* TransThread(void* argv)
{
	char a_msg [10240];
	static __thread int a_epoll_serv_fd;

	struct timespec a_sleep;
	memset(&a_sleep , 0 , sizeof(a_sleep));
	a_sleep.tv_nsec = 500000000;

	int a_session_id = *((int*) argv);
	trc("[%s: %d][session: %d] Trans thread start" , __FILE__ , __LINE__ , a_session_id);
	InitTransThread(a_session_id);

	// epoll用のfd生成
	static __thread struct epoll_event s_set_event;
	memset(&s_set_event , 0 , sizeof(s_set_event));
	s_set_event.events = EPOLLIN;



	// クライアントソケット
	static __thread  int s_epoll_cliant_fd;


	while ( 1 )
	{
		// メッセージ受信
		//------------------------------------------------------------------------------------------------------------
		memset(&a_msg , 0 , sizeof(a_msg));
		int a_return = RecvMQtimeout(CMP_NO_SESSION_COMMAND_ID , a_msg , sizeof(a_msg) , 500000000);
		if ( NORMAL_RETURN == a_return )
		{
			int a_index = 0;
			st_mq_header* a_header = (st_mq_header*) a_msg;
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
						trc("[%s: %d][session: %d]  END_RETURN start" , __FILE__ , __LINE__ , g_my_session_ptr->m_session_id);
						close(s_epoll_cliant_fd);
						close(a_epoll_serv_fd);
						shutdown(s_data_cliant_sock , SHUT_RDWR);
						close(s_data_sock);
						close(s_data_cliant_sock);
						CloseMQ(CMP_NO_CHILD , g_my_session_ptr->m_session_id , CHILD_TRANS);

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
						close(s_epoll_cliant_fd);
						close(a_epoll_serv_fd);
						shutdown(s_data_cliant_sock , SHUT_RDWR);
						close(s_data_sock);
						close(s_data_cliant_sock);
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
		struct epoll_event a_event;
		memset(&a_event , 0 , sizeof(a_event));


		switch ( g_my_session_ptr->m_Trans_cmd_state )
		{
			case CHILD_TRANS_STATE_WAIT_MSG:
				nanosleep(&a_sleep , NULL);
				continue;

			case CHILD_TRANS_STATE_CONNECT:

				// TCPソケット生成
				s_data_sock = socket(AF_INET , SOCK_STREAM , 0);

				memset(&s_dataSockAddr , 0 , sizeof(s_dataSockAddr));
				s_dataSockAddr.sin_family = AF_INET;
				s_dataSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

				switch ( g_my_session_ptr->m_session_flags.m_pasv_flags )
				{
					case PASV_ON:
						s_dataSockAddr.sin_port = 0;
						break;

					case PASV_PORT:
						s_dataSockAddr.sin_port = htons(g_my_session_ptr->m_session_flags.m_cliant_port);
						break;
				}

				// ソケットにポート紐づけ
				a_return = bind(s_data_sock , (struct sockaddr*) & s_data_sock , sizeof(s_data_sock));
				if ( a_return != 0 )
				{
					trc("[%s: %d] bind error" , __FILE__ , __LINE__);
					close(s_data_sock);
					st_msg_res_connect_port_child a_port_res_msg;
					memset(&a_port_res_msg , 0 , sizeof(a_port_res_msg));
					a_port_res_msg.m_mq_header.m_commandcode = FTP_MSG_RES_CONNECT_PORT_CHILD;
					a_port_res_msg.m_result = RESULT_NG;
					a_return = SendMQ_CHILD(CMP_NO_SESSION_COMMAND_ID , g_my_session_ptr->m_session_id , CHILD_CMD , &a_port_res_msg , sizeof(a_port_res_msg));
					g_my_session_ptr->m_Trans_cmd_state = CHILD_TRANS_STATE_WAIT_MSG;
					continue;
				}

				int a_return = listen(s_data_sock , MAXQUEUE);

				if ( 0 != a_return )
				{
					st_msg_res_connect_port_child a_port_res_msg;
					memset(&a_port_res_msg , 0 , sizeof(a_port_res_msg));
					a_port_res_msg.m_mq_header.m_commandcode = FTP_MSG_RES_CONNECT_PORT_CHILD;
					a_port_res_msg.m_result = RESULT_NG;
					a_return = SendMQ_CHILD(CMP_NO_SESSION_COMMAND_ID , g_my_session_ptr->m_session_id , CHILD_CMD , &a_port_res_msg , sizeof(a_port_res_msg));
					g_my_session_ptr->m_Trans_cmd_state = CHILD_TRANS_STATE_WAIT_MSG;
					continue;
				}


				g_my_session_ptr->m_port = ntohs(s_dataSockAddr.sin_port);

				a_epoll_serv_fd = epoll_create1(0);
				a_return = epoll_ctl(a_epoll_serv_fd , EPOLL_CTL_ADD , s_data_sock , &s_set_event);
				if ( -1 == a_return )
				{
					st_msg_res_connect_port_child a_port_res_msg;
					memset(&a_port_res_msg , 0 , sizeof(a_port_res_msg));
					a_port_res_msg.m_mq_header.m_commandcode = FTP_MSG_RES_CONNECT_PORT_CHILD;
					a_port_res_msg.m_result = RESULT_NG;
					a_return = SendMQ_CHILD(CMP_NO_SESSION_COMMAND_ID , g_my_session_ptr->m_session_id , CHILD_CMD , &a_port_res_msg , sizeof(a_port_res_msg));
					g_my_session_ptr->m_Trans_cmd_state = CHILD_TRANS_STATE_WAIT_MSG;
					continue;
				}

				a_return = epoll_wait(a_epoll_serv_fd , &a_event , 256 , 500);
				if ( 0 == a_return )
				{
					st_msg_res_connect_port_child a_port_res_msg;
					memset(&a_port_res_msg , 0 , sizeof(a_port_res_msg));
					a_port_res_msg.m_mq_header.m_commandcode	= FTP_MSG_RES_CONNECT_PORT_CHILD;
					a_port_res_msg.m_result						= RESULT_NG;
					a_return = SendMQ_CHILD(CMP_NO_SESSION_COMMAND_ID , g_my_session_ptr->m_session_id , CHILD_CMD , &a_port_res_msg , sizeof(a_port_res_msg));
					g_my_session_ptr->m_Trans_cmd_state			= CHILD_TRANS_STATE_WAIT_MSG;
					continue;
				}
				else if ( 0 > a_return )
				{
					st_msg_res_connect_port_child a_port_res_msg;
					memset(&a_port_res_msg , 0 , sizeof(a_port_res_msg));
					a_port_res_msg.m_mq_header.m_commandcode = FTP_MSG_RES_CONNECT_PORT_CHILD;
					a_port_res_msg.m_result = RESULT_NG;
					a_return = SendMQ_CHILD(CMP_NO_SESSION_COMMAND_ID , g_my_session_ptr->m_session_id , CHILD_CMD , &a_port_res_msg , sizeof(a_port_res_msg));
					g_my_session_ptr->m_Trans_cmd_state = CHILD_TRANS_STATE_WAIT_MSG;
					continue;
				}


				s_data_cliant_sock = accept(s_data_sock , NULL , NULL);

				st_msg_res_connect_port_child a_port_res_msg;
				memset(&a_port_res_msg , 0 , sizeof(a_port_res_msg));
				a_port_res_msg.m_mq_header.m_commandcode = FTP_MSG_RES_CONNECT_PORT_CHILD;
				a_port_res_msg.m_result = RESULT_OK;

				s_epoll_cliant_fd = epoll_create1(0);
				a_return = epoll_ctl(s_epoll_cliant_fd , EPOLL_CTL_ADD , s_data_cliant_sock , &s_set_event);
				if ( -1 == a_return )
				{
					st_msg_res_connect_port_child a_port_res_msg;
					memset(&a_port_res_msg , 0 , sizeof(a_port_res_msg));
					a_port_res_msg.m_mq_header.m_commandcode = FTP_MSG_RES_CONNECT_PORT_CHILD;
					a_port_res_msg.m_result = RESULT_NG;
					a_return = SendMQ_CHILD(CMP_NO_SESSION_COMMAND_ID , g_my_session_ptr->m_session_id , CHILD_CMD , &a_port_res_msg , sizeof(a_port_res_msg));
					g_my_session_ptr->m_Trans_cmd_state = CHILD_TRANS_STATE_WAIT_MSG;
					return NULL;
				}

				a_return = SendMQ_CHILD(CMP_NO_SESSION_COMMAND_ID , g_my_session_ptr->m_session_id , CHILD_CMD , &a_port_res_msg , sizeof(a_port_res_msg));
				if (ERROR_RETURN == a_return )
				{
					st_msg_res_connect_port_child a_port_res_msg;
					memset(&a_port_res_msg , 0 , sizeof(a_port_res_msg));
					a_port_res_msg.m_mq_header.m_commandcode = FTP_MSG_RES_CONNECT_PORT_CHILD;
					a_port_res_msg.m_result = RESULT_NG;
					a_return = SendMQ_CHILD(CMP_NO_SESSION_COMMAND_ID , g_my_session_ptr->m_session_id , CHILD_CMD , &a_port_res_msg , sizeof(a_port_res_msg));
					g_my_session_ptr->m_Trans_cmd_state = CHILD_TRANS_STATE_WAIT_MSG;
					return NULL;
				}

				g_my_session_ptr->m_Trans_cmd_state = CHILD_TRANS_STATE_WAIT_MSG;
				break;

				case CHILD_TRANS_STATE_CHECK_CMD:
				switch ( g_my_session_ptr->m_session_command )
				{ 
					case CMD_STOR:
						g_my_session_ptr->m_Trans_cmd_state = CHILD_TRANS_STATE_RECVING;
					case CMD_RETR:
						g_my_session_ptr->m_Trans_cmd_state = CHILD_TRANS_STATE_WAIT_PUSH;
					case CMD_NLIST:
						g_my_session_ptr->m_Trans_cmd_state = CHILD_TRANS_STATE_WAIT_PUSH;
					case CMD_LIST:
						g_my_session_ptr->m_Trans_cmd_state = CHILD_TRANS_STATE_WAIT_PUSH;
				}
				break;

			case CHILD_TRANS_STATE_WAIT_END:
				close(s_epoll_cliant_fd);
				close(a_epoll_serv_fd);
				shutdown(s_data_cliant_sock , SHUT_RDWR);
				close(s_data_sock);
				close(s_data_cliant_sock);
				break;
			default:
				nanosleep(&a_sleep , NULL);
				continue;
		}
	}
}

static int32_t InitTransThread(int e_session_id)
{
	g_my_session_ptr = SearchSession(e_session_id);
	if ( NULL == g_my_session_ptr )
	{
		return ERROR_RETURN;
	}

	int32_t a_return = OpenMQ_CHILD(g_my_session_ptr->m_session_id , CHILD_TRANS);
	if ( ERROR_RETURN == a_return )
	{
		return ERROR_RETURN;
	}

	pthread_attr_t a_attr;
	pthread_attr_init(&a_attr);
	pthread_attr_setdetachstate(&a_attr , PTHREAD_CREATE_DETACHED);
	// FILEIFスレッド起動
	int a_return = pthread_create(&g_my_session_ptr->m_fileif_thread_id , &a_attr , FileifThread , (void*) & (g_my_session_ptr->m_session_id));
	if ( -1 == a_return )
	{
		trc("[%s: %d] Create_Cmp error" , __FILE__ , __LINE__);
		return ERROR_RETURN;
	}

	trc("[%s: %d] First file thread start" , __FILE__ , __LINE__);

	return NORMAL_RETURN;
}

static int32_t ReqConnectPort(char* e_message)
{
	g_my_session_ptr->m_Trans_cmd_state = CHILD_TRANS_STATE_CONNECT;
	return NORMAL_RETURN;
}

static int32_t NtfWakeDoneFileif(char* e_message)
{
	st_msg_ntf_wake_done_trans a_msg;
	memset(&a_msg , 0 , sizeof(a_msg));
	a_msg.m_mq_header.m_commandcode = FTP_MSG_NTF_WAKE_DONE_TRANS;
	a_msg.m_result = RESULT_OK;

	int a_return = SendMQ_CHILD(CMP_NO_CHILD , g_my_session_ptr->m_session_id , CHILD_CMD , &a_msg , sizeof(a_msg));

	return a_return;
}

static int32_t NtfStartIdleTrans(char* e_message)
{
	st_msg_ntf_start_idle_fileif a_msg;
	memset(&a_msg , 0 , sizeof(a_msg));
	a_msg.m_mq_header.m_commandcode = FTP_MSG_NTF_START_IDLE_FILEIF;

	int a_return = SendMQ_CHILD(CMP_NO_CHILD , g_my_session_ptr->m_session_id , CHILD_FILEIF , &a_msg , sizeof(a_msg));

	return NORMAL_RETURN;
}
