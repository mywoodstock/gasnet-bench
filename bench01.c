/***
  *  Project:
  *
  *  File: bench01.c
  *  Created: Aug 02, 2015
  *
  *  Author: Abhinav Sarje <asarje@lbl.gov>
  */

#include <stdio.h>
#include <sys/time.h>

#define GASNET_SEQ 1
#define GASNET_CONDUIT_ARIES 1

#include <gasnet.h>
#include <gasnet_vis.h>


static int gasnet_started = 0;
static gasnet_seginfo_t *myseginfo_table;


typedef struct {
  int node;
  int stridelevels;
  void *dst;
  void *src;
  size_t *dstride;
  size_t *sstride;
  size_t *count;
} gets_op_t;


void print_strided_transfer(int p, void *dst, const size_t *dstride,
                            void *src, const size_t *sstride,
                            const size_t *count, int levels) {
  printf("GASNET %d <- %d: { src: %p, dst: %p, levels: %d, sstride: ",
         gasnet_mynode(), p, src, dst, levels);
  for(int i = 0; i < levels; ++ i) printf("%d, ", sstride[i]);
  printf("dstride: ");
  for(int i = 0; i < levels; ++ i) printf("%d, ", dstride[i]);
  printf("count: ");
  for(int i = 0; i < levels + 1; ++ i) printf("%d, ", count[i]);
  printf("}\n");
  fflush(stdout);
} // print_strided_transfer()


void wait_all_get() {
  gasnet_wait_syncnbi_gets();
} // wait_all_get()

// initiate individual get operations
int do_get(int p, void *dst, const size_t *dstride,
           void *src, const size_t *sstride,
           const size_t *count, int levels) {
  switch(levels) {
    case 0:
      gasnet_get_nbi(dst, p, src, count[0]);
      break;

    case 1:
      for(int i = 0; i < count[1]; ++ i) {
        gasnet_get_nbi(dst + dstride[0] * i, p, src + sstride[0] * i, count[0]);
      } // for
      break;

    default:
      printf("%d: The case with levels > 1 has not been implemented!\n", gasnet_mynode());
      return GASNET_ERR_RESOURCE;
  } // switch()
  return GASNET_OK;
} // do_get()


// initiate a single gets operation
int do_gets(int p, void *dst, const size_t *dstride,
            void *src, const size_t *sstride,
            const size_t *count, int levels) {
  gasnet_gets_nbi_bulk(dst, dstride, p, src, sstride, count, levels);
  return GASNET_OK;
} // do_gets()


int read_gets_operations(char* fname, gets_op_t** ops) {
  int num_ops = 0;
  FILE * fp = fopen(fname, "r");
  char buf[200];
  while(fgets(buf, 200, fp) != NULL) ++ num_ops;
  num_ops /= 5;   // each operation takes 5 lines
  num_ops -= 1;   // the first 5 lines are header
  // rewind the fp
  rewind(fp);

  (*ops) = (gets_op_t*) malloc(num_ops * sizeof(gets_op_t));

  for(int i = 0; i < 5; ++ i) fgets(buf, 200, fp);  // ignore the header
  for(int i = 0; i < num_ops; ++ i) {
    int node, levels;
    size_t *count, *dstride, *sstride;
    fscanf(fp, "%d", &node);
    fscanf(fp, "%d", &levels);
    count = (size_t*) malloc((levels + 1) * sizeof(size_t));
    dstride = (size_t*) malloc(levels * sizeof(size_t));
    sstride = (size_t*) malloc(levels * sizeof(size_t));
    for(int j = 0; j < levels + 1; ++ j) fscanf(fp, "%d", &(count[j]));
    for(int j = 0; j < levels; ++ j) fscanf(fp, "%d", &(dstride[j]));
    for(int j = 0; j < levels; ++ j) fscanf(fp, "%d", &(sstride[j]));
    (*ops)[i].node = node;
    (*ops)[i].stridelevels = levels;
    (*ops)[i].count = count;
    (*ops)[i].dstride = dstride;
    (*ops)[i].sstride = sstride;
  } // for

  fclose(fp);

  return num_ops;
} // read_gets_operations()


void perform_gets_operations(gets_op_t* ops, int num_ops, double* get_time, double* gets_time) {
  void* dst = myseginfo_table[gasnet_mynode()].addr;

  struct timeval get_time_start, get_time_end;
  gettimeofday(&get_time_start, NULL);
  for(int i = 0; i < num_ops; ++i) {
    void* src = myseginfo_table[ops[i].node].addr;
    //print_strided_transfer(ops[i].node, dst, ops[i].dstride, src, ops[i].sstride, ops[i].count, ops[i].stridelevels);
    do_get(ops[i].node, dst, ops[i].dstride, src, ops[i].sstride, ops[i].count, ops[i].stridelevels);
    gasnet_wait_syncnbi_gets();
  } // for
  gettimeofday(&get_time_end, NULL);
  *get_time = ((get_time_end.tv_sec * 1e6 + get_time_end.tv_usec) -
                    (get_time_start.tv_sec * 1e6 + get_time_start.tv_usec)) / 1e3;

  printf("GASNET %d: GET time = %.3f msec.\n", gasnet_mynode(), *get_time);

  gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
  gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);

  struct timeval gets_time_start, gets_time_end;
  gettimeofday(&gets_time_start, NULL);
  for(int i = 0; i < num_ops; ++i) {
    void* src = myseginfo_table[ops[i].node].addr;
    do_gets(ops[i].node, dst, ops[i].dstride, src, ops[i].sstride, ops[i].count, ops[i].stridelevels);
    gasnet_wait_syncnbi_gets();
  } // for
  gettimeofday(&gets_time_end, NULL);
  *gets_time = ((gets_time_end.tv_sec * 1e6 + gets_time_end.tv_usec) -
                     (gets_time_start.tv_sec * 1e6 + gets_time_start.tv_usec)) / 1e3;

  printf("GASNET %d: GETS time = %.3f msec.\n", gasnet_mynode(), *gets_time);
} // perform_gets_operations()


int main(int narg, char** args) {

  // init gasnet
  gasnet_init(&narg, &args);

  if(gasnet_attach(NULL, 0, gasnet_getMaxLocalSegmentSize(), 0) != GASNET_OK) {
    printf("gasnet_attach() failed!\n");
    return(0);
  } // if

  myseginfo_table = malloc(gasnet_nodes() * sizeof(gasnet_seginfo_t));
  gasnet_getSegmentInfo(myseginfo_table, gasnet_nodes());
  gasnet_started = 1;
  printf("Node %d / %d ready!\n", gasnet_mynode(), gasnet_nodes());
  for(int i = 0; i < gasnet_nodes(); ++ i) {
    printf("== %d: %d = %p [%u]\n", gasnet_mynode(), i, myseginfo_table[i].addr, myseginfo_table[i].size);
  } // for

  char fname[32];
  sprintf(fname, "gasnet_gets_operations%d.raw", gasnet_mynode());

  gets_op_t * ops;
  int num_ops = read_gets_operations(fname, &ops);

  printf("%d has %d operations to perform\n", gasnet_mynode(), num_ops);

  gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
  gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);

  int niter = 1;
  double avg_get_time = 0.0, avg_gets_time = 0.0;
  for(int i = 0; i < niter; ++ i) {
    double get_time = 0.0, gets_time = 0.0;
    perform_gets_operations(ops, num_ops, &get_time, &gets_time);
    avg_get_time += get_time;
    avg_gets_time += gets_time;
  } // for

  printf("GASNET %d: Requests = %d, average GET time = %.3f\n", gasnet_mynode(), num_ops, avg_get_time / niter);
  printf("GASNET %d: Requests = %d, average GETS time = %.3f\n", gasnet_mynode(), num_ops, avg_gets_time / niter);

  // exit gasnet
  gasnet_exit(0);
} // main()
