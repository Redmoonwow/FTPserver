///////////////////////////////////////////////////////////////////
// @file msg_h
// @date 2020/06/21
///////////////////////////////////////////////////////////////////

#ifndef _MSG_H_
#define _MSG_H_
#include "headers.h"

#define RESULT_OK	1
#define RESULT_NG	2
#define RESULT_BUSY 3

//#### 保守メッセージ 0x0000
#define FTP_MSG_BASE_MANTENANCE				0x00000000
#define FTP_MSG_NTF_WAKE_DONE				FTP_MSG_BASE_MANTENANCE		| 0x0000 // 起動完了通知
#define FTP_MSG_NTF_START_IDLE				FTP_MSG_BASE_MANTENANCE		| 0x0001 // 運用開始通知
#define FTP_MSG_REQ_ALIVE_CHECK				FTP_MSG_BASE_MANTENANCE		| 0x0002 // アライブチェック要求
#define FTP_MSG_RES_ALIVE_CHECK				FTP_MSG_BASE_MANTENANCE		| 0x0003 // アライブチェック応答
#define FTP_MSG_REQ_END_ALL_SESSION			FTP_MSG_BASE_MANTENANCE		| 0x0004 // 全セッション終了要求
#define FTP_MSG_RES_END_ALL_SESSION			FTP_MSG_BASE_MANTENANCE		| 0x0004 // 全セッション終了応答

//#### 子プロセス間メッセージ　CMDスレッド

// common
#define FTP_MSG_BASE_CHILD_THREAD			0xF0000000
#define FTP_MSG_NTF_WAKE_DONE_CHILD			FTP_MSG_BASE_CHILD_THREAD	| 0x0000 // 子プロセス起動完了通知
#define FTP_MSG_NTF_START_IDLE_CHILD		FTP_MSG_BASE_CHILD_THREAD	| 0x0001 // 子プロセス運用開始通知
#define FTP_MSG_REQ_ACCEPT_CHILD			FTP_MSG_BASE_CHILD_THREAD	| 0x0002 // 子プロセス接続完了要求
#define FTP_MSG_RES_ACCEPT_CHILD			FTP_MSG_BASE_CHILD_THREAD	| 0x0003 // 子プロセス接続完了応答
#define FTP_MSG_NTF_FINISH_CHILD			FTP_MSG_BASE_CHILD_THREAD	| 0x0004 // 子プロセス終了通知
#define FTP_MSG_REQ_RESET_CHILD				FTP_MSG_BASE_CHILD_THREAD	| 0x0005 // 子プロセスリセット要求
#define FTP_MSG_RES_RESET_CHILD				FTP_MSG_BASE_CHILD_THREAD	| 0x0006 // 子プロセスリセット応答
#define FTP_MSG_REQ_END_ALL_SESSION_CHILD	FTP_MSG_BASE_CHILD_THREAD	| 0x0007 // 子プロセス全セッション終了要求
#define FTP_MSG_RES_END_ALL_SESSION_CHILD	FTP_MSG_BASE_CHILD_THREAD	| 0x0008 // 子プロセス全セッション終了応答

// CMD <==> CMD MANAGER
#define FTP_MSG_REQ_QUIT_CHILD				FTP_MSG_BASE_CHILD_THREAD	| 0x0100 // 子プロセスQUIT受信要求
#define FTP_MSG_RES_QUIT_CHILD				FTP_MSG_BASE_CHILD_THREAD	| 0x0101 // 子プロセスQUIT受信応答
#define FTP_MSG_REQ_TRANSFER_CMD_CHILD		FTP_MSG_BASE_CHILD_THREAD	| 0x0102 // 子プロセス転送要求CMD
#define FTP_MSG_RES_TRANSFER_CMD_CHILD		FTP_MSG_BASE_CHILD_THREAD	| 0x0103 // 子プロセス転送応答CMD

// CMD <==> TRANS
#define FTP_MSG_REQ_TRANSFER_TRANS_CHILD	FTP_MSG_BASE_CHILD_THREAD	| 0x0200 // 子プロセス転送要求
#define FTP_MSG_RES_TRANSFER_TRANS_CHILD	FTP_MSG_BASE_CHILD_THREAD	| 0x0201 // 子プロセス転送応答
#define FTP_MSG_REQ_CONNECT_PORT_CHILD		FTP_MSG_BASE_CHILD_THREAD	| 0x0202 // 子プロセスport接続要求
#define FTP_MSG_RES_CONNECT_PORT_CHILD		FTP_MSG_BASE_CHILD_THREAD	| 0x0203 // 子プロセスport接続応答
#define FTP_MSG_NTF_WAKE_DONE_TRANS			FTP_MSG_BASE_CHILD_THREAD	| 0x0204 // 起動完了通知(TRANS)
#define FTP_MSG_NTF_START_IDLE_TRANS		FTP_MSG_BASE_CHILD_THREAD	| 0x0205 // 運用開始通知(TRANS)
#define FTP_MSG_REQ_END_THREAD_TRANS		FTP_MSG_BASE_CHILD_THREAD	| 0x0206 // スレッド終了要求(TRANS)
#define FTP_MSG_RES_END_THREAD_TRANS		FTP_MSG_BASE_CHILD_THREAD	| 0x0207 // スレッド終了応答(TRANS)

// FILEIF <==> TRANS
#define FTP_MSG_REQ_FILE_READ_CHILD			FTP_MSG_BASE_CHILD_THREAD	| 0x0300 // 子プロセス読取要求
#define FTP_MSG_RES_FILE_READ_CHILD			FTP_MSG_BASE_CHILD_THREAD	| 0x0301 // 子プロセス読取応答
#define FTP_MSG_REQ_FILE_WRITE_CHILD		FTP_MSG_BASE_CHILD_THREAD	| 0x0302 // 子プロセス書込要求
#define FTP_MSG_RES_FILE_WRITE_CHILD		FTP_MSG_BASE_CHILD_THREAD	| 0x0303 // 子プロセス書込応答
#define FTP_MSG_REQ_FILE_LIST_CHILD			FTP_MSG_BASE_CHILD_THREAD	| 0x0304 // 子プロセスリスト要求
#define FTP_MSG_RES_FILE_LIST_CHILD			FTP_MSG_BASE_CHILD_THREAD	| 0x0305 // 子プロセスリスト応答
#define FTP_MSG_REQ_FILE_NLIST_CHILD		FTP_MSG_BASE_CHILD_THREAD	| 0x0304 // 子プロセスNリスト要求
#define FTP_MSG_RES_FILE_NLIST_CHILD		FTP_MSG_BASE_CHILD_THREAD	| 0x0305 // 子プロセスNリスト応答
#define FTP_MSG_NTF_WAKE_DONE_FILEIF        FTP_MSG_BASE_CHILD_THREAD	| 0x0306 // 起動完了通知(FILEIF)
#define FTP_MSG_NTF_START_IDLE_FILEIF		FTP_MSG_BASE_CHILD_THREAD	| 0x0307 // 運用開始通知(FILEIF)
#define FTP_MSG_REQ_END_THREAD_FILEIF		FTP_MSG_BASE_CHILD_THREAD	| 0x0208 // スレッド終了要求(FILEIF)
#define FTP_MSG_RES_END_THREAD_FILEIF		FTP_MSG_BASE_CHILD_THREAD	| 0x0209 // スレッド終了応答(FILEIF)

// 共有ヘッダ
typedef struct st_mq_header
{
	int				m_dst_id;
	int				m_src_id;
	int				m_session_id;
	int32_t			m_commandcode;
}st_mq_header;

//#### 保守メッセージ 0x0000
// 起動完了通知
typedef struct st_msg_ntf_wake_done
{
	st_mq_header	m_mq_header;
	int32_t			m_result;
}st_msg_ntf_wake_done;

typedef st_msg_ntf_wake_done st_msg_ntf_wake_done_trans;		// 起動完了通知(TRANS)
typedef st_msg_ntf_wake_done st_msg_ntf_wake_done_fileif;		// 起動完了通知(FILEIF)

// 運用開始通知
typedef struct st_msg_ntf_start_idle
{
	st_mq_header	m_mq_header;
}st_msg_ntf_start_idle;

typedef st_msg_ntf_start_idle st_msg_ntf_start_idle_trans;		// 運用開始通知(TRANS)
typedef st_msg_ntf_start_idle st_msg_ntf_start_idle_fileif;		// 運用開始通知(FILEIF) 

// アライブチェック要求
typedef struct st_msg_req_alive_check
{
	st_mq_header	m_mq_header;
}st_msg_req_alive_check;

// アライブチェック応答
typedef struct st_msg_res_alive_check
{
	st_mq_header	m_mq_header;
	int32_t			m_result;
}st_msg_res_alive_check;

// 全セッション終了要求
typedef struct st_msg_req_end_all_session
{
	st_mq_header	m_mq_header;
}st_msg_req_end_all_session;

// 全セッション終了応答
typedef struct st_msg_res_end_all_session
{
	st_mq_header	m_mq_header;
}st_msg_res_end_all_session;

//#### 子プロセス間メッセージ　CMDスレッド

// 子プロセス起動完了通知
typedef struct st_msg_ntf_wake_done_child
{
	st_mq_header	m_mq_header;
	int32_t			m_result;
}st_msg_ntf_wake_done_child;

// 子プロセス運用開始通知
typedef struct st_msg_ntf_start_idle_child
{
	st_mq_header	m_mq_header;
}st_msg_ntf_start_idle_child;

// 子プロセス接続完了要求
typedef struct st_msg_req_accept_done_child
{
	st_mq_header	m_mq_header;
}st_msg_req_accept_done_child;

// 子プロセス接続完了応答
typedef struct st_msg_res_accept_done_child
{
	st_mq_header	m_mq_header;
}st_msg_res_accept_done_child;

// 子プロセスQUIT受信要求
typedef struct st_msg_req_recvquit_child
{
	st_mq_header	m_mq_header;
}st_msg_req_recvquit_child;

// 子プロセスQUIT受信応答
typedef struct st_msg_res_recvquit_child
{
	st_mq_header	m_mq_header;
}st_msg_res_recvquit_child;

// 子プロセス転送要求CMD
typedef struct st_msg_req_transcmd_child
{
	st_mq_header	m_mq_header;
	int32_t			m_command;
}st_msg_req_transcmd_child;

// 子プロセス転送応答CMD
typedef struct st_msg_res_transcmd_child
{
	st_mq_header	m_mq_header;
	int32_t			m_command;
}st_msg_res_transcmd_child;

// 子プロセス終了通知
typedef struct st_msg_ntf_finish_child
{
	st_mq_header	m_mq_header;
}st_msg_ntf_finish_child;

// 子プロセスリセット要求
typedef struct st_msg_req_reset_child
{
	st_mq_header	m_mq_header;
}st_msg_req_reset_child;

// 子プロセスリセット応答
typedef struct st_msg_res_reset_child
{
	st_mq_header	m_mq_header;
}st_msg_res_reset_child;

// 子プロセス全セッション終了要求
typedef struct st_msg_req_end_all_session_child
{
	st_mq_header	m_mq_header;
}st_msg_req_end_all_session_child;

// 子プロセス全セッション終了応答
typedef struct st_msg_res_end_all_session_child
{
	st_mq_header	m_mq_header;
}st_msg_res_end_all_session_child;

// スレッド終了要求(TRANS)
typedef struct st_msg_req_end_thread_trans
{
	st_mq_header	m_mq_header;
}st_msg_req_end_thread_trans;

// スレッド終了応答(TRANS)
typedef struct st_msg_res_end_thread_trans
{
	st_mq_header	m_mq_header;
	int32_t			m_result;
}st_msg_res_end_thread_trans;

// スレッド終了応答(FILEIF)
typedef st_msg_req_end_thread_trans st_msg_req_end_thread_fileif;

// スレッド終了応答(FILEIF)
typedef st_msg_res_end_thread_trans st_msg_res_end_thread_fileif;

// スレッド終了応答(TRANS)
typedef struct st_msg_req_trans_child
{
	st_mq_header	m_mq_header;
}st_msg_req_end_thread_trans;

// 子プロセス転送要求TRANS
typedef struct st_msg_req_trans_child
{
	st_mq_header	m_mq_header;
}st_msg_req_trans_child;

// 子プロセス転送応答TRANS
typedef struct st_msg_res_trans_child
{
	st_mq_header	m_mq_header;
}st_msg_res_trans_child;

// 子プロセスport接続要求
typedef struct st_msg_req_connect_port_child
{
	st_mq_header	m_mq_header;
}st_msg_req_connect_port_child;

// 子プロセスport接続応答
typedef struct st_msg_res_connect_port_child
{
	st_mq_header	m_mq_header;
	int				m_result;
}st_msg_res_connect_port_child;

// 子プロセス読取要求
typedef struct st_msg_req_file_read_child
{
	st_mq_header	m_mq_header;
}st_msg_req_file_read_child;

// 子プロセス読取応答
typedef struct st_msg_res_file_read_child
{
	st_mq_header	m_mq_header;
}st_msg_res_file_read_child;

// 子プロセス書込要求
typedef struct st_msg_req_file_write_child
{
	st_mq_header	m_mq_header;
}st_msg_req_file_write_child;

// 子プロセス書込応答
typedef struct st_msg_res_file_write_child
{
	st_mq_header	m_mq_header;
}st_msg_res_file_write_child;

// 子プロセスリスト要求
typedef struct st_msg_req_file_list_child
{
	st_mq_header	m_mq_header;
}st_msg_req_file_list_child;

// 子プロセスリスト応答
typedef struct st_msg_res_file_list_child
{
	st_mq_header	m_mq_header;
}st_msg_res_file_list_child;

// 子プロセスNリスト要求
typedef struct st_msg_req_file_nlist_child
{
	st_mq_header	m_mq_header;
}st_msg_req_file_nlist_child;

// 子プロセスNリスト応答
typedef struct st_msg_res_file_nlist_child
{
	st_mq_header	m_mq_header;
}st_msg_res_file_nlist_child;







#endif


