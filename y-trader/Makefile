# Makefile 


CC:=g++
DEBUG:=y
TOP:=..
DIST_DIR:=$(TOP)/y-trader/dist
INCPATH:=-I$(TOP)/3rd/ctp/include -I$(TOP)/3rd/tinyxml/include \
		 -I$(TOP)/3rd/utils/include -I$(TOP)/3rd/zce_md/include \
		 -I$(TOP)/3rd/dce_md/include -I$(TOP)/3rd/shfe_md/include

LIBPATH:=-L$(TOP)/3rd/boost/lib -L$(TOP)/3rd/ctp/lib \
		 -L$(TOP)/3rd/tinyxml/lib -L$(TOP)/3rd/utils/lib \
		 -L$(TOP)/3rd/zce_md/lib


CPPFLAGS:=-std=c++11 $(INCPATH)
CFLAGS:=
LDFLAGS:=
OBJPATH:=./obj
BINPATH:=./bin

LIBS:=-lpthread -ltinyxml -lutils -ldl \
	  -lbowsprit -lcheck -lclogger -lcork -lvrt -luuid \
	  -lboost_filesystem -lboost_system -lthosttraderapi_se \
	  -lLinuxDataCollect -lthostmduserapi_se

DEPS:=.depends
OUT:=ytrader
PACKAGE_NAME:=$(OUT)

SUBDIR:=$(TOP)/y-trader/src

vpath %.cpp $(SUBDIR)

ifeq ($(strip $(DEBUG)),y)
	CFLAGS+= -g3 -O0
	OUT:=$(addsuffix d, $(OUT))
else
	CFLAGS+=-O2
endif

SRCS:=$(foreach d, $(SUBDIR), $(wildcard $(d)/*.cpp))
OBJS:=$(patsubst %.cpp,%.o,$(SRCS))
OBJS:=$(addprefix $(OBJPATH)/, $(notdir $(OBJS)))
OUT:=$(addprefix $(BINPATH)/, $(OUT))

all:$(OUT)
	echo "all"
	@echo $(OUT)	

$(OUT):$(DEPS) $(OBJS)
	echo "eeeeeeeee -o eeeeeeeeee"
	-@mkdir -p $(BINPATH)
	$(CC) $(OBJS) -o $@  $(LDFLAGS) $(LIBPATH) $(LIBS)	
	@echo "---------build target finshed-----------"


$(OBJPATH)/%.o:%.cpp
	echo "iiiiiiiiii debug iiiiiiiii"
	-@mkdir -p $(OBJPATH)
	$(CC) -c $< $(CPPFLAGS) $(CFLAGS) $(INCPATH)  -o $@

$(DEPS):$(SRCS)
	echo "eeeeeeeeeee debug iiiiiiiii"
	-@rm $(DEPS)
	$(CC)  -MM $(CPPFLAGS) $(INCPATH) $^  >>$(DEPS)

-include $(DEPS)

dist:
	-rm -fr $(DIST_DIR)/$(PACKAGE_NAME)
	-mkdir -p $(DIST_DIR)/$(PACKAGE_NAME)
	-cp -arf $(TOP)/y-trader/bin/* $(DIST_DIR)/$(PACKAGE_NAME)/
	-cp -arf $(TOP)/3rd/ctp/lib/* $(DIST_DIR)/$(PACKAGE_NAME)/
	-cp -arf $(TOP)/3rd/gfortran/lib/* $(DIST_DIR)/$(PACKAGE_NAME)/
	-cp -arf $(TOP)/3rd/zce_md/lib/* $(DIST_DIR)/$(PACKAGE_NAME)/
	-cp -arf /usr/local/lib/libboost_filesystem.so*  $(DIST_DIR)/$(PACKAGE_NAME)/
	-cp -arf /usr/local/lib/libboost_system.so*  $(DIST_DIR)/$(PACKAGE_NAME)/
	-cp -arf /usr/local/lib/libboost_date_time.so*  $(DIST_DIR)/$(PACKAGE_NAME)/
	-cp -arf /usr/local/lib/libboost_thread.so*  $(DIST_DIR)/$(PACKAGE_NAME)/
	-cp -Lp /usr/local/lib/libbowsprit.so.2 $(DIST_DIR)/$(PACKAGE_NAME)/
	-cp -Lp /lib64/libcheck.so.0 $(DIST_DIR)/$(PACKAGE_NAME)/
	-cp -Lp /usr/local/lib/libclogger.so.2 $(DIST_DIR)/$(PACKAGE_NAME)/
	-cp -Lp /usr/local/lib/libcork.so.15 $(DIST_DIR)/$(PACKAGE_NAME)/
	-cp -Lp /usr/local/lib/libvrt.so.2 $(DIST_DIR)/$(PACKAGE_NAME)/
	-cp -arf $(TOP)/3rd/zce_md/config/* $(DIST_DIR)/$(PACKAGE_NAME)/
	-cp -a $(TOP)/y-trader/scripts/pos*.* 						$(DIST_DIR)/$(PACKAGE_NAME)/
	-cp -a $(TOP)/y-trader/scripts/st*.sh 						$(DIST_DIR)/$(PACKAGE_NAME)/
	-mkdir -p $(DIST_DIR)/$(PACKAGE_NAME)/tools/
	-cp -a $(TOP)/y-trader/scripts/configurator.py 			$(DIST_DIR)/$(PACKAGE_NAME)/tools/
	-cp -a $(TOP)/y-trader/scripts/update.sh 					$(DIST_DIR)/$(PACKAGE_NAME)/tools/
	-cp -a $(TOP)/y-trader/scripts/stra_sett_dce_day100.csv 	$(DIST_DIR)/$(PACKAGE_NAME)/tools/
	-mkdir -p $(DIST_DIR)/$(PACKAGE_NAME)/log/
	-mkdir -p $(DIST_DIR)/$(PACKAGE_NAME)/ev/
	-mkdir -p $(DIST_DIR)/$(PACKAGE_NAME)/iv/
	-cd $(DIST_DIR); \
		tar -cvzf $(PACKAGE_NAME).tar.gz  $(PACKAGE_NAME)/*     

distclean:
	-@rm $(TOP)/dist/$(PACKAGE_NAME).tar.gz	
	-@rm -rf $(DIST_DIR)

clean:
	-@rm $(OUT)
	-@rm $(OBJS)

help:
	@echo "make (all):\t This is the default command when target unspecified "
	@echo "make clean:\t clean intermediate objects, target"
	@echo "make distclean:\t clean depends, intermediate objects, target"
