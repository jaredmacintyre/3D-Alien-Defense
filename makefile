

# framework information for older version of MACOS
#INCLUDES = -F/System/Library/Frameworks -framework OpenGL -framework GLUT -lm

# frameworks for newer MACOS, where include files are moved 
INCLUDES = -F/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/ -framework OpenGL -framework GLUT -lm -Wno-deprecated-declarations

OS = $(shell uname)
ifeq ($(OS), Darwin)
	# frameworks for newer MACOS, where include files are moved 
	INCLUDES = -F/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/ -framework OpenGL -framework GLUT -lm -Wno-deprecated-declarations
else ifeq ($(OS), Linux)
	# frameworks for Linux
	INCLUDES = -lGL -lm -lGLU -lglut -DLINUX
endif

3Dgame: main.c graphics.c visible.c graphics.h
	gcc main.c graphics.c visible.c -o 3Dgame $(INCLUDES)

clean:
	rm -f 3Dgame

