/*
 * $Header: $
 *--------------------------------------------------------------------------------
 * Copyright (c) 2013, Lawrence Livermore National Security, LLC. Produced at 
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
 *        Sep 04 2013 DHA: Code cleanup. Hide progrp vectors to private.
 *        Aug 19 2013 DHA: Created file.
 */

#ifndef SDBG_BASE_RMAPI_LAUNCHMON_IMPL_HXX
#define SDBG_BASE_RMAPI_LAUNCHMON_IMPL_HXX 1

#include "sdbg_std.hxx"

#ifndef LINUX_CODE_REQUIRED
#error This source file requires a LINUX OS
#endif

#include <lmon_api/lmon_api_std.h>

#if HAVE_POLL_H
# include <poll.h>
#else
# error poll.h is required
#endif 

#if HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#else
# error sys/socket.h is required
#endif

#if HAVE_ARPA_INET_H
# include <arpa/inet.h>
#else
# error arpa/inet.h is required
#endif

#include "sdbg_base_rmapi_launchmon.hxx"
#include "sdbg_base_rm_procgrp_status.hxx"
#include "sdbg_base_rm_procgrp_status_impl.hxx"


////////////////////////////////////////////////////////////////////
//
// PRIVATE INTERFACES (class rm_api_launchmon_base_t)
//
////////////////////////////////////////////////////////////////////

//
// These definitions prevent copy constructors and assignment
// operator from being called.
//


//!
/*! rm_api_launchmon_base_t constructors

*/
rm_api_launchmon_base_t::rm_api_launchmon_base_t (
		 const rm_api_launchmon_base_t &o)
{
    pcount          = o.pcount;
    FE_sockfd       = o.FE_sockfd;
    API_mode        = o.API_mode;
    myopt           = o.myopt;
    last_seen       = o.last_seen;
    warm_period     = o.warm_period;
    MODULENAME      = o.MODULENAME;
}


//!
/*!  rm_api_launchmon_base_t operator=

*/
rm_api_launchmon_base_t &
rm_api_launchmon_base_t::operator=(
                 const rm_api_launchmon_base_t &l)
{
    pcount          = l.pcount;
    FE_sockfd       = l.FE_sockfd;
    API_mode        = l.API_mode;
    myopt           = l.myopt;
    last_seen       = l.last_seen;
    warm_period     = l.warm_period;
    MODULENAME      = l.MODULENAME;

    return *this;
}


//!
/*!  rm_api_launchmon_base_t handle_fen_cntl_msg

*/
rmapi_lmon_rc_e
rm_api_launchmon_base_t::handle_fen_cntl_msg (
                 lmonp_fe_to_fe_msg_e t) 
{
    rmapi_lmon_rc_e rc = RMAPI_LMON_OK;

    switch (t) {
    case lmonp_detach: 

        self_trace_t::trace ( LEVELCHK(level1),
            MODULENAME,
            0,
            "front-end requested detach...");

        if ( handle_detach_command () == RMAPI_LMON_OK ) 
            rc = RMAPI_LMON_STOP;
        else
            rc = RMAPI_LMON_FAILED;
        
        break;

    case lmonp_kill: 
        self_trace_t::trace ( LEVELCHK(level1),
            MODULENAME,
            0,
            "front-end requested kill...");

        if ( handle_kill_command () != RMAPI_LMON_OK ) 
            rc = RMAPI_LMON_FAILED;

        break;

    case lmonp_shutdownbe: 
        self_trace_t::trace ( LEVELCHK(level1),
            MODULENAME,
            0,
            "front-end requested deamon shutdown...");
        if ( handle_shutdownbe_command () != RMAPI_LMON_OK ) 
            rc = RMAPI_LMON_FAILED;

        break;

    case lmonp_cont_launch_bp: 
        self_trace_t::trace ( LEVELCHK(level1),
            MODULENAME,
            0,
            "front-end requests unlocking"
            " the launcher from launch-bp...");
        // FLUX TODO: Need procgrp control 
        // request_cont_launch_bp ( p );
        break;

    default: 
        self_trace_t::trace ( LEVELCHK(level1),
            MODULENAME, 1,
            "ill-formed msg");

        rc = RMAPI_LMON_FAILED;
    }

    return rc;
}




////////////////////////////////////////////////////////////////////
//
// PUBLIC INTERFACES (class rm_api_launchmon_base_t)
//
////////////////////////////////////////////////////////////////////

//!
/*! rm_api_launchmon_base_t constructors

*/
rm_api_launchmon_base_t::rm_api_launchmon_base_t ()
    : pcount (-1),
      FE_sockfd (-1),
      API_mode (false),
      myopt (NULL),
      MODULENAME (self_trace_t::rm_launchmon_module_trace.module_name)
{
    char *warm_interval;
    last_seen = gettimeofdayD ();
    warm_period = DefaultWarmPeriods;
    warm_interval = getenv ("LMON_ENGINE_WARM_INTERVAL"); 
    if ( warm_interval ) 
	warm_period = (double) atoi (warm_interval);
}


//!
/*!  rm_api_launchmon_base_t destructor
 
*/
rm_api_launchmon_base_t::~rm_api_launchmon_base_t ()
{
  if (!proctable_copy.empty()) {
      std::map<std::string, std::vector<MPIR_PROCDESC_EXT *> >::iterator 
	  iter;

      for(iter = proctable_copy.begin();
	  iter != proctable_copy.end(); ++iter) {

          if (!(iter->second.empty())) {
              std::vector<MPIR_PROCDESC_EXT *>::iterator viter;

              for (viter = iter->second.begin(); 
		   viter != iter->second.end(); ++viter) {
                  if ((*viter)->pd.host_name) {
                      free((*viter)->pd.host_name);
		  }

                  if ((*viter)->pd.executable_name) {
                      free((*viter)->pd.executable_name);
		  }
                  //free(*viter);
	      }
	  }
      }
      proctable_copy.clear();
 }
}


std::map<std::string, std::vector<MPIR_PROCDESC_EXT *> > & 
rm_api_launchmon_base_t::get_proctable_copy() 
{ 
    return proctable_copy; 
}


opts_args_t * 
rm_api_launchmon_base_t::get_myopt ()
{
    return myopt;
}


void 
rm_api_launchmon_base_t::set_myopt (opts_args_t *o)
{
    myopt = o;
}


rmapi_lmon_rc_e 
rm_api_launchmon_base_t::push_back_procgrp (
		 job_procgrp_status_pair_t *pgp,
		 rm_procgrp_type_e t) 
{
    rmapi_lmon_rc_e rc = RMAPI_LMON_OK;

    switch (t) {
    case pg_type_application:
	target_app_vector.push_back (pgp);
	break;

    case pg_type_be_daemons:
	coloc_grp_vector.push_back (pgp);
	break;	

    case pg_type_mw_daemons:
	mw_grp_vector.push_back (pgp);
	break;	

    default:	
	rc = RMAPI_LMON_FAILED;
	break;
    }

    return rc;
}


rmapi_lmon_rc_e 
rm_api_launchmon_base_t::remove_procgrp (
		 job_procgrp_status_pair_t *pgp,
		 rm_procgrp_type_e t) 
{
    rmapi_lmon_rc_e rc = RMAPI_LMON_OK;
    std::vector<job_procgrp_status_pair_t *>::iterator i;

    switch (t) {
    case pg_type_application:
	for (i=target_app_vector.begin(); 
	     i != target_app_vector.end(); ++i) {

	    if ((*i) == pgp) {
		target_app_vector.erase (i);
		break;
	    }
	}	     
	break;

    case pg_type_be_daemons:
	for (i=coloc_grp_vector.begin(); 
	     i != coloc_grp_vector.end(); ++i) {

	    if ((*i) == pgp) {
		coloc_grp_vector.erase (i);	
		break;
	    }
	}
	break;	

    case pg_type_mw_daemons:
	for (i=mw_grp_vector.begin(); 
	     i != mw_grp_vector.end(); ++i) {

	    if ((*i) == pgp) {
		mw_grp_vector.erase (i);
		break;
	    }
	}
	break;	

    default:	
	rc = RMAPI_LMON_FAILED;
	break;
    }

    return rc;
}


int
rm_api_launchmon_base_t::get_procgrp_size (
		 rm_procgrp_type_e t) 
{
    int size = -1;

    switch (t) {
    case pg_type_application:
	size = target_app_vector.size ();
	break;

    case pg_type_be_daemons:
	size = coloc_grp_vector.size ();
	break;	

    case pg_type_mw_daemons:
	size = mw_grp_vector.size ();
	break;	

    default:	
	break;
    }

    return size;
}


job_procgrp_status_pair_t *
rm_api_launchmon_base_t::get_procgrp_at (
                 int index, rm_procgrp_type_e t)
{
    job_procgrp_status_pair_t *rc = NULL;

    switch (t) {
    case pg_type_application:
	rc = (index < target_app_vector.size ())?
	    target_app_vector[index] : NULL;
	break;

    case pg_type_be_daemons:
	rc = (index < coloc_grp_vector.size ())?
	    coloc_grp_vector[index] : NULL;
	break;	

    case pg_type_mw_daemons:
	rc = (index < mw_grp_vector.size ())?
	    target_app_vector[index] : NULL;	
	break;	

    default:	
	break;
    }

    return rc;
}


rmapi_lmon_rc_e
rm_api_launchmon_base_t::say_fetofe_msg (
                 lmonp_fe_to_fe_msg_e msg_type )
{
    rmapi_lmon_rc_e rc = RMAPI_LMON_OK;
    lmonp_t msgheader;
  
    if ( !get_API_mode() ) {
	rc = RMAPI_LMON_FAILED;
	goto ret_loc;
    }

    set_msg_header ( &msgheader,
		     lmonp_fetofe,
		     (int) msg_type,
		     0,0,0,0,0,0,0 );
  
    if ( (write_lmonp_long_msg ( get_FE_sockfd(), 
		 &msgheader, 
		 sizeof ( msgheader ) )) < 0 ) {
	rc = RMAPI_LMON_FAILED;
	goto ret_loc;
    }

ret_loc:
    return rc;
}


rmapi_lmon_rc_e
rm_api_launchmon_base_t::update_socket_events ()
{
    int pollret = 0;
    int timeout = 0;
    int numbytes= 0;
    double timestamp = 0.0f;
    struct pollfd fds[1] = {0};
    lmonp_t msg;

    if ( !get_API_mode() ) {
        self_trace_t::trace ( LEVELCHK(level3),
             MODULENAME, 0,
             "standalone launchmon must not have"
	     " to poll the FE socket");

        return RMAPI_LMON_OK;
    }

    fds[0].fd = get_FE_sockfd();
    fds[0].events = POLLIN;

    timestamp = gettimeofdayD ();
    if ( timestamp > (last_seen + warm_period) ) {
	timeout = 10; /* 10 milisecond blocking if no event */
    }

    do {
        pollret = poll ( fds, 1, timeout );
    } while ( (pollret < 0) && (errno == EINTR) );

    if (( pollret <= 0) || !(fds[0].revents & POLLIN)) {
	return RMAPI_LMON_OK;
    }
    
#if VERBOSE
    self_trace_t::trace ( LEVELCHK(level1),
        MODULENAME, 0,
        "poll returned an event ");
#endif

    init_msg_header (&msg);
    numbytes 
        = read_lmonp_msgheader (get_FE_sockfd(), &msg);

    //
    // First handle socket disconnection and oddly formed message
    //
    if ( numbytes == -1) {
	//
	// A.C.1. If the tool FE fails, the engine 
        // first detects the socket disconnection, 
        // at which point it tries to kill  the  RM_daemon  
	// process and detaches  from  the  RM_job  process.  
        // However, if for some reason the engine also 
        // gets into trouble, the engine would perform  A.1  instead;
        // obviously  in  this  case,  the failing launchmon 
        // engine will keep the RM_daemon process running, and 
	// won't be able to do A.1.3.
	//
	// This attempts to stop all of the threads
	// and mark "detach." If a kill/detach request has been 
        // already registered, don't bother
	self_trace_t::trace ( LEVELCHK(level1),
            MODULENAME,
	    0,
            "The channel with front-end disconnected."
            " Starting cleanup...");

	goto error;
    } // if ( numbytes == -1)
    else if ( (numbytes != sizeof (msg))
	      || ((numbytes == sizeof (msg))
		  && (msg.msgclass != lmonp_fetofe)) ) {
	self_trace_t::trace ( LEVELCHK(level1),
            MODULENAME, 1,
            "read_lmonp_msgheader pulled out an ill-formed message");

	goto error;
    }

    //
    // OK, FE socket is connected and we received a legit message 
    //
    return handle_fen_cntl_msg (msg.type.fetofe_type);

error:
    return RMAPI_LMON_FAILED;
}


rmapi_lmon_rc_e 
rm_api_launchmon_base_t::invoke_handler (
                 job_procgrp_status_pair_t *pgp)
{
    rmapi_lmon_rc_e rc = RMAPI_LMON_OK;

    switch (pgp->get_reqaction()) {
    case pg_action_update:
	//
	// FLUX TODO send an appropriate LMONP event 
	// to FE if needed.
	// rc = handle_status_update_action ();
	pgp->set_updatereq (false);
	break;

    case pg_action_spawn_daemons:
	if (pgp->get_type () == pg_type_application) {
	    rc = handle_spawn_daemons_action (pgp);
	}
	pgp->set_updatereq (false);
	break;

    case pg_action_kill:
        rc = handle_kill_req_action (pgp);
        pgp->set_updatereq (false);
        break;

    case pg_action_cleanup:
	if (pgp->get_type () == pg_type_application) {
	    rc = handle_app_cleanup_action (pgp);
	}
	else if ((pgp->get_type () == pg_type_be_daemons)
		 || (pgp->get_type () == pg_type_mw_daemons)) {
	    rc = handle_daemon_cleanup_action (pgp);
	}
	pgp->set_updatereq (false);
	break;

    case pg_action_noop:
	pgp->set_updatereq (false);
	break;

    case pg_action_invalid:
    default:
	pgp->set_updatereq (false);
	rc = RMAPI_LMON_FAILED;
	break;
    }

    return rc;
}


rmapi_lmon_rc_e 
rm_api_launchmon_base_t::ship_proctab_msg (
                 lmonp_fe_to_fe_msg_e t)
{
    using namespace std;
    map<string, unsigned int> execHostName;
    vector<char *> orderedEHName;
    map<string, vector<MPIR_PROCDESC_EXT *> >::const_iterator 
	pos;
    vector<MPIR_PROCDESC_EXT *>::const_iterator vpos;
    vector<char *>::const_iterator EHpos;
    unsigned int offset = 0;
    unsigned int num_unique_exec = 0;
    unsigned int num_unique_hn = 0;
    int msgsize;

    //
    // If the engine is not driven via the FE API, 
    // this method returns RMAPI_LMON_FAILED
    //
    if ( !get_API_mode() ) {    
      self_trace_t::trace ( LEVELCHK(level3), 
	     MODULENAME, 0, 
	     "standalone mode does not ship"
	     " proctab via LMONP messages");  
      return RMAPI_LMON_FAILED;
    }
  
    //
    // Establishing a map and an ordered vector 
    // to pack a string table
    //
    for ( pos = proctable_copy.begin(); 
	  pos != proctable_copy.end(); ++pos ) {

	for( vpos = pos->second.begin(); 
	     vpos != pos->second.end(); ++vpos ) {

	    map<string, unsigned int>::const_iterator 
		finditer;
	    
	    finditer = execHostName.find ( 
	        string((*vpos)->pd.executable_name) );
	    if ( finditer == execHostName.end () ) {
		execHostName[
		    string((*vpos)->pd.executable_name)] 
                        = offset;
		orderedEHName.push_back( 
                    (*vpos)->pd.executable_name );
		num_unique_exec++;
		offset += ( strlen ( 
                    (*vpos)->pd.executable_name ) + 1 );
	    }

	    finditer = execHostName.find (
                string((*vpos)->pd.host_name));
	    if ( finditer == execHostName.end()) {
		execHostName[
                    string((*vpos)->pd.host_name)] = offset;
		orderedEHName.push_back (
                    (*vpos)->pd.host_name);
		num_unique_hn++;
		offset += (strlen ( 
                    (*vpos)->pd.host_name ) + 1 );
	    }
	}
    }

  //
  // This message can be rather long as the size is
  // an lmonp header size + (N_Fields_MPIR_PROCDESC_EXT x sizeof(int) 
  // per-task entry for each task + the string table size.
  // The fixed per-task entry consists of exec index, 
  // hostname index, pid, and rank, and cnodeid, each of which
  // is sizeof(int).
  //
  msgsize = sizeof(lmonp_t) 
      + N_Fields_MPIR_PROCDESC_EXT*sizeof(int)*pcount 
      + offset;
  lmonp_t *sendbuf = (lmonp_t *) malloc ( msgsize );
  memset ( sendbuf, 0, msgsize );
  if ( pcount < LMON_NTASKS_THRE) {
      set_msg_header ((lmonp_t*) sendbuf, 
		  lmonp_fetofe, 
		  (int)t, 
		  pcount, 
		  0,
		  num_unique_exec,
		  num_unique_hn,
		  0,
		  (N_Fields_MPIR_PROCDESC_EXT*sizeof(int)*pcount)+offset,
		  0);
  }
  else {
      set_msg_header ((lmonp_t*) sendbuf, 
		  lmonp_fetofe, 
		  (int)t, 
		  LMON_NTASKS_THRE, 
		  0,
		  num_unique_exec,
		  num_unique_hn,
		  pcount,
		  (N_Fields_MPIR_PROCDESC_EXT*sizeof(int)*pcount)+offset,
		  0);
  }

  char *payload_cp_ptr 
      = get_lmonpayload_begin (sendbuf);


  //
  // Serializing the process table into a send buffer.
  // Number of memcpy must be equal to N_Fields_MPIR_PROCDESC_EXT
  //
  for ( pos = proctable_copy.begin(); 
            pos != proctable_copy.end(); ++pos ) {
      for(vpos = pos->second.begin(); 
               vpos != pos->second.end(); ++vpos ) {
	  memcpy ((void *) payload_cp_ptr,
		  (void *) &(execHostName[
                      string((*vpos)->pd.host_name)]),
		  sizeof ( unsigned int ) );
	  payload_cp_ptr += sizeof ( unsigned int );
	  
	  memcpy ((void *) payload_cp_ptr,
		  (void *) 
		  &(execHostName[
                      string((*vpos)->pd.executable_name)]),
		  sizeof ( unsigned int ) );
	  payload_cp_ptr += sizeof ( unsigned int );
	  
	  memcpy ((void *) payload_cp_ptr,
		  (void *) &((*vpos)->pd.pid),
		  sizeof (int) );
	  payload_cp_ptr += sizeof ( int );
	  
	  memcpy ((void *) payload_cp_ptr,
		  (void *) &((*vpos)->mpirank),
		  sizeof ( int ) );
	  payload_cp_ptr += sizeof ( int ); 

	  memcpy ((void *) payload_cp_ptr,
		  (void *) &((*vpos)->cnodeid),
		  sizeof ( int ) );
	  payload_cp_ptr += sizeof ( int ); 
      }
  }
 
  //
  // serializing the string table into the send buffer. 
  //
  for ( EHpos = orderedEHName.begin(); 
	EHpos != orderedEHName.end(); ++EHpos ) {
      int leng = strlen ((*EHpos)) + 1;
      memcpy ( (void *) payload_cp_ptr, 
               (void *) ( *EHpos ),
	       leng );
      payload_cp_ptr += leng;
  }
  
  write_lmonp_long_msg ( get_FE_sockfd(),
			 (lmonp_t*) sendbuf, 
			 msgsize );

  {    
    self_trace_t::trace ( LEVELCHK(level2), 
        MODULENAME, 0, 
        "a proctable message shipped out"); 
  }
  
  free(sendbuf);

  return RMAPI_LMON_OK;
}


rmapi_lmon_rc_e
rm_api_launchmon_base_t::ship_resourcehandle_msg (
                 lmonp_fe_to_fe_msg_e t, int rid)
{
    using namespace std;

    lmonp_t msg;
    int msgsize = -1;
    char *sendbuf = NULL;
    char *payload_cp_ptr = NULL;  

    //
    // If the engine is not driven via the FE API, this method
    // returns RMAPI_LMON_FAILED
    //  
    if ( !get_API_mode() ){    
      self_trace_t::trace ( LEVELCHK(level3), 
          MODULENAME, 0, 
	  "standalone mode does not ship"
          " resource handle via LMONP");
      return RMAPI_LMON_FAILED;
    }

  init_msg_header (&msg);
  msg.msgclass = lmonp_fetofe;
  msg.type.fetofe_type = t;
  msg.lmon_payload_length = sizeof(rid);
  msgsize = sizeof(msg) 
      + msg.lmon_payload_length 
      + msg.usr_payload_length; 
  sendbuf = (char*) malloc (msgsize); 
  
  payload_cp_ptr = sendbuf;
  memcpy (payload_cp_ptr, &msg, sizeof(msg));  
  payload_cp_ptr += sizeof(msg);
  memcpy(payload_cp_ptr, &rid, sizeof(rid));
  
  write_lmonp_long_msg ( get_FE_sockfd(),
			 (lmonp_t*)sendbuf,
			 msgsize );  

  {    
      self_trace_t::trace ( LEVELCHK(level2), 
	  MODULENAME, 0, 
          "a reshandle message shipped out"); 
  }

  free(sendbuf);  

  return RMAPI_LMON_OK;
}
 

rmapi_lmon_rc_e
rm_api_launchmon_base_t::ship_rminfo_msg (
                 lmonp_fe_to_fe_msg_e t, 
		 int rmpid,
		 rm_catalogue_e rmtype)
{
    lmonp_t msg;
    int msgsize = -1;
    char *sendbuf = NULL;
    char *payload_cp_ptr = NULL;  
    uint32_t rminfo_pair[2];

    //
    // If the engine is not driven via the FE API, 
    // this method
    // returns RMAPI_LMON_FAILED
    //  
    if ( !get_API_mode() ) {    
	self_trace_t::trace ( LEVELCHK(level3), 
	    MODULENAME, 0, 
	    "standalone mode does not ship"
	    " resource handle via LMONP");
      return RMAPI_LMON_FAILED;
    }

    rminfo_pair[0] = (uint32_t) rmpid;
    rminfo_pair[1] = (uint32_t) rmtype;

    init_msg_header (&msg);
    msg.msgclass = lmonp_fetofe;
    msg.type.fetofe_type = t;
    msg.lmon_payload_length = sizeof(rminfo_pair);
    msgsize = sizeof(msg) 
	+ msg.lmon_payload_length 
	+ msg.usr_payload_length; 
    sendbuf = (char*) malloc (msgsize); 
  
    payload_cp_ptr = sendbuf;
    memcpy (payload_cp_ptr, 
	    &msg, 
	    sizeof(msg));  
    payload_cp_ptr += sizeof(msg);
    memcpy (payload_cp_ptr, 
	    rminfo_pair, 
	    sizeof(rminfo_pair));
  
    write_lmonp_long_msg ( get_FE_sockfd(),
			   (lmonp_t *)sendbuf,
			   msgsize );  

    {    
	self_trace_t::trace ( LEVELCHK(level2), 
	    MODULENAME, 0, 
	    "a reshandle message shipped out"); 
    }

    free(sendbuf);  

    return RMAPI_LMON_OK;
}


rmapi_lmon_rc_e
rm_api_launchmon_base_t::invoke_action_handlers ()
{    
    int i = 0;
    int size = 0;
    rmapi_lmon_rc_e rc = RMAPI_LMON_FAILED;
    int t = (int) pg_type_application;	
    job_procgrp_status_pair_t *procgrp = NULL;

    //
    // Loop that iterates over pg_type_application,
    // pg_type_be_daemons, and pg_type_mw_daemons.
    //
    for (t = (int) pg_type_application; 
	 t < (int) pg_type_reserved; ++t) {	
	size = get_procgrp_size ((rm_procgrp_type_e)t);

	for (i=0; i < size; ++i) {

	    procgrp = get_procgrp_at (i, (rm_procgrp_type_e)t);

	    if (procgrp->get_updatereq ()) {

		rc = invoke_handler (procgrp);
		if (rc != RMAPI_LMON_OK) {
		    goto ret_loc;
		}
	    }
	}
    }

ret_loc:
    return rc;
}


//!
/*!  rm_api_launchmon_base_t handle_detach_command

*/
rmapi_lmon_rc_e
rm_api_launchmon_base_t::handle_detach_command ()
{
    //
    // Under the new engine scheme, detach is NOOP 
    // This is just to support legacy code
    //
    return say_fetofe_msg (lmonp_detach_done);
}


//!
/*!  rm_api_launchmon_base_t handle_shutdownbe_command

*/
rmapi_lmon_rc_e
rm_api_launchmon_base_t::handle_shutdownbe_command ()
{
    // 
    // Iterate through all of the be grocgrps and change
    // the status to pg_status_kill_requested
    //
    job_procgrp_status_pair_t *procgrp = NULL;
    int size = get_procgrp_size (pg_type_be_daemons);
    if (size == 0) {
        // If no mw daemons, nothing to shutdown
        say_fetofe_msg (lmonp_shutdownbe_done);
    }  
    else {
        int i=0;

        for (i=0; i < size; ++i ) {
           procgrp = get_procgrp_at (i, pg_type_be_daemons); 
           procgrp->set_status (pg_status_kill_requested);
        } 
    }

    return RMAPI_LMON_OK;
}


//!
/*!  rm_api_launchmon_base_t handle_shutdownmw_command

*/
rmapi_lmon_rc_e
rm_api_launchmon_base_t::handle_shutdownmw_command ()
{
    // 
    // Iterate through all of the be grocgrps and change
    // the status to pg_status_kill_requested
    //
    job_procgrp_status_pair_t *procgrp = NULL;
    int size = get_procgrp_size (pg_type_mw_daemons);

    if (size == 0) {
        // If no mw daemons, nothing to shutdown
        say_fetofe_msg (lmonp_shutdownmw_done);
    }  
    else {
        int i=0;
        for (i=0; i < size; ++i ) {
            procgrp = get_procgrp_at (i, pg_type_mw_daemons); 
            procgrp->set_status (pg_status_kill_requested);
        } 
    } 

    return RMAPI_LMON_OK;
}


//!
/*!  rm_api_launchmon_base_t handle_kill_command

*/
rmapi_lmon_rc_e
rm_api_launchmon_base_t::handle_kill_command ()
{
    // 
    // Iterate through all of the be and app grocgrps and change
    // the status to pg_status_kill_requested
    //
    job_procgrp_status_pair_t *procgrp = NULL;
    int size1 = get_procgrp_size (pg_type_application);
    int size2 = get_procgrp_size (pg_type_be_daemons);
    int size3 = get_procgrp_size (pg_type_mw_daemons);

    if ( (size1 == 0) && (size2 == 0) && (size3 == 0) ) {
        // If nothing to kill 
        say_fetofe_msg (lmonp_kill_done);
    }
    else {
        int i=0;
        for (i=0; i < size1; ++i ) {
            procgrp = get_procgrp_at (i, pg_type_application); 
            procgrp->set_status (pg_status_kill_requested);
        } 

        i=0;
        for (i=0; i < size2; ++i ) {
            procgrp = get_procgrp_at (i, pg_type_be_daemons);
            procgrp->set_status (pg_status_kill_requested);
        }

        i=0;
        for (i=0; i < size3; ++i ) {
            procgrp = get_procgrp_at (i, pg_type_mw_daemons);
            procgrp->set_status (pg_status_kill_requested);
        }
    }

    return RMAPI_LMON_OK;
}

#endif // SDBG_BASE_RMAPI_LAUNCHMON_IMPL_HXX
