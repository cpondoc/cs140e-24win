PROGS += 0-read-test.c 
PROGS += 1-read-test.c 
# note: you can't do "make check" for this one.
<<<<<<< HEAD
PROGS += 0-tty-test.c
PROGS += 1-tty-test.c
PROGS += 2-tty-test.c
=======
# PROGS += 0-tty-test.c
>>>>>>> upstream/main
COMMON_SRC = 

ifdef CS140E_STAFF
COMMON_SRC += $(CS140E_STAFF_2024_PATH)/libunix/staff-find-ttyusb.c
COMMON_SRC += $(CS140E_STAFF_2024_PATH)/libunix/read-file.c
CAN_EMIT=1
endif

RUN=0

ifndef CS140E_2024_PATH
    $(error "CS140E_2024_PATH not defined!");
else
    include $(CS140E_2024_PATH)/libunix/mk/Makefile.unix
endif

