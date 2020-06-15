///////////////////////////////////////////////////////////////////
// @file command.c
// @date 2020/06/21
///////////////////////////////////////////////////////////////////

#include "command.h"

int D_USER(char* e_args);	//実装状況 △ jsonでの実装予定
int D_PASS(char* e_args);	//実装状況 △ jsonでの実装予定
int D_QUIT(char* e_args);	//実装状況 〇
int D_CWD(char* e_args);	//実装状況 〇 DEBUG
int D_CDUP(char* e_args);	//実装状況 〇 DEBUG
int D_REIN(char* e_args);	//実装状況 ×
int D_PORT(char* e_args);	//実装状況 〇 DEBUG
int D_PASV(char* e_args);	//実装状況
int D_TYPE(char* e_args);	//実装状況
int D_RETR(char* e_args);	//実装状況
int D_STOR(char* e_args);	//実装状況
int D_APPE(char* e_args);	//実装状況
int D_RNFR(char* e_args);	//実装状況
int D_RNFO(char* e_args);	//実装状況
int D_ABOR(char* e_args);	//実装状況
int D_DELE(char* e_args);	//実装状況
int D_RMD(char* e_args);	//実装状況
int D_MKD(char* e_args);	//実装状況
int D_PWD(char* e_args);	//実装状況
int D_XPWD(char* e_args);	//実装状況
int D_NLST(char* e_args);	//実装状況
int D_LIST(char* e_args);	//実装状況
int D_SYST(char* e_args);	//実装状況
int D_STAT(char* e_args);	//実装状況
int D_HELP(char* e_args);	//実装状況
int D_NOOP(char* e_args);	//実装状況
int D_FEAT(char* e_args);	//実装状況

static int responce(char* e_res , int e_res_size);
static int responcepsv(int e_port);
#define RESPONCE(e_res) responce(e_res , strlen(e_res))

/// Global
extern __thread int					g_cliantSock;
extern __thread int					g_servSock;
extern __thread st_session_data*	g_my_session_ptr;

static char s_def_dir_name[256] = "/home";

st_command_list g_command_list [] =
{
	//	command			command func
	{	"USER"			,D_USER		},
	{	"PASS"			,D_PASS		},
	{	"QUIT"			,D_QUIT		},
	{	"CWD"			,D_CWD		},
	{	"CDUP"			,D_CDUP		},
	{	"REIN"			,D_REIN		},
	{	"PORT"			,D_PORT		},
	{	"PASV"			,D_PASV		},
	{	"TYPE"			,D_TYPE		},
	{	"RETR"			,D_RETR		},
	{	"STOR"			,D_STOR		},
	{	"APPE"			,D_APPE		},
	{	"RNFR"			,D_RNFR		},
	{	"RNFO"			,D_RNFO		},
	{	"ABOR"			,D_ABOR		},
	{	"DELE"			,D_DELE		},
	{	"RMD"			,D_RMD		},
	{	"MKD"			,D_MKD		},
	{	"PWD"			,D_PWD		},
	{	"XPWD"			,D_XPWD		},
	{	"NLST"			,D_NLST		},
	{	"LIST"			,D_LIST		},
	{	"SYST"			,D_SYST		},
	{	"STAT"			,D_STAT		},
	{	"HELP"			,D_HELP		},
	{	"NOOP"			,D_NOOP		},
	{	"FEAT"			,D_FEAT		},
	{	0				,NULL		}
};

int D_USER(char* e_args)
{
	// TODO:jsonファイルに最終的にする
	static char user [] = "user";

	if ( 0 == strcmp(user , e_args) )
	{
		// PASS一致
		return RESPONCE(RES_PLS_PASS);

	}
	else
	{
		// PASS不一致
		return RESPONCE(RES_FAIL_ROGIN);
	}
	return ERROR_RETURN;
}

int D_PASS(char* e_args)
{
	// TODO:jsonファイルに最終的にする
	static char pass [] = "pass";

	if ( 0 == strcmp(pass , e_args) )
	{
		// デフォルトカレント設定
		chdir(s_def_dir_name);
		// PASS一致
		return RESPONCE(RES_ROGIN_SUCCESS);

	}
	else
	{
		// PASS不一致
		return RESPONCE(RES_FAIL_ROGIN);
	}
	return ERROR_RETURN;
}

int D_QUIT(char* e_args)
{
	trc("[%s: %d] select QUIT" , __FILE__ , __LINE__);
	g_my_session_ptr->m_session_flags.m_QUIT_flags = 1;
	return NORMAL_RETURN;
}

int D_CWD(char* e_args)
{
	char* a_realpath = NULL;
	if ( NULL == realpath(e_args , a_realpath) )
	{
		return RESPONCE(RES_FAIL_CD);
	}

	// ワーキングディレクトリ変更
	if ( 0 == chdir(e_args) )
	{
		return RESPONCE(RES_REQ_END);
	}
	else
	{
		return RESPONCE(RES_FAIL_CD);
	}

	return ERROR_RETURN;
}

int D_CDUP(char* e_args)
{
	char* a_realpath = NULL;
	if ( NULL == realpath(".." , a_realpath) )
	{
		return RESPONCE(RES_FAIL_CD);
	}

	// ディレクトリが存在するか確認
	if ( 0 == chdir("a_realpath") )
	{
		free(a_realpath);
		return RESPONCE(RES_REQ_END);
	}
	else
	{
		free(a_realpath);
		return RESPONCE(RES_FAIL_CD);
	}

	return ERROR_RETURN;
}

int D_REIN(char* e_args)
{
	return RESPONCE(RES_COMMAND_NONE);
}

// XXX:せぐふぉ対策してない
int D_PORT(char* e_args)
{
	g_my_session_ptr->m_session_flags.m_pasv_flags = PASV_PORT;
	int a_addr_get_done = 0;
	int a_index = 0;
	char a_buf [4];
	int a_buf_index = 0;
	int a_port_high = 0;
	int a_port_high_done = 0;
	int a_port_row = 0;

	int a_timeout = 0;
	memset(&a_buf , 0 , sizeof(a_buf));
	while ( 1 )
	{
			// ip addr
		if ( '(' == *e_args ||
			 ')' == *e_args ||
			 ' ' == *e_args)
		{
			e_args++;
			continue;
		}

		if ( ',' == *e_args )
		{
			if ( 0 != a_buf [0] )
			{
				if ( 0 == a_addr_get_done )
				{
					g_my_session_ptr->m_session_flags.m_cliant_addr [a_index] = atoi(a_buf);

					memset(&a_buf , 0 , sizeof(a_buf));
					a_buf_index = 0;
					if ( 3 <= a_index )
					{
						a_addr_get_done = 1;
					}
					else
					{
						a_index++;
					}
				}
				else
				{
					if ( a_port_high_done == 0 )
					{
						a_port_row = atoi(a_buf);

						g_my_session_ptr->m_session_flags.m_cliant_port = ((a_port_high * 255) + a_port_row);
						break;
					}
					else
					{
						a_port_high = atoi(a_buf);
					}
				}
			}
		}

		a_buf [a_buf_index] = *e_args;
		a_buf_index++;

		a_timeout++;

		if ( 300 == a_timeout )
		{
			return ERROR_RETURN;;
		}
	}

	g_my_session_ptr->m_session_flags.m_port_req_flag = 1;

	return NORMAL_RETURN;
}

int D_PASV(char* e_args)
{
	g_my_session_ptr->m_session_flags.m_pasv_flags = PASV_ON;
	g_my_session_ptr->m_session_flags.m_port_req_flag = 1;
	return NORMAL_RETURN;
}

int D_TYPE(char* e_args)
{
	if ( 0 == strcmp("A" , e_args) )
	{
		g_my_session_ptr->m_session_flags.m_typemode_flag = TYPE_ASCII;
		return RESPONCE(RES_COMMAND_OK_A);
	}
	else if ( 0 == strcmp("B" , e_args) )
	{
		g_my_session_ptr->m_session_flags.m_typemode_flag = TYPE_BINARY;
		return RESPONCE(RES_COMMAND_OK_B);
	}
	else
	{
		return RESPONCE(RES_NOT_ARGS);
	}

	return ERROR_RETURN;
}

int D_RETR(char* e_args)
{
	return 0;
}

int D_STOR(char* e_args)
{
	return 0;
}

int D_APPE(char* e_args)
{
	return RESPONCE(RES_COMMAND_NONE);
}

int D_RNFR(char* e_args)
{
	struct stat a_dummy [256];
	char a_temp_buf [256];
	memset(a_temp_buf , 0 , sizeof(a_temp_buf));

	if ( '.' == *e_args )
	{
		// 相対パスの場合
		e_args++;
		char a_current_dir [256];
		// カレントディレクトリ文字列取得
		getcwd(a_current_dir , sizeof(a_current_dir));
		// カレントディレクトリ文字列とファイル名を結合
		sprintf(a_temp_buf , "%s%s" , a_current_dir , e_args);
	}
	else
	{
		// 絶対パスの場合
		memcpy(a_temp_buf , e_args , (sizeof(char) * 256));
	}

	if ( 0 == stat(a_temp_buf , a_dummy) )
	{
		// ファイルが存在する
		return RESPONCE(RES_READY_RNFO);
	}
	else
	{
		return RESPONCE(RES_COMMAND_ARGS);
	}
	return ERROR_RETURN;
}

int D_RNFO(char* e_args)
{

	return RESPONCE(RES_COMMAND_ARGS);
}

int D_ABOR(char* e_args)
{
	return RESPONCE(RES_COMMAND_NONE);
}

int D_DELE(char* e_args)
{
	char a_temp_buf [256];
	memset(a_temp_buf , 0 , sizeof(a_temp_buf));

	if ( '.' == *e_args )
	{
		// 相対パスの場合
		e_args++;
		char a_current_dir [256];
		// カレントディレクトリ文字列取得
		getcwd(a_current_dir , sizeof(a_current_dir));
		// カレントディレクトリ文字列とファイル名を結合
		sprintf(a_temp_buf , "%s%s" , a_current_dir , e_args);
	}
	else
	{
		// 絶対パスの場合
		memcpy(a_temp_buf , e_args , (sizeof(char) * 256));
	}

	if ( 0 == remove(a_temp_buf) )
	{
		// ファイルが存在する
		if ( 0 == remove(a_temp_buf) )
		{
			return RESPONCE(RES_REQ_END);
		}
		else
		{
			return RESPONCE(RES_COMMAND_ARGS);
		}
	}
	else
	{
		return RESPONCE(RES_COMMAND_ARGS);
	}

	return ERROR_RETURN;
}

int D_RMD(char* e_args)
{
	char a_temp_buf [256];
	memset(a_temp_buf , 0 , sizeof(a_temp_buf));

	if ( '.' == *e_args )
	{
		// 相対パスの場合
		e_args++;
		char a_current_dir [256];
		// カレントディレクトリ文字列取得
		getcwd(a_current_dir , sizeof(a_current_dir));
		// カレントディレクトリ文字列とファイル名を結合
		sprintf(a_temp_buf , "%s%s" , a_current_dir , e_args);
	}
	else
	{
		// 絶対パスの場合
		memcpy(a_temp_buf , e_args , (sizeof(char) * 256));
	}

	if ( 0 == rmdir(a_temp_buf) )
	{
		return RESPONCE(RES_REQ_END);
	}
	else
	{
		return RESPONCE(RES_COMMAND_ARGS);
	}
	

	return RESPONCE(RES_COMMAND_ARGS);
}

int D_MKD(char* e_args)
{
	char a_temp_buf [256];
	memset(a_temp_buf , 0 , sizeof(a_temp_buf));

	if ( '.' == *e_args )
	{
		// 相対パスの場合
		e_args++;
		char a_current_dir [256];
		// カレントディレクトリ文字列取得
		getcwd(a_current_dir , sizeof(a_current_dir));
		// カレントディレクトリ文字列とファイル名を結合
		sprintf(a_temp_buf , "%s%s" , a_current_dir , e_args);
	}
	else
	{
		// 絶対パスの場合
		memcpy(a_temp_buf , e_args , (sizeof(char) * 256));
	}

	if ( 0 == rmdir(a_temp_buf) )
	{
		return RESPONCE(RES_REQ_END);
	}
	else
	{
		return RESPONCE(RES_COMMAND_ARGS);
	}

	return RESPONCE(RES_COMMAND_ARGS);
}

int D_PWD(char* e_args)
{
	char a_buf [256];
	char a_current_dir [256];
	memset(a_buf , 0 , sizeof(a_buf));
	memset(a_current_dir , 0 , sizeof(a_current_dir));

	getcwd(a_current_dir , sizeof(a_current_dir));
	sprintf(a_buf , RES_PWD , a_current_dir);

	return RESPONCE(a_buf);
}

int D_XPWD(char* e_args)
{
	return D_PWD(e_args);
}

int D_NLST(char* e_args)
{
	return RESPONCE(RES_COMMAND_NONE);
}

int D_LIST(char* e_args)
{
	return 0;
}

int D_SYST(char* e_args)
{
	return RESPONCE(RES_SYST);
}

int D_STAT(char* e_args)
{
	return RESPONCE(RES_COMMAND_NONE);;
}

int D_HELP(char* e_args)
{
	return RESPONCE(RES_COMMAND_NONE);;
}

int D_NOOP(char* e_args)
{
	return RESPONCE(RES_REQ_END);;
}

int D_FEAT(char* e_args)
{
	return RESPONCE(RES_COMMAND_NONE);
}

static int responce(char* e_res,int e_res_size)
{
	// 送信
	int a_return = send(g_cliantSock , e_res , e_res_size , MSG_DONTWAIT);
	if ( 0 == a_return )
	{
		trc("[%s: %d] send error" , __FILE__ , __LINE__);
		return ERROR_RETURN;
	}
	return 0;
}

static int responcepsv(int e_port)
{
	char a_buf [256];
	sprintf(a_buf,RES_PASV , 192 , 168 , 1 , 244 , e_port / 256 , (e_port - (e_port % 256)));
	return RESPONCE(a_buf);
}

int Respsv(void)
{
	return responcepsv(g_my_session_ptr->m_port);
}

int Respsvfail(void)
{
	return RESPONCE(RES_COMMAND_ARGS);
}