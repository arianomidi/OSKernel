#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Defining Constants
#define RAM_SPACE 1000

char *ram[RAM_SPACE];
int nextFreeCell;


void clearRAM(int start, int end){
	for (int i = start; i <= end; i++){
		ram[i] = NULL;
	}

	nextFreeCell = start;
}

int addToRAM(FILE *p, int *start, int *end){
	char buffer[1000];
	int k = nextFreeCell;

	start = nextFreeCell;

	while (!feof(program)){
     		fgets(buffer, 999, p);
		
		if (k < RAM_SPACE && ram[k] == NULL) {
			ram[k] = strdup(buffer);
			k++;
		} else {
			printf("ERROR: Not enough RAM to add program.\n");
			clearRAM(start, k - 1);
			return 1;
		}	
    	}

	nextFreeCell = k;
	end = k - 1;
	return 0;
}

void emptyRAM(){
	clearRAM(0, RAM_SPACE - 1);
}