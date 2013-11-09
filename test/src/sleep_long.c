#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>


void
sighandler (int sig)
{
  if ( sig == SIGUSR1)
  {
      exit(0);
  }
  return;
}

int 
main(int argc, char *argv[])
{
    signal(SIGUSR1, sighandler);
    sleep (300);	   
}
