# USE: mingw32-make BUILD=release
CC = x86_64-w64-mingw32-gcc
RC = windres

# Output
BIN_DIR = bin
OBJ_DIR = obj/$(BUILD)
TARGET = $(BIN_DIR)/CBRZoptimizer.exe
MANIFEST = CBRZoptimizer.exe.manifest

# Source files
MINIZ = src/miniz/miniz.c
SRC = window.c src/functions.c src/aboutDialog.c src/instructionsDialog.c src/rar_handle.c src/zip_handle.c src/pdf_handle.c src/folder_handle.c src/image_handle.c src/gui.c $(MINIZ)

# Object files
OBJ = $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC))
RES = $(OBJ_DIR)/resources.res

# Include paths
INCLUDES = -Iexternal -Isrc -Isrc/miniz -Isrc/stb -I.

# Libraries
LIBS = -lmsimg32 -lcomctl32 -luxtheme -lversion -lshlwapi -lole32 -luuid

# Build mode
BUILD ?= debug

# Shared compiler flags
CFLAGS_COMMON = -mwindows -finput-charset=UTF-8 -fexec-charset=UTF-8 \
  -DUNICODE -D_UNICODE -municode -fwide-exec-charset=UCS-2LE $(INCLUDES)

CFLAGS_WARNINGS = -Wall -Wextra -Wpedantic -Wformat=2 -Wcast-align -Wconversion \
  -Wnull-dereference -Wdouble-promotion -Wstrict-prototypes -Wmissing-prototypes \
  -Wshadow -Wpointer-arith -Wunused -Wuninitialized

ifeq ($(BUILD),release)
CFLAGS = $(CFLAGS_COMMON) $(CFLAGS_WARNINGS) -Os -DNDEBUG
else
CFLAGS = $(CFLAGS_COMMON) $(CFLAGS_WARNINGS) -g -DDEBUG -O0
endif

# Build executable
$(TARGET): $(OBJ) $(RES)
	@echo === BUILD MODE: $(BUILD)
	@if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)"
	$(CC) $(OBJ) $(RES) -o $@ $(CFLAGS) $(LIBS)
ifeq ($(BUILD),release)
	strip $@
	copy /Y "$(MANIFEST)" "$(BIN_DIR)\\" > nul
	copy /Y "LICENSE"                          "$(BIN_DIR)\\LICENSE-Apache-2.0.txt" > nul
	@if not exist "$(BIN_DIR)\\thirdparty" mkdir "$(BIN_DIR)\\thirdparty"
	copy /Y "external\\license.txt"           "$(BIN_DIR)\\thirdparty\\LICENSE-UnRAR.txt" > nul
	copy /Y "src\\miniz\\LICENSE"             "$(BIN_DIR)\\thirdparty\\LICENSE-miniz.txt" > nul
	copy /Y "src\\stb\\LICENSE"               "$(BIN_DIR)\\thirdparty\\LICENSE-stb.txt" > nul
	copy /Y "external\\UnRAR64.dll"   			"$(BIN_DIR)\\" > nul
else
	copy /Y "$(MANIFEST)" "$(BIN_DIR)\\" > nul
	copy /Y "external\\UnRAR64.dll"   			"$(BIN_DIR)\\" > nul
endif

# Specialized rule: compile miniz.c with relaxed warning flags
$(OBJ_DIR)/src/miniz/miniz.o: src/miniz/miniz.c
	@mkdir $(subst /,\,$(dir $@)) 2>nul || exit 0
	$(CC) -c $< -o $@ $(CFLAGS) -Wno-sign-conversion -Wno-conversion -Wno-type-limits

# Generic rule: compile .c → .o
$(OBJ_DIR)/%.o: %.c
	@mkdir $(subst /,\,$(dir $@)) 2>nul || exit 0
	$(CC) -c $< -o $@ $(CFLAGS)

# Compile .rc → resources.res
$(RES): resources.rc $(MANIFEST)
	@mkdir $(subst /,\,$(dir $@)) 2>nul || exit 0
	$(RC) -O coff -i $< -o $@

# Clean build artifacts
clean:
	cmd /C "rd /S /Q obj" 2>nul
	cmd /C "del /Q $(BIN_DIR)\\*.exe" 2>nul
	cmd /C "del /Q $(BIN_DIR)\\*.dll" 2>nul
	cmd /C "del /Q $(BIN_DIR)\\*.manifest" 2>nul
	cmd /C "del /Q $(BIN_DIR)\\LICENSE-*.txt" 2>nul
	cmd /C "rd /S /Q $(BIN_DIR)\\thirdparty" 2>nul