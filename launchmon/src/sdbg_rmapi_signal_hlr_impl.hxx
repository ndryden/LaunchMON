/*
 * $Header: /usr/gapps/asde/cvs-vault/sdb/launchmon/src/sdbg_signal_hlr_impl.hxx,v 1.4.2.2 2008/02/20 17:37:57 dahn Exp $
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
 *        Dec 12 2013 DHA: File created      
 */ 

#ifndef SDBG_RMAPI_SIGNAL_HLR_IMPL_HXX
#define SDBG_RMAPI_SIGNAL_HLR_IMPL_HXX 1

#ifndef HAVE_LAUNCHMON_CONFIG_H
#include "config.h"
#endif

#include <lmon_api/common.h>

extern "C" {
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#else
# error sys/types.h is required
#endif

#if HAVE_SIGNAL_H
# include <signal.h>  
#else
# error signal.h is required 
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#else
# error unistd.h is required 
#endif

#if HAVE_UNISTD_H
# include <libgen.h>
#else
# error libgen.h is required 
#endif
}

#if HAVE_ASSERT_H
# include <cassert>
#else
# error cassert is required
#endif

#include "sdbg_rmapi_signal_hlr.hxx"

rm_event_manager_t *
rmapi_signal_handler_t::evman         = NULL;

rm_api_launchmon_base_t *
rmapi_signal_handler_t::lmon          = NULL;

std::vector<int>
rmapi_signal_handler_t::monitoring_signals;

std::string 
rmapi_signal_handler_t::MODULENAME;

////////////////////////////////////////////////////////////////////
//
// PUBLIC INTERFACES (class rmapi_signal_handler_t)
//
//


//! PUBLIC: rmapi_signal_handler_t
/*!
    Default constructor.
*/
rmapi_signal_handler_t::rmapi_signal_handler_t ( )
{

  //
  // Some signals would make it difficult to 
  // continue with the cleanup routine, but
  // it is better to try it and fail 
  // than not to try at all, which will always fail. 
  //
  monitoring_signals.push_back(SIGFPE);
  monitoring_signals.push_back(SIGINT);
  monitoring_signals.push_back(SIGBUS);
  monitoring_signals.push_back(SIGTERM);
  monitoring_signals.push_back(SIGSEGV);  
  monitoring_signals.push_back(SIGILL);  

  MODULENAME = self_trace_t::sighandler_module_trace.module_name;
}


rmapi_signal_handler_t::rmapi_signal_handler_t ( 
		 const rmapi_signal_handler_t& s )
{
  
}


//! PUBLIC: rmapi_signal_handler_t
/*!
    Default destructor.
*/
rmapi_signal_handler_t::~rmapi_signal_handler_t ( )
{
  
}


void
rmapi_signal_handler_t::set_mask ()
{
    sigset_t mask;
    sigemptyset (&mask);
    sigaddset (&mask, SIGPIPE);
    sigprocmask (SIG_SETMASK, &mask, NULL);
  
    return;
}


void 
rmapi_signal_handler_t::install_hdlr_for_all_sigs ( )
{
  using namespace std;
  
  if ( monitoring_signals.empty() )
    return;

  vector<int>::const_iterator pos;
  for (pos = monitoring_signals.begin(); 
         pos != monitoring_signals.end(); pos++)      
    signal (*pos, sighandler);   
}


void 
rmapi_signal_handler_t::sighandler ( int sig )
{
  using namespace std;

  if ( monitoring_signals.empty() )
    return; 

  self_trace_t::trace ( LEVELCHK(quiet), 
    MODULENAME,
    0,
    "A signal (%d) received. Starting cleanup...", sig);

  const int nMaxEvs = 15;
  int i=0;
  while ( evman->multiplex_events (*lmon) )
    {
      i++;
      if (i >= nMaxEvs)
        break;
    }

    abort();
}

#endif //SDBG_RMAPI_SIGNAL_HLR_IMPL_HXX
