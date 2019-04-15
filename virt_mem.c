//
//  virt_mem.c
//  virt_mem
//
//  Created by William McCarthy on 3/23/19.
//  Copyright © 2019 William McCarthy. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ARGC_ERROR 1
#define FILE_ERROR 2
#define BUFLEN 256
#define FRAME_SIZE  256


//-------------------------------------------------------------------
unsigned int getpage(size_t x) { return (0xff00 & x) >> 8; }
unsigned int getoffset(unsigned int x) { return (0xff & x); }
unsigned int physMemConvert(unsigned int physAdd, unsigned int off){
	unsigned int dummy = physAdd << 8;
	dummy =  (dummy | off);
	printf("\nDummy: %d\n", dummy);
	return dummy;
}

char * physMem;

unsigned int next_avail_frame = 0;

unsigned int next_avail_tlb = 0;

unsigned int pageTable[FRAME_SIZE];

struct tlb{
	unsigned int page,frame;
};

unsigned int tlbPage[16];
unsigned int tlbFrame[16];

void init(){
	physMem = (char *)malloc (FRAME_SIZE * FRAME_SIZE);
	memset(physMem, -1, FRAME_SIZE);
	for(int i = 0 ; i < FRAME_SIZE; i++){
     		pageTable[i] = -1;
  	}
	for(int i = 0; i < 16; i++){
		tlbPage[i] = -1;
		tlbFrame[i] = -1;
	}

}

int pagetable_getframe(unsigned int page, FILE* fbacking) {

  //if (page > PAGES) { exit(OVERFLOW_ERROR); }

  unsigned int frame;
  printf("pageTable[page]: %d\n ",pageTable[page]);
  size_t nread;
  if ((frame = pageTable[page]) == -1) {
    printf("(pgFlt:%3u), ", page);
    frame = next_avail_frame;
    char* p = physMem + frame * FRAME_SIZE;        // read from backing store to start of frame

    fseek(fbacking, page * FRAME_SIZE, SEEK_SET);    // seek to start of page in backing store (seek from START of file)
    if ((nread = fread(p, sizeof(char), FRAME_SIZE, fbacking)) < FRAME_SIZE) {
      fprintf(stderr, "ERROR: bytes read (%zu) is less than FRAME_SIZE\n", nread);
      //exit(READ_ERROR);
    }
    pageTable[page] = frame;    // update the page table
    tlbPage[next_avail_tlb]= page;    // update the tlb
    tlbFrame[next_avail_tlb] = frame;

    next_avail_tlb = (next_avail_tlb + 1) % 16;    // wraps-around
    ++next_avail_frame;     //    and update next_available_frame
  } else { printf("............ "); }
  return frame;
}


void getpage_offset(unsigned int x) {
  unsigned int page = getpage(x);
  unsigned int offset = getoffset(x);
  printf("x is: %u, page: %u, offset: %u, address: %u, paddress: %u\n", x, page, offset,
         (page << 8) | getoffset(x), page * 256 + offset);
}

int main(int argc, const char * argv[]) {
  init();
  FILE* fadd = fopen("addresses.txt", "r");
  if (fadd == NULL) { fprintf(stderr, "Could not open file: 'addresses.txt'\n");  exit(FILE_ERROR);  }

  FILE* fcorr = fopen("correct.txt", "r");
  if (fcorr == NULL) { fprintf(stderr, "Could not open file: 'correct.txt'\n");  exit(FILE_ERROR);  }
  FILE* backing_store;
  backing_store = fopen("BACKING_STORE.bin", "rb");
  if(backing_store == NULL){
        fprintf(stderr, "Could not open file: 'BACKING_STORE.bin'\n");  exit(FILE_ERROR);
  }

 
  int nextAvailable = 0;

  char buf[BUFLEN];
  unsigned int page, offset, physical_add, frame = 0;
  unsigned int logic_add;                  // read from file address.txt
  unsigned int virt_add, phys_add, value;  // read from file correct.txt
      // not quite correct -- should search page table before creating a new entry
      //   e.g., address # 25 from addresses.txt will fail the assertion
      // TODO:  add page table code
      // TODO:  add TLB code
    int tlbCounter = 0;
  char inTLB = 'f';
  char full = 'f';
  int fullAccum = 0;

  while (frame < 1000) {
    fscanf(fcorr, "%s %s %d %s %s %d %s %d", buf, buf, &virt_add,
           buf, buf, &phys_add, buf, &value);  // read from file correct.txt
    fscanf(fadd, "%d", &logic_add);  // read from file address.txt
    page = getpage(logic_add);
    offset = getoffset(logic_add);
    for(int i = 0; i < 16;i++){
	if(tlbPage[i] == page){
		inTLB = 't';
		tlbCounter = i;
	}
	else if(tlbPage[i] == -1){
		fullAccum++;
	}
    }  
    if(inTLB == 'f'){
    	if(pageTable[page] == -1){
		frame = pagetable_getframe(page,backing_store);
	//frame = nextAvailable;    
	//printf("%d\n",pageTable[page]);
	//tlb[tlbCounter] = page;
	//tlbOffset[tlbCounter] = frame;
	//tlbCounter++;
	//char * p = physMem + frame * FRAME_SIZE;
	//fseek(backing_store,((FRAME_SIZE) * page), SEEK_SET);
   	//fread(p,sizeof(char),FRAME_SIZE,backing_store);
	//pageTable[page] = frame;
	//nextAvailable++;
		inTLB = 'f';
    	}
    }
    else if(fullAccum < 16 && inTLB == 't'){    
	frame = tlbFrame[tlbCounter];
	printf("tlb FRAME: %d\n", frame);	
    }
    else if(fullAccum == 16 && inTLB == 'f'){
	next_avail_tlb = 0;
	next_avail_frame = 0;
	tlbFrame[0] = -1;
	tlbPage[0] = -1;
	pageTable[0] = -1;
	frame = pagetable_getframe(page,backing_store);
	printf("FIFO");
    }
    //create a list to 
    //
    //todo Create a function to left shift the frame and OR it with the offset
    physical_add = physMemConvert(frame,offset);
    printf("Physical Address: %d\n", physical_add);
    int v = 0;
    v = *(physMem + physical_add);
    printf("phys_add: %d\n", phys_add);
    printf("V: %d\n", v);
    printf("Value: %d\n", value);
    if(!(physical_add == phys_add)){
	fullAccum = 0;
	for(int i = 0; i < 16; i++){
		if(tlbPage[i] != -1){
			fullAccum++;
		}
	}
	printf("Full Accum: %d\n", fullAccum);
    } 
    // todo: read BINARY_STORE and confirm value matches read value from correct.txt
    assert(v == value);


    printf("logical: %5u (page:%3u, offset:%3u) ---> physical: %5u -- passed\n", logic_add, page, offset, physical_add);
    if (frame % 5 == 0) { printf("\n"); }
  }
  fclose(fcorr);
  fclose(fadd);
  free(physMem);
  printf("ALL logical ---> physical assertions PASSED!\n");
  printf("!!! This doesn't work passed entry 24 in correct.txt, because of a duplicate page table entry\n");
  printf("--- you have to implement the PTE and TLB part of this code\n");
  printf("\n\t\t...done.\n");
  return 0;
}
