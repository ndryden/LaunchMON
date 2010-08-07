/*
 * $Header: Exp $
 *--------------------------------------------------------------------------------
 * Copyright (c) 2008, Lawrence Livermore National Security, LLC. Produced at 
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
 *  Update Log:
 *        Jun 12 2008 DHA: Added GNU build system support. 
 *        Feb 09 2008 DHA: Added LLNS Copyright
 *        Aug 15 2006 DHA: File created
 */

#ifndef HAVE_LAUNCHMON_CONFIG_H
#include "config.h"
#endif

#include <lmon_api/common.h>

#if TIME_WITH_SYS_TIME
# include <time.h>
# include <sys/time.h>
#else
# error ctime and sys_time.h is required
#endif

static struct timeval st, et;

int
begin_timer ()
{
  return (gettimeofday ( &st, NULL ));
}


int
time_stamp ( const char* description )
{
  int rc;

  if ( ! (timerisset (&st)) )
    return -1;

  rc = gettimeofday ( &et, NULL ); 

  if ( et.tv_usec >= st.tv_usec ) 
    {
      fprintf ( stdout, 
		"%s: %ld (%ld - %ld) seconds %ld (%ld - %ld) usec \n",
		description, 
		(et.tv_sec - st.tv_sec), et.tv_sec, st.tv_sec,
		(et.tv_usec - st.tv_usec), et.tv_usec, st.tv_usec);
    }
  else
    {
      fprintf ( stdout, 
		"%s: %ld (%ld - %ld) seconds %ld (%ld - %ld) usec \n",
		description,
		( ( et.tv_sec - 1 ) - st.tv_sec), et.tv_sec, st.tv_sec,
		( (et.tv_usec+1000000) - st.tv_usec), et.tv_usec, st.tv_usec );
    } 

  return rc;
}

void
fill_mpirun_args(const char *nP, const char *nN, const char *part, const char *app, 
                 const char *mylauncher, char ***largvptr)
{
 char numprocs_opt[128];
 char numnodes_opt[128];
 char partition_opt[128];
#if RM_BG_MPIRUN
  //
  // This will exercise CO or SMP on BlueGene
  //
  (*largvptr) = (char **) malloc(8*sizeof(char *));
  (*largvptr)[0] = strdup(mylauncher);
  (*largvptr)[1] = strdup("-verbose");
  (*largvptr)[2] = strdup("1");
  (*largvptr)[3] = strdup("-np");
  (*largvptr)[4] = strdup(nP)
  (*largvptr)[5] = strdup("-exe"); 
  (*largvptr)[6] = strdup(app);
  (*largvptr)[7] = NULL;
#elif RM_SLURM_SRUN
  snprintf(numprocs_opt, 128, "-n%s", nP);
  snprintf(numnodes_opt, 128, "-N%s", nN);
  snprintf(partition_opt, 128, "-p%s", part);
  (*largvptr) = (char **) malloc (7*sizeof(char*));
  (*largvptr)[0] = strdup(mylauncher);
  (*largvptr)[1] = strdup(numprocs_opt);
  (*largvptr)[2] = strdup(numnodes_opt);
  (*largvptr)[3] = strdup(partition_opt);
  (*largvptr)[4] = strdup("-l");
  (*largvptr)[5] = strdup(app);
  (*largvptr)[6] = NULL;
#elif RM_ALPS_APRUN
  snprintf(numprocs_opt, 128, "-n%s", nP);
  (*largvptr) = (char **) malloc(4*sizeof(char*));
  (*largvptr)[0] = strdup(mylauncher);
  (*largvptr)[1] = strdup(numprocs_opt);
  (*largvptr)[2] = strdup(app);
  (*largvptr)[3] = NULL;
#elif RM_ORTE_ORTERUN
  (*largvptr) = (char **) malloc(5*sizeof(char*));
  (*largvptr)[0] = strdup(mylauncher);
  /* launcher_argv[1] = strdup("-gmca");
  launcher_argv[2] = strdup("orte_enable_debug_cospawn_while_running");
  launcher_argv[3] = strdup("1");*/
  (*largvptr)[1] = strdup("-np");
  (*largvptr)[2] = strdup(nP);
  (*largvptr)[3] = strdup(app);
  (*largvptr)[4] = NULL;
#else
# error add support for the RM of your interest here
#endif
}
