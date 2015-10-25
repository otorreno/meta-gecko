/* @file fragsFun.c
 * @author Fernando Moreno Jabato <jabato@uma.es>
 * @description this file contains the functions necesary for
 * 		correct operation of frags.c code. 
 */

#include "frags.h"


int readGenomeSet(char* genomeSetPath,dictionaryG* genomes){
	// Variables
	DIR *gFolder;
	struct dirent *ent;
	char *wD, *pD;
	int numGenomes = 0, currentMax = MAX_GENOME_SET;
  FILE *WD, *PD;

  // Allocate memory for genome dirs
  if((genomes = (dictionaryG*) malloc(sizeof(dictionaryG)*MAX_GENOME_SET))==NULL){
    fprintf(stderr, "Error allocating memory for genome dictionary set.\n");
    return -1;
  }

	// Open genomes folder
	if((gFolder = opendir(genomeSetPath))==NULL){
		fprintf(stderr, "Error opening genomes folder.\n");
		return -1;
  }

	// Take genome dictionary files
	while((ent = readdir(gFolder))!=NULL){
    // Check for memory
    if(numGenomes>=currentMax){
      if((genomes = realloc(genomes,sizeof(dictionaryG)*(currentMax + MAX_GENOME_SET)))==NULL){
        fprintf(stderr, "Error reallicatin memory for genome dictionary set.\n");
        return -1;
      }
    }
		// Files are sorted alphabetically
		// Should appear first d2hP than d2hW
		if(strstr(ent->d_name,".d2hP")!=NULL){ // New dictionary
			// Save name
			memcpy(genomes[numGenomes].name,ent->d_name,strlen(ent->d_name)-5);
			// Save location dictionary
			memcpy(genomes[numGenomes].P,ent->d_name,sizeof(ent->d_name));
			//Next file should be d2hW dictionary
			if((ent = readdir(gFolder))==NULL){
				fprintf(stderr, "Error: incomplete genome pair dictionary. End of file list.\n");
				return -1;
			}
			if(strstr(ent->d_name,".d2hW")!=NULL){
				// Save word dictionary
				memcpy(genomes[numGenomes].W,ent->d_name,sizeof(ent->d_name));
				numGenomes++;
			}else if(strstr(ent->d_name,".metag.d2hR")!=NULL){
        fprintf(stderr, "Error: it's a metagenome dictionary.\n");
        return -1;
      }else{
				fprintf(stderr, "Error: incomplete genome pair dictionary.\n");
				return -1;
			}
		}
	}

  // Close dir
  closedir(gFolder);

  return numGenomes;
}