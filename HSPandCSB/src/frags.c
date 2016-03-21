/*
 * @author Fernando Moreno Jabato <jabato@uma.es>
 * @description This file encodes the workflow of GECKO for create
 *    metagenome dictionaries.
 * @licence all rights reserved to the author and BitLAB group (University
 *    of Malaga).
 */
#include "frags.h"

int main(int ac, char** av){
	// Check arguments
	if(ac!=9){
		fprintf(stderr, "Bad call error.\nUSE: frags metagDic metagFile genoDic genoFile out minS minL f/r\n");
		return -1;
	}

	// Variables
	FILE *mW,*mP,*gW,*gP; // Dictionaries
	FILE *hIndx,*hts; // Intermediate files
	FILE *fr; // Fragments file
	Hit *buffer;
	uint64_t hitsInBuffer = 0, genomeLength, nStructs;
	uint16_t gWL = 32,mWL;
	uint16_t BytesGenoWord = 8, BytesMetagWord, MinBytes, MaxBytes;
	buffersWritten = 0;
	S_Threshold = (uint64_t) atoi(av[6]);
	L_Threshold = (uint64_t) atoi(av[7]);
	Sequence *genome;
	Read *metagenome;
	char *fname;
	bool removeIntermediataFiles = true;	

	// Allocate necessary memory
	// Memory for buffer
	if((buffer = (Hit*) malloc(sizeof(Hit)*MAX_BUFF))==NULL){
		fprintf(stderr, "Error allocating memory for hits buffer.\n");
		return -1;
	}

	// Memory for file names handler
	if((fname = (char*) malloc(sizeof(char)*MAX_FILE_LENGTH))==NULL){
		fprintf(stderr, "Error allocating memory for file names handler.\n");
		return -1;
	}

	// Open current necessary files
	// Open metagenome positions file
	strcpy(fname,av[1]);
	if((mP = fopen(strcat(fname,".d2hP"),"rb"))==NULL){
		fprintf(stderr, "Error opening metagenome positions dictionaries.\n");
		return -1;
	}

	// Open metagenome words file
	strcpy(fname,av[1]);
	if((mW = fopen(strcat(fname,".d2hW"),"rb"))==NULL){
		fprintf(stderr, "Error opening metagenome words dictionaries.\n");
		return -1;
	}
	// Read words header = WordLength
		// Check
		if(fread(&mWL,sizeof(uint16_t),1,mW)!=1){ 
			fprintf(stderr, "Error, couldn't find word length.\n");
			return -1;
		}else if(mWL % 4 != 0){
			fprintf(stderr, "Error, word length of metagenome dictionary isn't a 4 multiple.\n");
			return -1;
		}else{
			BytesMetagWord = mWL/4;
			if(BytesMetagWord != BytesGenoWord){
				fprintf(stderr, "Error: metagenome and genome dictionaries have differents word lengths.\n");
				return -1;
			}
		}

	// Open genome postions file
	strcpy(fname,av[3]);
	if((gP = fopen(strcat(fname,".d2hP"),"rb"))==NULL){
		fprintf(stderr, "Error opening genome positions dictionaries.\n");
		return -1;
	}

	// Open genome words file
	strcpy(fname,av[3]);
	if((gW = fopen(strcat(fname,".d2hW"),"rb"))==NULL){
		fprintf(stderr, "Error opening genome words dictionaries.\n");
		return -1;
	}

	//Open intermediate files
	strcpy(fname,av[5]); // Copy outDic name
	if((hIndx = fopen(strcat(fname,".hindx"),"wb"))==NULL){
		fprintf(stderr, "Error opening buffer index file.\n");
		return -1;
	}

	// Open hits repo
	strcpy(fname,av[5]);
	if((hts = fopen(strcat(fname,".hts"),"wb"))==NULL){
		fprintf(stderr, "Error opening hits repository.\n");
		return -1;
	}
//////////////////////////////////////////////////////////////////////////
//fprintf(stderr, "TEST\n");
//////////////////////////////////////////////////////////////////////////

	// Search hits
		// Prepare necessary variables
		WordEntry we[2]; // [0]-> Metagenome [1]-> Genome
		// Take memory
		if((we[0].seq = (unsigned char *) malloc(sizeof(unsigned char)*BytesMetagWord))==NULL){
			fprintf(stderr, "Error allocating memory for metagenome entrance.\n");
			return -1;
		}else we[0].WB = BytesMetagWord;

		if((we[1].seq = (unsigned char *) malloc(sizeof(unsigned char)*BytesGenoWord))==NULL){
			fprintf(stderr, "Error allocating memory for metagenome entrance.\n");
			return -1;
		}else we[1].WB = BytesGenoWord;

	// Read first entrances
	if(readWordEntrance(&we[0],mW,BytesMetagWord)<0) return -1;
	readHashEntry(&we[1],gW);

	// Search
	int cmp;
	while(!feof(mW) && !feof(gW)){
		if((cmp = wordcmp(we[0].seq,we[1].seq,BytesGenoWord))==0) // Hit
			generateHits(buffer,we[0],we[1],mP,gP,hIndx,hts,&hitsInBuffer);
		// Load next word
		if(cmp >= 0) // New genome word is necessary
			readHashEntry(&we[1],gW);
		if(cmp <= 0) // New metagenome word is necessary
			if(readWordEntrance(&we[0],mW,BytesMetagWord)<0) return -1;
	}
//////////////////////////////////////////////////////////////////////////
//fprintf(stderr, "TEST3\n");
//////////////////////////////////////////////////////////////////////////

	// Write buffered hits
	if(hitsInBuffer > 0){
		if(buffersWritten > 0)
			writeHitsBuff(buffer,hIndx,hts,hitsInBuffer);
		else{ // Only one buffer
//////////////////////////////////////////////////////////////////////////
//fprintf(stderr, "\tONE ONLY\n");
//////////////////////////////////////////////////////////////////////////
			// Load sequences
			genome = LeeSeqDB(av[4], &genomeLength, &nStructs);
			metagenome = LoadMetagenome(av[2]);

			// Sort buffer
			quicksort_H(buffer,0,hitsInBuffer-1);
			
			// Close unnecesary files
			fclose(mW); fclose(gW);
			fclose(mP); fclose(gP);
			fclose(hIndx);
			fclose(hts);

			// Free unnecesary variables 
			free(we[0].seq);
			free(we[1].seq);

			// Declare necessary variables
			FragFile frag;
			uint64_t index;
			float newSimilarity;
			int64_t dist;
			
			// Init first fragment
			frag.block = 0;
			frag.strand = av[8][0];
//////////////////////////////////////////////////////////////////////////
//fprintf(stderr, "TEST1\n");
//////////////////////////////////////////////////////////////////////////

			// Write first fragment
			Read *currRead;
				// Search first read
				currRead = metagenome;
				while(currRead->seqIndex != buffer[0].seqX){
					if(currRead->next == NULL){
						fprintf(stderr, "Error searching first read.\n");
						return -1;
					}
					currRead = currRead->next;
				}
//////////////////////////////////////////////////////////////////////////
//fprintf(stderr, "TEST2\n");
//////////////////////////////////////////////////////////////////////////
			// Open final files
			// Open final fragments file
			strcpy(fname,av[5]); // Copy outDic name
			if((fr = fopen(strcat(fname,".frags"),"wb"))==NULL){
				fprintf(stderr, "Error opening fragments final file.\n");
				return -1;
			}

			// Generate first fragment
			FragFromHit(&frag,&buffer[0],currRead,genome,genomeLength,nStructs,fr);
			
//////////////////////////////////////////////////////////////////////////
//fprintf(stderr, "TEST4 - %"PRIu64"\n",hitsInBuffer);
//////////////////////////////////////////////////////////////////////////

			// Generate fragments
			for(index=1; index < hitsInBuffer; ++index){
				if(buffer[index].diag == frag.diag &&
						buffer[index].seqX == frag.seqX && 
						buffer[index].seqY == frag.seqY){ // Possible fragment
					// Check if are collapsable
					dist = (int64_t)(buffer[index].posX - frag.xEnd);
					if(dist > hitLength){ // Not collapsable by extension
						// Generate fragment 
						FragFromHit(&frag, &buffer[index],currRead,genome,genomeLength,nStructs,fr);
					}
				}else{ // New fragment
					// Check correct read index
					//currRead = metagenome;
					while(currRead->seqIndex != buffer[index].seqX){
						if(currRead->next == NULL){
							fprintf(stderr, "Error searching read index.\n");
							return -1;
						}
						currRead = currRead->next;
					}
					// Generate new fragment
					FragFromHit(&frag, &buffer[index],currRead,genome,genomeLength,nStructs,fr);
				}
			}
//////////////////////////////////////////////////////////////////////////
//fprintf(stderr, "TEST5\n");
//////////////////////////////////////////////////////////////////////////

			// Close output file
			fclose(fr);
			freeReads(&metagenome);

			// Free unnecesary memory
			free(buffer);
//////////////////////////////////////////////////////////////////////////
//fprintf(stderr, "TEST6\n");
//////////////////////////////////////////////////////////////////////////
			// Remove intermediate files
			if(removeIntermediataFiles){
				strcpy(fname,av[5]);
				remove(strcat(fname,".hts"));
				strcpy(fname,av[5]);
				remove(strcat(fname,".hindx"));
			}

			free(fname);

			// End program
			return 0;
		}
	}else if(hitsInBuffer == 0 && buffersWritten == 0){
		// Free auxiliar buffers
		free(we[0].seq);
		free(we[1].seq);
		free(buffer);

		// Close files
		fclose(mW); fclose(gW);
		fclose(mP); fclose(gP);
		fclose(hIndx);
		fclose(hts);
		freeReads(&metagenome);

		// Remove intermediate files
		if(removeIntermediataFiles){
			strcpy(fname,av[5]);
			remove(strcat(fname,".hts"));
			strcpy(fname,av[5]);
			remove(strcat(fname,".hindx"));
		}

		free(fname);

		// End program
		return 0;
	}

	// Free auxiliar buffers
	free(we[0].seq);
	free(we[1].seq);
	free(buffer);

	// Close files
	fclose(mW); fclose(gW);
	fclose(mP); fclose(gP);
	fclose(hIndx);
	fclose(hts);

	// Load sequences
	genome = LeeSeqDB(av[4], &genomeLength, &nStructs);
	metagenome = LoadMetagenome(av[2]);
//////////////////////////////////////////////////////////////////////////
//fprintf(stderr, "TEST4\n");
//////////////////////////////////////////////////////////////////////////

	// Open necessary files
	// Open intermediate files
	// Index file
	strcpy(fname,av[5]); // Copy outDic name
	if((hIndx = fopen(strcat(fname,".hindx"),"rb"))==NULL){
		fprintf(stderr, "Error opening buffer index file (read).\n");
		return -1;
	}

	// Open hits repo
	strcpy(fname,av[5]);
	if((hts = fopen(strcat(fname,".hts"),"rb"))==NULL){
		fprintf(stderr, "Error opening hits repository (read).\n");
		return -1;
	}

	// Open final fragments file
	strcpy(fname,av[5]); // Copy outDic name
	if((fr = fopen(strcat(fname,".frags"),"wb"))==NULL){
		fprintf(stderr, "Error opening fragments final file.\n");
		return -1;
	}

	// Prepare necessary variables
	node *hitsList = NULL;
	uint64_t hitsUnread[buffersWritten];
	uint64_t positions[buffersWritten];
	uint64_t lastLoaded, activeBuffers = buffersWritten;
	Hit *HitsBlock;
	FragFile frag;

	// Take memory for hits
	if((HitsBlock = (Hit*) malloc(sizeof(Hit)*activeBuffers*READ_BUFF_LENGTH))==NULL){
		fprintf(stderr, "Error allocating memory for hits block.\n");
		return -1;
	}


	// Read buffers info
	uint64_t i = 0;
	do{
		fread(&positions[i],sizeof(uint64_t),1,hIndx);
		fread(&hitsUnread[i],sizeof(uint64_t),1,hIndx);
		++i;
	}while(i < activeBuffers);

	// Load first hits
	node *currNode;
	uint64_t read, blockIndex = 0;
	for(i=0 ;i<activeBuffers; ++i, blockIndex += READ_BUFF_LENGTH){
		currNode = (node*) malloc(sizeof(node));
		currNode->next = hitsList;
		currNode->hits = &HitsBlock[blockIndex];
		currNode->buff = i;
		fseek(hts,positions[i],SEEK_SET);
		read = loadHit(&currNode->hits,hts,hitsUnread[i]);
		currNode->index = 0;
		currNode->hits_loaded = read;
		// Update info
		positions[i] = (uint64_t) ftell(hts);
		hitsUnread[i]-=read;
		lastLoaded = i;
		hitsList = currNode;
	}

	// Assign head
	hitsList = currNode;

	// Sort hits
	sortList(&hitsList);	

	// Init fragment info
	frag.block = 0;
	frag.strand = av[8][0];

	// Write first fragment
	Read *currRead;
		// Search first read
		currRead = metagenome;
		while(currRead->seqIndex != hitsList->hits[0].seqX){
			if(currRead->next == NULL){
				fprintf(stderr, "Error searching first read.\n");
				return -1;
			}
			currRead = currRead->next;
		}

	// Generate first fragment
	FragFromHit(&frag,&hitsList->hits[0],currRead,genome,genomeLength,nStructs,fr);

	// Search hits and generate fragmetents
	int64_t dist;
//////////////////////////////////////////////////////////////////////////
//fprintf(stderr, "TEST5\n");
//////////////////////////////////////////////////////////////////////////

	// Read hits & generate fragments
	while(activeBuffers > 0){
		if(hitsList->hits[hitsList->index].diag == frag.diag &&
			hitsList->hits[hitsList->index].seqX == frag.seqX && 
				hitsList->hits[hitsList->index].seqY == frag.seqY){ // Possible fragment
			// Check if are collapsable
//////////////////////////////////////////////////////////////////////////
//fprintf(stderr, "A -> ");
//////////////////////////////////////////////////////////////////////////
			dist = (int64_t)hitsList->hits[hitsList->index].posX - (int64_t)frag.xEnd;
			if(dist > hitLength){ // Not collapsable by xtension
//////////////////////////////////////////////////////////////////////////
//fprintf(stderr, "B -> ");
//////////////////////////////////////////////////////////////////////////
				// Generate fragment 
				FragFromHit(&frag, &hitsList->hits[hitsList->index],currRead,genome,genomeLength,nStructs,fr);
			}
		}else{ // Different diag or seq
//////////////////////////////////////////////////////////////////////////
//fprintf(stderr, "C -> ");
//////////////////////////////////////////////////////////////////////////
			// Check correct read index
			//currRead = metagenome;
			while(currRead->seqIndex != hitsList->hits[hitsList->index].seqX){
				if(currRead->next == NULL){
					fprintf(stderr, "Error searching read index.\n");
					return -1;
				}
				currRead = currRead->next;
			}
//////////////////////////////////////////////////////////////////////////
//fprintf(stderr, "D -> ");
//////////////////////////////////////////////////////////////////////////
			// Generate new fragment
			FragFromHit(&frag, &hitsList->hits[hitsList->index],currRead,genome,genomeLength,nStructs,fr);
		}
//////////////////////////////////////////////////////////////////////////
//fprintf(stderr, "E -> ");
//////////////////////////////////////////////////////////////////////////

		// Move to next
		hitsList->index +=1;
		// Load new hit
		if(hitsList->index >= hitsList->hits_loaded){
			if(hitsUnread[hitsList->buff] > 0){
				if(hitsList->buff != lastLoaded){
					fseek(hts,positions[hitsList->buff],SEEK_SET);
					lastLoaded = hitsList->buff;
				}
				read = loadHit(&hitsList->hits,hts,hitsUnread[hitsList->buff]);
				hitsList->index = 0;
				hitsList->hits_loaded = read;
				positions[hitsList->buff] = (uint64_t) ftell(hts);
				hitsUnread[hitsList->buff]-=read;
				checkOrder(&hitsList,false);
//////////////////////////////////////////////////////////////////////////
//fprintf(stderr, "(%"PRIu64") [%"PRIu64" - %"PRIu64"] -> ",activeBuffers,hitsList->buff,hitsUnread[hitsList->buff]);
//////////////////////////////////////////////////////////////////////////
			}else{
//////////////////////////////////////////////////////////////////////////
//fprintf(stderr, "+++ (%"PRIu64")-> ",activeBuffers-1);
//////////////////////////////////////////////////////////////////////////
				checkOrder(&hitsList,true);
				activeBuffers--;
			}
		}else{
			checkOrder(&hitsList,false);
//////////////////////////////////////////////////////////////////////////
//fprintf(stderr, "2 -> ");
//////////////////////////////////////////////////////////////////////////
		}
	}
//////////////////////////////////////////////////////////////////////////
//fprintf(stderr, "TEST6\n");
//////////////////////////////////////////////////////////////////////////

	// Close files
	free(HitsBlock);
	fclose(hIndx);
	fclose(hts);
	fclose(fr);
	freeReads(&metagenome);
//////////////////////////////////////////////////////////////////////////
//fprintf(stderr, "TEST7\n");
//////////////////////////////////////////////////////////////////////////

	// Remove intermediate files
	if(removeIntermediataFiles){
		strcpy(fname,av[5]);
		remove(strcat(fname,".hts"));
		strcpy(fname,av[5]);
		remove(strcat(fname,".hindx"));
	}

	free(fname);

	// Everything finished OK
	return 0;
}