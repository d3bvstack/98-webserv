# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/04/27 22:36:40 by dbarba-v          #+#    #+#              #
#    Updated: 2026/06/11 23:51:36 by dbarba-v         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = webserv
CXX ?= c++
CXXFLAGS ?= -Wall -Wextra -Werror -std=c++98 -pedantic
RM = rm -f

INCLUDES = -Iinclude

OBJDIR = obj

SRCS =	src/main.cpp \
		src/Server.cpp

OBJS = $(patsubst src/%.cpp,$(OBJDIR)/%.o,$(SRCS))
OBJDIRS = $(sort $(dir $(OBJS)))

BINDIR = bin

all: $(BINDIR)/$(NAME)

$(BINDIR)/$(NAME): $(OBJS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(OBJS)

$(BINDIR):
	mkdir -p $@

$(OBJDIRS):
	mkdir -p $@

$(OBJDIR)/%.o: src/%.cpp | $(OBJDIRS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	$(RM) $(OBJS)
	$(RM) -r $(OBJDIR)

fclean: clean
	$(RM) $(BINDIR)/$(NAME)
	$(RM) -r $(BINDIR)

run: all
	./$(BINDIR)/$(NAME)

re: fclean all

asan:
	bash scripts/asan-test.sh

hooks:
	git config core.hooksPath .githooks
	chmod +x .githooks/commit-msg 2>/dev/null || true
	chmod +x .githooks/pre-commit 2>/dev/null || true

.PHONY: all clean fclean re run hooks asan
