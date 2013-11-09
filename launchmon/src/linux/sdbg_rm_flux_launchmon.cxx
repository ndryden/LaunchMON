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
 *        Sep 04 2013 DHA: Code clean-up
 *        Aug 01 2013 DHA: Created file.
 */

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


#include "sdbg_rm_flux_launchmon.hxx"
#include "sdbg_rm_flux_lwj_status.hxx"
#include "sdbg_rm_flux_lwj_status_impl.hxx"
#include <lmon_api/lmon_proctab.h>
#include <lmon_api/lmon_lmonp_msg.h>
#include "lmon_api/lmon_say_msg.hxx"
#include "flux_api.h"


////////////////////////////////////////////////////////////////////
//
// PRIVATE INTERFACES (class rm_flux_api_launchmon_t)
//
////////////////////////////////////////////////////////////////////


rm_flux_api_launchmon_t::rm_flux_api_launchmon_t ( 
                 const rm_flux_api_launchmon_t &d) 
{
    MODULENAME = d.MODULENAME;
}


rm_flux_api_launchmon_t & 
rm_flux_api_launchmon_t::operator=( 
                 const rm_flux_api_launchmon_t &rhs )
{

    MODULENAME = rhs.MODULENAME;
    
    return *this;
}


rmapi_lmon_rc_e
rm_flux_api_launchmon_t::init_fe_api (opts_args_t *opt)
{
    char *tokenize     = NULL;
    char *FEip         = NULL;
    char *FEport       = NULL;
    int optval         = 1;
    int optlen         = sizeof(optval);
    int clientsockfd   = -1;
    rmapi_lmon_rc_e rc = RMAPI_LMON_OK;
    struct sockaddr_in servaddr;

    if (!opt->get_my_opt()->remote) {
        //
	// if this isn't API mode, apparently you don't have to 
	// do anything.
	//
	goto ret_loc;
    }

    //
    // parsing ip:port info (connection infomation to the 
    // front end
    //
    tokenize = strdup(opt->get_my_opt()->remote_info.c_str());
    FEip = strtok ( tokenize, ":" );
    FEport = strtok ( NULL, ":" );
    
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons((uint16_t) atoi(FEport));


    //
    // Converting the text IP (or hostname) to binary
    //
    if ( inet_pton(AF_INET, 
		   (const char*) FEip,
		   &(servaddr.sin_addr)) < 0 ) {

        self_trace_t::trace ( LEVELCHK(level1),
	    MODULENAME,1,
	    "inet_pton failed in the rm launchmon init.");
	
	    rc =  RMAPI_LMON_FAILED;
	    goto ret_loc;
    }

    free(tokenize);

    if ( ( clientsockfd = socket ( AF_INET, 
				   SOCK_STREAM, 0 )) < 0 ) {
        self_trace_t::trace ( LEVELCHK(level1),
	    MODULENAME,1,
            "socket failed in the rm launchmon init handler.");

	    rc =  RMAPI_LMON_FAILED;
	    goto ret_loc;
    }

    if( setsockopt(clientsockfd, SOL_SOCKET, 
		   SO_KEEPALIVE, &optval, optlen) < 0 ) {
        self_trace_t::trace ( LEVELCHK(level1),
	    MODULENAME,1,
	    "setting socket keepalive failed.");

	    rc =  RMAPI_LMON_FAILED;
	    goto ret_loc;
    }

    if ( ( connect ( clientsockfd, 
		     (struct sockaddr *)&servaddr,
		     sizeof(servaddr) )) < 0 ) {
        self_trace_t::trace ( LEVELCHK(level1),
            MODULENAME,1,
	    "connect failed in the rm launchmon's init handler.");

	    rc =  RMAPI_LMON_FAILED;
	    goto ret_loc;
    }

    //
    // registering the FD for FE-client and engine connection
    //
    set_FE_sockfd ( clientsockfd );

    //
    // setting API mode flag
    //
    set_API_mode ( true );

    {
	self_trace_t::trace ( LEVELCHK(level2),
            MODULENAME,0,
            "init_fe_api initialized.");
    }

    //
    // communicate to the FE API that there was a parse error
    //
    if (opt->get_has_parse_error()) {
        say_fetofe_msg ( lmonp_conn_ack_parse_error );
	self_trace_t::trace ( true,
            MODULENAME,1,
            "the RM launchmon engine deteced parsing errors.");

	rc =  RMAPI_LMON_FAILED;
	goto ret_loc;
    } 
    else {
        say_fetofe_msg ( lmonp_conn_ack_no_error );
    }

 ret_loc:
    return rc;
}


//!
/*!  rm_flux_api_launchmon_t init_flux_api

*/
rmapi_lmon_rc_e
rm_flux_api_launchmon_t::init_flux_api (opts_args_t *opt)
{   
    flux_lwj_id_t fid;
    int nNodes                  = 0;
    int nProcs                  = 0;
    flux_lwj_status_e lwjstatus = status_null;
    int sync                    = 1;
    char **newargv              = NULL;
    rmapi_lmon_rc_e rc          = RMAPI_LMON_FAILED;
    flux_lwj_status_pair_t *newpair = NULL;
    flux_lwj_info_t lwjinfo;


    set_myopt (opt);

    if ( FLUX_init() != FLUX_OK ) {
	self_trace_t::trace ( true,
            MODULENAME,1,
            "FLUX_init failed.");

	return rc;
    }


    //
    // FLUX TODO: Add LMON_DONT_STOP_APP support here
    // Question  2: How to handle stdin/stdout/stderr 
    //              in this case
    //
    newpair = new flux_lwj_status_pair_t();
    if ( opt->get_my_opt()->attach ) {

        //////////////////////////////////////////////////////	
        //                  ATTACH MODE  
	//
        //////////////////////////////////////////////////////
        flux_starter_info_t starter;

        starter.pid = opt->get_my_opt()->launcher_pid;
        starter.hostname = (char *) malloc (128);
        if ( gethostname (starter.hostname, 128) < 0 ) {
	    self_trace_t::trace ( true,
                MODULENAME,1,
	        "gethostname failed.");

	    goto ret_loc;  
        }
        if ( FLUX_query_pid2LWJId (&starter, &fid) != FLUX_OK ) {
	    self_trace_t::trace ( true,
                MODULENAME,1,
	        "FLUX_query_pid2LWJId returned an error.");

	    goto ret_loc;  
	}
        free (starter.hostname);

        if (FLUX_query_LWJId2JobInfo (&fid, &lwjinfo)
            != FLUX_OK ) {
            self_trace_t::trace ( true,
                MODULENAME,1,
                "FLUX_query_LWJId2JobInfo returned an error.");

            goto ret_loc;
        }

        newpair->set_procgrp_id ((void *) &(lwjinfo.lwjid));
	newpair->set_type (pg_type_application);

	if ( FLUX_query_LWJStatus (&fid, &lwjstatus) != FLUX_OK ) {
	    self_trace_t::trace ( true,
                MODULENAME,1,
	        "FLUX_query_LWJStatus returned an error.");

	    goto ret_loc;  
	}
	newpair->process_event (lwjstatus);
	if (newpair->get_updatereq ()) {
	    rc = invoke_handler ((job_procgrp_status_pair_t *) newpair);
	}
	else {
	    rc = RMAPI_LMON_OK;
	}
    }
    else {

        //////////////////////////////////////////////////////	
        //                  LAUNCH MODE  
	//
        //////////////////////////////////////////////////////
        
        if ( FLUX_update_createLWJCxt (&fid) 
	     != FLUX_OK ) {

	    self_trace_t::trace ( true,
                MODULENAME,1,
	        "FLUX_update_createLWJCxt returned an error.");

	    goto ret_loc;  
        }

        if (FLUX_query_LWJId2JobInfo (&fid, &lwjinfo)
            != FLUX_OK ) {
            self_trace_t::trace ( true,
                MODULENAME,1,
                "FLUX_query_LWJId2JobInfo returned an error.");

            goto ret_loc;
        }

        newpair->set_procgrp_id ((void *) &(lwjinfo.lwjid));
	newpair->set_type (pg_type_application);

	if ( FLUX_query_LWJStatus (&fid, &lwjstatus) != FLUX_OK ) {

	    self_trace_t::trace ( true,
                MODULENAME,1,
	        "FLUX_query_LWJStatus returned an error.");

	    goto ret_loc;  
	}
	newpair->process_event (lwjstatus);
	if (newpair->get_updatereq ()) {
	    invoke_handler ((job_procgrp_status_pair_t *) newpair);
	}

        // 
	// TODO: ultimately decide how to parse arg/opt of wreckrun
        //       only have to deal with -N and -n.
	//
        char **t = &(opt->get_my_opt()->remaining[1]);
        int procs_per_node = 1;
        int nodes = 1;
        
        while (t != NULL) {
            if (t[0][0] == '-') {
                if (t[0][1] == 'n') {
                    procs_per_node 
                        = atoi ((const char *) &(t[0][2]));
                }
                else if (t[0][1] == 'N') {
                    nodes 
                        = atoi ((const char *) &(t[0][2]));
                }    
            }
            else {
                break;
            }
            t++;
        }
        const char *lwjpath = *t;
        char **newargv = t;
        if ( FLUX_launch_spawn (
	         (const flux_lwj_id_t *) newpair->get_procgrp_id(), 
	         sync, 
	         NULL,
	         lwjpath,
	         (char * const *) newargv,
	         0,
                 nodes, 
                 procs_per_node) != FLUX_OK) {

	    self_trace_t::trace ( true,
                MODULENAME,1,
	        "FLUX_launch_spawn returned an error.");

	    goto ret_loc;
	}

	rc = RMAPI_LMON_OK;
    }

    push_back_procgrp (newpair, pg_type_application);

ret_loc:
    return rc;;
}


rmapi_lmon_rc_e 
rm_flux_api_launchmon_t::gather_flux_proctable (
                 const void *target)
{
    MPIR_PROCDESC_EXT *ptab = NULL;
    size_t ptabsize         = 0;
    size_t ret_ptabsize     = 0;
    int i                   = 0;
    const flux_lwj_id_t *fid= (const flux_lwj_id_t *) target;
    rmapi_lmon_rc_e rc = RMAPI_LMON_FAILED;

    if ( FLUX_query_globalProcTableSize (
			 fid, &ptabsize) != FLUX_OK ) {

	self_trace_t::trace ( true,
            MODULENAME,1,
	    "FLUX_query_globalProcTableSize returned an error.");

	goto ret_loc;  
    }

    ptab = (MPIR_PROCDESC_EXT *) 
	malloc (ptabsize*sizeof(MPIR_PROCDESC_EXT));

    if (!ptab) {
	self_trace_t::trace ( true,
            MODULENAME,1,
	    "malloc returned NULL.");

	goto ret_loc;  	
    }

    if ( FLUX_query_globalProcTable (
             fid, ptab, ptabsize, &ret_ptabsize ) != FLUX_OK ) {

	self_trace_t::trace ( true,
            MODULENAME,1,
	    "FLUX_query_globalProcTable returned an error.");

	    goto ret_loc;  
    }

    set_pcount (ptabsize);

    for (i=0; i < ptabsize; ++i) {
	get_proctable_copy()[
            std::string(ptab[i].pd.host_name)].push_back(&ptab[i]);
    }

    rc = ship_proctab_msg (lmonp_proctable_avail);

 ret_loc:
    return rc;
}


rmapi_lmon_rc_e 
rm_flux_api_launchmon_t::gather_flux_rm_info (
                 job_procgrp_status_pair_t *pgp)
{
    flux_lwj_info_t lwjInfo;
    rmapi_lmon_rc_e rc   = RMAPI_LMON_FAILED;
    rc_rm_t *rm_obj = NULL;
    bool found_matched_rm = false;
    std::vector<resource_manager_t>::const_iterator r_i;

    if ( FLUX_query_LWJId2JobInfo ( (flux_lwj_id_t *)
		                    pgp->get_procgrp_id (),
		                    &lwjInfo) != FLUX_OK ) {
	self_trace_t::trace ( true,
	    MODULENAME,1,
	    "FLUX_query_LWJId2JobInfo returned an error.");

	goto ret_loc;  
    }
    
    if ( (ship_resourcehandle_msg ( lmonp_resourcehandle_avail, 
		           lwjInfo.lwjid )) != RMAPI_LMON_OK) {
	self_trace_t::trace ( true,
	    MODULENAME,1,
	    "ship_resourcehandle_msg returned an error.");

	goto ret_loc;
    }

    rm_obj = get_myopt()->get_my_rmconfig();
    for (r_i = rm_obj->get_supported_rms().begin();
	 r_i != rm_obj->get_supported_rms().end(); r_i++) {

	if ((*r_i).get_const_rm () == RC_flux) {
	    rm_obj->set_resource_manager(*r_i);
	    found_matched_rm = true;
	    break;
	}
    }

    if (!found_matched_rm) {
	self_trace_t::trace ( true,
	    MODULENAME,1,
	    "FLUX is not supported...");
	
	goto ret_loc;
    }

    if ( ( ship_rminfo_msg ( lmonp_rminfo,0,
	         rm_obj->get_resource_manager().get_rm()))
	         != RMAPI_LMON_OK) {
	self_trace_t::trace ( true,
	    MODULENAME,1,
	    "ship_resourcehandle_msg returned an error.");

	goto ret_loc;
    }
    
    rc = RMAPI_LMON_OK;

 ret_loc:
    return rc;
}


rmapi_lmon_rc_e 
rm_flux_api_launchmon_t::co_locate_flux_daemons (
		 const void *target,
		 rm_procgrp_type_e type )
{    
    using namespace std;

    flux_lwj_id_t fid;
    flux_lwj_id_t *tfid   = (flux_lwj_id_t *) target;
    flux_lwj_status_e lwjstatus = status_null;
    int nnodes            = 0;
    int nprocs            = 0;
    size_t indx           = 0;
    char *tokenize2       = NULL; 
    char *sharedsecret    = NULL;
    char *randomID        = NULL;
    char **newargv        = NULL;
    bool found_matched_rm = false;
    rc_rm_t *rm_obj       = NULL;
    opt_struct_t *o       = NULL;
    rmapi_lmon_rc_e rc    = RMAPI_LMON_FAILED;
    flux_lwj_status_pair_t *newpair = NULL;
    flux_lwj_info_t jinfo; 
    flux_lwj_info_t target_jinfo; 
    string expanded_string;
    list<string> alist;
    list<string>::iterator iter;

    //
    // Create a new LWJ context
    //
    if ( FLUX_update_createLWJCxt (&fid) 
	 != FLUX_OK ) {

	self_trace_t::trace ( true,
            MODULENAME,1,
	    "FLUX_update_createLWJCxt returned an error.");

        goto ret_loc;  
    }

    if (FLUX_query_LWJId2JobInfo (&fid, &jinfo)
        != FLUX_OK ) {
	self_trace_t::trace ( true,
            MODULENAME,1,
	    "FLUX_query_LWJId2JobInfo returned an error.");

        goto ret_loc;  
    }
    if (FLUX_query_LWJId2JobInfo (tfid, &target_jinfo)
        != FLUX_OK ) {
	self_trace_t::trace ( true,
            MODULENAME,1,
	    "FLUX_query_LWJId2JobInfo returned an error.");

        goto ret_loc;  
    }

    newpair = new flux_lwj_status_pair_t();
    newpair->set_procgrp_id ((void *) &(jinfo.lwjid));
    newpair->set_type (type);

    if ( FLUX_query_LWJStatus (&fid, &lwjstatus) 
	 != FLUX_OK ) {

	self_trace_t::trace ( true,
            MODULENAME,1,
	    "FLUX_query_LWJStatus returned an error.");

	    goto ret_loc;  
    }

    newpair->process_event (lwjstatus);
    if (newpair->get_updatereq ()) {
	invoke_handler (
	    (job_procgrp_status_pair_t *) newpair);
    }

    
    //
    // Bulk-launch ... For FLUX
    // coloc info is set here instead of 
    // in init_rm_instance
    //
    rm_obj = get_myopt()->get_my_rmconfig();

    o = get_myopt()->get_my_opt();
    rm_obj->get_coloc_paramset().rm_daemon_path
	= o->tool_daemon;
    rm_obj->get_coloc_paramset().rm_daemon_args
	= o->tool_daemon_opts;
    rm_obj->get_coloc_paramset().febe_conn_info
	= o->febeconn_info;
    rm_obj->get_coloc_paramset().femw_conn_info
	= o->femwconn_info;
	
    tokenize2 = strdup (o->lmon_sec_info.c_str());
    sharedsecret = strtok (tokenize2, ":");
    randomID = strtok (NULL, ":");
    rm_obj->set_paramset (get_proctable_copy().size(),
			  1, 
			  sharedsecret,
			  randomID,
			  target_jinfo.lwjid,
			  jinfo.lwjid,
			  NULL);
	
    alist = rm_obj->expand_newlaunch_string (
	         expanded_string);

    newargv = (char **) 
                 malloc (sizeof(char *)*(alist.size()+1));
    if (!newargv) {
	self_trace_t::trace ( true, 
	    MODULENAME,
	    0,
	    "malloc returned NULL.");

	goto ret_loc;
    }
    for (iter = alist.begin(); iter != alist.end(); iter++) {
	    newargv[indx] = strdup((*iter).c_str());
	    indx++;
    }
    newargv[indx] = NULL;

    if ( FLUX_launch_spawn (
	   (const flux_lwj_id_t *) 
	   newpair->get_procgrp_id(),
	   0, /* sync flag */
           (const flux_lwj_id_t *) tfid, 
	   o->tool_daemon.c_str(),
	   (char * const *) newargv,
	   1, /* coloc flag */
	   rm_obj->get_coloc_paramset().nnodes, 
	   rm_obj->get_coloc_paramset().ndaemons) != FLUX_OK) {

	self_trace_t::trace ( true,
	    MODULENAME,1,
	    "FLUX_launch_spawn returned an error.");

	goto ret_loc;
    }

    push_back_procgrp (newpair, pg_type_be_daemons);
    rc = RMAPI_LMON_OK;

 ret_loc:
    return rc; 
}


////////////////////////////////////////////////////////////////////
//
// PUBLIC INTERFACES (class rm_flux_api_launchmon_t)
//
////////////////////////////////////////////////////////////////////

rm_flux_api_launchmon_t::rm_flux_api_launchmon_t ()
{
    MODULENAME 
	=  self_trace_t::rm_driver_module_trace.module_name;
}


rm_flux_api_launchmon_t::~rm_flux_api_launchmon_t ()
{
    
}


rmapi_lmon_rc_e
rm_flux_api_launchmon_t::init (opts_args_t *opt)
{
    rmapi_lmon_rc_e rc = RMAPI_LMON_OK;

    if ( (rc = init_fe_api(opt)) != RMAPI_LMON_OK ) {
	self_trace_t::trace ( true,
            MODULENAME,1,
            "FE API init failed.");

	rc =  RMAPI_LMON_FAILED;
	goto ret_loc;	
    }


    if ( (rc = init_flux_api(opt)) != RMAPI_LMON_OK ) {
	self_trace_t::trace ( true,
            MODULENAME,1,
            "RM API init failed.");

	rc =  RMAPI_LMON_FAILED;
	goto ret_loc;	
    }

ret_loc:
    set_last_seen (gettimeofdayD ());
    return rc;
}


rmapi_lmon_rc_e
rm_flux_api_launchmon_t::update_rm_events ()
{
    int i                 = 0;
    int size              = 0;
    flux_lwj_status_e lwjstatus = status_null;
    flux_lwj_id_t *mylwj = NULL;
    rmapi_lmon_rc_e rc   = RMAPI_LMON_FAILED;
    int t  = (int) pg_type_application;	
    job_procgrp_status_pair_t *procgrp = NULL;

    //
    // Loop that iterates over pg_type_application,
    // pg_type_be_daemons, and pg_type_mw_daemons.
    //
    for (t = pg_type_application; 
	 t < (int) pg_type_reserved; ++t) {	
	size = get_procgrp_size ((rm_procgrp_type_e)t);
	lwjstatus = status_null;

	for (i=0; i < size; ++i) {
	    procgrp = get_procgrp_at (i, (rm_procgrp_type_e)t);
	    mylwj = (flux_lwj_id_t *) 
		(procgrp->get_procgrp_id ());

	    if ( FLUX_query_LWJStatus (mylwj, &lwjstatus) 
		 != FLUX_OK ) {
		self_trace_t::trace ( true,
 	            MODULENAME,1,
	            "FLUX_query_LWJStatus returned an error.");

		goto ret_loc;  
	    }
	    procgrp->process_event (lwjstatus);
	}
    }

    rc = RMAPI_LMON_OK;

ret_loc:
    return rc;
}


rmapi_lmon_rc_e 
rm_flux_api_launchmon_t::handle_spawn_daemons_action (
                 job_procgrp_status_pair_t *pgp)
{  
    rmapi_lmon_rc_e rc = RMAPI_LMON_FAILED;
    
    //
    // Say to the FEN 
    //
    if ( (rc = say_fetofe_msg (
	       lmonp_stop_at_launch_bp_spawned) ) != RMAPI_LMON_OK) {

	self_trace_t::trace ( true,
 	    MODULENAME,1,
	    "Sending a MSG to FE returned an error.");

	goto ret_loc;
    }

    //
    // Gather proctable
    //
    if ( ( rc = gather_flux_proctable (
	            pgp->get_procgrp_id())) != RMAPI_LMON_OK) {

	self_trace_t::trace ( true,
 	    MODULENAME,1,
	    "Gathering proctable returned an error");

	goto ret_loc;
    }

    //
    // Gather flux rm info
    //
    if ( ( rc = gather_flux_rm_info (pgp)) != RMAPI_LMON_OK ) {

	self_trace_t::trace ( true,
 	    MODULENAME,1,
	    "Gathering FLUX RM inforeturned an error");

	goto ret_loc;
    }    

    //
    // Launch back-end daemons
    //
    if ( ( rc = co_locate_flux_daemons (
                 pgp->get_procgrp_id(),
		 pg_type_be_daemons) ) != RMAPI_LMON_OK) {

	self_trace_t::trace ( true,
 	    MODULENAME,1,
	    "Co-locating daemons returned an error");

	goto ret_loc;
    }
   
    rc = RMAPI_LMON_OK;

ret_loc:
    set_last_seen (gettimeofdayD ());
    return rc;
}

 
//!
/*!  rm_flux_api_launchmon_t handle_app_cleanup_action
     perform cleanup
*/
rmapi_lmon_rc_e 
rm_flux_api_launchmon_t::handle_app_cleanup_action (
                 job_procgrp_status_pair_t *pgp)
{
    rmapi_lmon_rc_e rc = RMAPI_LMON_OK;

    switch (pgp->get_type ()) {

    case pg_type_application:
        say_fetofe_msg ( lmonp_exited );    
        rc = RMAPI_LMON_MAINPROCGRP_EXITED;
        break;

    case pg_type_be_daemons:
    case pg_type_mw_daemons:
	self_trace_t::trace ( true,
 	    MODULENAME,1,
	    "group type daemon called app-cleanup action!");
        rc = RMAPI_LMON_FAILED;
        break;

    default:
	self_trace_t::trace ( true,
 	    MODULENAME,1,
	    "unknown group type!");
        rc = RMAPI_LMON_FAILED;
        break;
    }

    set_last_seen (gettimeofdayD ());
    return rc;
}


//!
/*!  rm_flux_api_launchmon_t handle_daemon_cleanup_action
     perform cleanup
*/
rmapi_lmon_rc_e 
rm_flux_api_launchmon_t::handle_daemon_cleanup_action (
                 job_procgrp_status_pair_t *pgp)
{
    rmapi_lmon_rc_e rc = RMAPI_LMON_OK;

    switch (pgp->get_type ()) {

    case pg_type_be_daemons:
        say_fetofe_msg ( lmonp_bedmon_exited );
        rc = RMAPI_LMON_BEPROCGRP_EXITED;
        break;

    case pg_type_mw_daemons:
        say_fetofe_msg ( lmonp_mwdmon_exited );
        rc = RMAPI_LMON_MWPROCGRP_EXITED;
        break;

    case pg_type_application:
	self_trace_t::trace ( true,
 	    MODULENAME,1,
	    "group type app called daemon cleanup action!");
        rc = RMAPI_LMON_FAILED;
        break;

    default:
	self_trace_t::trace ( true,
 	    MODULENAME,1,
	    "unknown group type!");
        rc = RMAPI_LMON_FAILED;
        break;
    }

    set_last_seen (gettimeofdayD ());
    return rc;
}



//////////////////////////////////////////////////////////////////
//
// PUBLIC INTERFACES (class linux_flux_driver_t<>)
//
//////////////////////////////////////////////////////////////////

linux_flux_driver_t::linux_flux_driver_t()
{

}


linux_flux_driver_t::~linux_flux_driver_t()
{

}


int
linux_flux_driver_t::driver_main (int argc, char **argv)
{
  driver_error_e error_code; 

  rm_event_manager_t *rm_em;
  rm_api_launchmon_base_t *rm_lm;

  rm_em = new rm_event_manager_t ();
  rm_lm = new rm_flux_api_launchmon_t ();
   
  this->set_rm_evman (rm_em);
  this->set_rm_lmon (rm_lm);

  error_code 
    = rm_driver_base_t::drive (argc, argv);

  return ( error_code == SDBG_DRIVER_OK) ? 0 : 1;
}

