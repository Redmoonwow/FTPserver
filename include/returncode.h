///////////////////////////////////////////////////////////////////
// @file returncode.h
// @date 2020/06/21
///////////////////////////////////////////////////////////////////

#ifndef __RETURNCODE_H_
#define __RETURNCODE_H_
// RESTコマンドのためのマーカー返答である
#define RES_REST						"110 Restart marker reply.\r\n" 
// サービスは停止しているが、nnn分後に準備できる
#define RES_NOTSERVICE					"120 Service ready in %d minutes.\r\n"
// データコネクションはすでに確立されている。このコネクションで転送を開始する
#define RES_ALREADY_DATA_CONNET			"125 Data connection already open; transfer starting.\r\n" 
// ファイルステータスは正常である。データコネクションを確立する
#define RES_DATA_CONECT					"150 File status okay; about to open data connection.\r\n"
// ディレクトリ情報送る
#define RES_LIST_NLIST					"150 Here comes the directory listing.\r\n"
// データを送る RETR
#define RES_RETR						"150 Opening BINARY mode data connection for %s (%ld bytes)...\r\n"
// データを送る STOR
#define RES_STOR						"150 OK to send data.\r\n"
// コマンドは正常に受け入れられた
#define RES_COMMAND_OK					"200 Command okay."
#define RES_COMMAND_OK_B				"200 Okay Binary mode"
#define RES_COMMAND_OK_A				"200 Okay ASCII mode recommand Binary mode\r\n"
// コマンドは実装されていない。SITEコマンドでOSコマンドが適切でない場合など
#define RES_COMMAND_NOT					"202 Command not implemented , superfluous at this site.\r\n"
// STATコマンドに対するレスポンス
#define RES_STAT						"211 System status , or system help reply.\r\n"
// STATコマンドによるディレクトリ情報を示す
#define RES_STAT_DIR					"212 Directory status.\r\n" 
// STATコマンドによるファイル情報を示す
#define RES_STAT_FILE					"213 File status.\r\n" 
// HELPコマンドに対するレスポンス
#define RES_HELP						"214 Help message.\r\n" 
// SYSTコマンドに対するレスポンス
#define RES_SYST						"215 UNIX system type.\r\n" 
// 新規ユーザー向けに準備が整った。ログイン時に表示される場合を想定している
#define RES_CONNECT_DONE				"220 Service ready for new user.\r\n" 
// コントロールコネクションを切断する。QUITコマンド時のレスポンス
#define RES_CLOSE_CONNECT				"221 Service closing control connection.\r\n" 
// データコネクションを確立した。データの転送は行われていない
#define RES_DATA_CONNECT_DONE			"225 Data connection open; no transfer in progress.\r\n" 
// 要求されたリクエストは成功した。データコネクションをクローズする
#define RES_REQ_DONE_CLOSE_DATA_CONNECT	"226 Closing data connection.\r\n" 
#define RES_REQ_DONE_STOR_RETR			"226 Transfer Complete.\r\n" 
#define RES_REQ_DONE_NLIST_LIST			"226 Directory send ok.\r\n" 
// PASVコマンドへのレスポンス。h1～h4はIPアドレス、p1～p2はポート番号を示す
#define RES_PASV						"227 Entering Passive Mode(%d , %d , %d , %d , %d , %d).\r\n" 
// ユーザーログインの成功
#define RES_ROGIN_SUCCESS				"230 User logged in , proceed.\r\n" 
// 要求されたコマンドによる操作は正常終了した
#define RES_REQ_END						"250 Requested file action okay , completed.\r\n" 
// ファイルやディレクトリを作成したというのがRFCでの意味だが、MKDコマンドの結果以外にも、実際にはPWDコマンドの結果にも用いられる
#define RES_CREATED						"257 %s created.\r\n" 
// PWD応答
#define RES_PWD							"257 %s HERE.\r\n" 
// パスワードの入力を求める
#define RES_PLS_PASS					"331 User name okay , need password.\r\n" 
// ACCTコマンドで課金情報を指定する必要がある
#define RES_ACCT						"332 Need account for login.\r\n" 
// 他の何らかの情報を求めている
#define RES_PLS_INFO					"350 Requested file action pending further information\r\n" 
// RNFO待ち
#define RES_READY_RNFO					"350 Ready for RNFO\r\n" 
//サービスを提供できない。コントロールコネクションを終了する。サーバのシャットダウン時など
#define RES_NOT_SERVICE_NOW				"421 Service not available , closing control connection.\r\n" 
// データコネクションをオープンできない
#define RES_FAIL_DATA_CONNECT			"425 Can't open data connection.\r\n" 
// 何らかの原因により、コネクションをクローズし、データ転送も中止した
#define RES_ABORT						"426 Connection closed; transfer aborted.\r\n" 
// ローカルエラーのため処理を中止した
#define RES_ERROR						"451 Requested action aborted.Local error in processing.\r\n" 
// ディスク容量の問題で実行できない
#define RES_DISK_ERRO					"452 Requested action not taken.\r\n" 
// コマンドの文法エラー
#define RES_COMMAND_ERROR				"500 Syntax error , command unrecognized.\r\n" 
// 引数やパラメータの文法エラー
#define RES_COMMAND_ARGS				"501 Syntax error in parameters or arguments.\r\n" 
// コマンドは未実装である
#define RES_COMMAND_NONE				"502 Command not implemented.\r\n" 
// コマンドを用いる順番が間違っている
#define RES_BAD_SEQ						"503 Bad sequence of commands.\r\n" 
// 引数やパラメータが未実装
#define RES_NOT_ARGS					"504 Command not implemented for that parameter.\r\n" 
// ユーザーはログインできなかった
#define RES_FAIL_ROGIN					"530 Not logged in.\r\n" 
// ファイル送信には、ACCTコマンドで課金情報を確認しなくてはならない
#define RES_PLS_ACCT					"532 Need account for storing files.\r\n" 
// 要求されたリクエストはアクセス権限やファイルシステムの理由で実行できない
#define RES_NOT_PERM					"450 Requested file action not taken.\r\n" 
#define RES_NOT_FILE					"550 Requested action not taken.\r\n"
#define RES_FAILED						"550 Requested FAILED\r\n"
// ディレクトリがなんらかの理由で変更できなかった
#define RES_FAIL_CD						"550 fail changed directory\r\n"
// ページ構造のタイプの問題で実行できない
#define RES_UNK_PAGE					"551 Requested action aborted.Page type unknown.\r\n" 
// ディスク容量の問題で実行できない
#define RES_ERROR_DISK					"552 Requested file action aborted.\r\n" 
// ファイル名が間違っているため実行できない
#define RES_FILE_NAME_ERROR				"553 Requested action not taken.\r\n" 

#endif