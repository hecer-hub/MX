CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
LDFLAGS = -static

MX_TARGET = mx

MX_OBJS = mx_main.o mxa_functions.o mgrip_internal.o

all: $(MX_TARGET)

$(MX_TARGET): $(MX_OBJS)
	$(CC) $(MX_OBJS) -o $(MX_TARGET) $(LDFLAGS)

mx_main.o: mx_main.c mxa_functions.h
	$(CC) $(CFLAGS) -c $<

mxa_functions.o: mxa_functions.c mxa_functions.h
	$(CC) $(CFLAGS) -c $<

mgrip_internal.o: mgrip_internal.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(MX_TARGET) $(MX_OBJS) mxa cat ls grep cd

install: all
	@echo "Creating symlinks for BusyBox-like behavior..."
	@ln -sf $(MX_TARGET) mxa
	@ln -sf $(MX_TARGET) cat
	@ln -sf $(MX_TARGET) ls
	@ln -sf $(MX_TARGET) grep
	@ln -sf $(MX_TARGET) cd
	@echo "Run examples:"
	@echo "  ./mx mxa pack -n archive.mxa file.txt"
	@echo "  ./mxa pack -n archive.mxa file.txt (via symlink)"
	@echo "  ./mx cat file.txt"
	@echo "  ./cat file.txt (via symlink)"
	@echo "  ./mx ls"
	@echo "  ./ls (via symlink)"
	@echo "  ./mx grep pattern file.txt"
	@echo "  ./grep pattern file.txt (via symlink)"
	@echo "  ./mx cd /tmp"
	@echo "  ./cd /tmp (via symlink)"
	@echo "  ./mx list"