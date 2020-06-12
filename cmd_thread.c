///////////////////////////////////////////////////////////////////
// @file cmd_thread.c
// @date 2020/06/21
///////////////////////////////////////////////////////////////////

#include "headers.h"
#include "cmd_thread.h"

__thread struct timeval				g_wait_time;					// タイムアウト 500ms
__thread fd_set						g_cliant_fd;					// クライアントディスクリプタ
__thread int						g_QUIT_flags = 0;				// QUITフラグ
__thread int						g_END_flags = 0;
__thread int						g_pasv_flags = 0;
__thread int						g_cliant_port [6] = { 0 };		// ip1 ip2 ip3 ip4 port1 port2
__thread int						g_typemode = TYPE_BINARY;
__thread int						g_abort_flag = 0;
__thread int						g_cliantSock = 0;
__thread st_session_data*			g_my_session_ptr;
__thread fd_set						g_serv_fd;
extern int									g_servSock;						//server socket descriptor

static int32_t InitCmdThread(int e_session_id);
static int32_t Wait_ConnectFTP(void);
static int32_t SepareteCommand(char* e_string , char* e_command , char* e_args);

static int32_t RecvNtfStartIdle(char* e_message);
static int32_t RecvResAcceptChild(char* e_message);
static int32_t RecvResQuitChild(char* e_message);
static int32_t RecvResTransferChild(char* e_message);
static int32_t RecvReqResetChild(char* e_message);
static int32_t RecvReqEndSessionChild(char* e_message);

static st_function_msg_list s_function_list [] =
{
		//	COMMAND						,FUNC
	{	FTP_MSG_NTF_START_IDLE_CHILD		,RecvNtfStartIdle			},
	{	FTP_MSG_RES_ACCEPT_CHILD			,RecvResAcceptChild			},
	{	FTP_MSG_RES_QUIT_CHILD				,RecvResQuitChild			},
	{	FTP_MSG_RES_TRANSFER_CMD_CHILD		,RecvResTransferChild		},
	{	FTP_MSG_REQ_RESET_CHILD				,RecvReqResetChild			},
	{	FTP_MSG_REQ_END_ALL_SESSION_CHILD	,RecvReqEndSessionChild		},
	{	0xFFFF								,NULL						}
};

void* Cmdthread(void* argv)
{
	char a_msg [10240];
	char a_message [4096];
	char a_command [4096];
	char a_args [4096];

	struct timespec a_sleep;
	memset(&a_sleep , 0 , sizeof(a_sleep));
	a_sleep.tv_nsec = 500000000;

	int a_session_id = *(( int* ) argv );
	trc("[%s: %d][session: %d] command thread start" , __FILE__ , __LINE__,a_session_id);
	InitCmdThread(a_session_id);

	// epool用のfd生成
	static __thread struct epoll_event s_set_event;
	memset(&s_set_event , 0 , sizeof(s_set_event));
	s_set_event.events = EPOLLIN;

	// サーバーソケット
	static __thread int a_epoll_serv_fd;
	a_epoll_serv_fd  = epoll_create1(0);
	int a_return = epoll_ctl(a_epoll_serv_fd , EPOLL_CTL_ADD , g_servSock , &s_set_event);
	if ( -1 == a_return )
	{
		st_msg_ntf_finish_child a_send_msg;
		memset(&a_send_msg , 0 , sizeof(a_send_msg));
		a_send_msg.m_mq_header.m_commandcode = FTP_MSG_NTF_FINISH_CHILD;
		close(a_epoll_serv_fd);
		a_return = SendMQ_CHILD(CMP_NO_SESSION_COMMAND_ID, g_my_session_ptr->m_session_id , CHILD_CMD , &a_send_msg , sizeof(a_send_msg));
		CloseMQ(CMP_NO_CHILD , g_my_session_ptr->m_session_id , CHILD_CMD);
		return NULL;
	}

	// クライアントソケット
	static __thread  int s_epoll_cliant_fd;
	s_epoll_cliant_fd = epoll_create1(0);

	   
	while ( 1 )
	{
		// メッセージ受信
		//------------------------------------------------------------------------------------------------------------
		memset(&a_msg , 0 , sizeof(a_msg));
		a_return = RecvMQtimeout(CMP_NO_SESSION_COMMAND_ID , a_msg , sizeof(a_msg),500000000);
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
						trc("[%s: %d][session: %d]  func error" , __FILE__ , __LINE__,g_my_session_ptr->m_session_id);
					}

					// 終了処理
					if ( END_RETURN == a_return )
					{
						trc("[%s: %d][session: %d]  END_RETURN start" , __FILE__ , __LINE__,g_my_session_ptr->m_session_id);
						shutdown(g_cliantSock, SHUT_RDWR);
						close(s_epoll_cliant_fd);
						close(a_epoll_serv_fd);
						CloseMQ(CMP_NO_CHILD , g_my_session_ptr->m_session_id,CHILD_CMD);

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
						shutdown(g_cliantSock , SHUT_RDWR);
						CloseMQ(CMP_NO_CHILD , g_my_session_ptr->m_session_id , CHILD_CMD);
						return NULL;
					}
					break;
				}
				else if ( s_function_list [a_index].m_commandcode == 0xFFFF )
				{
					trc("[%s: %d][session: %d]  MSG unknown" , __FILE__ , __LINE__,g_my_session_ptr->m_session_id);
					break;
				}
				a_index++;
			}
		}
		//------------------------------------------------------------------------------------------------------------
		struct epoll_event a_event;
		memset(&a_event , 0 , sizeof(a_event));

		
		switch ( g_my_session_ptr->m_child_cmd_state )
		{
			// クライアントの接続待ち
			case CHILD_CMD_STATE_WAIT_ACCEPT:
				a_return = epoll_wait(a_epoll_serv_fd , &a_event , 256 , 500);
				if ( 0 == a_return )
				{
					//trc("[%s: %d] CHILD_CMD_STATE_WAIT_ACCEPT timeout" , __FILE__ , __LINE__);
					continue;
				}
				else if ( 0 > a_return )
				{
					trc("[%s: %d][session: %d]  select error" , __FILE__ , __LINE__,g_my_session_ptr->m_session_id);
					continue;
				}

				a_return = Wait_ConnectFTP( );
				if ( NORMAL_RETURN != a_return )
				{
					trc("[%s: %d][session: %d]  Wait_ConnectFTP error" , __FILE__ , __LINE__,g_my_session_ptr->m_session_id);
					close(s_epoll_cliant_fd);
					close(a_epoll_serv_fd);
					shutdown(g_cliantSock , SHUT_RDWR);

					st_msg_ntf_finish_child a_send_msg;
					memset(&a_send_msg , 0 , sizeof(a_send_msg));
					a_send_msg.m_mq_header.m_commandcode = FTP_MSG_NTF_FINISH_CHILD;

					a_return = SendMQ_CHILD(CMP_NO_SESSION_COMMAND_ID , g_my_session_ptr->m_session_id , CHILD_CMD , &a_send_msg , sizeof(a_send_msg));
					CloseMQ(CMP_NO_CHILD , g_my_session_ptr->m_session_id , CHILD_CMD);
					return NULL;
				}

				a_return = epoll_ctl(s_epoll_cliant_fd , EPOLL_CTL_ADD , g_cliantSock , &s_set_event);
				if ( ERROR_RETURN == a_return )
				{
					st_msg_ntf_finish_child a_send_msg;
					memset(&a_send_msg , 0 , sizeof(a_send_msg));
					a_send_msg.m_mq_header.m_commandcode = FTP_MSG_NTF_FINISH_CHILD;

					a_return = SendMQ_CHILD(CMP_NO_SESSION_COMMAND_ID , g_my_session_ptr->m_session_id , CHILD_CMD , &a_send_msg , sizeof(a_send_msg));
					CloseMQ(CMP_NO_CHILD , g_my_session_ptr->m_session_id , CHILD_CMD);
					return NULL;
				}

				st_msg_req_accept_done_child a_send_msg;
				memset(&a_send_msg , 0 , sizeof(a_send_msg));
				a_send_msg.m_mq_header.m_commandcode = FTP_MSG_REQ_ACCEPT_CHILD;

				a_return = SendMQ_CHILD(CMP_NO_SESSION_COMMAND_ID , g_my_session_ptr->m_session_id , CHILD_CMD , &a_send_msg , sizeof(a_send_msg));
				if ( NORMAL_RETURN != a_return )
				{
					trc("[%s: %d] send error" , __FILE__ , __LINE__);
					return NULL;
				}

				// コマンド待ちに遷移
				g_my_session_ptr->m_child_cmd_state = CHILD_CMD_STATE_WAIT_COMMAND;
				trc("[%s: %d][session: %d]  Cliant connected!!" , __FILE__ , __LINE__,g_my_session_ptr->m_session_id);
				break;
			// コマンド待ち
			case CHILD_CMD_STATE_WAIT_COMMAND:
			// コマンド処理待ち
			case CHILD_CMD_STATE_WAIT_PROCESS_DONE:

				a_return = epoll_wait(s_epoll_cliant_fd , &a_event , 256 , 500);
				if ( 0 == a_return )
				{
					//trc("[%s: %d] CHILD_CMD_STATE_WAIT_PROCESS_DONE timeout" , __FILE__ , __LINE__);
					continue;
				}
				else if ( 0 > a_return )
				{
					trc("[%s: %d][session: %d]  select error" , __FILE__ , __LINE__,g_my_session_ptr->m_session_id);
					continue;
				}

				memset(&a_message , 0 , sizeof(a_message));
				memset(&a_args , 0 , sizeof(a_args));
				memset(&a_command , 0 , sizeof(a_command));

				// 受信
				static __thread int s_error_cnt = 0; // エラー回数
				a_return = recv(g_cliantSock , &a_message , sizeof(a_message) , MSG_DONTWAIT);
				if ( 0 == a_return )
				{
					s_error_cnt++;
					trc("[%s: %d][session: %d]  recv error" , __FILE__ , __LINE__,g_my_session_ptr->m_session_id);

					/* ポーリングが通っているのに受信エラーする場合はソケットの接続切れの可能性が高いので500ms * 5cnt = 2.5sec待っても連続でエラー
					 * する場合は接続切れと判定してセッション終了処理を行う。
					 */

					if ( 10 == s_error_cnt )
					{
						trc("[%s: %d][session: %d]  recv error timeout" , __FILE__ , __LINE__,g_my_session_ptr->m_session_id);

						close(s_epoll_cliant_fd);
						close(a_epoll_serv_fd);
						shutdown(g_cliantSock , SHUT_RDWR);

						st_msg_ntf_finish_child a_send_msg;
						memset(&a_send_msg , 0 , sizeof(a_send_msg));
						a_send_msg.m_mq_header.m_commandcode = FTP_MSG_NTF_FINISH_CHILD;

						a_return = SendMQ_CHILD(CMP_NO_SESSION_COMMAND_ID , g_my_session_ptr->m_session_id , CHILD_CMD , &a_send_msg , sizeof(a_send_msg));
						if ( 0 == a_return )
						{
							trc("[%s: %d] send error" , __FILE__ , __LINE__);
							return NULL;
						}
						CloseMQ(CMP_NO_CHILD , g_my_session_ptr->m_session_id , CHILD_CMD);
						return NULL;
					}
					else
					{
						struct timespec s_error_sleep;
						memset(&s_error_sleep , 0 , sizeof(s_error_sleep));
						s_error_sleep.tv_nsec = 400000000;
						nanosleep(&s_error_sleep , NULL);
					}
					continue;
				}
				else
				{
					s_error_cnt = 0;
				}

				trc("[%s: %d][session: %d]  recv %s" , __FILE__ , __LINE__ , g_my_session_ptr->m_session_id,a_message);

				// コマンド、引数検索
				a_return = SepareteCommand(a_message , a_command , a_args);

				trc("[%s: %d] separate %s %s" , __FILE__ , __LINE__ , a_command , a_args);
				int a_index;
				for ( a_index = 0; ; a_index++ )
				{
					// コマンド検索
					if ( 0 == strcmp(g_command_list [a_index].command , a_command) )
					{
						a_return = g_command_list [a_index].Do_command(a_args);
						break;
					}

				}

				if ( NORMAL_RETURN != a_return )
				{
					trc("[%s: %d][session: %d]  read command error" , __FILE__ , __LINE__,g_my_session_ptr->m_session_id);


					// readできなくなったらどうするか？
					//memcpy(( void*) a_message , (void*)"QUIT" , sizeof("QUIT"));
				}

				if ( 1 == g_QUIT_flags )
				{
					st_msg_req_recvquit_child a_msg;
					memset(&a_msg , 0 , sizeof(a_msg));
					a_msg.m_mq_header.m_commandcode = FTP_MSG_REQ_QUIT_CHILD;

					a_return = SendMQ_CHILD(CMP_NO_SESSION_COMMAND_ID , g_my_session_ptr->m_session_id , CHILD_CMD , &a_msg , sizeof(a_msg));
					if ( ERROR_RETURN == a_return )
					{
						// TODO: 送れない場合どうしよう？
						return NULL;
					}
					break;
				}
				break;
			default:
				nanosleep(&a_sleep , NULL);
				continue;
		}
	}
}

/*
 * @func		初期化処理
 * @return		ERROR_RETURN,NORMAL_RETURN
 * @param		e_session_id	セッションID
 */
int32_t InitCmdThread(int e_session_id)
{
	g_my_session_ptr = SearchSession(e_session_id);
	if ( NULL == g_my_session_ptr )
	{
		return ERROR_RETURN;
	}

	int32_t a_return = OpenMQ_CHILD(g_my_session_ptr->m_session_id , CHILD_CMD);
	if ( ERROR_RETURN == a_return )
	{
		return ERROR_RETURN;
	}

	st_msg_ntf_wake_done_child a_msg;
	memset(&a_msg , 0 , sizeof(a_msg));
	a_msg.m_mq_header.m_commandcode = FTP_MSG_NTF_WAKE_DONE_CHILD;
	a_msg.m_result = RESULT_OK;

	a_return = SendMQ_CHILD(CMP_NO_SESSION_COMMAND_ID , g_my_session_ptr->m_session_id , CHILD_CMD , &a_msg , sizeof(a_msg));
	if ( ERROR_RETURN == a_return )
	{
		return ERROR_RETURN;
	}

	return NORMAL_RETURN;
}

/*
 * @func		TCP接続処理
 * @return		ERROR_RETURN,NORMAL_RETURN
 */
static int32_t Wait_ConnectFTP(void)
{
	g_cliantSock = accept(g_servSock , NULL , NULL);

	if ( 0 > g_cliantSock )
	{
		trc("[%s: %d][session: %d]  accept error" , __FILE__ , __LINE__,g_my_session_ptr->m_session_id);
		return ERROR_RETURN;
	}

	// 送信
	int a_return = send(g_cliantSock , RES_CONNECT_DONE , strlen(RES_CONNECT_DONE) , MSG_DONTWAIT);
	if ( 0 == a_return )
	{
		trc("[%s: %d][session: %d]  send error" , __FILE__ , __LINE__,g_my_session_ptr->m_session_id);
		return ERROR_RETURN;
	}
	g_my_session_ptr->m_child_cmd_state = CHILD_CMD_STATE_WAIT_COMMAND;
	return NORMAL_RETURN;

}

/*
 * @func		受信文字列分割処理
 * @return		ERROR_RETURN,NORMAL_RETURN
 * @param		e_string	セッションID
 *				e_command	コマンド格納ポインタ
 *				e_args		引数格納ポインタ
 */
static int32_t SepareteCommand(char* e_string , char* e_command , char* e_args)
{
	int a_command_got = 0;		// コマンド抽出完了
	int a_args_got = 0;			// 引数抽出完了
	int a_command_first = 0;	// コマンドの開始文字認識
	int a_args_first = 0;		// 引数の開始文字認識
	while ( 1 )
	{
		// コマンド抽出
		if ( 0 == a_command_got )
		{
			// "_USER args"
			//  ↑この部分確認し削除
			if ( ' ' == *e_string && 0 == a_command_first )
			{
				e_string++;
				continue;
			}
			else
			{
				if ( ' ' == *e_string || '\r' == *e_string || '\n' == *e_string )
				{
					// " USER_args"
					//       ↑この部分確認し削除
					a_command_got = 1;
					e_string++;
				}
				else
				{
					a_command_first = 1;
					*e_command = *e_string;
					e_string++;
					e_command++;
				}

			}
		}
		// 引数抽出
		else
		{
			if ( ' ' == *e_string && 0 == a_args_first )
			{
				e_string++;
				continue;
			}
			else
			{
				if ( '\r' == *e_string || '\n' == *e_string )
				{
					// " USER_args"
					//       ↑この部分確認し削除
					a_args_got = 1;
					e_string++;
				}
				else
				{
					a_args_first = 1;
					*e_args = *e_string;
					e_string++;
					e_args++;
				}
			}
		}

		if ( 1 == a_command_got && 1 == a_args_got )
		{
			break;
		}
	};
	return NORMAL_RETURN;
}

int32_t RecvNtfStartIdle(char* e_message)
{
	int a_return = listen(g_servSock , MAXQUEUE);

	if ( 0 != a_return )
	{
		trc("[%s: %d][session: %d]  listen error" , __FILE__ , __LINE__,g_my_session_ptr->m_session_id);
		return ERROR_RETURN;
	}

	trc("[%s: %d][session: %d]  wait connect..." , __FILE__ , __LINE__,g_my_session_ptr->m_session_id);

	g_my_session_ptr->m_child_cmd_state = CHILD_CMD_STATE_WAIT_ACCEPT;

	return NORMAL_RETURN;
}

int32_t RecvResAcceptChild(char* e_message)
{
	return NORMAL_RETURN;
}

int32_t RecvResQuitChild(char* e_message)
{
	trc("[%s: %d][session: %d]  call RecvResQuitChild" , __FILE__ , __LINE__,g_my_session_ptr->m_session_id);
	g_my_session_ptr->m_child_cmd_state = CHILD_CMD_STATE_WAIT_END;
	return END_RETURN;
}

int32_t RecvResTransferChild(char* e_message)
{
	// つくってない！
	return NORMAL_RETURN;
}

int32_t RecvReqResetChild(char* e_message)
{
	return RESET_RETURN;
}

int32_t RecvReqEndSessionChild(char* e_message)
{
	int a_return = send(g_cliantSock , RES_ERROR , sizeof(RES_ERROR) , MSG_DONTWAIT);
	if ( 0 == a_return )
	{
		trc("[%s: %d] send error" , __FILE__ , __LINE__);
		return ERROR_RETURN;
	}

	st_msg_res_end_all_session_child a_msg;
	memset(&a_msg , 0 , sizeof(a_msg));
	a_msg.m_mq_header.m_commandcode = FTP_MSG_RES_END_ALL_SESSION_CHILD;

	SendMQ_CHILD(CMP_NO_SESSION_COMMAND_ID , g_my_session_ptr->m_session_id , CHILD_CMD , &a_msg , sizeof(a_msg));

	return RESET_RETURN;
}

