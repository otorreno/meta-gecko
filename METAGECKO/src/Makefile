CC=gcc
CFLAGS=-O3 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -Wall -Wextra -fopenmp
BIN=../bin

all: meta_gecko stats write

stats: dictStats metagStats fragStats hdStat

meta_gecko: dict frags

dict: dict.c
	$(CC) $(CFLAGS) -fopenmp dict.c dictFun.c common_functions.c -o $(BIN)/dict

frags: frags.c
	$(CC) $(CFLAGS) -fopenmp frags.c fragsFun.c common_functions.c -o $(BIN)/frag

combineFragments: combineFragments.c
	$(CC) $(CFLAGS) combineFragments.c -o $(BIN)/combineFragments

write: write.c
	$(CC) $(CFLAGS) write.c -o $(BIN)/write

dictStats: metagdictstats.c
	$(CC) $(CFLAGS) metagdictstats.c common_functions.c -o $(BIN)/metagdictstats

metagStats: metagfilestats.c
	$(CC) $(CFLAGS) metagfilestats.c -o $(BIN)/metagfilestats

fragStats: metagfragstats.c
	$(CC) $(CFLAGS) metagfragstats.c fragsFun.c common_functions.c -o $(BIN)/metagfragstats
	
hdStat: hdStat.c
	$(CC) $(CFLAGS) hdStat.c common_functions.c -o $(BIN)/hdStat
