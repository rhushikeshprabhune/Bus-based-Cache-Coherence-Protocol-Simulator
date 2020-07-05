/*******************************************************
                          main.cc
********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fstream>
using namespace std;

#include "cache.h"

int main(int argc, char *argv[])
{
	
	ifstream fin;
	FILE * pFile;

	if(argv[1] == NULL){
		 printf("input format: ");
		 printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
		 exit(0);
        }

	int cache_size = atoi(argv[1]);
	int cache_assoc= atoi(argv[2]);
	int blk_size   = atoi(argv[3]);
	int num_processors = atoi(argv[4]);
	int protocol   = atoi(argv[5]);
	char *fname =  (char *)malloc(20);
 	fname = argv[6];

	printf("===== 506 Personal information =====\n");
    printf("RHUSHIKESH ANAND PRABHUNE\n");
    printf("rprabhu\n");
    printf("ECE492 Students? NO\n");
    printf("===== 506 SMP Simulator configuration =====\n");
    printf("L1_SIZE: %d\n", cache_size);
    printf("L1_ASSOC: %d\n", cache_assoc);
    printf("L1_BLOCKSIZE: %d\n", blk_size);
    printf("NUMBER OF PROCESSORS: %d\n", num_processors);
    if(protocol==0){
        printf("COHERENCE PROTOCOL: MSI\n");
    }
    if(protocol==1){
    	printf("COHERENCE PROTOCOL: MESI\n");
    }
    if(protocol==2){
    	printf("COHERENCE PROTOCOL: Dragon\n");
    }
    printf("TRACE FILE: trace/%s \n", fname);  
 
	//*********************************************//
        //*****create an array of caches here**********//
	//*********************************************//	
   Cache **smp_cache = new Cache*[num_processors];
	for (int i = 0; i < num_processors; i++)
	{
		smp_cache[i] = new Cache(cache_size, cache_assoc, blk_size);
	}


	pFile = fopen (fname,"r");
	if(pFile == 0)
	{   
		printf("Trace file problem\n");
		exit(0);
	}
	///******************************************************************//
	//**read trace file,line by line,each(processor#,operation,address)**//
	//*****propagate each request down through memory hierarchy**********//
	//*****by calling cachesArray[processor#]->Access(...)***************//
	///******************************************************************//
	int processor_id;
	uchar operation;
	int address;
	while(!feof(pFile)){
		
		fscanf(pFile, "%d", &processor_id);
		fscanf(pFile, " ");
		operation = fgetc(pFile);
		fscanf(pFile, " ");
		fscanf(pFile, "%x", &address);
		fscanf(pFile, "\n");

		smp_cache[processor_id]->othercachecopies = 0;
		for(int j=0; j< num_processors; j++){
			if(j != processor_id){
			    if(smp_cache[j]->findLine(address)){
				smp_cache[processor_id]->othercachecopies = 1;
				}
			}
		}


		switch(protocol){
            case 0: smp_cache[processor_id]->AccesstoMSI(smp_cache, processor_id, num_processors, address, operation, protocol);
                    break;
            case 1: smp_cache[processor_id]->AccesstoMESI(smp_cache, processor_id, num_processors, address, operation, protocol);
                    break;
            case 2: smp_cache[processor_id]->AccesstoDragon(smp_cache, processor_id, num_processors, address, operation, protocol);
                    break;
        }


		// if(protocol==0) {
		// 	smp_cache[processor_id]->AccesstoMSI(smp_cache, processor_id, num_processors, address, operation, protocol);
		// }
		// else if(protocol==1) {
		// 	smp_cache[processor_id]->AccesstoMESI(smp_cache, processor_id, num_processors, address, operation, protocol);
		// }
		// else if(protocol==2) {
		// 	smp_cache[processor_id]->AccesstoDragon(smp_cache, processor_id, num_processors, address, operation, protocol);
		// }
	}
	fclose(pFile);
	//********************************//
	//print out all caches' statistics //
	//********************************//
	for(int j=0;j<num_processors;j++){
		printf("============ Simulation results (Cache %d) ============\n", j);
		smp_cache[j]->printStats();
	}
}
