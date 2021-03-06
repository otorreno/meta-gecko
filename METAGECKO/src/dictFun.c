/*
 * @author Fernando Moreno Jabato <jabato@uma.es>
 * @licence all rights reserved to the author and BitLAB group (University
 *    of Malaga).
 */
#include "dict.h"
#include <omp.h>

/* This function is used to shift left bits in a unsigned char array
 *	@param w: word structure where char array to be shifted is stored.
 */
void shift_word_left(Word *w) {
    unsigned int i;
    for (i = 0; i < BYTES_IN_WORD - 1; i++) {
        w->b[i] <<= 2;
        w->b[i] |= (w->b[i + 1] >> 6);
    }
    w->b[BYTES_IN_WORD - 1] <<= 2;
}


/* This function is used to shift right bits in a unsigned char array
 *	@param w: word structure where char array to be shifted is stored.
 */
void shift_word_right(Word *w) {
    int i;
    for (i = BYTES_IN_WORD - 1; i > 0; i--) {
        w->b[i] >>= 2;
        w->b[i] |= (w->b[i - 1] << 6);
    }
    w->b[i] >>= 2;
}

/* This function is used to store a given word in another word variable.
 *  @param container variable where word will be stored.
 *  @param word taht will be stored.
 */
inline void storeWord(wentry *container, wentry word) {
    memcpy(&container->loc, &word.loc, sizeof(LocationEntry));
    memcpy(container->w.b, word.w.b, BYTES_IN_WORD);
}


/* This function is used to write a given set of words in two intermediate files.
 *  @param buff set of words to be written.
 *  @param index file of intermediate files.
 *  @param words file of intermediate files.
 *  @param numWords number of instances on buff set.
 *  @return a negative number if any error happens or a non negative number if
 *     the process finished correctly.
 */
int writeBuffer(wentry *buff, FILE *index, FILE *words, uint64_t numWords) {
    // Sort buffer
    quicksort_W(buff, 0, numWords - 1);

    // Write buffer info on buffer index file
    uint64_t pos = (uint64_t) ftello(words); // Buffer start position
    fwrite(&pos, sizeof(uint64_t), 1, index);
    fwrite(&numWords, sizeof(uint64_t), 1, index); // Number of words
    // Write words on words file


    for (pos = 0; pos < numWords; ++pos) {
        if (fwrite(&buff[pos].loc, sizeof(LocationEntry), 1, words) != 1)
            terror("writeBuffer:: Error writing word Location");

        if (fwrite(buff[pos].w.b, sizeof(unsigned char), BYTES_IN_WORD, words) != BYTES_IN_WORD)
            terror("writeBuffer:: Error writing word array");

    }
    // Buffer correctly written on intermediate files
    return 0;
}

/* Function used to compare two wentry variables
 *  @param w1 word to be compared.
 *  @param w2 word to be compared
 *  @return zero if w2 are greater or equal and a positive number if
 *     w1 is greater.
 */
int GT(wentry w1, wentry w2) {
    unsigned int i;
    for (i = 0; i < BYTES_IN_WORD; i++)
        if (w1.w.b[i] < w2.w.b[i]) return 0;
        else if (w1.w.b[i] > w2.w.b[i]) return 1;

    if (w1.loc.seq > w2.loc.seq) return 1;
    else if (w1.loc.seq < w2.loc.seq) return 0;

    if (w1.loc.pos > w2.loc.pos) return 1;
    else if (w1.loc.pos < w2.loc.pos) return 0;

    if (w1.loc.strand == 'f' && w2.loc.strand == 'r') return 1;
    else return 0;
}


/* This function is necessary for quicksort functionality.
 *  @param arr array to be sorted.
 *  @param left inde of the sub-array.
 *  @param right index of the sub-array.
 */
int partition(wentry *arr, int left, int right) {
    int i = left;
    int j = right + 1;
    wentry t;

    // left sera el pivote
    // y contendra la mediana de left, r y (l+r)/2
    int mid = (left + right) / 2;

    if (GT(arr[mid], arr[right])) {
        SWAP_W(arr[mid], arr[right], t);
    }

    if (GT(arr[mid], arr[left])) {
        SWAP_W(arr[mid], arr[left], t);
    }

    if (GT(arr[left], arr[right])) {
        SWAP_W(arr[left], arr[right], t);
    }

    while (1) {
        do {
            ++i;
        } while (i <= right && !GT(arr[i], arr[left]));
        do {
            --j;
        } while (j >= left && GT(arr[j], arr[left]));

        if (i >= j) break;

        SWAP_W(arr[i], arr[j], t)
    }

    SWAP_W(arr[left], arr[j], t)

    return j;
}


/* This function is used to sort a wentry array.
 *  @param arr array to be sorted.
 *  @param left index where start to sort.
 *  @param right index where end sorting action.
 */
int quicksort_W(wentry *arr, int left, int right) {
    int j;

    if (left < right) {
        // divide and conquer
        j = partition(arr, left, right);

        if (right - left > PAR_THRESHOLD) {
            omp_set_nested(1);
#pragma omp parallel num_threads(2)
            {
#pragma omp sections
                {
#pragma omp section
                    quicksort_W(arr, left, j - 1);
#pragma omp section
                    quicksort_W(arr, j + 1, right);
                }
            }
        } else {
            quicksort_W(arr, left, j - 1);
            quicksort_W(arr, j + 1, right);
        }
    }
    return 0;
}


/* This function is used to load an array of words from a words intermediate file.
 *  @param words array where set will be loaded.
 *  @param wFile pointer to words intermediate file.
 *  @param unread rest of words on intermediate file.
 *  @return Number of words read from intermediate file. Or a negative number if any error hapens.
 */
uint64_t loadWord(wentry *words, FILE *wFile, int64_t unread) {
    uint64_t j;
    for (j = 0; j < READ_BUFF_LENGTH && unread > 0; ++j) {
        if (fread(&words[j].loc, sizeof(LocationEntry), 1, wFile) != 1) {
            fprintf(stderr, "loadWord:: Error reading Location.\n");
            return -1;
        }
        if (fread(words[j].w.b, sizeof(unsigned char), BYTES_IN_WORD, wFile) != BYTES_IN_WORD) {
            fprintf(stderr, "loadWord:: Error reading sequence.\n");
            return -1;
        }
        unread--;
    }
    return j;
}


/* This function is used to write an entrance on dictionary files. The order of
 * each entrance on dictionaries are:
 *     - WDictionary : Word<unsigned char*BYTES_IN_WORD> + PosOnPDic<uint64_t> + NumReps<uint16_t>
 *     - PDictionary : ReadIndex<uint32_t> + PosOnRead<uint64_t>
 *  @param word to be written.
 *  @param wDic words dictioanry.
 *  @param pDic positions dictionary.
 *  @param sameThanLastWord boolean value that indicate if the current word is the same than the last written.
 *  @param words equal than last written.
 */
inline void writeWord(wentry *word, FILE *w, FILE *p, bool sameThanLastWord, uint32_t *words) {
    if (!sameThanLastWord) { // Write new word
        uint64_t aux;
        fwrite(words, sizeof(uint32_t), 1, w); // Write num of repetitions
        fwrite(word->w.b, sizeof(unsigned char), BYTES_IN_WORD, w); // Write new word
        aux = (uint64_t) ftello(p);
        fwrite(&aux, sizeof(uint64_t), 1, w); // Write new positions on positions dictionary
        *words = 0; // Update value
    }
    fwrite(&word->loc, sizeof(LocationEntry), 1, p); // Location
    *words += 1; // Increment number of repetitions
}


/* This function is used to check the correc order of the first node of a linked list.
 * If it's incorrect, this function sort it.
 *  @param list linked list to be checked.
 *  @param discardFirst a boolean value that indicate if first node should be deleted.
 */
void checkOrder(node_W **list, bool discardFirst) {
    node_W *aux;
    if (discardFirst) {
        aux = *list;
        *list = (*list)->next;
        free(aux);
    } else if ((*list)->next != NULL) { // Check new position
        // Search new position
        if (GT((*list)->word[(*list)->index], (*list)->next->word[(*list)->next->index]) == 1) {
            node_W *curr = (*list)->next;
            while (1) {
                if (curr->next == NULL) break; // End of list
                else if (GT((*list)->word[(*list)->index], curr->next->word[curr->next->index]) == 0)
                    break; // position found
                else curr = curr->next;
            }
            aux = (*list)->next;
            (*list)->next = curr->next;
            curr->next = *list;
            *list = aux;
        }
    }
}


/* This method push node B after A (A->C ==PUSH==> A->B->C)
 *  @param A node after B will be pushed.
 *  @param B node to be pushed.
 */
void push(node_W **A, node_W **B) {
    (*B)->next = (*A)->next;
    (*A)->next = *B;
}


/* Move node after B to after A position and make linked list consistent.
 *  @param A reference node.
 *  @param B node after it will be moved.
 */
void move(node_W **A, node_W **B) {
    node_W *temp = (*B)->next->next;
    push(A, &(*B)->next);
    (*B)->next = temp;
}


/* This emthod sort a wentry linked list
 *  @param first node of the linked list.
 */
void sortList(node_W **first) {
    if ((*first)->next == NULL) return; // Linked list with only one element

    node_W *current = *first;
    node_W *aux;
    bool sorted = false;
    // Do until end
    while (!sorted) {
        if (current->next == NULL) sorted = true;
        else if (GT(current->next->word[current->index], current->word[current->next->index]) == 0) { // Next is smaller
            // Search position
            if (GT(current->next->word[current->next->index], (*first)->word[(*first)->index]) == 0) { // New first node
                aux = current->next->next;
                current->next->next = *first;
                *first = current->next;
                current->next = aux;
            } else { // Search position
                aux = *first;
                while (1) {
                    if (GT(aux->next->word[aux->next->index], current->next->word[current->next->index]) == 1)
                        break; // Position found
                    else aux = aux->next;
                }
                move(&aux, &current);
                // Chekc if it's the last node
                if (current->next == NULL) sorted = true;
            }
        } else { // Next is bigger, go next
            current = current->next;
            if (current->next == NULL) { // End of the list
                sorted = true;
            }
        }
    }
}

