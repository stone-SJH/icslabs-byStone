// csim.c
// 5130379072 shi jiahao

#include "cachelab.h"
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>

typedef struct{
  int E;
  int s;
  int b;
  int hits;
  int misses;
  int evicts;
} state_t;

typedef struct{//here some changes
  int valid;
  int used_num;//a counter that holds the total number that the line is used to fix the least_recently_used strategy. 
  unsigned long long tag;
  char* blocks;//here change offset->data.
} line_t;

typedef struct{
  line_t* lines;
} set_t;

typedef struct{
  set_t* sets;
} cache_t;

cache_t buildCache(long long set_num,int line_num,long long block_size){
  cache_t new_cache;
  set_t new_set;
  line_t new_line;
  new_cache.sets = (set_t *)malloc(sizeof(set_t) * set_num);
  for (int i = 0; i < set_num; i++){
    new_set.lines = (line_t *)malloc(sizeof(line_t) * line_num);
    new_cache.sets[i] = new_set;
    for(int j = 0; j < line_num; j++){
       new_line.used_num = 0;  
       new_line.valid = 0;
       new_line.tag = 0;
       new_set.lines[j] = new_line;
    }
  }
  return new_cache;
}
int getFreeLine(set_t dst_set,state_t now_state){
  int line_num = now_state.E;
  for (int i = 0; i < line_num; i++){
    if(dst_set.lines[i].valid == 0) return i;
  }
  return -1;  
}
int getEvicts(set_t dst_set,state_t now_state,int *used_lines){
  int line_num = now_state.E;
  int max_used = dst_set.lines[0].used_num;
  int min_used = dst_set.lines[0].used_num;
  int min = 0;
  for(int i = 1; i < line_num; i++){
    if (min_used > dst_set.lines[i].used_num){
       min_used = dst_set.lines[i].used_num;
       min = i;
    }
    if (max_used < dst_set.lines[i].used_num){
       max_used = dst_set.lines[i].used_num;
    }
  }
  used_lines[0] = min_used;
  used_lines[1] = max_used;
  return min;
}

//core
state_t simulate(cache_t cache,state_t now_state,unsigned long long addr){
  int full = 1;
  int line_num = now_state.E;
  int prev_hits = now_state.hits;//comparion to misses
  int tag_size = 64 - now_state.s - now_state.b;
  unsigned long long now_tag = addr >> (now_state.s + now_state.b);
  unsigned long long set_pos = addr << (tag_size) >> (tag_size + now_state.b);
  set_t now_set = cache.sets[set_pos];
  for (int i = 0; i < line_num; i++){
     line_t line = now_set.lines[i];
     if (line.valid){
        if (line.tag == now_tag){
           line.used_num++;
           now_state.hits++;
           now_set.lines[i] = line;
        }
     }
     else if (!line.valid && full) full = 0;
  }
  if (prev_hits == now_state.hits) now_state.misses++;
  else return now_state;
//miss dealing process
  int *used_lines = (int *)malloc(sizeof(int) * 2);
  int min = getEvicts(now_set,now_state,used_lines);
  if (full){
     now_state.evicts++;
     now_set.lines[min].tag = now_tag;
     now_set.lines[min].used_num = used_lines[1]+1;
  }
  else{
     int free = getFreeLine(now_set,now_state);
     now_set.lines[free].used_num = used_lines[1]+1;
     now_set.lines[free].tag = now_tag;
     now_set.lines[free].valid = 1;
  }
  free(used_lines);
  return now_state;
}
void printUsage(char* argv[])
{
  printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
  printf("Options:\n");
  printf("  -h         Print this help message.\n");
  printf("  -v         Optional verbose flag.\n");
  printf("  -s <num>   Number of set index bits.\n");
  printf("  -E <num>   Number of lines per set.\n");
  printf("  -b <num>   Number of block offset bits.\n");
  printf("  -t <file>  Trace file.\n");
  printf("\nExamples:\n");
  printf("  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
  printf("  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
  exit(0);
}
int main(int argc, char **argv)
{
  cache_t cache;
  state_t state;
  memset(&state,0,sizeof(state));
  char *trace_file;
  char c;
    
  while ( (c=getopt(argc,argv,"s:E:b:t:vh"))!= -1){
     switch (c){
     case 's':
       state.s = atoi(optarg);
       break;
     case 'E':
       state.E = atoi(optarg);
       break;
     case 'b':
       state.b = atoi(optarg);
       break;
     case 't':
       trace_file = optarg;
       break;
     case 'v':
       break;
     case 'h':
       printUsage(argv);
       exit(0);
     default:
       printUsage(argv);
       exit(1);
     }
  }
  if (state.s == 0||state.E == 0||state.b == 0||trace_file == NULL){
     printf("%s: Missing required command line argument\n", argv[0]);
     printUsage(argv);
     exit(1);
  }
  long long set_num = 1<<state.s;
  long long block_size = 1<<state.b;
  state.hits = 0;
  state.misses = 0;
  state.evicts = 0;
  cache = buildCache(set_num,state.E,block_size);
  FILE *read_trace = fopen(trace_file,"r");
  char cmd;
  int size;
  unsigned long long addr;
  if (read_trace != NULL){
      while(fscanf(read_trace," %c %llx,%d",&cmd,&addr,&size) == 3)
         switch(cmd){
         case 'I':
            break;
         case 'L':
            state = simulate(cache,state,addr);
            break;
         case 'S':
            state = simulate(cache,state,addr);
            break;
         case 'M':
            state = simulate(cache,state,addr);
            state = simulate(cache,state,addr);
            break;
         default:
            break;
         }
  }
  printSummary(state.hits,state.misses,state.evicts);
  fclose(read_trace);
  return 0;
}
