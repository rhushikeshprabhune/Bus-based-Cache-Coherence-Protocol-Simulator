/*******************************************************
                          cache.cc
********************************************************/
#include<math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "cache.h"
using namespace std;

Cache::Cache(int s,int a,int b )
{
   ulong i, j;
   reads = readMisses = writes = writeMisses = writeBacks = 0; 

   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));   
  
   //*******************//
   //initialize your counters here//
   //*******************//
   cc_transfers = invalidations = interventions = memory_transactions = flushes = num_busRdx = 0;
   othercachecopies = 0;
   currentCycle = 0;
   bus_rd = bus_rdx = bus_update = bus_upgr = 0;
   
 
   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
		tagMask <<= 1;
        tagMask |= 1;
   }
   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
	   cache[i][j].invalidate();
      }
   }      
   
}

/**you might add other parameters to Access()
since this function is an entry point 
to the memory hierarchy (i.e. caches)**/
void Cache::AccesstoMSI(Cache** smp_cache, int processor_id, int num_processors, ulong addr,uchar op, int protocol){
    
    bus_rd = bus_rdx = 0;
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
        	
	if(op == 'w'){
	 writes++;
	}
    if(op == 'r'){         
		reads++;
	}
	
	cacheLine * line = findLine(addr);
	if(line == NULL)/*miss*/
	{
		if(op == 'w'){ 
			writeMisses++;
		}
		if(op == 'r'){ 
			readMisses++;
		}

		memory_transactions = memory_transactions + 1;

		cacheLine *newline = fillLine(addr);
		if(newline->getFlags() == M && newline!=0){
			writeBacks++;
			memory_transactions++;
		}
		if(newline!=0){
			newline->setFlags(S);
		}
   		
   		if(op == 'w'){
   			newline->setFlags(M);
   			num_busRdx++;
   			bus_rdx=1;
   		}
   		
   		else if(op == 'r'){
   			newline->setFlags(S);
   			bus_rd=1;
   		}		
	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);

		if((line->getFlags() == S) && (op=='w')){
			line->setFlags(M);
			bus_rdx=1;
			num_busRdx++;
			memory_transactions++;
		}
	}

	if(othercachecopies == 1){
        for(int i=0; i < num_processors; i++){
           	if(i != processor_id){
           	  	 if(bus_rd==1){
           	  		smp_cache[i]->BusRd_MSI(addr);
           	  	 }
           	  	 if(bus_rdx==1){
           	  		smp_cache[i]->BusRdX_MSI(addr);
           	  	 }
            }
        }

        }  

   	}



void Cache::BusRd_MSI(ulong addr){
	cacheLine *line = findLine(addr);
	if(line!=NULL){
		if(line->getFlags()==M){
           flushes++;
           line->setFlags(S);
           writeBacks++;
           interventions++;
           memory_transactions++;
		}
	}
}

void Cache::BusRdX_MSI(ulong addr){
	cacheLine *line = findLine(addr);
	if(line){
	  if(line->getFlags() == M){
		flushes++;
		memory_transactions++;
		writeBacks++;
	  }
		line->invalidate();
		invalidations++;
	  
	}	
}

void Cache::AccesstoMESI(Cache** smp_cache, int processor_id, int num_processors, ulong addr,uchar op, int protocol){
    bus_rd = bus_rdx = bus_upgr = 0;
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
        	
	if(op == 'w') writes++;
	else          reads++;
	
	cacheLine * line = findLine(addr);
	if(line == NULL)/*miss*/
	{
		if(op == 'w') writeMisses++;
		else readMisses++;

        if(othercachecopies == 0)
		memory_transactions++;
	    else
	    cc_transfers++;

		cacheLine *newline = fillLine(addr);
		if(newline->getFlags() == M && newline!=0){
			writeBacks++;
			memory_transactions++;
		}
		if(newline!=0){
			newline->setFlags(S);
		}    
   		
   		if(op == 'w'){
   			newline->setFlags(M);
   			num_busRdx++;
   			bus_rdx++;
   		}
   		
   		else if(op == 'r'){
   			if(othercachecopies == 1){
   			    newline->setFlags(S);
   			}
   		    if(othercachecopies == 0){
   		    	newline->setFlags(E);
   		    }

   			bus_rd++;
   		}		
	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);

		if(line->getFlags() == E && op=='w'){
			line->setFlags(M);
		}

		if(line->getFlags() == S && op=='w'){
			line->setFlags(M);
			bus_upgr++;
		}
	}

	if(othercachecopies == 1){
        for(int i=0; i < num_processors; i++)
           	if(i != processor_id){
           	  	 if(bus_rd==1){
           	  		smp_cache[i]->BusRd_MESI(addr);
           	  	 }
           	  	 if(bus_rdx==1){
           	  		smp_cache[i]->BusRdX_MESI(addr);
           	  	 }
           	  	 if(bus_upgr==1){
           	  	 	smp_cache[i]->BusUpgrade(addr);
           	  	 }

        }       
   	}

}

void Cache::BusRd_MESI(ulong addr){
	cacheLine *line = findLine(addr);
	if(line!=NULL){
		if(line->getFlags()==M){
           flushes++;
           line->setFlags(S);
           writeBacks++;
           interventions++;
           memory_transactions++;
		}

		if(line->getFlags()==E){
			interventions++;
			line->setFlags(S);
		}
	}
}

void Cache::BusRdX_MESI(ulong addr){
	cacheLine *line = findLine(addr);
	if(line!=NULL){
	  if(line->getFlags() == M){
		flushes++;
		memory_transactions++;
		writeBacks++;
		line->invalidate();
		invalidations++;
	  }
	  else if(line->getFlags() == S){
		line->invalidate();
		invalidations++;
	  }
	  else if(line->getFlags() == E){
		line->invalidate();
		invalidations++;
	  }

	}	
}

void Cache::BusUpgrade(ulong addr){
    cacheLine *line = findLine(addr);
	if(line!=NULL){
		if(line->getFlags()==S){
			line->invalidate();
			invalidations++;
		}
	}
}

void Cache::AccesstoDragon(Cache** smp_cache, int processor_id, int num_processors, ulong addr,uchar op, int protocol){
    bus_rd = bus_rdx = bus_update = 0;
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
        	
	if(op == 'w') writes++;
	else          reads++;
	
	cacheLine * line = findLine(addr);
	if(line == NULL)/*miss*/
	{
		memory_transactions++;

		if(op == 'w') writeMisses++;
		else readMisses++;

		cacheLine *newline = fillLine(addr);
		if((newline->getFlags() == M || newline->getFlags() == SM) && newline!=0){
			writeBacks++;
			memory_transactions++;
		}
		if(newline!=0){
			newline->setFlags(S);
		}

   		if(op == 'w'){
   			newline->setFlags(M);					
			bus_rd = 1;
   		}

   		if(othercachecopies == 1){
        	if(op == 'w'){
        		newline->setFlags(SM);
        		//bus_rd=1;
        		bus_update=1;
        	}
        	if(op == 'r'){
        		newline->setFlags(SC);
        		bus_rd=1;
        	}
        }
        else if(othercachecopies == 0){
        	if(op == 'w'){
        		newline->setFlags(M);
        		bus_rd=1;
        	}
        	if(op == 'r'){
        		newline->setFlags(E);
        		bus_rd=1;
        	}
        }    
	}
	else
	{
        /**since it's a hit, update LRU and update dirty flag**/
	 	updateLRU(line);

	 	if(line->getFlags() == E && op=='w'){
	 		line->setFlags(M);
	 	}

	 	if(op=='w'){
	 		if(line->getFlags() == SC){
	 			if(othercachecopies == 0){
	 				line->setFlags(M);
	 				bus_update=1;
	 			}
	 			else if(othercachecopies == 1){
	 				line->setFlags(SM);
	 				bus_update=1;
	 			}   
	 		}
	 		else if(line->getFlags() == SM){
	 			if(othercachecopies == 0){
	 				line->setFlags(M);
	 				bus_update=1;
	 			}
	 			else if(othercachecopies == 1){
	 				bus_update=1;
	 			}   
	 		}	
	 	}
	}
         for(int i=0; i < num_processors; i++){
            	if(i != processor_id){
            	  	 if(bus_rd==1){
            	  		smp_cache[i]->BusRd_Dragon(addr);
            	  	 }
            	  	 if(bus_update==1){
            	  	 	smp_cache[i]->BusUpdate(addr);
            	  	 }
            	}  
         }     
    
}

void Cache::BusRd_Dragon(ulong addr){
	cacheLine *line = findLine(addr);
	if(line!=NULL){
		
		if(line->getFlags()==M){
           //flushes++;
           line->setFlags(SM);
           //writeBacks++;
           interventions++;
           //memory_transactions++;
		}

		if(line->getFlags()==E){
			interventions++;
			line->setFlags(SC);
		}

		if (line->getFlags() == SM){
			flushes++;//even no state transaction, still add flushes counter.
		}


	}

}

void Cache::BusUpdate(ulong addr){
	cacheLine *line = findLine(addr);
	if (line!=NULL)
	{
		if (line->getFlags() == SM){
			line->setFlags(SC);
		}
	}
}

/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
	if(cache[i][j].isValid())
	        if(cache[i][j].getTag() == tag)
		{
		     pos = j; break; 
		}
   if(pos == assoc)
	return NULL;
   else
	return &(cache[i][pos]); 
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
  line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) return &(cache[i][j]);     
   }   
   for(j=0;j<assoc;j++)
   {
	 if(cache[i][j].getSeq() <= min) { victim = j; min = cache[i][j].getSeq();}
   } 
   assert(victim != assoc);
   
   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine * victim = getLRU(addr);
   updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   //if(victim->getFlags() == DIRTY) writeBack(addr); 	
   tag = calcTag(addr);   
   victim->setTag(tag);
   //victim->setFlags(S);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

void Cache::printStats(){ 

double miss_rate = ((readMisses+writeMisses)*100.0f)/(reads+writes);

 printf("01. number of reads:           %lu\n",reads);
 printf("02. number of read misses:        %lu\n",readMisses);
 printf("03. number of writes:             %lu\n",writes);
 printf("04. number of write misses:       %lu\n",writeMisses);
 printf("05. total miss rate:           %.2f%%\n",miss_rate);
 printf("06. number of writebacks:         %lu\n",writeBacks);
 printf("07. number of cache-to-cache transfers: %lu\n",cc_transfers);
 printf("08. number of memory transactions:      %lu\n",memory_transactions);
 printf("09. number of interventions:         %lu\n",interventions);
 printf("10. number of invalidations:         %lu\n",invalidations);
 printf("11. number of flushes:            %lu\n",flushes);
 printf("12. number of BusRdX:             %lu\n",num_busRdx);
}
