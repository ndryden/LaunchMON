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
 *        Sep 04 2013 DHA: Code clean-up.
 *        Aug 01 2013 DHA: Created file.
 */

#ifndef SDBG_RM_FLUX_LAUNCHMON_HXX
#define SDBG_RM_FLUX_LAUNCHMON_HXX 1

#include "sdbg_opt.hxx"
#include "sdbg_base_driver.hxx"
#include "sdbg_base_launchmon.hxx"
#include "sdbg_base_rmapi_launchmon.hxx"


//! class linux_flux_driver_t
/*!
    flux driver that derives from the rm_driver_base_t. This
    initializes flux-specific event manager and new launchmon
    and registers them as base class objects to the 
    base driver class.
*/
class linux_flux_driver_t : public rm_driver_base_t
{
public:

    ////////////////////////////////////////////////////////////
    //
    // Public Constructor/Destructor
    //
    ////////////////////////////////////////////////////////////
    linux_flux_driver_t();
    ~linux_flux_driver_t();


    ////////////////////////////////////////////////////////////
    //
    // Public interfaces implemented in this class
    //
    ////////////////////////////////////////////////////////////

    //!*
    /*! "main" for the native rm launchmon engine 
     */
    int driver_main (int argc, char *argv[]);

private:

    ////////////////////////////////////////////////////////////
    //
    // Prive Constructor/Destructor to disable copying 
    //
    ////////////////////////////////////////////////////////////
    linux_flux_driver_t (const linux_flux_driver_t& d);
    linux_flux_driver_t & operator=(const linux_flux_driver_t& d);  
};


class rm_flux_api_launchmon_t : public rm_api_launchmon_base_t 
{
public: 

    ////////////////////////////////////////////////////////////
    //
    // Public Constructor/Destructor
    //
    ////////////////////////////////////////////////////////////
                 
    rm_flux_api_launchmon_t ();
    virtual ~rm_flux_api_launchmon_t ();


    ////////////////////////////////////////////////////////////
    //
    // Public interfaces that implement abstract interfaces 
    // of rm_api_launchmon_base_t
    //
    ////////////////////////////////////////////////////////////

    
    //!
    /*! FLUX implementation of init. This method fills all 
        FLUX-specific initialization procedures. If this is
	launch mode, this will launch the specified lightweight 
	job (LWJ) using the FLUX API call.
    */
    virtual 
    rmapi_lmon_rc_e 
    init ( opts_args_t *opt );

    //!
    /*! FLUX update_rm_event implementation. This method will 
        poll the state of all of the procgrp objects and mark 
	updates when state changes are detected.
    */
    virtual
    rmapi_lmon_rc_e 
    update_rm_events ();

    //!
    /*! FLUX implementation of handle_spawn_daemons_action.
        This method will spawn daemons in the right places 
	based on the info passed via pgp.
    */
    virtual 
    rmapi_lmon_rc_e 
    handle_spawn_daemons_action (
                 job_procgrp_status_pair_t *pgp);

    //!
    /*! FLUX implementation of kill request action.
        This method will kill all processes in pgp.
    */
    virtual
    rmapi_lmon_rc_e
    handle_kill_req_action (
                 job_procgrp_status_pair_t *pgp);

    //!
    /*! FLUX implementation of handle_app_cleanup_action
        This method will perform clean-up actions based 
	on the state reported in pgp: the target application 
	has been terminated for a reason captured in pgp.
    */
    virtual 
    rmapi_lmon_rc_e 
    handle_app_cleanup_action (
                 job_procgrp_status_pair_t *pgp);

    //!
    /*! FLUX implementation of handle_daemon_cleanup_action 
        This method will perform clean-up actions based 
	on the state reported in pgp: the daemons have been 
	terminated for a reason specified in pgp.
    */
    virtual 
    rmapi_lmon_rc_e 
    handle_daemon_cleanup_action (
                 job_procgrp_status_pair_t *pgp);


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
    rm_flux_api_launchmon_t (
                 const rm_flux_api_launchmon_t &o);
    rm_flux_api_launchmon_t & operator=(
                 const rm_flux_api_launchmon_t &rhs);


    //!
    /*! private initializer of front-end connection. This is 
        called by the public init method.
    */
    rmapi_lmon_rc_e 
    init_fe_api (opts_args_t *opt);

    //!
    /*! private initializer of FLUX API. This is also
        called by the public init method.
    */
    rmapi_lmon_rc_e 
    init_flux_api (opts_args_t *opt);


    //!
    /*! private method that gathers proctable using FLUX API
    */
    rmapi_lmon_rc_e 
    gather_flux_proctable (const void *target);


    //!
    /*! private method that gathers RM info using FLUX API
    */
    rmapi_lmon_rc_e
    gather_flux_rm_info (
		 job_procgrp_status_pair_t *pgp);

    //!
    /*! private method that spawn/co-locate daemons
        using FLUX API
    */
    rmapi_lmon_rc_e 
    co_locate_flux_daemons (
		 const void *target,
		 rm_procgrp_type_e type );


    ////////////////////////////////////////////////////////////
    //
    // Prive members
    //
    ////////////////////////////////////////////////////////////

    //
    // For self tracing
    //
    std::string MODULENAME;
};


#endif // SDBG_RM_FLUX_LAUNCHMON_HXX
