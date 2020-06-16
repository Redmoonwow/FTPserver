///////////////////////////////////////////////////////////////////
// @file sessionAPI.h
// @date 2020/06/21
///////////////////////////////////////////////////////////////////

#ifndef SESSIONAPI_H_
#define SESSIONAPI_H_

#include "headers.h"
#if 0
typedef struct st_cmd_thread_list
{
	char*				nextptr;
	char*				preptr;

	st_session_data*	m_data_ptr;
};
#endif

typedef enum
{
	CHILD_STATE_INIT = 0x0000,
	CHILD_CMD_STATE_WAIT_ACCEPT,
	CHILD_CMD_STATE_WAIT_COMMAND,
	CHILD_CMD_STATE_WAIT_PROCESS_DONE,
	CHILD_CMD_STATE_WAIT_END,

	CHILD_TRANS_STATE_WAIT_MSG = 0x1000,
	CHILD_TRANS_STATE_CONNECT,
	CHILD_TRANS_STATE_WAIT_PUSH,
	CHILD_TRANS_STATE_CHECK_CMD,
	CHILD_TRANS_STATE_TRANSING,
	CHILD_TRANS_STATE_RECVING,
	CHILD_TRANS_STATE_WAIT_END
}en_childstate;

#if 1 // 202006012 cloneä÷êîÇ≈threadê∂ê¨Ç…ïœçX
typedef struct st_session_data
{
	en_childstate	m_child_cmd_state;
	en_childstate	m_Trans_cmd_state;
	int32_t			m_session_id;
	char			m_use_filepath [PATH_MAX];
	int32_t			m_session_command;
	int32_t			m_port;
	pthread_t		m_command_thread_id;
	pthread_t		m_trans_thread_id;
	pthread_t		m_fileif_thread_id;
	struct st_session_flags
	{
		int				m_QUIT_flags;					// QUITÉtÉâÉO
		int				m_END_flags;
		int				m_pasv_flags;
		uint8_t			m_cliant_addr[4];				// ip1 ip2 ip3 ip4
		uint16_t		m_cliant_port;
		int				m_typemode_flag;
		int				m_abort_flag;
		int				m_port_req_flag;
		int				m_reserve;
	}m_session_flags;
}st_session_data;

#else
typedef struct st_session_data
{
	en_childstate	m_child_cmd_state;
	en_childstate	m_Trans_cmd_state;
	int32_t			m_session_id;
	char			m_use_filepath [PATH_MAX];
	int32_t			m_session_command;
	int32_t			m_port;
	pid_t			m_command_thread_id;
	pid_t			m_trans_thread_id;
	pid_t			m_fileif_thread_id;
	char*			m_command_thread_mq_name;
	char*			m_trans__mq_name;
	char*			m__fileif_thread_mq_name;
}st_session_data;
#endif

extern int32_t InitSession(void);
extern int32_t FreeSession(void);
extern int32_t ClearSession(int32_t e_session_id , int e_thread_type);
extern st_session_data* CreateSession(void);
extern st_session_data* SearchSession(int32_t e_session_id);
extern int32_t WakeThreadCommand(st_session_data* e_session_ptr);
extern int32_t WakeThreadDATA(st_session_data* e_session_ptr);
extern int32_t GetUsedSessionID(void);
#if 0
char* SearchTopList(int32_t list_id);
char* NextList(char* list_ptr);
char* PreList(char* list_ptr);
#endif
#endif