///////////////////////////////////////////////////////////////////
// @file cmd_thread.c
// @date 2020/06/21
///////////////////////////////////////////////////////////////////

#include "headers.h"
#include "cmd_thread.h"

__thread struct timeval				g_wait_time;					// �^�C���A�E�g 500ms
__thread fd_set						g_cliant_fd;					// �N���C�A���g�f�B�X�N���v�^
__thread int						g_QUIT_flags = 0;				// QUIT�t���O
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

	// epool�p��fd����
	static __thread struct epoll_event s_set_event;
	memset(&s_set_event , 0 , sizeof(s_set_event));
	s_set_event.events = EPOLLIN;

	// �T�[�o�[�\�P�b�g
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

	// �N���C�A���g�\�P�b�g
	static __thread  int s_epoll_cliant_fd;
	s_epoll_cliant_fd = epoll_create1(0);

	   
	while ( 1 )
	{
		// ���b�Z�[�W��M
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

					// �I������
					if ( END_RETURN == a_return )
					{
						trc("[%s: %d][session: %d]  END_RETURN start" , __FILE__ , __LINE__,g_my_session_ptr->m_session_id);
						shutdown(g_cliantSock, SHUT_RDWR);
						close(s_epoll_cliant_fd);
						close(a_epoll_serv_fd);
						CloseMQ(CMP_NO_CHILD , g_my_session_ptr->m_session_id,CHILD_CMD);

						// �I���ʒm���M
						st_msg_ntf_finish_child a_send_msg;
						memset(&a_send_msg , 0 , sizeof(a_send_msg));
						a_send_msg.m_mq_header.m_commandcode = FTP_MSG_NTF_FINISH_CHILD;

						a_return = SendMQ_CHILD(CMP_NO_SESSION_COMMAND_ID , g_my_session_ptr->m_session_id , CHILD_CMD , &a_send_msg , sizeof(a_send_msg));
						return NULL;
					}
					// ���Z�b�g�̏ꍇ�A�����Z�b�V�����ŃX���b�h���ė����グ���Ă���̂ŏI���ʒm�𑗂�Ȃ�
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
			// �N���C�A���g�̐ڑ��҂�
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

				// �R�}���h�҂��ɑJ��
				g_my_session_ptr->m_child_cmd_state = CHILD_CMD_STATE_WAIT_COMMAND;
				trc("[%s: %d][session: %d]  Cliant connected!!" , __FILE__ , __LINE__,g_my_session_ptr->m_session_id);
				break;
			// �R�}���h�҂�
			case CHILD_CMD_STATE_WAIT_COMMAND:
			// �R�}���h�����҂�
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

				// ��M
				static __thread int s_error_cnt = 0; // �G���[��
				a_return = recv(g_cliantSock , &a_message , sizeof(a_message) , MSG_DONTWAIT);
				if ( 0 == a_return )
				{
					s_error_cnt++;
					trc("[%s: %d][session: %d]  recv error" , __FILE__ , __LINE__,g_my_session_ptr->m_session_id);

					/* �|�[�����O���ʂ��Ă���̂Ɏ�M�G���[����ꍇ�̓\�P�b�g�̐ڑ��؂�̉\���������̂�500ms * 5cnt = 2.5sec�҂��Ă��A���ŃG���[
					 * ����ꍇ�͐ڑ��؂�Ɣ��肵�ăZ�b�V�����I���������s���B
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

				// �R�}���h�A��������
				a_return = SepareteCommand(a_message , a_command , a_args);

				trc("[%s: %d] separate %s %s" , __FILE__ , __LINE__ , a_command , a_args);
				int a_index;
				for ( a_index = 0; ; a_index++ )
				{
					// �R�}���h����
					if ( 0 == strcmp(g_command_list [a_index].command , a_command) )
					{
						a_return = g_command_list [a_index].Do_command(a_args);
						break;
					}

				}

				if ( NORMAL_RETURN != a_return )
				{
					trc("[%s: %d][session: %d]  read command error" , __FILE__ , __LINE__,g_my_session_ptr->m_session_id);


					// read�ł��Ȃ��Ȃ�����ǂ����邩�H
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
						// TODO: ����Ȃ��ꍇ�ǂ����悤�H
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
 * @func		����������
 * @return		ERROR_RETURN,NORMAL_RETURN
 * @param		e_session_id	�Z�b�V����ID
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
 * @func		TCP�ڑ�����
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

	// ���M
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
 * @func		��M�����񕪊�����
 * @return		ERROR_RETURN,NORMAL_RETURN
 * @param		e_string	�Z�b�V����ID
 *				e_command	�R�}���h�i�[�|�C���^
 *				e_args		�����i�[�|�C���^
 */
static int32_t SepareteCommand(char* e_string , char* e_command , char* e_args)
{
	int a_command_got = 0;		// �R�}���h���o����
	int a_args_got = 0;			// �������o����
	int a_command_first = 0;	// �R�}���h�̊J�n�����F��
	int a_args_first = 0;		// �����̊J�n�����F��
	while ( 1 )
	{
		// �R�}���h���o
		if ( 0 == a_command_got )
		{
			// "_USER args"
			//  �����̕����m�F���폜
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
					//       �����̕����m�F���폜
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
		// �������o
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
					//       �����̕����m�F���폜
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
	// �����ĂȂ��I
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

