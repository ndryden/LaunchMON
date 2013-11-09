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
 *        Aug 01 2013 DHA: Created file.
 */

#ifndef SDBG_RM_FLUX_LWJ_STATUS_IMPL_HXX
#define SDBG_RM_FLUX_LWJ_STATUS_IMPL_HXX 1

#include <string.h>
#include "sdbg_rm_flux_lwj_status.hxx"
#include "flux_api.h"



////////////////////////////////////////////////////////////////////
//
// PRIVATE INTERFACES (class rm_flux_status_pair_t)
//
////////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////
//
// PUBLIC INTERFACES (class rm_flux_status_pair_t)
//
////////////////////////////////////////////////////////////////////

flux_lwj_status_pair_t::flux_lwj_status_pair_t ()
{

}


flux_lwj_status_pair_t::flux_lwj_status_pair_t (
		 const flux_lwj_status_pair_t &o) 
{
    lwj = o.lwj;
}


flux_lwj_status_pair_t::~flux_lwj_status_pair_t () 
{	    

    FLUX_update_destoryLWJCxt (&lwj);
}


const void *
flux_lwj_status_pair_t::get_procgrp_id ()
{
    return &lwj;
}


void 
flux_lwj_status_pair_t::set_procgrp_id (void * id) 
{
    memcpy ((void *) &lwj, id, sizeof (flux_lwj_id_t)); 
}


rm_procgrp_status_event_e
flux_lwj_status_pair_t::translate (int status)
{
    flux_lwj_status_e e = (flux_lwj_status_e) status;
    rm_procgrp_status_event_e rc; 

    switch (e) {	
    case status_null:
	rc = pg_status_null;
	break;
    case status_registered:
	rc = pg_status_registered;
	break;
    case status_spawn_requested:
	rc = pg_status_spawn_requested;
	break;
    case status_spawned_stopped:
	rc = pg_status_spawned_stopped;
	break;
    case status_spawned_running:
	rc = pg_status_spawned_running;
	break;
    case status_running:
	rc = pg_status_running;
	break;
    case status_attach_requested:
	rc = pg_status_attach_requested;
	break;
    case status_kill_requested:
	rc = pg_status_kill_requested;
	break;
    case status_completed:
	rc = pg_status_completed;
	break;
    case status_unregistered:
	rc = pg_status_unregistered;
	break;
    case status_reserved:
	rc = pg_status_reserved;
	break;
    default:
	rc = pg_status_reserved;
	break;	   
    }
    
    return rc;
}

#endif /* SDBG_RM_FLUX_LWJ_STATUS_IMPL_HXX */
