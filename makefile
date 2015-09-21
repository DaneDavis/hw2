#################
#   variables   #
#################

# files
EXECUTABLE  = words
SOURCES  = words.c 

OBJECTS  = $(SOURCES:.c=.o)

# compilation and linking
CC      = gcc
CFLAGS  = -c -std=c99 -D_GNU_SOURCE
LDFLAGS = -lpthread
WARN    = -Wall -Wextra -pedantic
COMPILE.c = $(CC) $(CFLAGS) $(CPPFLAGS) $(WARN)
LINK.c    = $(CC)

# define paths
vpath %.c src
vpath %.h include

#################
#     targets   #
#################

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(LINK.c) $^ -o $@ $(LDFLAGS)

$(OBJECTS): %.o: %.c
	$(COMPILE.c) $< -o $@

# phony targets
.PHONY: clean

# remove object files, emacs temporaries
clean:
	rm -f *.o *~

# print-VAR prints the value of the variable VAR
print-%  : ; @echo $* = $($*)



