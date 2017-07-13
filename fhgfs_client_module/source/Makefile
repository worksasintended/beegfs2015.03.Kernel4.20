obj-m	+= ${TARGET}.o

${TARGET}-y	:= $(patsubst $(src)/%.c,%.o,$(shell find $(src)/ -name '*.c'))
${TARGET}-y += $(patsubst $(src)/../opentk/source/%.c,../opentk/source/%.o, \
	$(shell find $(src)/../opentk/source -name '*.c'))

ifneq ($(OFED_INCLUDE_PATH),)

ifeq ($(shell [ -f $(OFED_INCLUDE_PATH)/linux/compat-2.6.h ] && echo 1),1)
$(src)/../opentk/source/net/sock/ibvsocket/IBVSocket.o: CC+=-include $(OFED_INCLUDE_PATH)/linux/compat-2.6.h
endif

$(src)/../opentk/source/net/sock/ibvsocket/IBVSocket.o: CC+=-I$(OFED_INCLUDE_PATH) -I$(OFED_INCLUDE_PATH)/uapi 
endif
