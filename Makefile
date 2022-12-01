CC = gcc
CFLAGS = -fno-stack-protector -z execstack -Wall -Iutil -Iatm -Ibank -Irouter -I. -I/usr/include/openssl
LDLIBS =  -lcrypto

all: bin bin/atm bin/bank bin/router bin/init

bin:
	mkdir -p bin

bin/init : 
	cp init.sh bin/init
	chmod u+x bin/init

bin/atm : atm/atm-main.c atm/atm.c util/misc_util.c
	${CC} ${CFLAGS} $^ ${LDLIBS} -o bin/atm

bin/bank : bank/bank-main.c bank/bank.c util/hash_table.c util/list.c util/misc_util.c
	${CC} ${CFLAGS} $^ ${LDLIBS} -o bin/bank

bin/router : router/router-main.c router/router.c
	${CC} ${CFLAGS} $^ -o bin/router

test : util/list.c util/list_example.c util/hash_table.c util/hash_table_example.c
	${CC} ${CFLAGS} util/list.c util/list_example.c -o bin/list-test
	${CC} ${CFLAGS} util/list.c util/hash_table.c util/hash_table_example.c -o bin/hash-table-test

clean:
	cd bin && rm -f atm bank router list-test hash-table-test init
