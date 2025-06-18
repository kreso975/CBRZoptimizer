CC = gcc
RC = windres
CFLAGS = -mwindows -lmsimg32 -lcomctl32 -finput-charset=UTF-8 -fexec-charset=UTF-8
CFLAGS += -DUNICODE -D_UNICODE -municode
CFLAGS += -fwide-exec-charset=UCS-2LE

# Linker libraries (include uxtheme)
LIBS = -lmsimg32 -lcomctl32 -luxtheme

MINIZ = src/miniz/miniz.c
SRC = window.c functions.c $(MINIZ)
OBJ = $(SRC:.c=.o)
RES = resources.res

# Target executable
TARGET = CBRZoptimizer.exe

# Build executable
$(TARGET): $(OBJ) $(RES)
	$(CC) $(OBJ) $(RES) -o $(TARGET) $(CFLAGS) $(LIBS)

%.o: %.c
	$(CC) -c $< -o $@

resources.res: resources.rc CBRZoptimizer.exe.manifest
	$(RC) -O coff -i resources.rc -o resources.res

clean:
	cmd /C "del /Q *.o *.res $(TARGET)"
