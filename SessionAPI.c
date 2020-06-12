///////////////////////////////////////////////////////////////////
// @file sessionAPI.c
// @date 2020/06/21
///////////////////////////////////////////////////////////////////

#include "SessionAPI.h"

st_session_data* g_session_data_ptr [SESSION_SUPPORT_MAX];

/////////////////////////////////////////////////////////////////////////
//
// Session API
//
/////////////////////////////////////////////////////////////////////////
int32_t InitSession(void)
{
	trc("[%s: %d] InitSession start" , __FILE__ , __LINE__);
	int a_index = 0;
	for ( a_index = 0; a_index < SESSION_SUPPORT_MAX; a_index++ )
	{
		g_session_data_ptr [a_index] = ( st_session_data*) malloc(sizeof(st_session_data));
		memset(g_session_data_ptr [a_index] , 0 , sizeof(st_session_data));
		trc("[%s: %d] g_session_data_ptr %d = %p" , __FILE__ , __LINE__, a_index, g_session_data_ptr [a_index]);
		if ( NULL == g_session_data_ptr )
		{
			return ERROR_RETURN;
		}
	}
	return NORMAL_RETURN;
}

int32_t FreeSession(void)
{
	trc("[%s: %d] FreeSession start" , __FILE__ , __LINE__);
	int a_index = 0;
	for ( a_index = 0; a_index < SESSION_SUPPORT_MAX; a_index++ )
	{
		free(g_session_data_ptr [a_index]);
	}
	return NORMAL_RETURN;
}

int32_t ClearSession(int32_t e_session_id,int e_thread_type)
{
	trc("[%s: %d] ClearSession start" , __FILE__ , __LINE__);
	int a_index = 0;
	// session id 検索
	for ( a_index = 0; a_index <= SESSION_SUPPORT_MAX; a_index++ )
	{
		if ( e_session_id == g_session_data_ptr[a_index]->m_session_id )
		{
			break;
		}
	}

	if ( SESSION_SUPPORT_MAX == a_index )
	{
		trc("[%s: %d] session not found" , __FILE__ , __LINE__);
		return ERROR_RETURN;
	}

	switch ( e_thread_type )
	{
		case CHILD_CMD:
			g_session_data_ptr [a_index]->m_command_thread_id = 0;
			memset(&g_session_data_ptr [a_index]->m_command_attr , 0 , sizeof(pthread_attr_t));
			break;
		case CHILD_TRANS:
			g_session_data_ptr [a_index]->m_trans_thread_id = 0;
			memset(&g_session_data_ptr [a_index]->m_trans_thread_attr , 0 , sizeof(pthread_attr_t));
			g_session_data_ptr [a_index]->m_fileif_thread_id = 0;
			memset(&g_session_data_ptr [a_index]->m_fileif_thread_attr , 0 , sizeof(pthread_attr_t));
			break;
		default:
			break;
	}

	if ( 0 == g_session_data_ptr [a_index]->m_command_thread_id &&
		 0 == g_session_data_ptr [a_index]->m_trans_thread_id &&
		 0 == g_session_data_ptr [a_index]->m_fileif_thread_id )
	{
		memset(g_session_data_ptr [a_index] , 0 , sizeof(st_session_data));
		return NORMAL_RETURN;
	}

	return ERROR_RETURN;
}

st_session_data* CreateSession(void)
{
	trc("[%s: %d] CreateSession start" , __FILE__ , __LINE__);
	static uint32_t s_cnt = 1;
	int a_index = 0;
	// session id 検索
	for ( a_index = 0; a_index <= SESSION_SUPPORT_MAX; a_index++ )
	{
		if ( 0 == g_session_data_ptr [a_index]->m_session_id )
		{
			break;
		}
	}

	if ( 60 == a_index )
	{
		trc("[%s: %d] All session used" , __FILE__ , __LINE__);
		return NULL;
	}
	else
	{
		if ( 0xFFFFF == s_cnt )
		{
			if ( 0 == g_session_data_ptr [a_index + 1]->m_session_id )
			{
				s_cnt = 1;
			}
			else
			{
				trc("[%s: %d] scnt full used" , __FILE__ , __LINE__);
				return NULL;
			}
		};
		memset(g_session_data_ptr [a_index] , 0 , sizeof(st_session_data));
		g_session_data_ptr [a_index]->m_session_id = s_cnt;
		s_cnt++;
		return g_session_data_ptr [a_index];
	}
	trc("[%s: %d] CreateSession end" , __FILE__ , __LINE__);
}

st_session_data* SearchSession(int32_t e_session_id)
{
	int a_index = 0;
	// session id 検索
	for ( a_index = 0; a_index <= SESSION_SUPPORT_MAX; a_index++ )
	{
		if ( e_session_id == g_session_data_ptr [a_index]->m_session_id )
		{
			break;
		}
	}

	if ( 20 == a_index )
	{
		trc("[%s: %d] session not found" , __FILE__ , __LINE__);
		return NULL;
	}
	else
	{
		return g_session_data_ptr[a_index];
	}
}

/*
 * 以下の関数は戻り値が特殊で
 * 存在するセッションIDをすべてコールごとに1つずつ返す
 *　例）稼働セッション４つの場合 2 -> 5 -> 4 -> 3 -> 0 -> 2 -> 5 -> 4 -> 3 -> 0
 * 稼働セッションがすでに報告が終わっているならば0を返す
 */

int32_t GetUsedSessionID(void)
{
	static __thread int s_index = 0;
	static __thread int s_first_return_id = 0;
	// session id 検索
	for ( ;; )
	{
		if ( 0 != g_session_data_ptr [s_index]->m_session_id )
		{
			if ( 0 == s_first_return_id )
			{
				s_first_return_id = g_session_data_ptr [s_index]->m_session_id;
			}
			else if(s_first_return_id == g_session_data_ptr [s_index]->m_session_id)
			{
				s_first_return_id = 0;
				return 0;
			}

			return g_session_data_ptr [s_index]->m_session_id;
		}
		
		s_index++;

		if ( s_index <= SESSION_SUPPORT_MAX )
		{
			s_index = 0;
		}
	}
}

#if 0
int32_t WakeThreadCommand(st_session_data* e_session_ptr)
{
	if ( NULL == e_session_ptr )
	{
		return ERROR_RETURN;
	}


}

int32_t WakeThreadDATA(st_session_data* e_session_ptr)
{
	if ( NULL == e_session_ptr )
	{
		return ERROR_RETURN;
	}
}
#endif