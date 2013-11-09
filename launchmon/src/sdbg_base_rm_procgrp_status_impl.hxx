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
 *  Update Log:
 *        Aug 27 2013 DHA: Created file.
 */

#ifndef SDBG_BASE_RM_PROCGRP_STATUS_IMPL_HXX
#define SDBG_BASE_RM_PROCGRP_STATUS_IMPL_HXX 1


#include <string.h>
#include "sdbg_base_rm_procgrp_status.hxx"


////////////////////////////////////////////////////////////////////
//
// PRIVATE INTERFACES (class rm_flux_status_pair_t)
//
////////////////////////////////////////////////////////////////////

//! determine_action()
/*!
    return action an state transition should/can trigger
*/
rm_procgrp_action_e 
job_procgrp_status_pair_t::determine_action()
{
    rm_procgrp_action_e action = pg_action_invalid;

    switch (status) {

    case pg_status_registered: {

	if (oldstatus == pg_status_null) {
	    action = pg_action_update;
	}
	break;
    }

    case pg_status_spawn_requested: {

	if (oldstatus == pg_status_registered) {
	    action = pg_action_update;
	}
	break;
    }

    case pg_status_spawned_stopped: {

	if ( (oldstatus == pg_status_spawn_requested)
	    || (oldstatus == pg_status_registered) ) {
	    action = pg_action_spawn_daemons;
	}
	break;
    }

    case pg_status_spawned_running: {

	if ( (oldstatus == pg_status_spawn_requested)
	    || (oldstatus == pg_status_registered) ) {
	    action = pg_action_spawn_daemons;
	}
	break;
    }

    case pg_status_running: {

	if ( (oldstatus == pg_status_null) 
            || (oldstatus == pg_status_registered) ) {
	    action = pg_action_spawn_daemons;
	}
	else if ( (oldstatus == pg_status_spawned_stopped)
		  || (oldstatus == pg_status_spawned_running)
		  || (oldstatus == pg_status_attach_requested) ) {
	    action = pg_action_update;
	}
	break;
    }

    case pg_status_attach_requested: {

	if (oldstatus == pg_status_running) {
	    action = pg_action_spawn_daemons;
	}
	break;
    }

    case pg_status_kill_requested: {

	if ( (oldstatus == pg_status_running)
	     || (oldstatus == pg_status_spawned_stopped)
	     || (oldstatus == pg_status_spawned_running)) {		 
	    action = pg_action_update;
	}
	break;

    }

    case pg_status_aborted: {

	if ( (oldstatus == pg_status_running)
	     || (oldstatus == pg_status_spawned_stopped)
	     || (oldstatus == pg_status_spawned_running)
	     || (oldstatus == pg_status_kill_requested)) {		 
	    action = pg_action_cleanup;
	}

	break;
    }

    case pg_status_completed: {

	if (oldstatus == pg_status_running) {
	    action = pg_action_cleanup;
	}

	break;
    }

    case pg_status_unregistered: {

	if ((oldstatus == pg_status_completed) 
	    || (oldstatus == pg_status_aborted)) {
	    action = pg_action_update;
	}

	break;
    }

    case pg_status_null: {

	if (oldstatus == pg_status_unregistered) {
	    action = pg_action_update;
	}

	break;
    }
	
    default:
	break;
    }

    return action;
}


////////////////////////////////////////////////////////////////////
//
// PUBLIC INTERFACES (class rm_flux_status_pair_t)
//
////////////////////////////////////////////////////////////////////


//! job_procgrp_status_pair_t ctor/dtor
/*!
    
*/
job_procgrp_status_pair_t::job_procgrp_status_pair_t ()
                 : type (pg_type_reserved), 
		   oldstatus (pg_status_null), 
		   status (pg_status_null),
		   updatereq (false),
		   reqaction(pg_action_invalid)
{

} 


job_procgrp_status_pair_t::job_procgrp_status_pair_t (
                 const job_procgrp_status_pair_t &o)
{
    oldstatus = o.oldstatus;
    status = o.status;
} 


job_procgrp_status_pair_t::~job_procgrp_status_pair_t ()
{

}


//! job_procgrp_status_pair_t accessors
/*!
    
*/

rm_procgrp_type_e 
job_procgrp_status_pair_t::get_type ()
{
    return type;
}


void 
job_procgrp_status_pair_t::set_type (rm_procgrp_type_e t)
{
    type = t;
}


rm_procgrp_status_event_e 
job_procgrp_status_pair_t::get_status () 
{
    return status;
}


rm_procgrp_status_event_e 
job_procgrp_status_pair_t::get_oldstatus () 
{
    return oldstatus;
}


void
job_procgrp_status_pair_t::set_status (rm_procgrp_status_event_e s) 
{
    oldstatus = status;
    status = s;
}


bool
job_procgrp_status_pair_t::get_updatereq ()
{
    return updatereq;
}


void 
job_procgrp_status_pair_t::set_updatereq (bool u)
{
    updatereq = u;
}


rm_procgrp_action_e 
job_procgrp_status_pair_t::get_reqaction ()
{
    return reqaction;
}


void 
job_procgrp_status_pair_t::set_reqaction (rm_procgrp_action_e a)
{
    reqaction = a;
}


//! process_event
/*!
    process procgrp state changes
*/
bool
job_procgrp_status_pair_t::process_event (int ev)
{
    rm_procgrp_status_event_e myev = translate (ev);
    
    if (myev == status) {
	updatereq = false;
	reqaction = pg_action_noop;
	return false;
    }

    oldstatus = status;
    status = myev;    
    reqaction = determine_action ();    
    updatereq = true;
    
    return true;
}


#endif // SDBG_BASE_RM_PROCGRP_STATUS_IMPL_HXX
