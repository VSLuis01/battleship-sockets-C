CC_VERBOSE = $(CC)
CC_NO_VERBOSE = @echo "Building $@..."; $(CC)

ifeq ($(VERBOSE),YES)
  V_CC = $(CC_VERBOSE)
  AT :=
else
  V_CC = $(CC_NO_VERBOSE)
  AT := @
endif

C_FILES = BattleShip.c
O_FILES = ofiles/BattleShip.o
O_DIR = ofiles

LIBS=-lpthread -w
FLAGS=-Wall -Wextra -pedantic

.PHONY: all clean
.DEFAULT: all

all: servidor cliente
servidor:

servidor: $(O_FILES)
	$(V_CC) -o $@ $^ server.c $(LIBS) $(FLAGS)

cliente: $(O_FILES)
	$(V_CC) -o $@ $^ client.c $(LIBS) $(FLAGS)

build:
	$(AT)mkdir -p $(O_DIR)

#ofiles/server.o: build
#	$(V_CC) -c $(FLAGS) server.c -o $@
#
#ofiles/client.o: build
#	$(V_CC) -c $(FLAGS) client.c -o $@

$(O_FILES): $(C_FILES) | build
	$(V_CC) -c $(FLAGS) $< -o $@

clean:
	@echo Removing object files
	$(AT)-rm -f $(O_DIR)/*.o
	@echo Removing applications
	$(AT)-rm -f cliente
	$(AT)-rm -f servidor
	@echo Removing ofiles directory
	$(AT)-rm -rf ofiles

#all: BattleShip.o
#	gcc BattleShip.o client.c -o cliente -lpthread -w -Wall -Wextra -pedantic
#	gcc BattleShip.o server.c -o servidor -lpthread -w -Wall -Wextra -pedantic
#BattleShip.o: BattleShip.h
#	gcc -c BattleShip.c
#
#clean:
#	rm -rf *.o