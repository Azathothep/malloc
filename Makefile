ifeq (${HOSTTYPE},)
	HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

#-------------- SOURCES --------------#

SRC =		malloc.c \
		free.c \
		realloc.c \
		utils.c \
		show_free_mem.c \
		show_alloc_mem.c \
		scan_memory.c \

SRC_DIR =	src

OBJ	=	${SRC:%.c=${OBJ_DIR}/%.o}

OBJ_DIR	=	obj

#------------ COMPILATION ------------#

NAME =		libft_malloc.so

LIBHOST =	libft_malloc_${HOSTTYPE}.so

CC =		gcc

FLAGS =		-Wall -Wextra -Werror -g

INCLUDES =	${SRC_DIR}/malloc.h \
		${SRC_DIR}/utils.h \
		ft_malloc.h \
		#${LIBFT_INCLUDE}

LIBFT_NAME =	libft.a

LIBFT_DIR =	libft

LIBFT_INCLUDE =	${LIBFT_DIR}/libft.h

LIBFT =		${LIBFT_DIR}/${LIBFT_NAME}

#--------------- RULES --------------#

all:		makelibft ${NAME}

${OBJ_DIR}/%.o:	${SRC_DIR}/%.c ${INCLUDES} | ${OBJ_DIR}
		${CC} ${FLAGS} -fPIC -c $< -o $@ -I. #-I${LIBFT_DIR}

${NAME}:	${OBJ} ${INCLUDES} #${LIBFT} 
		${CC} ${FLAGS} ${OBJ} -shared -o ${LIBHOST} # -lft -L${LIBFT_DIR}
		ln -sf ${LIBHOST} ${NAME}

${OBJ_DIR}:	
		@mkdir -p ${OBJ_DIR}

makelibft:	
		#${MAKE} -C ${LIBFT_DIR}

clean:		
		rm -rf ${OBJ_DIR}
		#${MAKE} -C ${LIBFT_DIR} clean

fclean:		clean
		rm -f ${NAME} ${LIBHOST}
		#${MAKE} -C ${LIBFT_DIR} fclean

re:		fclean all

.PHONY: clean fclean all re	
