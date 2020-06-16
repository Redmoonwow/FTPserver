///////////////////////////////////////////////////////////////////
// @file utility.cpp
// @date 2020/06/21
///////////////////////////////////////////////////////////////////

#include "utility.h"

static int Gettimeprintformat(char* e_time_ptr , int32_t e_ptr_datasize); // 現時刻取得

static FILE*					s_trc_ptr;								// トレースログファイル
static const int8_t				s_template_filename [] = "FTPSERVER";
static int32_t					s_template_file_no = 0;
static char						s_trc_filename [256];
static uint8_t					s_Init_done = 0;
static __thread	uint32_t		s_msg_ref;
static __thread struct sigevent s_sigevt;
static __thread mqd_t			s_my_mqdt = 0;

typedef struct st_queue_header
{
	char* m_pre_ptr;
	char* m_next_ptr;
	char* m_data;
}st_queue_body;

/// ユーティリティAPI初期化
int InitUtilitis(void)
{
	struct timespec a_now_time;		// 取得時間
	struct tm a_localtime;			// 変換時間
	struct stat a_filesize;

	// convert char type
	localtime_r(&(a_now_time.tv_sec) , &a_localtime);

	memset(s_trc_filename , 0 , sizeof(s_trc_filename));
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
	trc("[%s: %d] OpenMQ START" , __FILE__ , __LINE__);
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

	trc("[%s: %d] OpenMQ START" , __FILE__ , __LINE__ );
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

// キューAPI

#define IS_FULL 0xF0000001
#define IS_EMPTY 0xF0000002
#define IS_NONE	0xF0000003

st_queue* QUEUE_init(uint32_t e_size , int e_size_unit , uint32_t e_create_number)
{
	st_queue* a_queue_ptr = NULL;
	st_queue_body* a_queue_body_ptr = NULL;
	st_queue_body* a_queue_body_next_ptr = NULL;
	uint64_t a_create_num = 0;

	switch ( e_size_unit )
	{
		case BYTE_B:
			a_create_num = 1;
			break;
		case BYTE_KB:
			a_create_num = 1 * 1000;
			break;
		case BYTE_MB:
			a_create_num = 1 * 1000 * 1000;
			
		default:
			trc("[%s: %d] QUEUE create num unknown" , __FILE__ , __LINE__);
			return NULL;
	}

	a_queue_ptr = (st_queue*)malloc(sizeof(st_queue));
	if ( NULL == a_queue_ptr )
	{
		trc("[%s: %d] malloc error" , __FILE__ , __LINE__);
		return NULL;
	}

	memset(a_queue_ptr , 0 , sizeof(st_queue));

	// 最初だけ単体で取得する
	a_queue_body_ptr = (st_queue_body*) malloc(sizeof(st_queue_body));
	if ( NULL == a_queue_ptr )
	{
		free(a_queue_ptr);
		trc("[%s: %d] malloc error" , __FILE__ , __LINE__);
		return NULL;
	}
	memset(a_queue_body_ptr , 0 , sizeof(st_queue_body));
	a_queue_body_ptr->m_pre_ptr = NULL;

	a_queue_body_ptr->m_data = malloc(e_size * a_create_num);
	if ( NULL == a_queue_ptr )
	{
		free(a_queue_ptr);
		trc("[%s: %d] malloc error" , __FILE__ , __LINE__);
		return NULL;
	}
	memset(a_queue_body_ptr->m_data , 0 , sizeof(e_size * a_create_num));

	a_queue_ptr->m_empty_top_ptr = a_queue_body_ptr;

	int a_index;
	for ( a_index = 0 ; a_index >= e_create_number - 2 ;a_index++)
	{
		// 次のキュー管理データを生成
		a_queue_body_next_ptr = (st_queue_body*) malloc(sizeof(st_queue_body));
		if ( NULL == a_queue_ptr )
		{
			free(a_queue_ptr);
			trc("[%s: %d] malloc error" , __FILE__ , __LINE__);
			return NULL;
		}

		memset(a_queue_body_next_ptr , 0 , sizeof(st_queue_body));

		// キューデータを生成
		a_queue_body_next_ptr->m_data = (char*)malloc(e_size * a_create_num);
		if ( NULL == a_queue_ptr )
		{
			free(a_queue_ptr);
			trc("[%s: %d] malloc error" , __FILE__ , __LINE__);
			return NULL;
		}
		memset(a_queue_body_next_ptr->m_data , 0 , sizeof(e_size * a_create_num));

		// 前のキュー管理に次のポインタアドレスを代入
		a_queue_body_ptr->m_next_ptr = a_queue_body_next_ptr;

		// 次のキュー管理に前のポインタアドレスを代入
		a_queue_body_next_ptr->m_pre_ptr = a_queue_body_ptr;

		// 次のキュー管理を保持
		a_queue_body_ptr = NULL;
		a_queue_body_ptr = a_queue_body_next_ptr;
		a_queue_body_next_ptr = NULL;

	}

	a_queue_body_ptr->m_next_ptr = NULL;
	a_queue_ptr->m_empty_tail_ptr = a_queue_body_ptr;
	a_queue_ptr->m_data_size = (e_size * a_create_num);

	return a_queue_ptr;
}

uint32_t QUEUE_push(st_queue* e_queue , void* e_data)
{
	st_queue_body* a_insert_data_ptr = NULL;
	st_queue_body* a_insert_data_temp_ptr = NULL;
	uint64_t a_size = e_queue->m_data_size;

	// 空きキューがあるか確認
	if ( NULL == e_queue->m_empty_top_ptr )
	{
		return IS_FULL;
	}

	// データを切り離す
	a_insert_data_ptr = e_queue->m_empty_top_ptr;
	a_insert_data_temp_ptr = a_insert_data_ptr->m_next_ptr;

	a_insert_data_ptr->m_next_ptr = NULL;
	a_insert_data_temp_ptr->m_pre_ptr = NULL;

	// データを挿入
	memcpy(a_insert_data_ptr->m_data , e_data , sizeof(a_size));

	// データの連結

	// 一つも使用済みがない
	if ( NULL == e_queue->m_top_ptr )
	{
		e_queue->m_top_ptr = a_insert_data_ptr;
	}
	else
	{
		a_insert_data_temp_ptr = e_queue->m_top_ptr;

		while ( NULL == a_insert_data_temp_ptr->m_next_ptr )
		{
			a_insert_data_temp_ptr = a_insert_data_temp_ptr->m_next_ptr;
		}

		a_insert_data_temp_ptr->m_next_ptr = a_insert_data_ptr;

		a_insert_data_ptr->m_pre_ptr = a_insert_data_temp_ptr;
	}

	return NORMAL_RETURN;
}

int32_t QUEUE_pop(st_queue* e_queue , char** e_data_ptr)
{
	st_queue_body* a_data_ptr = NULL;
	st_queue_body* a_data_temp_ptr = NULL;

	uint64_t a_size = e_queue->m_data_size;

	// 使用キューがあるか確認
	if ( NULL == e_queue->m_top_ptr )
	{
		return NULL;
	}

	// データをコピー
	a_data_ptr = e_queue->m_top_ptr;
	a_data_temp_ptr = a_data_ptr->m_next_ptr;
	memcpy(*e_data_ptr , a_data_ptr->m_data , sizeof(e_queue->m_data_size));

	// 使用済先頭ポインタを切り離す
	e_queue->m_top_ptr = NULL;
	e_queue->m_top_ptr = a_data_temp_ptr;
	a_data_temp_ptr->m_pre_ptr = NULL;

	// 未使用ポインタの最後尾に連結
	memset(a_data_ptr , 0 , sizeof(st_queue_body));
	a_data_temp_ptr = e_queue->m_empty_tail_ptr;
	a_data_temp_ptr->m_next_ptr = a_data_ptr;
	a_data_ptr->m_pre_ptr = a_data_temp_ptr;
	e_queue->m_empty_tail_ptr = a_data_ptr;

	return NORMAL_RETURN;
}

int32_t QUEUE_isEmpty(st_queue* e_queue)
{
	if ( NULL == e_queue->m_top_ptr &&
		NULL == e_queue->m_tail_ptr )
	{
		return IS_EMPTY;
	}
	return IS_NONE;
}

int32_t QUEUE_isFull(st_queue* e_queue)
{
	if ( NULL == e_queue->m_empty_top_ptr &&
		NULL == e_queue->m_empty_tail_ptr )
	{
		return IS_FULL;
	}
	return IS_NONE;
}

int32_t QUEUE_isAvailable(st_queue* e_queue)
{
	return QUEUE_isFull(e_queue);
}

