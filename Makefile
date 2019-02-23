CC=gcc
JAVA = java-8-openjdk-amd64
NATIVE = src/main/resources/native
INCLUDES = -IC:/PROGRA~1/Java/jdk1.8.0_144/include\
	   -IC:/PROGRA~1/Java/jdk1.8.0_144/include/win32\
	   -Itarget/native/include\
	   -I/usr/local/include
CFLAGS = -Ofast -Wall -fPIC $(INCLUDES)
LDFLAGS = -Wl,-soname,libjni_ghook.dll,-rpath='$$ORIGIN' -shared
LDPATH = -L$(NATIVE)
LIBS = -lcomctl32
SOURCES = src/main/native/ghook_GlobalHook.c
TARGETLIB = libjni_ghook.dll

all: libjni_ghook.dll

clean:
	rm -f *.o *~ $(TARGETLIB)

$(TARGETLIB) : $(SOURCES:.c=.o)
	$(CC) $(LDFLAGS) $(LDPATH) -o $@ $^ $(LIBS)

install: $(TARGETLIB)
	mv $(TARGETLIB) $(NATIVE)
	rm src/main/native/*.o

.c.o:
	$(CC) -c  $(CFLAGS) $< -o $@
