include util.mk

LIB_DIR ?= ..

include $(LIB_DIR)/defines.mk

all: $(UTIL_OBJS)

# Create expansions for dependencies
UTIL_H := ${UTIL_OBJS:.o=.h}

CFLAGS+= -I$(LIB_DIR)/install/include

$(UTIL_OBJS): %.o: %.c $(UTIL_H) $(LIBMK)
	$(QUIET_CC)$(CC) $(CFLAGS) -Wall -I../../headers -c -o $@ $<

clean:
	$(Q)rm -f $(UTIL_OBJS)
