///////////////////////////////////////////////////////////////////
// @file command.h
// @date 2020/06/21
///////////////////////////////////////////////////////////////////

#ifndef COMMAND_H_
#include "headers.h"

typedef struct
{
	char*		command;						// マッチングするコマンド文字列
	int			(* Do_command)(char* e_args);
}st_command_list;

extern st_command_list g_command_list[];

extern int D_USER(char* e_args);
extern int D_PASS(char* e_args);
extern int D_QUIT(char* e_args);
extern int D_CWD(char* e_args);
extern int D_CDUP(char* e_args);
extern int D_REIN(char* e_args);
extern int D_PORT(char* e_args);
extern int D_PASV(char* e_args);
extern int D_TYPE(char* e_args);
extern int D_RETR(char* e_args);
extern int D_STOR(char* e_args);
extern int D_APPE(char* e_args);
extern int D_RNFR(char* e_args);
extern int D_RNFO(char* e_args);
extern int D_ABOR(char* e_args);
extern int D_DELE(char* e_args);
extern int D_RMD(char* e_args);
extern int D_MKD(char* e_args);
extern int D_PWD(char* e_args);
extern int D_XPWD(char* e_args);
extern int D_NLST(char* e_args);
extern int D_LIST(char* e_args);
extern int D_SYST(char* e_args);
extern int D_STAT(char* e_args);
extern int D_HELP(char* e_args);
extern int D_NOOP(char* e_args);
extern int D_FEAT(char* e_args);
extern int Respsv(void);
extern int Respsvfail(void);
extern int responce(char* e_res , int e_res_size);
#endif



