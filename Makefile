CC = gcc
RC = windres
CFLAGS = -mwindows -finput-charset=UTF-8 -fexec-charset=UTF-8
CFLAGS += -DUNICODE -D_UNICODE -municode
CFLAGS += -fwide-exec-charset=UCS-2LE

# Linker libraries (include uxtheme)
LIBS = -lmsimg32 -lcomctl32 -luxtheme -lversion

BIN_DIR = bin
MANIFEST = CBRZoptimizer.exe.manifest

MINIZ = src/miniz/miniz.c
SRC = window.c functions.c aboutDialog.c $(MINIZ)
OBJ = $(SRC:.c=.o)
RES = resources.res

# Target executable
TARGET = $(BIN_DIR)/CBRZoptimizer.exe

# Build executable
$(TARGET): $(OBJ) $(RES)
	@if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)"
	$(CC) $(OBJ) $(RES) -o $(TARGET) $(CFLAGS) $(LIBS)
	copy /Y "$(MANIFEST)" "$(BIN_DIR)\\"

%.o: %.c
	$(CC) -c $< -o $@

resources.res: resources.rc $(MANIFEST)
	$(RC) -O coff -i resources.rc -o $@

clean:
	cmd /C "del /Q *.o *.res"
	cmd /C "del /Q $(BIN_DIR)\\*.exe"
