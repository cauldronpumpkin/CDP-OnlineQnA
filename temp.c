#include  <stdio.h>
#include  <signal.h>
#include  <sys/types.h>
#include  <pthread.h>

const int NTHREADS = 4;

void  SIGhandler(int);     

void  SIGhandler(int sig)
{
     printf("\nThread %lx Received a SIGUSR1.\n", pthread_self());
}

void* tfunc( void* arg )
{
  printf( "Thread %d(%lx) executing...\n", *((unsigned int*)arg), pthread_self() );

  while( 1 )
    ;
}

int main()
{
  int i;
  int tid[NTHREADS];
  pthread_t t[NTHREADS];

  signal(SIGUSR1, SIGhandler);

  for( i = 0; i < NTHREADS; i++ )
  {
    tid[i] = i;
    pthread_create( &t[i], NULL, tfunc, tid+i );
  }
  
  for( i = 0; i < NTHREADS; i++ )
    pthread_kill( t[i], SIGUSR1 );

  for( i = 0; i < NTHREADS; ++i) 
    pthread_join(t[i], NULL);

  return 0;
}