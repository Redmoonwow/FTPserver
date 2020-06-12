# プログラム名とオブジェクトファイル名
PROGRAM =	FTPSERVER

CFLAGS :=	-I./include			\
			-D DEBUG_MODE_ON	\
			
SOURCES=$(wildcard src/*.c)
OBJECTS=$(addprefix obj/,$(notdir $(SOURCES:.c=.o)))

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
$(PROGRAM): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $(PROGRAM) $^

.SUFFIXES: .o .c

# サフィックスルール
#.c.o:
obj/%.o : src/%.c
	$(CC) $(CFLAGS) -o $@ -c $<


# ファイル削除用ターゲット
.PHONY: clean
clean:
	$(RM) $(PROGRAM) $(OBJECTS)