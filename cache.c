#include <stdio.h>
#include "trace_item.h"
#include "skeleton.h"

#define TRACE_BUFSIZE 1024*1024

static FILE *trace_fd;
static int trace_buf_ptr;
static int trace_buf_end;
static struct trace_item *trace_buf;

// to keep statistics
unsigned int accesses = 0;
unsigned int read_accesses = 0;
unsigned int write_accesses = 0;
unsigned int hits = 0;
unsigned int misses = 0;
unsigned int misses_with_writeback = 0;

void trace_init()
{
    trace_buf = malloc(sizeof(struct trace_item) * TRACE_BUFSIZE);
    
    if (!trace_buf) {
        fprintf(stdout, "** trace_buf not allocatedn");
        exit(-1);
    }
    
    trace_buf_ptr = 0;
    trace_buf_end = 0;
}

void trace_uninit()
{
    free(trace_buf);
    fclose(trace_fd);
}

int trace_get_item(struct trace_item **item)
{
    int n_items;
    
    if (trace_buf_ptr == trace_buf_end) {
        // get new data
        n_items = fread(trace_buf, sizeof(struct trace_item), TRACE_BUFSIZE, trace_fd);
        if (!n_items) return 0;
        
        trace_buf_ptr = 0;
        trace_buf_end = n_items;
    }
    
    *item = &trace_buf[trace_buf_ptr];
    trace_buf_ptr++;
    
    return 1;
}

int main(int argc, char **argv)
{
    struct trace_item *tr_entry;
    size_t size;
    char *trace_file_name;
    int trace_view_on, cache_size, block_size;
    int associativity, replacement_policy;
	enum cache_policy policy;
	
	//define default
	trace_view_on = 1;
	cache_size = 1; //1 KB
	block_size = 4; //4 bytes = 1 word
	associativity = 1; //1-way associativity
	replacement_policy = 0; //0 for LRU, 1 for FIFO
    
    if (argc == 1) {
        fprintf(stdout, "nUSAGE: tv <trace_file> <switch - any character>n");
        fprintf(stdout, "n(switch) to turn on or off individual item view.nn");
        exit(0);
    }
   
    trace_file_name = argv[1];
	
	if (argc == 7)
	{
		trace_view_on = atoi(argv[2]) ;
		// here you should extract the cache parameters from the command line
		cache_size = atoi(argv[3]); //in kilobytes
		block_size = atoi(argv[4]);
		associativity = atoi(argv[5]);
		if ((cache_size == 1) || (cache_size%2 != 0) || (block_size%2 != 0) || (associativity == 1) || (associativity%2 != 0)) { //should be restricted to the power of 2
			fprintf(stdout, "Cache size, block size, and block associativity have to be a power of 2.");
			exit(0);		
		}
		replacement_policy = atoi(argv[6]);
		if ( !(replacement_policy == 0 || replacement_policy == 1) ){
		fprintf(stdout, "\nMake sure that you pick either 0 for LRU replacement or 1 for FIFO replacement.");
		fprintf(stdout, " %d is not a valid number.", replacement_policy);
		exit(0);
	}
	}
   
    trace_view_on = (argc == 3); // What?
    
    // here you should extract the cache parameters from the command line
    
    cache_size = atoi(argv[4]);
    block_size = atoi(argv[5]);
    associativity = atoi(argv[6]);
    replacement_policy = atoi(argv[7]);
    
    if(replacement_policy){
        policy = FIFO;
    }
    else{
        policy = LRU;
    }
	
    fprintf(stdout, "n ** opening file %sn", trace_file_name);
    
    trace_fd = fopen(trace_file_name, "rb");
    
    if (!trace_fd) {
        fprintf(stdout, "ntrace file %s not opened.nn", trace_file_name);
        exit(0);
    }
    
    trace_init();
    
	//print back all the parameters
	printf("\n\nParameters:");
	printf("\nTrace Name: %s", trace_file_name);
	printf("\nCache Size: %d KBYTES", cache_size);
	printf("\nBlock Size: %d BYTES", block_size);
	printf("\nAssociativity: %d", associativity);
	if (replacement_policy == 0) {
		printf("\nReplacement Policy: LRU");
	}
	else {
		printf("\nReplacement Policy: FIFO");			
	}
	
    // here should call cache_create(cache_size, block_size, associativity, replacement_policy)
    //create_cache(cache_size, block_size, associativity, policy);
    
    while(1) {
        size = trace_get_item(&tr_entry);
        
        if (!size) {       /* no more instructions to simulate */
			printf("\n\nResults:");
			printf("\nCache Accesses: %d", accesses);
			printf("\nCache Read Accesses: %d", read_accesses);
			printf("\nCache Write Accesses: %d", write_accesses);
			printf("\nCache Hits: %d", hits);
			printf("\nCache Misses: %d", misses);
			printf("\nCache Writebacks: %d", misses_with_writeback);
            break;
        }
        else{              /* process only loads and stores */;
            if (tr_entry->type == ti_LOAD) {
                if (trace_view_on) printf("LOAD %x n",tr_entry->Addr) ;
                accesses ++;
                read_accesses++ ;
                // call cache_access(struct cache_t *cp, tr_entry->Addr, access_type)
            }
            if (tr_entry->type == ti_STORE) {
                if (trace_view_on) printf("STORE %x n",tr_entry->Addr) ;
                accesses ++;
                write_accesses++ ;
                // call cache_access(struct cache_t *cp, tr_entry->Addr, access_type)
            }
            // based on the value returned, update the statisctics for hits, misses and misses_with_writeback
        }
		if(trace_view_on == 1) {
			printf("\n\nTrace:");
			printf("\nCache Accesses: %d", accesses);
			printf("\nCache Read Accesses: %d", read_accesses);
			printf("\nCache Write Accesses: %d", write_accesses);
			printf("\nCache Hits: %d", hits);
			printf("\nCache Misses: %d", misses);
			printf("\nCache Writebacks: %d", misses_with_writeback);
		}
    }
	
    trace_uninit();
    
    exit(0);
}