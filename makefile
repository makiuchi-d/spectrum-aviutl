

CC = bcc32
LN = bcc32
RC = brc32
CFLAG = -c -O1 -O2
LFLAG = -tWD

DLL = spectrum.auf
OBJ = filter.obj fftsg.obj
RES = spectrum.res


all: $(DLL)

$(DLL): $(OBJ) $(RES)
	$(LN) -e$(DLL) $(LFLAG) $(OBJ)
	$(RC) -fe$(DLL) $(RES)

filter.obj: filter.c fft.h resource.h
	$(CC) $(CFLAG) filter.c

fftsg.obj: fftsg.c
	$(CC) $(CFLAG) fftsg.c


$(RES): spectrum.rc resource.h
	$(RC) -r spectrum.rc
