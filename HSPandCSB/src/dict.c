/*
 * @author Fernando Moreno Jabato <jabato@uma.es>
 * @description This file encodes the workflow of GECKO for create
 *    metagenome dictionaries.
 * @licence all rights reserved to the author and BitLAB group (University
 *    of Malaga).
 */
#include "dict.h"

/**
 * This main function encodes the workflow of metagenome dictionary
 * creation. The workflow is:
 *   1 - Read a char of a read.
 *   2 - IF sequence is not long and good enough GO TO 1.
 *   3 - Store sequence (word) on words buffer.
 *   4 - IF bufer is full, write buffer sorted on intermediate files.
 *   5 - IF there are more Reads or chars unread on current Read GO 
 *       TO 1.
 *   6 - Write buffered words on intermediate files after sort it.
 *   7 - Read intermediate files word per word and write final dictionary
 *       files.
 */
int main(int ac, char** av){
	// Check arguments
	if(ac!=4){
		fprintf(stderr, "Bad call error.\nUSE: dict metag.IN dictName WL\n");
		return -1;
	}else if(atoi(av[3])%4 != 0){
		fprintf(stderr, "Error WL must be a 4 multiple and it's \"%s\".\n", av[3]);
		return -1;
	}

	// Variables
	uint16_t WL = (uint16_t) atoi(av[3]); // Word length
	BYTES_IN_WORD = WL / 4;
	wentry *buffer; //Buffer of read words
	FILE *metag; // Input files
	FILE *wDic,*pDic;  // Output files
	FILE *bIndx, *wrds; // Intermediate files
	uint64_t readW = 0, wordsInBuffer = 0,i; // Absolute and buffer read words and auxiliar variable(i)
	uint32_t numBuffWritten = 0;
	char *fname;
	int removeIntermediataFiles = 0; // Config it if you want save or not the itnermediate files

	// Allocate necessary memory
	// Memory for buffer
	if((buffer = (wentry*) malloc(sizeof(wentry)*BUFFER_LENGTH))==NULL){
		fprintf(stderr, "Error allocating memory for words buffer.\n");
		return -1;
	}

	// Memory for file names handler
	if((fname = (char*) malloc(sizeof(char)*MAX_FILE_LENGTH))==NULL){
		fprintf(stderr, "Error allocating memory for file names handler.\n");
		return -1;
	}

	// Open current necessary files
	// Open metagenome file
	if((metag = fopen(av[1],"rt"))==NULL){
		fprintf(stderr, "Error opening metagenome file.\n");
		return -1;
	}

	//Open intermediate files
	strcpy(fname,av[2]); // Copy outDic name
	if((bIndx = fopen(strcat(fname,".bindx"),"wb"))==NULL){
		fprintf(stderr, "Error opening buffer index file (write).\n");
		return -1;
	}
	// Write first line of buffer index file (WordLength)
	fwrite(&WL,sizeof(uint16_t),1,bIndx);


	// Open words repo
	strcpy(fname,av[2]);
	if((wrds = fopen(strcat(fname,".wrds"),"wb"))==NULL){
		fprintf(stderr, "Error opening words repository (write).\n");
		return -1;
	}

	// START WORKFLOW
	// Read metagenome file
	// Necessary variables
	uint64_t seqPos = 0, crrSeqL = 0;
	char c; // Auxiliar -> Read char per char
	wentry temp; // Auxiliar word variable
	if((temp.w.b = (unsigned char*) malloc(sizeof(unsigned char)*BYTES_IN_WORD))==NULL){
		fprintf(stderr, "Error allocating memory for temp.\n");
		return -1;
	}
	temp.seq = 0;
	temp.w.WL = WL;

	// Memory for buffer words
	for(i=0;i<BUFFER_LENGTH;++i){
		if((buffer[i].w.b = (unsigned char*)malloc(sizeof(unsigned char)*BYTES_IN_WORD))==NULL){
			fprintf(stderr, "Error allocating space for word.\n");
			return -1;
		}
	}

	// Start to read
	c = fgetc(metag);
	while(!feof(metag)){
		// Check if it's a special line
		if(!isupper(toupper(c))){ // Comment, empty or quality (+) line
			if(c=='>'){ // Comment line
				c = fgetc(metag);
				while(c != '\n') // Avoid comment line
					c = fgetc(metag);
				temp.seq++; // New sequence
				crrSeqL = 0; // Reset buffered sequence length
				seqPos = 0; // Reset index
			}
			c=fgetc(metag); // First char of next sequence
			continue;
		}
		shift_word(&temp.w); // Shift bits sequence
		// Add new nucleotid
		switch (c) {
			case 'A': // A = 00 
				crrSeqL++;
				break;
			case 'C': // C = 01
				temp.w.b[BYTES_IN_WORD-1]|=1;
				crrSeqL++;
				break;
			case 'G': // G = 10
				temp.w.b[BYTES_IN_WORD-1]|=2;
				crrSeqL++;
				break;
			case 'T': // T = 11
				temp.w.b[BYTES_IN_WORD-1]|=3;
				crrSeqL++;
				break;
			default : // Bad formed sequence
				crrSeqL = 0; break;
		}
		seqPos++;
		if(crrSeqL >= (uint64_t)WL){ // Full well formed sequence 
			temp.pos = seqPos - WL; // Take position on read
			// Store the new word
			storeWord(&buffer[wordsInBuffer],temp);
			readW++; wordsInBuffer++;
			if(wordsInBuffer == BUFFER_LENGTH){ // Buffer is full
				if(writeBuffer(buffer,bIndx,wrds,wordsInBuffer) < 0){
					return -1;
				}else{
					// Update info
					//readW += wordsInBuffer;
					wordsInBuffer = 0;
					numBuffWritten++;
				}
			}
		}
		c = fgetc(metag);
	}

	// Free&Close unnecessary varaibles
	fclose(metag);

	// Write buffered words
	if(wordsInBuffer != 0){
		if(writeBuffer(buffer,bIndx,wrds,wordsInBuffer) < 0) return -1;
		else numBuffWritten++;
	}

	// Free buffer space
	freeWArray(buffer,BUFFER_LENGTH);

	// Read Intermediate files and create final dictionary
		// Close write buffer and open read buffers
		fclose(bIndx);
		fclose(wrds);

		strcpy(fname,av[2]); // Copy outDic name
		if((bIndx = fopen(strcat(fname,".bindx"),"rb"))==NULL){
			fprintf(stderr, "Error opening buffer index file (read).\n");
			return -1;
		}
		fread(&WL,sizeof(uint16_t),1,bIndx); // Read header = word length

		// Open words repo
		strcpy(fname,av[2]);
		if((wrds = fopen(strcat(fname,".wrds"),"rb"))==NULL){
			fprintf(stderr, "Error opening words repository (read).\n");
			return -1;
		}

		// Open positions file
		strcpy(fname,av[2]); // Copy outDic name
		if((pDic = fopen(strcat(fname,".d2hP"),"wb"))==NULL){
			fprintf(stderr, "Error opening positions dictionary file.\n");
			return -1;
		}

		// Open dictionary words file 
		strcpy(fname,av[2]); // Copy outDic name
		if((wDic = fopen(strcat(fname,".d2hW"),"wb"))==NULL){
			fprintf(stderr, "Error opening words dictionary file.\n");
			return -1;
		}
		fwrite(&WL,sizeof(uint16_t),1,wDic); // Word length

	// Prepare necessary variables
	uint64_t arrPos[numBuffWritten];
	int64_t wordsUnread[numBuffWritten];
	wentry words[numBuffWritten];
	uint32_t reps = 0;
	uint64_t lastLoaded;

	// Read info about buffers
	i = 0;
	uint64_t aux64;
	do{
		fread(&arrPos[i],sizeof(uint64_t),1,bIndx); // Position on words file
		fread(&aux64,sizeof(uint64_t),1,bIndx); // Number of words on set
		wordsUnread[i] = (int64_t) aux64;
		++i;
	}while(i<numBuffWritten);

	// Take memory for words & read first set of words
	for(i=0 ;i<numBuffWritten ;++i){
		if((words[i].w.b = (unsigned char*)malloc(sizeof(unsigned char)*BYTES_IN_WORD))==NULL){
			fprintf(stderr, "Error allocating space for word.\n");
			return -1;
		}
		fseek(wrds,arrPos[i],SEEK_SET);
		loadWord(&words[i],wrds);
		// Update info
		arrPos[i] = (uint64_t) ftell(wrds);
		wordsUnread[i]--;
	}

	// First entrance
	i = lowestWord(words,numBuffWritten,&wordsUnread[0]);
	int k;
	for(k=0;k<BYTES_IN_WORD;++k)
		fwrite(&words[i].w.b[k],sizeof(unsigned char),1,wDic); // write first word
	uint64_t pos = (uint64_t)ftell(pDic);
	fwrite(&pos,sizeof(uint64_t),1,wDic); // position on pDic
	fwrite(&words[i].seq,sizeof(uint32_t),1,pDic); // Read index
	fwrite(&words[i].pos,sizeof(uint64_t),1,pDic); // Position on read
	reps++; // Increment number of repetitions
	storeWord(&temp,words[i]); // Update last word written
	if(wordsUnread[i] > 0){
		fseek(wrds,arrPos[i],SEEK_SET);
		loadWord(&words[i],wrds);
		lastLoaded = i;
		arrPos[i] = (uint64_t) ftell(wrds);
		wordsUnread[i]--;
	}

	// Write final dictionary file
	while(!finished(&wordsUnread[0],numBuffWritten)){
		i = lowestWord(words,numBuffWritten,&wordsUnread[0]);
		// Store word in buffer
		writeWord(&words[i],wDic,pDic,wordcmp(words[i].w,temp.w,BYTES_IN_WORD)!=0? false:true,&reps);
		storeWord(&temp,words[i]); // Update last word written
		// Load next word if it's possible
		if(wordsUnread[i] > 0){
			if(i != lastLoaded){
				fseek(wrds,arrPos[i],SEEK_SET);
				lastLoaded = i;
			}
			loadWord(&words[i],wrds);
			arrPos[i] = (uint64_t) ftell(wrds);
			wordsUnread[i]--;
		}else wordsUnread[i] = -1;
	}
	// Write last word index
	fwrite(&reps,sizeof(uint32_t),1,wDic); // Write num of repetitions

	// Deallocate words buffer
	for(i=0 ;i<numBuffWritten ;++i) // Free words
		free(words[i].w.b);

	if(removeIntermediataFiles){
		strcpy(fname,av[2]);
		remove(strcat(fname,".bindx"));
		strcpy(fname,av[2]);
		remove(strcat(fname,".wrds"));
	}

	// Free space
	free(fname);
	free(temp.w.b);
	// Everything finished. All it's ok.
	return 0;
}