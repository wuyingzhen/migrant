################################################################
# migrant file 系统makefile 
# by wuyingzhen@hotmail.com                           
#                                                               
#################################################################
CC = gcc

#======================================================
# 定义一般可执行程序的名称及所依赖的目标(.o)文件，
# 如果有多个，依次以EXE1、EBJ1，EXE2、EBJ2表示。
#======================================================
EXE1 = missue.out
OBJ1 = migrant.o thread.o  util.o  migrantlog.o myhiredis.o  readini.o  issue.o collect.o file.o

#路径设置
MY_HOME=/data/cts/wuyz

#头文件
APPIFLAG = -I.\
           -I$(MY_HOME)/include\
           -I/home/cts/cts1-new/include/\
          

#共享库
APPLFLAG = -L$(MY_HOME)/lib\
           -lcrypto \
           -levent  \
           -lpthread \
           -lhiredis    \
           -llua       \
           -ldl

#屏蔽一种警告
NO_WARN =  -Wno-write-strings


#和总
EXE = $(EXE1) 
OBJ = $(OBJ1)
IFLAGS = $(APPIFLAG)
LFLAGS = $(APPLFLAG)
 





#----------------------------------------------------------------
.c.o:
	@echo 
	@echo Compiling	$<...


	$(CC)  $(NO_WARN)  $(IFLAGS)  -c $*.c 
#	@echo
	@echo "=============== $@ successfully built =============== "

#----------------------------------------------------------------

###########################   process   ###########################
$(EXE):$(OBJ)
	@echo

	$(CC)    -o$@  $?  $(LFLAGS)
	rm -f $?
	cp $@ $(MY_HOME)/bin
	@echo
	@echo "===============  $@ successfully built =============== "
	
clean:
	rm -f *.o  core *.lis $ $(EXE)

