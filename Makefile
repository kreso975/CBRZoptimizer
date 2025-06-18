CC = gcc
RC = windres
CFLAGS = -mwindows -lmsimg32 -lcomctl32 -finput-charset=UTF-8 -fexec-charset=UTF-8
CFLAGS += -DUNICODE -D_UNICODE -municode
MINIZ = src/miniz/miniz.c
SRC = window.c functions.c $(MINIZ)
OBJ = $(SRC:.c=.o)
RES = app_icon.res

CBRZoptimizer.exe: $(OBJ) $(RES)
	$(CC) $(OBJ) $(RES) -o CBRZoptimizer.exe $(CFLAGS)

%.o: %.c
	$(CC) -c $< -o $@

%.res: %.rc
	$(RC) -O coff -i $< -o $@

clean:
	cmd /C "del /Q *.o *.res CBRZoptimizer.exe"
