///////////////////////////////////////////////////////////////////
// @file ftp_common.h
// @date 2020/06/21
///////////////////////////////////////////////////////////////////

#ifndef _FTP_COMMON_H_
#define _FTP_COMMON_H_

#define NORMAL_RETURN	0
#define ERROR_RETURN	-1
#define TIMEOUT_RETURN	-2
#define BUSY_RETURN		-3
#define RESET_RETURN	-4
#define END_RETURN		-5

#define PASV_ON		1
#define PASV_OFF	0

#define TYPE_BINARY 0
#define TYPE_ASCII  1

#define CANCEL_COMMAND_ON 1

#define CMD_STOR	1
#define CMD_RETR	0
#define CMD_NLIST	2
#define CMD_LIST	3


// ŒÅ’èID

#define CMP_NO_INIT_MANAGER_ID			0
#define CMP_NO_SESSION_COMMAND_ID		1
#define CMP_NO_SESSION_TRANS_ID			2
#define CMP_NO_SIGNAL_HANDLER			3
#define CMP_NO_CHILD					0xFFFFFFF

#define CHILD_CMD						0
#define CHILD_TRANS						1
#define CHILD_FILEIF					2

#endif
