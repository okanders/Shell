CFLAGS = -g3 -Wall -Wextra -Wconversion -Wcast-qual -Wcast-align -g
CFLAGS += -Winline -Wfloat-equal -Wnested-externs
CFLAGS += -pedantic -std=gnu99 -Werror
CC = gcc
FLAG1 = -D_GNU_SOURCE
FlAG2 = -std=gnu99
33SH = 33sh
33NOPROMPT = 33noprompt
DEPEND = sh.c jobs.c jobs.h
EXECS = $(33SH) $(33NOPROMPT) 
PROMPT = -DPROMPT

.PHONY: all clean

all: $(EXECS)

$(33SH): $(DEPEND)
	$(CC) $(CFLAGS) -o $(33SH) $(PROMPT) $(FLAG1) $(FlAG2) $(DEPEND)
$(33NOPROMPT): $(DEPEND)
	$(CC) $(CFLAGS) -o $(33NOPROMPT) $(FLAG1) $(FlAG2) $(DEPEND)
clean:
	rm -f $(EXECS)
