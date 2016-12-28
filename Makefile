################################################################
# migrant file ϵͳmakefile 
# by wuyingzhen@hotmail.com                           
#                                                               
#################################################################
CC = gcc

#======================================================
# ����һ���ִ�г�������Ƽ���������Ŀ��(.o)�ļ���
# ����ж����������EXE1��EBJ1��EXE2��EBJ2��ʾ��
#======================================================
EXE1 = missue.out
OBJ1 = migrant.o thread.o  util.o  migrantlog.o myhiredis.o  readini.o  issue.o collect.o file.o

#·������
MY_HOME=/data/cts/wuyz

#ͷ�ļ�
APPIFLAG = -I.\
           -I$(MY_HOME)/include\
           -I/home/cts/cts1-new/include/\
          

#�����
APPLFLAG = -L$(MY_HOME)/lib\
           -lcrypto \
           -levent  \
           -lpthread \
           -lhiredis    \
           -llua       \
           -ldl

#����һ�־���
NO_WARN =  -Wno-write-strings


#����
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

