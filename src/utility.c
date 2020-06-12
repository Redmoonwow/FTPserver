///////////////////////////////////////////////////////////////////
// @file utility.cpp
// @date 2020/06/21
///////////////////////////////////////////////////////////////////

#include "utility.h"


typedef struct st_thread_stack_ptr
{
	char*				m_stack_memory;			//8
	char*				m_stack_memory_top;		//8
	pid_t				m_tid;					//4
	struct user_desc	m_tls_d;				//16
	char				m_pad [4];				//4
}st_thread_stack_ptr;

static int Gettimeprintformat(char* e_time_ptr , int32_t e_ptr_datasize); // 現時刻取得

static FILE*					s_trc_ptr;								// トレースログファイル
static const int8_t				s_template_filename [] = "FTPSERVER";
static int32_t					s_template_file_no = 0;
static char						s_trc_filename [256];
static uint8_t					s_Init_done = 0;
static __thread	uint32_t		s_msg_ref;
static __thread struct sigevent s_sigevt;
static __thread mqd_t			s_my_mqdt = 0;
static st_thread_stack_ptr		s_thread_list [SESSION_SUPPORT_MAX * 3]; // FILEIF TRANS CMD分

/// ユーティリティAPI初期化
int InitUtilitis(void)
{
	struct timespec a_now_time;		// 取得時間
	struct tm a_localtime;			// 変換時間
	struct stat a_filesize;

	// convert char type
	localtime_r(&(a_now_time.tv_sec) , &a_localtime);

	memset(s_trc_filename , 0 , sizeof(s_trc_filename));
	memset(s_thread_list , 0 , (sizeof(st_thread_stack_ptr) * (SESSION_SUPPORT_MAX * 3)));
	// トレースログ名作成
	sprintf(s_trc_filename , "./trc_log/%s_%d_%d_NO:%d.log" , s_template_filename , a_localtime.tm_mon , a_localtime.tm_mday , s_template_file_no );

	// トレースログディレクトリがなければ生成
	if ( 0 != stat("./trc_log" , &a_filesize) )
	{
		mkdir("./trc_log" , S_IRWXU | S_IRWXO | S_IRWXG);
	}

	s_trc_ptr = fopen((const char*)&s_trc_filename , "wb");

	if ( NULL == s_trc_ptr )
	{
		printf("fail open trc_ptr\n");
		return ERROR_RETURN;
	}

	printf("UtilityAPI INIT Success!!\n");

	// 初期化完了フラグON
	s_Init_done = 1;

	return NORMAL_RETURN;
}

int CloseUtilities(void)
{
	fclose(s_trc_ptr);
	printf("UtilityAPI close\n");
	return 0;
}

/// 現時刻のhh:mm:ss形式の文字列生成
static int Gettimeprintformat(char* e_time_ptr , int32_t e_ptr_datasize)
{
	struct timespec a_now_time;		// 取得時間
	struct tm a_localtime;			// 変換時間

	if ( NULL != e_time_ptr &&
		((int32_t)9) >= e_ptr_datasize )
	{
		clock_gettime(CLOCK_REALTIME , &a_now_time);
		// convert char type
		localtime_r(&(a_now_time.tv_sec) , &a_localtime);
		sprintf(e_time_ptr , "%02d:%02d:%02d" , a_localtime.tm_hour , a_localtime.tm_min , a_localtime.tm_sec);
		return NORMAL_RETURN;
	}
	else
	{
		return ERROR_RETURN;
	}

	return ERROR_RETURN;
}

/// トレースログ出力関数
void trc(const char* e_string , ...)
{

	if ( 0 == s_Init_done )
	{
		return;
	}

	char a_printbuf [1024];

	va_list a_valist;

	// printf形式の文字列生成
	va_start( a_valist, e_string);

	vsprintf(a_printbuf , e_string , a_valist);

	va_end(a_valist);

	// 時間取得して追加
	char a_timeprintbuf [9];

	if ( NORMAL_RETURN != Gettimeprintformat(a_timeprintbuf , sizeof(a_timeprintbuf)) )
	{
		printf("Gettimeprintformat failed\n");
	}

	// デバッグプリント
	#ifdef DEBUG_MODE_ON
	printf("[%s] %s\n" , a_timeprintbuf , a_printbuf);
	#endif
	fprintf(s_trc_ptr , "[%s] %s\n" , a_timeprintbuf , a_printbuf);

	return;
}

/*
 * スレッドライブラリ
 *　pthreadライクなスレッド生成を行う // CMDスレッドなどはディレクトリFSを独自にもつひつようがあるため　
 *　
 */

#define ___PTHREAD_DEFAULT (1024 * 96)
pid_t Create_Cmp(int e_flag , void* (*e_start_routine) (void*) , void* e_arg)
{
	int a_index = 0;
	int a_found = 0;
	// 空いているスレッド情報リストを探す
	for ( a_index = 0; a_index < (SESSION_SUPPORT_MAX * 3) ; a_index++)
	{
		if ( 0 == s_thread_list [a_index].m_tid )
		{
			a_found = 1;
			break;
		}
	}

	if ( 0 == a_found )
	{
		trc("[%s: %d] thread_list use full" , __FILE__ , __LINE__ );
		return -1;
	}

	s_thread_list [a_index].m_stack_memory = malloc(___PTHREAD_DEFAULT);
	if ( NULL == s_thread_list [a_index].m_stack_memory )
	{
		trc("[%s: %d] malloc error" , __FILE__ , __LINE__);
	}
	s_thread_list [a_index].m_stack_memory_top = s_thread_list [a_index].m_stack_memory + ___PTHREAD_DEFAULT;

	s_thread_list [a_index].m_tid = clone((int(*)(void*))e_start_routine ,s_thread_list [a_index].m_stack_memory_top ,
										    e_flag | CLONE_SIGHAND | CLONE_VM | CLONE_THREAD | CLONE_SYSVSEM  | CLONE_PARENT | CLONE_SETTLS,
										   (e_arg),&s_thread_list [a_index].m_tls_d);

	if ( 0 == s_thread_list [a_index].m_tid )
	{
		trc("[%s: %d] thread create fail" , __FILE__ , __LINE__);
		s_thread_list [a_index].m_tid = 0;
		free(s_thread_list [a_index].m_stack_memory);
		s_thread_list [a_index].m_stack_memory_top = 0;
		return -1;
	}
	return s_thread_list [a_index].m_tid;
}

int CloseCmp(pid_t e_tid)
{
	int a_index = 0;
	int a_found = 0;
	// 空いているスレッド情報リストを探す
	for ( a_index = 0; a_index < (SESSION_SUPPORT_MAX * 3); a_index++ )
	{
		if ( e_tid == s_thread_list [a_index].m_tid )
		{
			a_found = 1;
			break;
		}
	}

	if ( 0 == a_found )
	{
		trc("[%s: %d] thread_list not found" , __FILE__ , __LINE__);
		return ERROR_RETURN;
	}

	s_thread_list [a_index].m_tid = 0;
	free(s_thread_list [a_index].m_stack_memory);
	s_thread_list [a_index].m_stack_memory_top = 0;

	return NORMAL_RETURN;
}

/*
 * メッセージキューライブラリ
 *
 */

void ref_mq_thread(union sigval e_val)
{
	s_msg_ref++;

	return;
}



int32_t OpenMQ(int32_t e_mq_name)
{

	struct mq_attr a_mq_attr;
	trc("[%s: %d] OpenMQ START PID= %d" , __FILE__ , __LINE__ , getppid( ));
	memset(&a_mq_attr , 0 , sizeof(a_mq_attr));
	a_mq_attr.mq_flags = 0;
	a_mq_attr.mq_maxmsg = 30;
	a_mq_attr.mq_msgsize = sizeof(sizeof(char) * 10240);

	char a_buf [256];
	memset(&a_buf , 0 , sizeof(a_buf));
	sprintf(a_buf , "/proc_%04d" , e_mq_name);
	mq_unlink(a_buf);

	s_my_mqdt =  mq_open(a_buf , O_CREAT | O_EXCL | O_RDWR , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH , NULL);

	if ( -1 == s_my_mqdt )
	{
		return ERROR_RETURN;
	}

	memset(&s_sigevt , 0 , sizeof(s_sigevt));
	s_sigevt.sigev_notify = SIGEV_THREAD;
	s_sigevt._sigev_un._sigev_thread._function = ref_mq_thread;
	s_sigevt._sigev_un._sigev_thread._attribute = NULL;
	int a_return = mq_notify(s_my_mqdt , &s_sigevt);
	if ( -1 == a_return )
	{
		return ERROR_RETURN;
	}
	return NORMAL_RETURN;
}

int32_t OpenMQ_CHILD(int32_t e_session_id,int e_child_type)
{

	struct mq_attr a_mq_attr;
	trc("[%s: %d] OpenMQ START PID= %d" , __FILE__ , __LINE__ , getppid( ));
	memset(&a_mq_attr , 0 , sizeof(a_mq_attr));
	a_mq_attr.mq_flags = 0;
	a_mq_attr.mq_maxmsg = 30;
	a_mq_attr.mq_msgsize = sizeof(sizeof(char) * 10240);

	char a_buf [1024];
	memset(&a_buf , 0 , sizeof(a_buf));
	switch ( e_child_type )
	{
		case CHILD_CMD:
			sprintf(a_buf , "/proc_child_cmd_%04d" , e_session_id);
			break;
		case CHILD_FILEIF:
			sprintf(a_buf , "/proc_child_fileif_%04d" , e_session_id);
			break;
		case CHILD_TRANS:
			sprintf(a_buf , "/proc_child_trans_%04d" , e_session_id);
			break;
	}
	mq_unlink(a_buf);

	s_my_mqdt = mq_open(a_buf , O_CREAT | O_EXCL | O_RDWR , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH , NULL);

	if ( -1 == s_my_mqdt )
	{
		return ERROR_RETURN;
	}

	memset(&s_sigevt , 0 , sizeof(s_sigevt));
	s_sigevt.sigev_notify = SIGEV_THREAD;
	s_sigevt._sigev_un._sigev_thread._function = ref_mq_thread;
	s_sigevt._sigev_un._sigev_thread._attribute = NULL;
	int a_return = mq_notify(s_my_mqdt , &s_sigevt);
	if ( -1 == a_return )
	{
		return ERROR_RETURN;
	}
	return NORMAL_RETURN;
}

int32_t CloseMQ(int e_src_mq_id , int e_session_id, int e_child_type)
{
	char a_buf [1024];
	memset(&a_buf , 0 , sizeof(a_buf));

	mq_close(s_my_mqdt);

	if ( CMP_NO_CHILD == e_src_mq_id )
	{
		switch ( e_child_type )
		{
			case CHILD_CMD:
				sprintf(a_buf , "/proc_child_cmd_%04d" , e_session_id);
				break;
			case CHILD_FILEIF:
				sprintf(a_buf , "/proc_child_fileif_%04d" , e_session_id);
				break;
			case CHILD_TRANS:
				sprintf(a_buf , "/proc_child_trans_%04d" , e_session_id);
				break;
		}
	}
	else
	{
		sprintf(a_buf , "/proc_%04d" , e_src_mq_id);
	}

	mq_unlink(a_buf);

	return NORMAL_RETURN;
}

int32_t	RefMQ(void)
{
	return s_msg_ref;
}

int32_t SendMQ(int e_dst_mq_id , int e_src_mq_id , void* e_send_msg , int32_t e_msg_size)
{
	struct timespec a_timeout;
	memset(&a_timeout , 0 , sizeof(a_timeout));

	a_timeout.tv_nsec = 500000000;

	char a_buf [256];
	memset(&a_buf , 0 , sizeof(a_buf));
	sprintf(a_buf , "/proc_%04d" , e_dst_mq_id);
	mqd_t a_dst_id = mq_open(a_buf , O_RDWR);
	if ( -1 == a_dst_id )
	{
		return ERROR_RETURN;
	}

	st_mq_header* a_header = (st_mq_header*) e_send_msg;

	a_header->m_dst_id = e_dst_mq_id;
	a_header->m_src_id = e_src_mq_id;
	int a_return = mq_timedsend(a_dst_id , (char*)e_send_msg , e_msg_size , 0 , &a_timeout);
	if ( -1 == a_return )
	{
		return ERROR_RETURN;
	}
	mq_close(a_dst_id);
	return NORMAL_RETURN;
}

int32_t SendMQ_CHILD(int e_dst_mq_id , int e_session_id , int e_child_type , void* e_send_msg , int32_t e_msg_size)
{
	struct timespec a_timeout;
	memset(&a_timeout , 0 , sizeof(a_timeout));

	a_timeout.tv_nsec = 500000000;

	char a_buf [256];
	memset(&a_buf , 0 , sizeof(a_buf));
	mqd_t a_dst_id;
	if ( CMP_NO_CHILD == e_dst_mq_id )
	{
		switch ( e_child_type )
		{
			case CHILD_CMD:
				sprintf(a_buf , "/proc_child_cmd_%04d" , e_session_id);
				break;
			case CHILD_FILEIF:
				sprintf(a_buf , "/proc_child_fileif_%04d" , e_session_id);
				break;
			case CHILD_TRANS:
				sprintf(a_buf , "/proc_child_trans_%04d" , e_session_id);
				break;
		}
	
		a_dst_id = mq_open(a_buf , O_RDWR);
		if ( -1 == a_dst_id )
		{
			return ERROR_RETURN;
		}

		st_mq_header* a_header = ( st_mq_header*) e_send_msg;

		a_header->m_dst_id = CMP_NO_CHILD;
		a_header->m_src_id = CMP_NO_CHILD;
		a_header->m_session_id = e_session_id;
	}
	else
	{
		sprintf(a_buf , "/proc_%04d" , e_dst_mq_id);
		a_dst_id = mq_open(a_buf , O_RDWR);
		if ( -1 == a_dst_id )
		{
			return ERROR_RETURN;
		}

		st_mq_header* a_header = ( st_mq_header*) e_send_msg;

		a_header->m_dst_id = CMP_NO_CHILD;
		a_header->m_src_id = CMP_NO_CHILD;
		a_header->m_session_id = e_session_id;
	}
	int a_return = mq_timedsend(a_dst_id , ( char*) e_send_msg , e_msg_size , 0 , &a_timeout);
	if ( -1 == a_return )
	{
		return ERROR_RETURN;
	}
	mq_close(a_dst_id);
	return NORMAL_RETURN;
}

int32_t RecvMQ(int e_src_mq_id , void* e_recv_msg , int32_t e_msg_size)
{
	//if ( 1024 <= (e_msg_size) )
	//{
	//	trc("[%s: %d] buf over flow" , __FILE__ , __LINE__);
	//	return ERROR_RETURN;
	//}

	struct timespec a_timeout;
	memset(&a_timeout , 0 , sizeof(a_timeout));

	a_timeout.tv_nsec = 500000000;

	char a_buf [256];
	memset(&a_buf , 0 , sizeof(a_buf));
	sprintf(a_buf , "/proc_%04d" , e_src_mq_id);

	//int a_return = mq_timedreceive(s_my_mqdt , e_recv_msg , e_msg_size , 0 , &a_timeout);
	int a_return = mq_receive(s_my_mqdt , e_recv_msg , e_msg_size , 0 );
	if ( -1 == a_return )
	{
		return ERROR_RETURN;
	}
	return NORMAL_RETURN;
}

int32_t RecvMQtimeout(int e_src_mq_id , void* e_recv_msg , int32_t e_msg_size,int32_t e_timeout)
{
	//if ( 1024 <= (e_msg_size) )
	//{
	//	trc("[%s: %d] buf over flow" , __FILE__ , __LINE__);
	//	return ERROR_RETURN;
	//}

	struct timespec a_timeout;
	memset(&a_timeout , 0 , sizeof(a_timeout));

	a_timeout.tv_nsec = e_timeout;

	char a_buf [256];
	memset(&a_buf , 0 , sizeof(a_buf));
	sprintf(a_buf , "/proc_%04d" , e_src_mq_id);

	int a_return = mq_timedreceive(s_my_mqdt , e_recv_msg , e_msg_size , 0 , &a_timeout);
	if ( -1 == a_return )
	{
		if ( ETIMEDOUT == errno )
		{
			return TIMEOUT_RETURN;
		}
		else
		{
			return ERROR_RETURN;
		}
	}
	return NORMAL_RETURN;
}
