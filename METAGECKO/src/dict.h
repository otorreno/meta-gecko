/*
 * @author Fernando Moreno Jabato <jabato@uma.es>
 * @licence all rights reserved to the author and BitLAB group (University
 *    of Malaga).
 */

#include "metag_common.h"

// VARIABLES
#define BUFFER_LENGTH 10000000
#define MAX_FILE_LENGTH 1024
#define FLAG 0
#define READ_BUFF_LENGTH 10000

// INLINE FUNCTIONS
#define SWAP_W(a, b, t) t=a; a=b; b=t;

// GLOBAL VARIABLES
unsigned int BYTES_IN_WORD;

// FUNCTIONS
void checkOrder(node_W **, bool);

int GT(wentry, wentry);

uint64_t loadWord(wentry *, FILE *, int64_t);

void move(node_W **, node_W **);

int partition(wentry *, int, int);

void push(node_W **, node_W **);

int quicksort_W(wentry *, int, int);

void shift_word_left(Word *);

void shift_word_right(Word *);

void sortList(node_W **);

inline void storeWord(wentry *, wentry);

int writeBuffer(wentry *, FILE *, FILE *, uint64_t);

inline void writeWord(wentry *, FILE *, FILE *, bool, uint32_t *);
