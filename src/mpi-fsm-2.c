/* 
   B. Estrade <estrabd@lsu.edu>

   General Description
   ^^^^^^^^^^^^^^^^^^^
    - the root process sends some sequence of messages (symbols) to each process
    - upon receipt of message, non-root processes will react to the message
      based on the transition matrix and any preconditions
    - each non-root process much receive at least 1 of each message;
    - when a non-root process reaches the final state, it will shutdown;

   Example 2 Description
   ^^^^^^^^^^^^^^^^^^^^^
   The main purpose of this example is to show how to structure a set of distributed FSM
   using MPI.  In particular, we focus on setting up the basic parts of each FSM and on
   facilitating the sending and receiving of messages.

   In this example
   ^^^^^^^^^^^^^^^
    - root sends RANDOM sequence of messages to all non-root processes
    - upon receipt, each process will change state based on the transition 
     function; there are no preconditions in this example
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mpi.h"
#define _ROOT 0;    // root node

// enumerate message types (i.e., symbols in the FSM's alphabet - \Sigma) 
#define NUM_SYMBOLS 3
enum {
  A =  0,
  B =  1,
  C =  2,
};       

// enum states - Q
enum {
  R0 = 0, // ROOT's one and only state in this example
  Q0 = 0, // treated as the start state
  Q1 = 1, // intermediate state
  Q2 = 2, // intermediate state
  Q3 = 3, // treated as a final state
};

/* 
   Transition functions - \delta
   ^^^^^^^^^^^^^^^^^^^^
   This transition matrix, taken with the symbols and states defined above
   will be put into a final state, ST_FINAL, once it has seen at least one
   of each message (symbol), in the order, A/B/C. The equivalent regular
   expression would be: "(B|C)*A(A|C)*B(A|B)*C" 
*/

int DELTA_PROC[4][NUM_SYMBOLS] = {{Q1,Q0,Q0},     // transition function for all non-root processes
                                  {Q1,Q2,Q1},
                                  {Q2,Q2,Q3},
                                  {Q3,Q3,Q3}};
int next_state_proc (int state, int symbol) {
  return DELTA_PROC[state][symbol];
}

int DELTA_ROOT[1][NUM_SYMBOLS] = {{R0,R0,R0}};    // transition function for root process
int next_state_root (int state, int symbol) {
  return DELTA_ROOT[state][symbol];
}

// Main Program 

int get_random_msg (void) {
  return rand() % NUM_SYMBOLS; // returns 0 thru NUM_SYMBOLS-1
}

int main(int argc, char** argv) {
  int ROOT=_ROOT;
  int i,j,k,source,my_rank,num_nodes,my_state;
  MPI_Status status;

  // initialize mpi stuff
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD,&num_nodes); 
  MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);  

  int msg = -1;
  if (ROOT == my_rank) {
    my_state = R0;
    for (i=0;i<50;i++) { // how to stop? large number of iterations for now, but need some comm back to root
                         // if it is a fixed interval, what bound is there?
      msg = get_random_msg();
      for (j=1;j<num_nodes;j++)
          MPI_Send(&msg,1,MPI_INT,j,0,MPI_COMM_WORLD); // blocking send, not ideal for efficiency
    }
  } else {
    my_state = Q0;
    int done = 0;
    while (!done) {
      MPI_Recv(&msg,1,MPI_INT,ROOT,0,MPI_COMM_WORLD,&status);  
      printf("Node %d received MSG=%d from Node %d\n",my_rank,msg,ROOT);

      // react based on msg
      switch (msg) {
        case A:
          if (Q0 == my_state)
              my_state = next_state_proc(my_state,msg); 
          break;
        case B:
          if (Q1 == my_state)
              my_state = next_state_proc(my_state,msg); 
          break;
        case C:
          if (Q2 == my_state) {
              my_state = next_state_proc(my_state,msg); 
              ++done;
          }
          break;
      }
    }
  }

  MPI_Finalize();
  exit(EXIT_SUCCESS);
}
