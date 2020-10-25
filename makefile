#OBJS specifies which files to compile as part of the project
OBJS = barnsley_fern.c

#CC specifies which compiler we're using
CC = gcc

#COMPILER_FLAGS specifies the additional compilation options we're using
# -w suppresses all warnings
COMPILER_FLAGS = -g -Wall -Werror -pedantic -fsanitize=address -fno-omit-frame-pointer

#LINKER_FLAGS specifies the libraries we're linking against
LINKER_FLAGS = -lSDL2 -lSDL2_ttf -lm

#OBJ_NAME specifies the name of our exectuable
OBJ_NAME = p_barnsley_fern_multi_thread

#This is the target that compiles our executable
all : $(OBJS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)
