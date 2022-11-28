CC = gcc

ifeq ($(CC),clang)
  STACK_FLAGS = -fno-stack-protector -Wl,-allow_stack_execute
else
  STACK_FLAGS = -fno-stack-protector -z execstack
endif

CFLAGS = ${STACK_FLAGS} -Wall -Iutil -Iatm -Ibank -Irouter -I. -I/usr/include/openssl 
LDLIBS =  -lcrypto

all: bin bin/atm bin/bank bin/router bin/init

bin:
	mkdir -p bin

bin/init : 
	mv init.sh bin/init

bin/atm : atm/atm-main.c atm/atm.c util/misc_util.c
	${CC} ${CFLAGS} $^ ${LDLIBS} -o bin/atm

bin/bank : bank/bank-main.c bank/bank.c
	${CC} ${CFLAGS} $^ ${LDLIBS} -o bin/bank

bin/router : router/router-main.c router/router.c
	${CC} ${CFLAGS} $^ -o bin/router

test : util/list.c util/list_example.c util/hash_table.c util/hash_table_example.c
	${CC} ${CFLAGS} util/list.c util/list_example.c -o bin/list-test
	${CC} ${CFLAGS} util/list.c util/hash_table.c util/hash_table_example.c -o bin/hash-table-test

clean:
	cd bin && rm -f atm bank router list-test hash-table-test
