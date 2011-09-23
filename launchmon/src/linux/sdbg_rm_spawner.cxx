/*
 * $Header: $
 *--------------------------------------------------------------------------------
 * Copyright (c) 2008 ~ 2010, Lawrence Livermore National Security, LLC. Produced at 
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
 *  Update Log:
 *        Jul 02 2010 DHA: Created file.
 */

#include "sdbg_std.hxx"

#ifndef LINUX_CODE_REQUIRED
#error This source file requires a LINUX OS
#endif

#if HAVE_STDIO_H
# include <cstdio>
#else
# error stdio.h is required
#endif

#if HAVE_STDLIB_H
# include <cstdlib>
#else
# error stdlib.h is required
#endif

#if HAVE_STRING
# include <string>
#else
# error string is required
#endif

#if HAVE_STRING_H
# include <string.h>
#else
# error string.h is required
#endif

#if HAVE_IOSTREAM
# include <iostream>
#else
# error iostream is required
#endif

#if HAVE_SSTREAM
# include <sstream>
#else
# error sstream is required
#endif

#if HAVE_VECTOR
# include <vector>
#else
# error vector is required
#endif

#if HAVE_MAP
# include <map>
#else
# error map is required
#endif

#include "lmon_api/lmon_say_msg.hxx"
#include "sdbg_rm_spawner.hxx"
# include <sys/time.h>
const char *LMON_MSG_PREFIX  = "<LMON SPAWNER>";

//////////////////////////////////////////////////////////////////////////
//
//   Public Interface
//
bool
spawner_rm_t::spawn()
{
    if( !create_launch_args() )
    {
        LMON_say_msg (LMON_MSG_PREFIX, true,
            "problem creating bulk launch arguments");
        return false;
    }

    pid_t temp;
    timeval t1;
    for(int i = 0; i<get_num_cmds(); i++ ){
        if ( !(temp = fork()))
        {
          if ( !execute_rm_bulk_launch(i) )
            {
              //
              // This is within the child process that failed to exec
              // So ** SINK HERE **
              //
              exit(1);
            }
        }
        execpid.push_back(temp);
    }
    return true;
}
bool
spawner_rm_t::combineHosts(std::vector<std::string> &combHosts)
{
  //maybe check to see if the execpid errored out?  too many procs/host?

  std::vector<std::string>::const_iterator iter;
  int s = combHosts.size();

  for (iter = get_hosts_vector().begin(); iter != get_hosts_vector().end(); ++iter)
    {
      combHosts.push_back((*iter));
    }

  return ((combHosts.size() - s) > 0)? true : false;
}

bool
spawner_rm_t::create_launch_args()
{
    //examine which variables are set and add options to args accordingly
    std::string bulk = basename( get_remote_launch_cmd().c_str() );

    // host list has been specified
    if( get_hosts_vector().size() > 0 )
    {
        if( bulk.compare("srun") == 0 )
        {
            if ( !create_slurm_nodelist_arbitrary() )
            //if ( !create_slurm_nodelist_blocks() )
            {
              LMON_say_msg (LMON_MSG_PREFIX, true,
                "problem creating the nodelist");
                return false;
            }
        }
        else
        {
            LMON_say_msg (LMON_MSG_PREFIX, true,
                "this resource manager has not yet been implemented in the MW.");
            return false;
        }
    }

    return true;
}

bool
spawner_rm_t::create_slurm_nodelist_blocks()
{

    //slurm host names are of the form basenameNUM
    //all host names must be of the same basename.
    //need to account for the same host name given multiple times
    std::vector<std::string>::const_iterator cur;
    std::map<int,int> host_map;
    std::map<int,int>::iterator iter;
    int num_start, hostnum, range_begin, range_end;
    std::string base_name;
    unsigned int max_per=0;
    unsigned int min_per=0;
    char* jobid = getenv( "SLURM_JOB_ID" );
    if( jobid == NULL )
    {
        LMON_say_msg (LMON_MSG_PREFIX, false,
            "could not find environment variable SLURM_JOB_ID");
    }

    if( get_hosts_vector().size() > 0 )
    {
        cur = get_hosts_vector().begin();
        num_start = (*cur).find_first_of("0123456789");
        base_name = (*cur).substr(0,num_start);

        //populate host_map
        while( cur != get_hosts_vector().end() )
        {
            if( (*cur).compare(0, num_start, base_name) != 0 )
            {
              LMON_say_msg (LMON_MSG_PREFIX, true,
                "all of the slurm host names must be have the same prefix");
                return false;  //must have the same base name
            }
            else
            {
                hostnum = atoi( (*cur).substr(num_start).c_str() );
                iter = host_map.find(hostnum);
                if( iter == host_map.end() )
                    host_map[hostnum] = 1;
                else
                    iter->second += 1;
            }
            cur++;
        }

        iter=host_map.begin();
        while( iter != host_map.end() ){
            if( min_per==0 || min_per>iter->second )
                min_per=iter->second;
            if( max_per<iter->second )
                max_per=iter->second;
            iter++;
        }

        //depopulate host_map
        int count;
        int n=0;
        for(int i=min_per; i<=max_per; i++ ){

            std::ostringstream list;
            std::ostringstream nodes;
            std::ostringstream tasks;
            list << "--nodelist=" << base_name << "[";
            count=0;

            iter = host_map.begin();
            if( iter != host_map.end() )
            {
                range_begin = -1;
                range_end = -1;

                while( iter != host_map.end() )
                {
                    if( iter->second == i )
                    {
                        count++;
                        if( range_begin == -1 && range_end == -1 )
                            range_begin = range_end = iter->first;
                        else{
                            if( iter->first == range_end+1 )
                                range_end++;
                            else
                            {
                                if( range_begin == range_end )
                                    list << range_begin;
                                else
                                    list << range_begin << "-" << range_end;
                                list << ",";
                                range_begin = range_end = iter->first;
                            }
                        }
                        host_map.erase( iter );
                    }
                    iter++;
                }
                if(count > 0){
                    if( range_begin == range_end )
                        list << range_begin;
                    else
                        list << range_begin << "-" << range_end;
                    list << "]";

                    bool multi_per_node_launch=true;
                    if(multi_per_node_launch){
                        nodes << "--nodes=" << count;
                        tasks << "--ntasks=" << count * i;

                        get_launch_cmd_args(n).push_back( std::string("--jobid=") + std::string(jobid) );
                        get_launch_cmd_args(n).push_back( std::string( nodes.str().c_str() ) );
                        get_launch_cmd_args(n).push_back( std::string(tasks.str().c_str()) );
                        //get_launch_cmd_args(n).push_back( std::string("--distribution=block") );
                        get_launch_cmd_args(n).push_back( std::string("--distribution=cyclic") );
                        get_launch_cmd_args(n).push_back( std::string( list.str().c_str() ) );
                        ///get_launch_cmd_args(n).push_back( std::string("-vvv") );
                        n++;
                    }
                    else{
                        nodes << "--nodes=" << count;
                        tasks << "--ntasks=" << count;
                        for(int temp = 0; temp<i; temp++){
                            get_launch_cmd_args(n).push_back( std::string("--jobid=") + std::string(jobid) );

                            get_launch_cmd_args(n).push_back( std::string( nodes.str().c_str() ) );

                            get_launch_cmd_args(n).push_back( std::string(tasks.str().c_str()) );
                            //get_launch_cmd_args(n).push_back( std::string("--distribution=block") );
                            get_launch_cmd_args(n).push_back( std::string("--distribution=cyclic") );
                            get_launch_cmd_args(n).push_back( std::string( list.str().c_str() ) );
                            n++;
                        }
                    }
                }//if
            }//if
        }//for
    }//if


    return true;
}

bool
spawner_rm_t::create_slurm_nodelist_arbitrary()
{
    //slurm host names are of the form basenameNUM
    //all host names must be of the same basename.
    //need to account for the same host name given multiple times

    std::vector<std::string>::const_iterator cur;
    std::map<int,int> host_map;
    std::map<int,int>::iterator iter;
    int num_start, hostnum, range_begin, range_end;
    std::string base_name;
    std::ostringstream list;
    std::ostringstream nodes;
    std::ostringstream tasks;

    char* jobid = getenv( "SLURM_JOB_ID" );
    if( jobid == NULL )
    {
        LMON_say_msg (LMON_MSG_PREFIX, false,
            "could not find environment variable SLURM_JOB_ID");
    }
    get_launch_cmd_args().push_back( std::string("--jobid=") + std::string(jobid) );
    
    if( get_hosts_vector().size() > 0 )
    {
        cur = get_hosts_vector().begin();
        num_start = (*cur).find_first_of("0123456789");
        base_name = (*cur).substr(0,num_start);
        
        //populate host_map
        while( cur != get_hosts_vector().end() )
        {
            if( (*cur).compare(0, num_start, base_name) != 0 )
            {
              LMON_say_msg (LMON_MSG_PREFIX, true,
                "all of the slurm host names must be have the same prefix");
                return false;  //must have the same base name
            }
            else
            {
                hostnum = atoi( (*cur).substr(num_start).c_str() );
                iter = host_map.find(hostnum);
                if( iter == host_map.end() )
                    host_map[hostnum] = 1;
                else
                    iter->second += 1;
            }
            cur++;
        }

        //depopulate host_map
        nodes << "--nodes=" << host_map.size();
        get_launch_cmd_args().push_back( nodes.str() );
        tasks << "--ntasks=" << get_hosts_vector().size();
        get_launch_cmd_args().push_back( tasks.str() );
        
        list << "--nodelist=" << base_name << "[";

        while( host_map.size() > 0 )
        {
            iter = host_map.begin();
            range_begin = range_end = -1;
            while( iter != host_map.end() )
            {
                if( range_begin == -1 && range_end == -1 )
                    range_begin = range_end = iter->first;
                else
                {
                    if( iter->first == range_end+1 )
                        range_end++;
                    else
                    {
                        if( range_begin == range_end )
                            list << range_begin;
                        else
                            list << range_begin << "-" << range_end;
                        list << ",";
                        range_begin = range_end = iter->first;
                    }
                }

                if( iter->second <= 1 )
                    host_map.erase( iter );
                else
                    iter->second--;

                iter++;
            }//while
            if( range_begin == range_end )
                list << range_begin;
            else
                list << range_begin << "-" << range_end;
            list << ",";
        }//while

        list.seekp( -1, std::ios_base::cur );  //overwrite last ,
        list << "]";

        get_launch_cmd_args().push_back( std::string("--distribution=arbitrary") );
        get_launch_cmd_args().push_back( list.str() );
//        get_launch_cmd_args().push_back( std::string("-vvv") );
    }

    return true;
}


bool
spawner_rm_t::execute_rm_bulk_launch( int n )
{
    // compute required malloc size
    // 1: launcher command
    // n: launcher command args
    // 1: deamonpath
    // m: daemonargs
    // 1: null-termination
    
    int nargvs = get_launch_cmd_args(n).size() + get_daemon_args().size() + 3;
    int i = 0;
    char **av = (char **) malloc (nargvs * sizeof(av));
    av[i++] = strdup (get_remote_launch_cmd().c_str());
    std::vector<std::string>::const_iterator iter;
    for (iter = get_launch_cmd_args(n).begin();
         iter != get_launch_cmd_args(n).end(); ++iter)
    {
      av[i++] = strdup((*iter).c_str());
    }

    av[i++] = strdup(get_daemon_path().c_str());
    for (iter = get_daemon_args().begin(); iter != get_daemon_args().end(); ++iter)
    {
      av[i++] = strdup ((*iter).c_str());
    }
    av[i++] = NULL;

    //
    // We use excvp so that we don't have to copy environ
    // If works, this is SINK
    //
    if ( execvp ( av[0], av) < 0 )
    {
      return false;
    }

    //
    // Unreachable but to satisfy the compiler
    //
    return true;
}
