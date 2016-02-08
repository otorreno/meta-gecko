/*
 * @author Fernando Moreno Jabato <jabato@uma.es>
 * @licence all rights reserved to the author and BitLAB group (University
 *    of Malaga).
 */
#include "frags.h"

/* This function compare two arrays of unsigned chars with the same length.
 *  @param w1: first array to be compared.
 *  @param w2: second array to be compared.
 *  @param n: length of BOTH arrays.
 *  @retun a positive number if w1>w2, a negative number if w1>w2 and zero if they are equal.
 */
int wordcmp(unsigned char *w1, unsigned char *w2, int n){
	int i;
	for(i=0;i<n;i++)
		if(w1[i] < w2[i]) return -1;
		else if(w1[i] > w2[i]) return +1;

	return 0;
}


/* Function used to compare two Hit variables. It function sort with this
 * criterion:
 *    1 - Sequence X
 *    2 - Sequence Y
 *    3 - Diagonal
 *    4 - Position on X
 *    5 - Length 
 *  @param h1 word to be compared.
 *  @param h2 word to be compared
 *  @return zero if both are equal, a positive number if w1 is greater or a
 *     negative number if w2 is greater.
 */
int HComparer(Hit h1, Hit h2){
	if(h1.seqX > h2.seqX) return 1;
	else if(h1.seqX < h2.seqX) return -1;

	if(h1.seqY > h2.seqY) return 1;
	else if(h1.seqY < h2.seqY) return -1;

	if(h1.diag > h2.diag) return 1;
	else if(h1.diag < h2.diag) return -1;

	if(h1.posX > h2.posX) return 1;
	else if(h1.posX < h2.posX) return -1;

	if(h1.length > h2.length) return 1;
	else if(h1.length < h2.length) return -1;

	return 0;
}


/* This function is used to load a hashentry from genome dictionary and
 * store it in a wordEntry.
 *  @param we word entry where info will be stored.
 *  @param wD genome dictionary file.
 *  @return zero if everything finished well or a negative number in other cases.
 */
void readHashEntry(WordEntry *we,FILE *wD){
	hashentry h;
	// Read hashentry
	if(fread(&h,sizeof(hashentry),1,wD)!=1){
		if(!feof(wD))
			fprintf(stderr, "Couldn't read hashentry.\n");
		return; // End process
	}

	// Store info
	we->metag = false;
	// we->WB <- It is written out of the function 
	we->pos = h.pos;
	we->reps = (uint32_t) h.num;
	int i;
	for(i=0;i<8;++i)
		we->seq[i] = h.w.b[i];
}


/* This function is used to load a hashentry from metagenome dictionary and
 * store it in a wordEntry.
 *  @param we word entry where info will be stored.
 *  @param wD metagenome dictionary file.
 *  @return zero if everything finished well or a negative number in other cases.
 */
int readWordEntrance(WordEntry *we,FILE *wD,uint16_t SeqBytes){
	we->metag = true;
	// Read sequence
	uint16_t i;
	for(i=0;i<SeqBytes;++i)
			if(fread(&we->seq[i],sizeof(unsigned char),1,wD)!=1){
				if(feof(wD)){
					return 1;
				}
				fprintf(stderr, "readWordEntrance:: Error loading sequence\n");
				return -1;
			}
	// Read position
	if(fread(&we->pos,sizeof(uint64_t),1,wD)!=1){
		fprintf(stderr, "readWordEntrance:: Error loading position.\n");
		return -1;
	}
	// Read repetitions
	if(fread(&we->reps,sizeof(uint32_t),1,wD)!=1){
		fprintf(stderr, "readWordEntrance:: Error loading repetitions.\n");
		return -1;
	}
	return 0;
}


/* This function is used to generate hits from two WordEntry coincidents.
 *  @param buff buffer where store hits.
 *  @param X word entry coincident with Y.
 *  @param Y word entry coincident with X.
 *  @param XPFile X sequence locations dictionary file.
 *  @param YPFile Y sequence locations dictionary file.
 *  @param outIndx intermediate file necessary if buffer get filled.
 *  @param outBuff intermediate file necessary if buffer get filled.
 *  @param hitsInBuff words stored in buffer.
 *  @retun a non-negative number if the process finished without errors or a
 *     a negative number in other cases.
 */
int generateHits(Hit* buff,WordEntry X,WordEntry Y,FILE* XPFile,FILE* YPFile,FILE* outIndx, FILE* outBuff, uint64_t* hitsInBuff){
	// Positionate on locations files
	if(fseek(XPFile,X.pos,SEEK_SET)!=0){
		fprintf(stderr, "generateHits:: Error positioning on X file.\n");
		return -1;
	}
	if(fseek(YPFile,Y.pos,SEEK_SET)!=0){
		fprintf(stderr, "generateHits:: Error positioning on Y file.\n");
		return -1;
	}

	// Prepare necessary variables
	uint64_t hitLength = (uint64_t)(X.WB*4);
	LocationEntry X_Arr[X.reps];
	LocationEntry Y_Arr[Y.reps];

	// Load entrances
	loadLocationEntrance(&X_Arr[0],XPFile,X.reps,X.metag);
	loadLocationEntrance(&Y_Arr[0],YPFile,Y.reps,Y.metag);

	// Check buffer space
	if(*hitsInBuff == MAX_BUFF){
		writeHitsBuff(buff,outIndx,outBuff,*hitsInBuff);
		*hitsInBuff=0;
	}

	// Generate all hits
	uint32_t i,j;
	for(i=0; i<X.reps; ++i)
		for(j=0; j<Y.reps; ++j){
			storeHit(&buff[*hitsInBuff],X_Arr[i],Y_Arr[j],hitLength);
			*hitsInBuff+=1;
			// Check buffer space
			if(*hitsInBuff == MAX_BUFF){
				writeHitsBuff(buff,outIndx,outBuff,*hitsInBuff);
				*hitsInBuff=0;
			}			
		}
///////////////////////////////////////////////////////////////////
//fprintf(stderr, "HitsInBuff=%"PRIu64"\n", *hitsInBuff);
///////////////////////////////////////////////////////////////////
	return 0;
}


/* This function is used to load a set of locations entry from a location dictionary file.
 *  @param arr array where locations will be stored.
 *  @param PFile from load the locations.
 *  @param reps number of locations to be loaded.
 *  @param metagenome if it's true PFile must be a new format file, else will be read as 
 *     an old format genome dictionary file.
 */
void loadLocationEntrance(LocationEntry* arr, FILE* PFile, uint32_t reps, bool metagenome){
	uint32_t i;
	if(metagenome){
		for(i=0; i<reps;++i){
			fread(&arr[i].seq,sizeof(uint32_t),1,PFile);
			fread(&arr[i].pos,sizeof(uint64_t),1,PFile);
///////////////////////////////////////////////////////////////////
//fprintf(stderr, "LoadM: Seq:%"PRIu32"  Pos:%"PRIu64"\n", arr[i].seq,arr[i].pos);
///////////////////////////////////////////////////////////////////
		}
	}else{
		location aux;
		for(i=0; i<reps;++i){
			fread(&aux,sizeof(location),1,PFile);
			arr[i].seq = (uint32_t) aux.seq;
			arr[i].pos = aux.pos;
///////////////////////////////////////////////////////////////////
//fprintf(stderr, "LoadG: Seq:%"PRIu32"  Pos:%"PRIu64"\n", arr[i].seq,arr[i].pos);
///////////////////////////////////////////////////////////////////
		}
	}
}


/* This function is used to load necessary info in a Hit variable.
 *  @param hit where info will be stored.
 *  @param X location of hit.
 *  @param Y location of hit.
 *  @param HitLength matched sequence legnth.
 */
inline void storeHit(Hit* hit,LocationEntry X,LocationEntry Y,uint64_t HitLength){
	hit->diag = X.pos - Y.pos;
	hit->posX = X.pos;
	hit->seqX = X.seq;
	hit->posY = Y.pos;
	hit->seqY = Y.seq;
	hit->length = HitLength;
}


/* This function is used to write a buffer in the intermediate files. The order
 * in each dictioanry is:
 *   - Index: each entrance: Pos<uint64_t> HitsInBuff<uint64_t>
 *   - Hits: each entrance: X<uint32_t> Y<uint32_t> Diag<uint64_t> PosX<uint64_t> PosY<uint64_t> Length<uint64_t>
 *  @param buff buffer t be written.
 *  @param index intermediate file.
 *  @param hits intermediate file.
 *  @param hitsInBuff number of words stored on buffer.
 */
void writeHitsBuff(Hit* buff,FILE* index,FILE* hits,uint64_t hitsInBuff){
/////////////////////////////////////////////////////////////////////
//fprintf(stderr, "Quicksort:\n\tUNSORTED\n");
//int k;
//for(k=0;k<hitsInBuff;++k)
//	fprintf(stderr,"\tHit: X=%"PRIu32" Y=%"PRIu32" Diag=%"PRId64" PosX=%"PRIu64" L=%"PRIu64"\n",buff[k].seqX,buff[k].seqY,buff[k].diag,buff[k].posX,buff[k].length);
/////////////////////////////////////////////////////////////////////
	// Sort buffer
	quicksort_H(buff,0,hitsInBuff-1);
/////////////////////////////////////////////////////////////////////
//fprintf(stderr, "\tSORTED\n");
//for(k=0;k<hitsInBuff;++k)
//	fprintf(stderr,"\tHit: X=%"PRIu32" Y=%"PRIu32" Diag=%"PRId64" PosX=%"PRIu64" L=%"PRIu64"\n",buff[k].seqX,buff[k].seqY,buff[k].diag,buff[k].posX,buff[k].length);
/////////////////////////////////////////////////////////////////////

	// Write info on index file
	uint64_t pos = (uint64_t) ftell(hits);
	uint64_t numHits = 0;
	Hit lastHit;
	fwrite(&pos,sizeof(uint64_t),1,index);
	
	// Write first hit
	fwrite(&buff[0].seqX,sizeof(uint32_t),1,hits);
	fwrite(&buff[0].seqY,sizeof(uint32_t),1,hits);
	fwrite(&buff[0].diag,sizeof(int64_t),1,hits);
	fwrite(&buff[0].posX,sizeof(uint64_t),1,hits);
	fwrite(&buff[0].posY,sizeof(uint64_t),1,hits);
	fwrite(&buff[0].length,sizeof(uint64_t),1,hits);
	numHits++;
	lastHit = buff[0];
		
	// Write hits in hits file
	for(pos=1; pos<hitsInBuff; ++pos){
		if(buff[pos].seqX == lastHit.seqX && buff[pos].seqY == lastHit.seqY && buff[pos].diag == lastHit.diag && buff[pos].posX < lastHit.posX + lastHit.length){
////////////////////////////////////////////////////////////////////
//fprintf(stderr, "Hit collapsed\n");
////////////////////////////////////////////////////////////////////
			continue; // Collapsable
		}
///////////////////////////////////////////////////////////////////
//fprintf(stderr, "Hit written.\n");
///////////////////////////////////////////////////////////////////
		fwrite(&buff[pos].seqX,sizeof(uint32_t),1,hits);
		fwrite(&buff[pos].seqY,sizeof(uint32_t),1,hits);
		fwrite(&buff[pos].diag,sizeof(int64_t),1,hits);
		fwrite(&buff[pos].posX,sizeof(uint64_t),1,hits);
		fwrite(&buff[pos].posY,sizeof(uint64_t),1,hits);
		fwrite(&buff[pos].length,sizeof(uint64_t),1,hits);
		lastHit = buff[pos];
		numHits++;
	}
///////////////////////////////////////////////////////////////////
//fprintf(stderr, "NumHitsWritten=%"PRIu64"\n", numHits);
///////////////////////////////////////////////////////////////////
	// Write final number of hits
	fwrite(&numHits,sizeof(uint64_t),1,index);
	buffersWritten++;
}


/* Function used to compare two Hit variables. It function sort with this
 * criterion:
 *    1 - Sequence X
 *    2 - Sequence Y
 *    3 - Diagonal
 *    4 - Position on X
 *    5 - Length 
 *  @param h1 word to be compared.
 *  @param h2 word to be compared
 *  @return zero if w2 are greater or equal and a positive number if
 *     w1 is greater.
 */
int GT(Hit h1, Hit h2){
	if(h1.seqX > h2.seqX) return 1;
	else if(h1.seqX < h2.seqX) return 0;

	if(h1.seqY > h2.seqY) return 1;
	else if(h1.seqY < h2.seqY) return 0;

	if(h1.diag > h2.diag) return 1;
	else if(h1.diag < h2.diag) return 0;

	if(h1.posX > h2.posX) return 1;
	else if(h1.posX < h2.posX) return 0;

	if(h1.length > h2.length) return 1;
	return 0;
}


/* This function is necessary for quicksort functionality.
 *  @param arr array to be sorted.
 *  @param left inde of the sub-array.
 *  @param right index of the sub-array.
 */
int partition(Hit* arr, int left, int right){
   int i = left;
   int j = right + 1;
   Hit t;

   // Pivot variable
   int pivot = (left+right)/2;

   if(GT(arr[pivot],arr[right]))
		 SWAP_H(&arr[pivot],&arr[right],t);

   if(GT(arr[pivot],arr[left]))
		 SWAP_H(&arr[pivot],&arr[left],t);

   if(GT(arr[left],arr[right]))
		 SWAP_H(&arr[left],&arr[right],t);

	while(1){
		do{
			++i;
		}while(!GT(arr[i],arr[left]) && i <= right);

		do{
			--j;
		}while(GT(arr[j],arr[left]) && j >= left);

		if(i >= j) break;

		SWAP_H(&arr[i],&arr[j],t);
	}

	SWAP_H(&arr[left],&arr[j],t);

	return j;
}


/* This function is used to sort a Hit array.
 *  @param arr array to be sorted.
 *  @param left index where start to sort.
 *  @param right index where end sorting action.
 *
 */
void quicksort_H(Hit* arr, int left,int right){
	int j;

	if(left < right){
		// divide and conquer
		j = partition(arr,left,right);
		quicksort_H(arr,left,j-1);
		quicksort_H(arr,j+1,right);
   }
}


/* This function is used to load a hit from a hits intermediate file.
 *  @param hit varaible where loaded hit will be stored.
 *  @param hFile pointer to hits intermediate file.
 */
inline void loadHit(Hit *hit,FILE* hFile){
	fread(&hit->seqX,sizeof(uint32_t),1,hFile);
	fread(&hit->seqY,sizeof(uint32_t),1,hFile);
	fread(&hit->diag,sizeof(int64_t),1,hFile);
	fread(&hit->posX,sizeof(uint64_t),1,hFile);
	fread(&hit->posY,sizeof(uint64_t),1,hFile);
	fread(&hit->length,sizeof(uint64_t),1,hFile);
}


/* This function is used to return the index of the lowest hit on a hit array.
 *  @param hits array where search.
 *  @param length of hits array.
 *  @return the index of the lowest hit on hits array.
 */
uint64_t lowestHit(Hit *hits,uint64_t length,int64_t *unread){
	uint64_t i;
	int64_t j=-1;
	// Search first readable
	for(i=0;i<length && j<0;++i)
		if(unread[i]>=0) j = i;

	for(i=j+1;i<length;++i)
		if(unread[i]>=0)
			if(HComparer(hits[j],hits[i]) > 0) j = i;

	return j;
}


/* This function is used to check if all hits has been read from intermediate file.
 *  @param unread array of unread hits of each buffer segment.
 *  @param length of the unread array.
 *  @return true if there are not unread hits and false in other cases.
 */
bool finished(int64_t *unread, uint64_t length){
	uint64_t i;
	for(i=0; i<length; ++i)
		if(unread[i] >= 0) return false;
	return true;
}


/* This function is used to write a fragment in fragment file. The order 
 * of a fragment entrance is:
 *    X<uint32_t> Y<uint32_t> diag<int64_t> xStart<uint64_t> yStart<uint64_t> xEnd<uint64_t> 
 *        yEnd<uint64_t> length<uint64_t> ident<uint64_t> score<uint64_t> similarity<float> 
 *        block<int64_t> strand<char>
 *  @param frag fragment to be written.
 *  @param fr fragment file where fragment will be written.
 */
inline void writeFragment(FragFile frag,FILE *fr){
	fwrite(&frag.seqX,sizeof(uint32_t),1,fr);
	fwrite(&frag.seqY,sizeof(uint32_t),1,fr);
	fwrite(&frag.diag,sizeof(int64_t),1,fr);
	fwrite(&frag.xStart,sizeof(uint64_t),1,fr);
	fwrite(&frag.yStart,sizeof(uint64_t),1,fr);
	fwrite(&frag.xEnd,sizeof(uint64_t),1,fr);
	fwrite(&frag.yEnd,sizeof(uint64_t),1,fr);
	fwrite(&frag.length,sizeof(uint64_t),1,fr);
	fwrite(&frag.ident,sizeof(uint64_t),1,fr);
	fwrite(&frag.score,sizeof(uint64_t),1,fr);
	fwrite(&frag.similarity,sizeof(float),1,fr);
	fwrite(&frag.block,sizeof(int64_t),1,fr);
	fwrite(&frag.strand,sizeof(char),1,fr);
}


void showWord(unsigned char *w, uint16_t wsize) {
	char Alf[] = { 'A', 'C', 'G', 'T' };
	char ws[wsize*4+4];
	uint16_t i;
	unsigned char c;
	fprintf(stdout, "Word:");
	for (i = 0; i < wsize; i++) {
		c = w[i];
		c = c >> 6;
		ws[4*i] = Alf[(int) c];
		fprintf(stdout,"%c",Alf[(int) c]);
		c = w[i];
		c = c << 2;
		c = c >> 6;
		ws[4*i+1] = Alf[(int) c];
		fprintf(stdout,"%c",Alf[(int) c]);
		c = w[i];
		c = c << 4;
		c = c >> 6;
		ws[4*i+2] = Alf[(int) c];
		fprintf(stdout,"%c",Alf[(int) c]);
		c = w[i];
		c = c << 6;
		c = c >> 6;
		ws[4*i+3] = Alf[(int) c];
		fprintf(stdout,"%c",Alf[(int) c]);
	}
	ws[wsize*4+4] = '\0';
	fprintf(stdout, "\n");
}


/*
 */
void SWAP_H(Hit* h1, Hit* h2, Hit t){
	copyHit(&t,*h1);
	copyHit(h1,*h2);
	copyHit(h2,t);
}

/*
 */
void copyHit(Hit* toCopy, Hit copy){
	toCopy->diag = copy.diag;
	toCopy->posX = copy.posX;
	toCopy->posY = copy.posY;
	toCopy->seqX = copy.seqX;
	toCopy->seqY = copy.seqY;
	toCopy->length = copy.length;
}