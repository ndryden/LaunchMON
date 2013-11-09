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
 *        Aug 01 2013 DHA: Created file.
 */

#ifndef SDBG_BASE_RMAPI_LAUNCHMON_HXX
#define SDBG_BASE_RMAPI_LAUNCHMON_HXX 1

#include <map>
#include <vector>

#include "sdbg_base_rm_procgrp_status.hxx"


//! enumerator launchmon_rc_e 
/*!
    Defines a set of return codes for the new launchmon 
    scheme (through extending rm_api_launchmon_base_t.
*/
enum rmapi_lmon_rc_e {
    RMAPI_LMON_OK = 0,
    RMAPI_LMON_FAILED,
    RMAPI_LMON_STOP,
    RMAPI_LMON_ABORT,
    RMAPI_LMON_BEPROCGRP_EXITED,
    RMAPI_LMON_MWPROCGRP_EXITED,
    RMAPI_LMON_MAINPROCGRP_EXITED
};


//! class rm_api_launchmon_base_t
/*!
    Abstract class that declares launchmon engine that uses 
    the native resource manager API. This defines alternative 
    mechanisms to interact with the resource manager. Its 
    derived classes must implement the engine of this type 
    for a variety of resource manager types.
*/
class rm_api_launchmon_base_t {

public:

    ////////////////////////////////////////////////////////////
    //
    // Public Constructor/Destructor
    //
    ////////////////////////////////////////////////////////////
    rm_api_launchmon_base_t ();
    virtual ~rm_api_launchmon_base_t ();


    ////////////////////////////////////////////////////////////
    //
    // Public interfaces implemented in this class
    //
    ////////////////////////////////////////////////////////////

    //
    // Accessors
    //
    //
    define_gset (int, pcount)
    define_gset (int, FE_sockfd)
    define_gset (bool, API_mode)
    define_gset (double, last_seen)
    define_gset (double, warm_period)

    std::map<std::string, std::vector<MPIR_PROCDESC_EXT *> > & 
    get_proctable_copy();

    opts_args_t *
    get_myopt (); 

    void 
    set_myopt (opts_args_t *o);

    //!
    /*! Method that appends a procgrp object of the type t
        into the procgrp vector of type t
     */ 
    rmapi_lmon_rc_e 
    push_back_procgrp (
		 job_procgrp_status_pair_t *pgp,
		 rm_procgrp_type_e t);
    
    //!
    /*! Method that removes a procgrp object of the type t
        from the procgrp vector of type t
    */ 
    rmapi_lmon_rc_e 
    remove_procgrp (
		 job_procgrp_status_pair_t *pgp,
		 rm_procgrp_type_e t);

    //!
    /*! Method that returns the size of the procgrp 
        vector of type t
     */ 
    int 
    get_procgrp_size (rm_procgrp_type_e t);

    //!
    /*! Method that returns the size of the procgrp 
        vector of type t
     */ 
    job_procgrp_status_pair_t *
    get_procgrp_at (int index, rm_procgrp_type_e t);

    //!
    /*! Method that sends an LMONP event msg to the FE
     */   
    rmapi_lmon_rc_e 
    say_fetofe_msg (lmonp_fe_to_fe_msg_e t);

    //!
    /*! Method that handles any message received from 
        the FE API client
    */
    rmapi_lmon_rc_e 
    update_socket_events ();

    //!
    /*! Method that invokes an action handler based upon 
        the state changes tracked through pgp
    */
    rmapi_lmon_rc_e 
    invoke_handler (job_procgrp_status_pair_t *pgp);

    //!
    /*! Method that ships the serialized global process
        table to the FE
    */
    rmapi_lmon_rc_e 
    ship_proctab_msg (lmonp_fe_to_fe_msg_e t);

    //!
    /*! Method that ships the resource handle to the FE

     */
    rmapi_lmon_rc_e
    ship_resourcehandle_msg (
                 lmonp_fe_to_fe_msg_e t, int rid);
  
    //!
    /*! Method that ships the rminfo to the FE
     */
    rmapi_lmon_rc_e
    ship_rminfo_msg (lmonp_fe_to_fe_msg_e t,
		 int rmpid,
		 rm_catalogue_e rmtype);

    //!
    /*! invoke action handlers based upon the state 
        transitions observed in target_app_vector,
        coloc_grp_vector and mw_grp_vector.
    */
    rmapi_lmon_rc_e 
    invoke_action_handlers ();


    ////////////////////////////////////////////////////////////
    //
    // Public abstract interfaces that derived classes must 
    // implement
    //
    ////////////////////////////////////////////////////////////

    //!
    /*! init must be implemented by a derived class. 
        This method fills all platform specific 
	initialization procedures. If this is launch mode, 
	this will launch the specified procgrp using the 
	resource manager API call.
    */
    virtual 
    rmapi_lmon_rc_e 
    init ( opts_args_t *opt )      = 0;

    //!
    /*! update_rm_event must be implemented by a derived
        class. This method will poll the state of all of
	the procgrp objects and mark updates when state
	changes are detected.
    */
    virtual 
    rmapi_lmon_rc_e 
    update_rm_events ()             = 0;
    
    //!
    /*! handle_spawn_daemons_action must be implemented 
        by a derived class. This method will spawn 
	daemons in the right places based on the info
	passed via pgp
    */
    virtual 
    rmapi_lmon_rc_e 
    handle_spawn_daemons_action (
                 job_procgrp_status_pair_t *pgp)  
	                            = 0;

    //!
    /*! handle_app_cleanup_action must be implemented
        by a derived class. This method will perform
	clean-up actions based on the state reported in
	pgp: the target application has been terminated
	for a reason captured in pgp.
    */
    virtual 
    rmapi_lmon_rc_e 
    handle_app_cleanup_action ( 
                 job_procgrp_status_pair_t *pgp)   
	                            = 0;

    //!
    /*! handle_daemon_cleanup_action ust be implemented
        by a derived class. This method will perform
	clean-up actions based on the state reported in
	pgp: the daemons have been terminated for a 
	reason specified in pgp.
    */
    virtual 
    rmapi_lmon_rc_e 
    handle_daemon_cleanup_action (
                 job_procgrp_status_pair_t *pgp)   
                                    = 0;


private:

    bool 
    LEVELCHK(self_trace_verbosity level) { 
	return (self_trace_t::
	    rm_launchmon_module_trace.verbosity_level 
		>= level); 
    }


    ////////////////////////////////////////////////////////////
    //
    // Prive Constructor/Destructor to disable copying 
    //
    ////////////////////////////////////////////////////////////
    rm_api_launchmon_base_t & operator=(
                 const rm_api_launchmon_base_t &o);
    rm_api_launchmon_base_t (
                 const rm_api_launchmon_base_t &o); 


    ////////////////////////////////////////////////////////////
    //
    // Prive members
    //
    ////////////////////////////////////////////////////////////

    // These handles are essentially arrays because
    // multiple procgrps could have been spawned incrementally
    // or we might have a hierarachical procgrp
    std::vector<job_procgrp_status_pair_t *> 
        target_app_vector;

    std::vector<job_procgrp_status_pair_t *> 
        coloc_grp_vector;

    std::vector<job_procgrp_status_pair_t *> 
        mw_grp_vector;

    //
    // The member containing all proctable entries
    // std::vector<MPIR_PROCDESC_EXT *> proctable_copy;
    //
    // This table is filled by the platform dependent layer
    //
    std::map<std::string, std::vector<MPIR_PROCDESC_EXT *> > 
        proctable_copy;
    int pcount;
    int FE_sockfd;
    bool API_mode;
    opts_args_t *myopt;

    //
    // To support two-phase polling scheme
    //
    double last_seen;
    double warm_period;

    //
    // For self tracing
    //
    std::string MODULENAME; 
};

#endif // SDBG_BASE_RMAPI_LAUNCHMON_HXX
