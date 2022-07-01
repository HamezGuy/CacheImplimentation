
////////////////////////////////////////////////////////////////////////////////
// Main File:        csim.c
// This File:        csim.c
// Semester:         CS 354 Fall 2020
// Instructor:       deppeler
// 
// Discussion Group: group 7
// Author:           james gui
// Email:            jgui2@wisc.edu
// CS Login:         Gui
//
/////////////////////////// OTHER SOURCES OF HELP //////////////////////////////
//                   fully acknowledge and credit all sources of help,
//                   other than Instructors and TAs.
//
// Persons:          Akshat - helped with identifying how to get t bits from the memory
//                  Hanna - helped with rearranging for loops to a more logical order,
//                  also helped with logic comparing t bits instead of using block size
//                  and helping with testing measures 
//
// Online sources:   N/A
//////////////////////////// 80 columns wide ///////////////////////////////////


/**
 * csim.c:  
 * A cache simulator that can replay traces (from Valgrind) and 
 * output statistics for the number of hits, misses, and evictions.
 * The replacement policy is LRU.
 *
 * Implementation and assumptions:
 *  1. Each load/store can cause at most 1 cache miss plus a possible eviction.
 *  2. Instruction loads (I) are ignored.
 *  3. Data modify (M) is treated as a load followed by a store to the same
 *  address. Hence, an M operation can result in two cache hits, or a miss and a
 *  hit plus a possible eviction.
 */  

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

/*****************************************************************************/
/* DO NOT MODIFY THESE VARIABLES *********************************************/

//Globals set by command line args.
int b = 0;  //number of block (b) bits
int s = 0;  //number of set (s) bits
int E = 0;  //number of lines per set

//Globals derived from command line args.
int B;  //block size in bytes: B = 2^b
int S;  //number of sets: S = 2^s

//Global counters to track cache statistics in access_data().
int hit_cnt = 0; 
int miss_cnt = 0; 
int evict_cnt = 0; 

//Global to control trace output
int verbosity = 0;  //print trace if set
/*****************************************************************************/
  
  
//Type mem_addr_t: Use when dealing with addresses or address masks.
typedef unsigned long long int mem_addr_t; 

//Type cache_line_t: Use when dealing with cache lines.
//TODO - COMPLETE THIS TYPE
typedef struct cache_line {                    
    char valid; 
    mem_addr_t tag; 
    int counter;
} cache_line_t; 

//Type cache_set_t: Use when dealing with cache sets
//Note: Each set is a pointer to a heap array of one or more cache lines.
typedef cache_line_t* cache_set_t; 
//Type cache_t: Use when dealing with the cache.
//Note: A cache is a pointer to a heap array of one or more sets.
typedef cache_set_t* cache_t; 

// Create the cache (i.e., pointer var) we're simulating.
cache_t cache;   
//
/**
 * init_cache:
 * Allocates the data structure for a cache with S sets and E lines per set.
 * Initializes all valid bits and tags with 0s.
 */                    
void init_cache() {          

    S = pow(2.0, s); //this is the number of sets avaliable
    B = pow(2.0, b); //this is the number of blocks avaliable

    cache = (cache_t) malloc( sizeof(cache_set_t) * S ); //initialize the cache 
    for(int i = 0; i < S; i ++)
    {
        cache[i] = (cache_set_t) malloc( E  * sizeof(cache_line_t));
        //cache will be a 2d array with S (cache_set_t) * E (cache_line_t);
    }

    if(cache == NULL){
        printf("stderr, an error occurred");
        exit(-1);
    }

    //for loop to initialize array
    for(int i = 0; i < S; i++)
    {
        for(int x = 0; x < E; x ++)
        {
            cache[i][x].valid = false;
            cache[i][x].tag = 0;
            cache[i][x].counter = 0;
        }
    }
    
}
  

/**
 * free_cache:
 * Frees all heap allocated memory used by the cache.
 */                    
void free_cache() {            
    for(int i = 0; i < E; i++)
    {
        free(cache[i]);
        cache[i] = NULL;
    }

    free(cache);
    cache = NULL;
}
   



/**
 *  increments all line counters in the cache  set
 * param cachevalue, a integer value that leads to
 * the cache set
*/
void increment_everything(int cachevalue)
{
    //for loop to all line counters in set. 
    for(int i = 0; i < E; i ++)
    {
        cache[cachevalue][i].counter ++ ;
    }
}


   
/**
 * access_data:
 * Simulates data access at given "addr" memory address in the cache.
 *
 * If already in cache, increment hit_cnt
 * If not in cache, cache it (set tag), increment miss_cnt
 * If a line is evicted, increment evict_cnt
 */                    
void access_data(mem_addr_t addr) {

    mem_addr_t tempTBits = 64 - s - b; //isolating T bits for calculation
    mem_addr_t tempMem = (addr << tempTBits) >> (tempTBits + b);
    //Isolating S bits 
     
    
    int tBitValue = (addr >> (s + b)); //value of t bits
    
    
    int tempInt = (int) tempMem; //value of s bits as a int
    
    //cycles through the lines in the choosen set tempInt (memory location)
    for(int i = 0; i < E; i ++)
    {
        if( cache[tempInt][i].tag == tBitValue)
        {
            if(cache[tempInt][i].valid != true)
            {
                //cache isn't valid then continue
                continue;
            }

            //return a hit and increment for LRU if a hit
            hit_cnt ++;

            increment_everything(tempInt);

            cache[tempInt][i].counter = 0;

            return;
        }
        
    }

    //after conferming above that there is not valid line, this
    //for loop checks if there is an empty line
    for(int i = 0; i < E; i ++)
    {
        if(cache[tempInt][i].valid == false)
            {
                //if empty space (not valid) then increment miss ctr and 
                //increment all lines in set for lru. 
                miss_cnt ++; 
                cache[tempInt][i].valid = true;
                cache[tempInt][i].tag = tBitValue;

                increment_everything(tempInt);

                cache[tempInt][i].counter = 0;
                
                return;
            }
    }

    //else evicts something using LRU
    evict_cnt ++; //increments evict count
    miss_cnt ++; //increments miss count


    int tempCount = cache[tempInt][0].counter;
    int tempLineInt = 0;

    for(int i = 0; i < E; i ++)
    {
        if(cache[tempInt][i].counter > tempCount)
        { 
            tempCount = cache[tempInt][i].counter;
            tempLineInt = i;
        }
    }

    increment_everything(tempInt);
    cache[tempInt][tempLineInt].tag = tBitValue; 
    cache[tempInt][tempLineInt].counter = 0; 
     
  return;
}
  
  
/**
 * replay_trace:
 * Replays the given trace file against the cache.
 *
 * Reads the input trace file line by line.
 * Extracts the type of each memory access : L/S/M
 * TRANSLATE each "L" as a load i.e. 1 memory access
 * TRANSLATE each "S" as a store i.e. 1 memory access
 * TRANSLATE each "M" as a load followed by a store i.e. 2 memory accesses 
 */                    
void replay_trace(char* trace_fn) {           
    char buf[1000];   
    mem_addr_t addr = 0; 
    unsigned int len = 0; 
    FILE* trace_fp = fopen(trace_fn, "r");  

    if (!trace_fp) { 
        fprintf(stderr, "%s: %s\n", trace_fn, strerror(errno)); 
        exit(1);    
    }

    while (fgets(buf, 1000, trace_fp) != NULL) {
        if (buf[1] == 'S' || buf[1] == 'L' || buf[1] == 'M') {
            sscanf(buf+3, "%llx,%u", &addr, &len); 
      
            if (verbosity)
                printf("%c %llx,%u ", buf[1], addr, len); 


            // addesses data based on S, L, or M
            switch (buf[1])
            {
            case 'S':
                access_data(addr);
                break;

            case 'L':
                access_data(addr);
                break;

            case 'M':
                access_data(addr);
                access_data(addr);
                break;

            default:
                break;
            }


            if (verbosity)
                printf("\n"); 
        }
    }

    fclose(trace_fp); 
}  
  
  
/**
 * print_usage:
 * Print information on how to use csim to standard output.
 */                    
void print_usage(char* argv[]) {                 
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]); 
    printf("Options:\n"); 
    printf("  -h         Print this help message.\n"); 
    printf("  -v         Optional verbose flag.\n"); 
    printf("  -s <num>   Number of s bits for set index.\n"); 
    printf("  -E <num>   Number of lines per set.\n"); 
    printf("  -b <num>   Number of b bits for block offsets.\n"); 
    printf("  -t <file>  Trace file.\n"); 
    printf("\nExamples:\n"); 
    printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]); 
    printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]); 
    exit(0); 
}  
  
  
/**
 * print_summary:
 * Prints a summary of the cache simulation statistics to a file.
 */                    
void print_summary(int hits, int misses, int evictions) {                
    printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions); 
    FILE* output_fp = fopen(".csim_results", "w"); 
    assert(output_fp); 
    fprintf(output_fp, "%d %d %d\n", hits, misses, evictions); 
    fclose(output_fp); 
}  
  
  
/**
 * main:
 * Main parses command line args, makes the cache, replays the memory accesses
 * free the cache and print the summary statistics.  
 */                    
int main(int argc, char* argv[]) {                      
    char* trace_file = NULL; 
    char c; 
    
    // Parse the command line arguments: -h, -v, -s, -E, -b, -t 
    while ((c = getopt(argc, argv, "s:E:b:t:vh")) != -1) {
        switch (c) {
            case 'b':
                b = atoi(optarg); 
                break; 
            case 'E':
                E = atoi(optarg); 
                break; 
            case 'h':
                print_usage(argv); 
                exit(0); 
            case 's':
                s = atoi(optarg); 
                break; 
            case 't':
                trace_file = optarg; 
                break; 
            case 'v':
                verbosity = 1; 
                break; 
            default:
                print_usage(argv); 
                exit(1); 
        }
    }

    //Make sure that all required command line args were specified.
    if (s == 0 || E == 0 || b == 0 || trace_file == NULL) {
        printf("%s: Missing required command line argument\n", argv[0]); 
        print_usage(argv); 
        exit(1); 
    }

    //Initialize cache.
    init_cache(); 

    //Replay the memory access trace.
    replay_trace(trace_file); 

    //Free memory allocated for cache.
    free_cache(); 

    //Print the statistics to a file.
    //DO NOT REMOVE: This function must be called for test_csim to work.
    print_summary(hit_cnt, miss_cnt, evict_cnt); 
    return 0;    
}  


// end csim.c
