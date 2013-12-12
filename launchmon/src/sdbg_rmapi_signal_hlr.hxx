/*
 * $Header: /usr/gapps/asde/cvs-vault/sdb/launchmon/src/sdbg_signal_hlr.hxx,v 1.3.2.1 2008/02/20 17:37:57 dahn Exp $
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
 *  Update Log:
 *        Dec 12  2006 DHA: File created
 */ 

#ifndef SDBG_RMAPI_SIGNAL_HLR_HXX
#define SDBG_RMAPI_SIGNAL_HLR_HXX 1

#if HAVE_VECTOR
# include <vector>
#else
# error vector is required
#endif

#include "sdbg_std.hxx"
#include "sdbg_self_trace.hxx"

class rmapi_signal_handler_t 
{
public:
  
  rmapi_signal_handler_t ( );
  rmapi_signal_handler_t ( const rmapi_signal_handler_t& s );
  ~rmapi_signal_handler_t ( );

  static void sighandler ( int sig );
  static void install_hdlr_for_all_sigs ( );

  static void set_evman ( rm_event_manager_t* em ) { evman = em; }
  static void set_lmon ( rm_api_launchmon_base_t* lm ) { lmon = lm; }
  static void set_mask ( );

private:
  static bool LEVELCHK(self_trace_verbosity level) 
       { return (self_trace_t::sighandler_module_trace.verbosity_level >= level); }
 
  static std::vector<int> monitoring_signals;  
  static rm_event_manager_t *evman;
  static rm_api_launchmon_base_t *lmon;

  //
  // For self tracing
  //
  static std::string MODULENAME;

};

#endif // SDBG_RMAPI_SIGNAL_HLR_HXX 
