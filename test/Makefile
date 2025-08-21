#-------------- SOURCES --------------#

SRC =		main.c \

SRC_DIR =	.

OBJ	=	${SRC:%.c=${OBJ_DIR}/%.o}

OBJ_DIR	=	obj

#------------ COMPILATION ------------#

NAME =		a.out

MALLOC_NAME =	ft_malloc

MALLOC =	${MALLOC_DIR}/lib${MALLOC_NAME}.so

MALLOC_DIR = 	${PWD}/..

CC =		gcc

FLAGS =		-Wall -Wextra -Werror -g

INCLUDES =	${MALLOC_DIR}/malloc.h

#--------------- RULES --------------#

all:		makemalloc ${NAME}

${OBJ_DIR}/%.o:	${SRC_DIR}/%.c ${INCLUDES} | ${OBJ_DIR}
		${CC} ${FLAGS} -c $< -o $@ -I. -I${MALLOC_DIR}

${NAME}:	${MALLOC} ${OBJ} ${INCLUDES}
		${CC} ${FLAGS} ${OBJ} -L${MALLOC_DIR} -Wl,-rpath=${MALLOC_DIR} -l${MALLOC_NAME} -o ${NAME}

${OBJ_DIR}:	
		@mkdir -p ${OBJ_DIR}

makemalloc:
		${MAKE} -C ${MALLOC_DIR}

clean:		
		rm -rf ${OBJ_DIR}
		${MAKE} -C ${MALLOC_DIR} clean

fclean:		clean
		rm -f ${NAME}
		${MAKE} -C ${MALLOC_DIR} fclean

re:		fclean all

.PHONY: clean fclean all re makemalloc
