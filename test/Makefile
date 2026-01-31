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

INCLUDES =	${MALLOC_DIR}/ft_malloc.h \
		${LIBFT_INCLUDES}

LIBFT_NAME =	libft.a

LIBFT_DIR =	${PWD}/libft

LIBFT =		${LIBFT_DIR}/${LIBFT_NAME}

LIBFT_INCLUDES = ${LIBFT_DIR}/libft.h

#--------------- RULES --------------#

all:		makemalloc makelibft ${NAME}

${OBJ_DIR}/%.o:	${SRC_DIR}/%.c ${INCLUDES} | ${OBJ_DIR}
		${CC} ${FLAGS} -c $< -o $@ -I. -I${MALLOC_DIR} -I${LIBFT_DIR}

${NAME}:	${MALLOC} ${OBJ} ${INCLUDES} ${LIBFT}
		${CC} ${FLAGS} ${OBJ} -L${MALLOC_DIR} -Wl,-rpath=${MALLOC_DIR} -l${MALLOC_NAME} -L${LIBFT_DIR} -lft -o ${NAME}

${OBJ_DIR}:	
		@mkdir -p ${OBJ_DIR}

std:		makelibft ${OBJ} ${INCLUDES} ${LIBFT}
		${CC} ${FLAGS} ${OBJ} -lft -L${LIBFT_DIR} -D STDLIB

makemalloc:
		${MAKE} -C ${MALLOC_DIR}

makelibft:
		${MAKE} -C ${LIBFT_DIR}

clean:		
		rm -rf ${OBJ_DIR}
		${MAKE} -C ${MALLOC_DIR} clean
		${MAKE} -C ${LIBFT_DIR} clean

fclean:		clean
		rm -f ${NAME}
		${MAKE} -C ${MALLOC_DIR} fclean
		${MAKE} -C ${LIBFT_DIR} fclean

re:		fclean all

.PHONY: clean fclean all re makemalloc
