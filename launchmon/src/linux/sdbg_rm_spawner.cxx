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
        std::ostringstream complete;
        for(int j=0; j<get_launch_cmd_args(i).size(); j++){
            complete << get_launch_cmd_args(i)[j] << " ";
        }
        gettimeofday( &t1, NULL );
        fprintf(stderr, "%ld.%ld %s\t", t1.tv_sec, t1.tv_usec, complete.str().c_str() );
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

//                        std::ostringstream extra;
//                        extra << "--lmon-local=" << i;
//                        get_extra_daemon_args(n).push_back( extra.str().c_str() );

                        //tasks << "--ntasks=" << (count * i);
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

typedef struct _c_range{
    int start;
    int end;
} c_range;

bool
spawner_rm_t::create_slurm_nodelist_arbitrary()
{
    //slurm host names are of the form basenameNUM
    //all host names must be of the same basename.
    //need to account for the same host name given multiple times

//    timeval t1, t2;
//    gettimeofday(&t1, NULL);

    std::vector<std::string>::const_iterator cur;
    std::map<int,int> host_map;
    std::map<int,int>::iterator iter;
    int num_start, hostnum, range_begin, range_end;
    std::string base_name;
    std::ostringstream list;
    std::ostringstream nodes;

    char* jobid = getenv( "SLURM_JOB_ID" );
    if( jobid == NULL )
    {
        LMON_say_msg (LMON_MSG_PREFIX, false,
            "could not find environment variable SLURM_JOB_ID");
    }
    get_launch_cmd_args().push_back( std::string("--jobid=") + std::string(jobid) );
    
    //fprintf(stderr, "%s(%i) hosts.size()=%i\n", __FUNCTION__, __LINE__, get_hosts_vector().size() );
    if( get_hosts_vector().size() > 0 )
    {
        cur = get_hosts_vector().begin();
        num_start = (*cur).find_first_of("0123456789");
        base_name = (*cur).substr(0,num_start);
        
        //fprintf(stderr, "%s(%i) cur=%s num_start=%i base_name=%s\n", __FUNCTION__, __LINE__, (*cur).c_str(), num_start, base_name.c_str() );
        
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

        //reorder the hosts_vector for a faster cobo initialization.
        std::vector<std::string> new_order;
        int i;
        iter = host_map.begin();
        while( iter != host_map.end() )
        {
            std::ostringstream name;
            name << base_name << iter->first;
            for(i=0; i < iter->second; i++)
                new_order.push_back( name.str() );
            iter++;
        }

        std::vector<int> next;
        std::vector<int> c_order;
        int c_me, low, high, mid;
        next.push_back(0);
        while( next.size() > 0 ){  //code base on cobo implementation
            c_me = next.front();
            c_order.push_back(c_me);
            next.erase( next.begin() );
            low  = 0;
            high = get_hosts_vector().size() - 1;
            while (high - low > 0) {
                mid = (high - low) / 2 + (high - low) % 2 + low;
                if (low == c_me )
                    next.push_back(mid);
                if (mid <= c_me) { low  = mid; }
                else             { high = mid-1; }
            }//while
        }//while

        std::vector<int>::iterator o_iter = c_order.begin();
        std::vector<std::string>::iterator h_iter = new_order.begin();
        while( o_iter != c_order.end() ){
            get_hosts_vector()[(*o_iter)] = (*h_iter);
            h_iter++;
            o_iter++;
        }


        //get_hosts_vector().erase( get_hosts_vector().begin(), get_hosts_vector().end()  );
        //get_hosts_vector().insert( get_hosts_vector().begin(), new_order.begin(), new_order.end() );


        //depopulate host_map
        nodes << "--nodes=" << host_map.size();
        get_launch_cmd_args().push_back( nodes.str() );
        
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
        //fprintf(stderr, "%s(%i) list=%s\n", __FUNCTION__, __LINE__, list.str().c_str() );

        get_launch_cmd_args().push_back( std::string("--distribution=arbitrary") );
//        get_launch_cmd_args().push_back( std::string("--share") );
        get_launch_cmd_args().push_back( list.str() );
//        get_launch_cmd_args().push_back( std::string("-vvv") );
    }

//    gettimeofday(&t2, NULL);
//    double d1 = ((double)t2.tv_sec + ((double)t2.tv_usec * 0.000001)) - ((double)t1.tv_sec + ((double)t1.tv_usec * 0.000001));
//    fprintf( stdout, "or:%f\t", d1 );

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
//    for (iter = get_extra_daemon_args(n).begin(); iter != get_extra_daemon_args(n).end(); ++iter)
//    {
//      av[i++] = strdup ((*iter).c_str());
//    }
//    av[i++] = "2>&1";
//    av[i++] = "|";
//    av[i++] = "/g/g20/goehner1/mrnet/tests/timelog";
    av[i++] = NULL;

//    std::ostringstream complete;
//    for(int j=0; j+1<i; j++){
//        complete << av[j] << " ";
//    }
//    complete << "2>&1 | /g/g20/goehner1/mrnet/tests/timelog > /g/g20/goehner1/mrnet/tests/srun_cobo" << n << getpid();
      //fprintf(stderr, "%s(%i) arg[%i]=%s\n", __FUNCTION__, __LINE__, j, av[j] );

//fprintf(stderr, "%s \n", complete.str().c_str());




    //
    // We use excvp so that we don't have to copy environ
    // If works, this is SINK
    //
    if ( execvp ( av[0], av) < 0 )
    {
      return false;
    }
//    if( system( complete.str().c_str() ) != 0 ){
//        return false;
//    }

    //
    // Unreachable but to satisfy the compiler
    //
    return true;
}
