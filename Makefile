# プログラム名とオブジェクトファイル名
PROGRAM =	FTPSERVER

CFLAGS :=	-I./include			\
			-D DEBUG_MODE_ON	\
			
OBJS	=	utility.o				\
			command.o				\
			session_cmd_manager.o	\
			session_Trans_manager.o	\
			SessionAPI.o			\
			cmd_thread.o			\
			main.o					\

# 定義済みマクロの再定義
CC = gcc
CFLAGS =	-Wall				\
			-O3					\
			-lrt				\
			-I./include			\
			-D DEBUG_MODE_ON	\
			-std=gnu11			\
			

LDFLAGS =	-O3					\
			-Wall				\
			-pthread			\
			-lrt				\

# サフィックスルール適用対象の拡張子の定義
.SUFFIXES: .c .o

# プライマリターゲット
$(PROGRAM): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROGRAM) $^

# サフィックスルール
.c.o:
	$(CC) $(CFLAGS) -c $<

# ファイル削除用ターゲット
.PHONY: clean
clean:
	$(RM) $(PROGRAM) $(OBJS)