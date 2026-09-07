CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -D_REENTRANT -I libtci -MMD -MP
BUILD   = -g -O0
RELEASE = -O2
LDFLAGS = $(shell sdl2-config --libs) -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lm
SRCS    = main.c vec2.c camera.c scaler.c map.c shot.c render.c audio.c
OBJS    = $(SRCS:.c=.o)
DEPS    = $(OBJS:.o=.d)
LIBS    = libtci/libtci.a
NAME    = getaway

.PHONY: all release clean fclean re

all: $(NAME)

$(NAME): $(OBJS) $(LIBS)
	$(CC) $(BUILD) $(OBJS) $(LIBS) $(LDFLAGS) -o $(NAME)

$(LIBS):
	$(MAKE) -C libtci

%.o: %.c
	$(CC) $(CFLAGS) $(BUILD) -c $< -o $@

-include $(DEPS)

release:
	@$(MAKE) --no-print-directory fclean
	@$(MAKE) --no-print-directory BUILD="$(RELEASE)"
	strip $(NAME)

clean:
	$(MAKE) -C libtci clean
	rm -f $(OBJS) $(DEPS)

fclean: clean
	$(MAKE) -C libtci fclean
	rm -f $(NAME)

re: fclean all
