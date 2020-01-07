# Makefile 

CC:=g++
DEBUG:=y
TOP:=..
DIST_DIR:=$(TOP)/dat2csv/dist
INCPATH:= -I$(TOP)/3rd/yao/include -I$(TOP)/3rd/ctp/include
LIBPATH:=
CPPFLAGS:=-std=c++11 $(INCPATH)
CFLAGS:=
LDFLAGS:= 
OBJPATH:=./obj
BINPATH:=./bin
LIBS:=-lboost_filesystem  -lboost_system
DEPS:=.depends
OUT:=dat2csv
PACKAGE_NAME:=$(OUT)

SUBDIR:=$(TOP)/dat2csv/src 

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
	@echo $(OUT)	

$(OUT):$(DEPS) $(OBJS)
	-@mkdir -p $(BINPATH)
	$(CC) $(OBJS) 	-o $@  $(LDFLAGS) $(LIBPATH) $(LIBS)	
	@echo "---------build target finshed-----------"


$(OBJPATH)/%.o:%.cpp
	-@mkdir -p $(OBJPATH)
	$(CC) -c $< $(CPPFLAGS) $(CFLAGS) $(INCPATH)  -o $@

$(DEPS):$(SRCS)
	-@rm $(DEPS)
	$(CC)  -MM $(CPPFLAGS) $(INCPATH) $^  >>$(DEPS) 

-include $(DEPS)

dist:
	-rm -fr $(DIST_DIR)/$(PACKAGE_NAME)
	-mkdir -p $(DIST_DIR)/$(PACKAGE_NAME)
	-cp -arf $(TOP)/dat2csv/bin/* $(DIST_DIR)/$(PACKAGE_NAME)/
	-cp -arf /usr/local/lib/libboost_filesystem.so*  $(DIST_DIR)/$(PACKAGE_NAME)/
	-cp -arf /usr/local/lib/libboost_system.so*  $(DIST_DIR)/$(PACKAGE_NAME)/
	-mkdir -p $(DIST_DIR)/$(PACKAGE_NAME)/tools/
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
