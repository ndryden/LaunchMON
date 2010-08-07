/*
 * $Header: $
 *--------------------------------------------------------------------------------
 * Copyright (c) 2008-2010, Lawrence Livermore National Security, LLC. Produced at 
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>. 
 * LLNL-CODE-409469. All rights reserved.
 *
 * This file is part of LaunchMON. For details, see 
 * https://computing.llnl.gov/?set=resources&page=os_projects
 *
 * Please also read LICENSE.txt -- Our Notice and GNU Lesser General Public License.
 *
 * 
 * This program is free software; you can redistribute it and/or modify it under the 
 * terms of the GNU General Public License (as published by the Free Software 
 * Foundation) version 2.1 dated February 1999.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *--------------------------------------------------------------------------------			
 *
 *
 *  ./fe_launch_smoketest.debug /bin/hostname 9 5 pdebug `pwd`/be_kicker.debug
 *
 *  Update Log:
 *        Nov 12 2009 DHA: Change BG mpirun options to cover /P running under IBM LL
 *        Mar 04 2009 DHA: Added generic BlueGene support
 *        Jun 16 2008 DHA: Added LMON_fe_recvUsrDataBe at the end to 
 *                         coordinate the testing result with back-end 
 *                         daemons better. 
 *        Jun 12 2008 DHA: Added GNU build system support.
 *        Mar 18 2008 DHA: Added BlueGene support.
 *        Mar 05 2008 DHA: Added invalid daemon path test.
 *        Feb 09 2008 DHA: Added LLNS Copyright.
 *        Jul 30 2007 DHA: Adjust this case for minor API changes  
 *        Dec 27 2006 DHA: Created file.
 */

#ifndef HAVE_LAUNCHMON_CONFIG_H
#include "config.h"
#endif

#include <lmon_api/common.h>

#if HAVE_UNISTD_H
# include <unistd.h>
#else
# error unistd.h is required
#endif

#if HAVE_LIMITS_H
# include <limits.h>
#else
# error limits.h is required 
#endif

#include <string>

#include <lmon_api/lmon_proctab.h>
#include <lmon_api/lmon_fe.h>

#define MIN_NARGS 6

extern "C" {
  int begin_timer ();
  int time_stamp ( const char* description );
  void fill_mpirun_args(const char *nP, const char *nN,
                   const char *part, const char *app,
                   const char *mylauncher, char ***launcher_argv);
}

static int
statusFunc ( int *status )
{
  int stcp = *status;
  fprintf (stdout, "**** status callback routine is invoked:0x%x ****\n", stcp);
  if (WIFREGISTERED(stcp))
    fprintf(stdout, "* session registered\n");
  else
    fprintf(stdout, "* session not registered\n");

  if (WIFBESPAWNED(stcp))
    fprintf(stdout, "* BE daemons spawned\n");
  else
    fprintf(stdout, "* BE daemons not spawned or exited\n");

  if (WIFMWSPAWNED(stcp))
    fprintf(stdout, "* MW daemons spawned\n");
  else
    fprintf(stdout, "* MW daemons not spawned or exited\n");

  if (WIFDETACHED(stcp))
    fprintf(stdout, "* the job is detached\n");
  else
    {
      if (WIFKILLED(stcp))
        fprintf(stdout, "* the job is killed\n");
      else
        fprintf(stdout, "* the job has not been killed\n");
    }

  return 0;
}

/*
 * OUR PARALLEL JOB LAUNCHER  
 */
const char *mylauncher    = TARGET_JOB_LAUNCHER_PATH;

int
main (int argc, char *argv[])
{
  using namespace std;

  int aSession                = 0;
  unsigned int psize          = 0;
  unsigned int proctabsize    = 0;
  int jobidsize               = 0;
  int i                       = 0;
  char jobid[PATH_MAX]        = {0};
  char **launcher_argv        = NULL;
  char **daemon_opts          = NULL;
  char *application           = NULL;
  char *nP                    = NULL;
  char *nN                    = NULL;
  char *part                  = NULL;
  char *be_daemon             = NULL;
  MPIR_PROCDESC_EXT *proctab  = NULL;
  lmon_rc_e rc;
  lmon_rm_info_t rminfo;


  if ( argc < MIN_NARGS )
    {
      fprintf ( stdout,
        "Usage: fe_launch_smoketest appcode numprocs numnodes partition daemonpath [daemonargs]\n" );
      fprintf ( stdout,
        "[LMON FE] FAILED\n" );
      return EXIT_FAILURE;
    }

  application = argv[1];
  nP = argv[2];
  nN = argv[3];
  part = argv[4];
  be_daemon = argv[5];

  if ( access(argv[1], X_OK) < 0 )
    {
      fprintf ( stdout,
        "%s cannot be executed\n",
        argv[1] );
      fprintf ( stdout,
        "[LMON FE] FAILED\n" );
      return EXIT_FAILURE;
    }

  if ( getenv ("LMON_INVALIDDAEMON_TEST") == NULL )
    {
      if ( access(be_daemon, X_OK) < 0 )
        {
          fprintf(stdout,
            "%s cannot be executed\n",
            be_daemon);
          fprintf(stdout,
            "[LMON FE] FAILED\n");
          return EXIT_FAILURE;
        }
    }

  if ( argc > MIN_NARGS )
    {
      daemon_opts = argv+MIN_NARGS;
    }

  fill_mpirun_args (nP, nN, part, application, mylauncher, &launcher_argv);
  fprintf (stdout, "[LMON_FE] launching the job/daemons via %s\n", mylauncher);

  if ( ( rc = LMON_fe_init ( LMON_VERSION ) ) 
              != LMON_OK )
    {
      fprintf ( stdout, "[LMON FE] FAILED\n" );
      return EXIT_FAILURE;
    }

  if ( ( rc = LMON_fe_createSession (&aSession))
              != LMON_OK)
    {
      fprintf ( stdout, "[LMON FE] FAILED\n");
      return EXIT_FAILURE;
    }

  if ( (rc = LMON_fe_getRMInfo ( aSession, &rminfo)) 
           != LMON_EDUNAV)
    {
      fprintf ( stdout, "[LMON FE] FAILED, rc: %d\n", rc);
      return EXIT_FAILURE;
    }
  else
    {
      fprintf ( stdout,
         "\n[LMON FE] RM type is %d\n", rminfo.rm_type);
    }

  if ( getenv ("LMON_STATUS_CB_TEST"))
    {
       if ( LMON_fe_regStatusCB(aSession, statusFunc) != LMON_OK )
         {
            fprintf ( stdout, "[LMON FE] FAILED\n");
            return EXIT_FAILURE;
         }
    }

#if MEASURE_TRACING_COST
  begin_timer ();
#endif

  if ( getenv ("FEN_RM_DISTRIBUTED") )
    {
      /*
       * If the third argument is not null, the launchMON engine
       * gets invoked via ssh or rsh. 
       */
      char hn[1024];
      gethostname ( hn, 1024);
      if ( ( rc = LMON_fe_launchAndSpawnDaemons( 
                    aSession, 
   	            hn,
	    	    launcher_argv[0],
		    launcher_argv,
		    be_daemon,
		    daemon_opts,
		    NULL,
		    NULL)) 
                  != LMON_OK )
        {
          if ( getenv ("LMON_INVALIDDAEMON_TEST") != NULL )
            {
              if ( rc == LMON_ETOUT )
                {
                  fprintf ( stdout,
                  "[LMON FE] PASS: returned the LMON_ETOUT error code\n");
                  return EXIT_SUCCESS;
                }
            }

          fprintf ( stdout, "[LMON FE] FAILED\n" );
          return EXIT_FAILURE;
      }
    }
  else
    {
      if ( ( rc = LMON_fe_launchAndSpawnDaemons( 
                    aSession, 
   	            NULL,
		    launcher_argv[0],
		    launcher_argv,
		    be_daemon,
		    daemon_opts,
		    NULL,
		    NULL)) 
                  != LMON_OK )
        {
          if ( getenv ("LMON_INVALIDDAEMON_TEST") != NULL )
            {
              if ( rc == LMON_ETOUT )
                {
                  fprintf ( stdout,
                    "[LMON FE] PASS: returned the LMON_ETOUT error code\n");
                  return EXIT_SUCCESS;
                }
            }

          fprintf ( stdout, "[LMON FE] FAILED\n" );
          return EXIT_FAILURE;
        }
    }

#if MEASURE_TRACING_COST
  time_stamp ( "LMON_fe_launchAndSpawnDaemons perf" );
#endif
 
  if ( ( rc = LMON_fe_getProctableSize (
                aSession, 
                &proctabsize ))
              !=  LMON_OK )
    {
       fprintf ( stdout, 
         "[LMON FE] FAILED in LMON_fe_getProctableSize\n");
       return EXIT_FAILURE;
    }

  if (proctabsize != atoi(argv[2])) 
    {
      fprintf ( stdout, 
        "[LMON FE] FAILED, proctabsize is not equal to the given: %ud\n", 
        proctabsize);
      return EXIT_FAILURE;
    }

  proctab = (MPIR_PROCDESC_EXT*) malloc (
                proctabsize*sizeof (MPIR_PROCDESC_EXT) );

  if ( !proctab )
    {
       fprintf ( stdout, "[LMON FE] malloc returned null\n");
       return EXIT_FAILURE;
    }

#if MEASURE_TRACING_COST
  begin_timer ();
#endif 
  if ( ( rc = LMON_fe_getProctable ( 
                aSession, 
                proctab,
                &psize,
                proctabsize )) 
              !=  LMON_OK )
    {
       fprintf ( stdout, "[LMON FE] FAILED\n");
       return EXIT_FAILURE;
    }

#if MEASURE_TRACING_COST
  time_stamp ( "LMON_fe_getProctable perf" );
#endif

  fprintf ( stdout, 
    "[LMON FE] Please check the correctness of the following proctable\n");

  for(i=0; i < psize; i++)
    {
      fprintf ( stdout, 
        "[LMON FE] host_name: %s\n", proctab[i].pd.host_name);
      fprintf ( stdout, 
        "[LMON FE] executable_name: %s\n", proctab[i].pd.executable_name);
      fprintf ( stdout, 
        "[LMON FE] pid: %d(rank %d)\n", proctab[i].pd.pid, proctab[i].mpirank);      
      fprintf ( stdout, "[LMON FE] \n");
    }

  rc = LMON_fe_getResourceHandle ( aSession, jobid,
                	         &jobidsize, PATH_MAX);
  if ((rc != LMON_OK) && (rc != LMON_EDUNAV))
    {
      if ( rc != LMON_EDUNAV ) 
        {
          fprintf ( stdout, "[LMON FE] FAILED\n");
           return EXIT_FAILURE;
        }
    }
  else
    { 
      if (rc != LMON_EDUNAV)
        { 	
          fprintf ( stdout, 
            "\n[LMON FE] Please check the correctness of the following resource handle\n");
          fprintf ( stdout, 
            "[LMON FE] resource handle[jobid or job launcher's pid]: %s\n", jobid);
          fprintf ( stdout, 
            "[LMON FE]");
       }
    }

  rc = LMON_fe_getRMInfo ( aSession, &rminfo);
  if (rc != LMON_OK)
    {
      fprintf ( stdout, "[LMON FE] FAILED\n");
      return EXIT_FAILURE;
    }
  else
   {
      fprintf ( stdout, 
         "\n[LMON FE] RM type is %d\n", rminfo.rm_type);
      fprintf ( stdout, 
         "\n[LMON FE] RM launcher's pid is %d\n", rminfo.rm_launcher_pid);
   }

  rc = LMON_fe_recvUsrDataBe ( aSession, NULL );

  if ( (rc == LMON_EBDARG ) 
       || ( rc == LMON_ENOMEM )
       || ( rc == LMON_EINVAL ) )
    {
      fprintf ( stdout, "[LMON FE] FAILED\n");
      return EXIT_FAILURE;
    } 

  //sleep (3); /* wait until all BE outputs are printed */

  if (getenv ("LMON_ADDITIONAL_FE_STALL"))
    {
      sleep (120);
    }

  fprintf ( stdout, 
    "\n[LMON FE] PASS: run through the end\n");

  return EXIT_SUCCESS;
}

