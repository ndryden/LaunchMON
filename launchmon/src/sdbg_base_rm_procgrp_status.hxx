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
 *        Aug 27 2013 DHA: Aded action support.
 *        Aug 01 2013 DHA: Created file.
 */

#ifndef SDBG_BASE_RM_PROCGRP_STATUS_HXX
#define SDBG_BASE_RM_PROCGRP_STATUS_HXX 1


typedef enum _rm_procgrp_type_e {
    pg_type_application = 0,   /*!< application process group */
    pg_type_be_daemons,        /*!< back-end daemons */
    pg_type_mw_daemons,        /*!< middleware daemons */
    pg_type_reserved,          /*!< reserved */  
} rm_procgrp_type_e;


//! rm_procgrop_status_event_e
/*!
    Enumerator for status change events for the target process group (procgrp).
    Procgrp state transitions are the following:
*/
typedef enum _rm_procgrp_status_event_e {
    pg_status_null = 0,        /*!< created but not registered */
    pg_status_registered,      /*!< registered */
    pg_status_spawn_requested, /*!< registered */
    pg_status_spawned_stopped, /*!< the target spawned and stopped */
    pg_status_spawned_running, /*!< the target spawned and running */
    pg_status_running,         /*!< the target running */
    pg_status_attach_requested,/*!< attach requested */ 
    pg_status_kill_requested,  /*!< kill requested */ 
    pg_status_aborted,         /*!< the target aborted */
    pg_status_completed,       /*!< the target completed */
    pg_status_unregistered,    /*!< unregistered */
    pg_status_reserved         /*!< reserved */ 
} rm_procgrp_status_event_e;


//! rm_procgrop_action_e
/*!
    actions state changes trigger
*/
typedef enum _rm_procgrp_action_e {
    pg_action_update = 0,      /*!< update the status */
    pg_action_spawn_daemons,   /*!< spawn daemons */
    pg_action_cleanup,         /*!< cleanup */
    pg_action_noop,            /*!< noop */
    pg_action_invalid,         /*!< invalid */
} rm_procgrp_action_e;


//! job_procgrp_status_t
/*!
    This is an abstract class that needs be implemented
    by actual RM-specific classes.
*/
class job_procgrp_status_pair_t 
{

public:

     //
     // ctor/dtor
     //
    job_procgrp_status_pair_t ();

    job_procgrp_status_pair_t (
                 const job_procgrp_status_pair_t &o); 

    virtual 
    ~job_procgrp_status_pair_t ();


    //
    // accessors. Note procgrp_id should be set by
    // platform-dependent layer
    //
    virtual 
    const void * get_procgrp_id ()         = 0;

    virtual 
    void set_procgrp_id (void * id)        = 0;

    rm_procgrp_type_e get_type ();

    void set_type (rm_procgrp_type_e t);

    rm_procgrp_status_event_e get_status ();

    rm_procgrp_status_event_e get_oldstatus ();

    void set_status (rm_procgrp_status_event_e s);

    bool get_updatereq ();

    void set_updatereq (bool u);

    rm_procgrp_action_e get_reqaction ();

    void set_reqaction (rm_procgrp_action_e a);
    
    //
    // process_event is mainly called by update_rm_events to
    // determine the status changes and actions. But this
    // does not call event handlers, as that should be the
    // resposibility of the upper layer.
    //    
    bool process_event (int ev);

    //
    // translate must be implemented by platform-depdent
    // layer to turn an event from a specific RM to a
    // normal procgrp event
    //
    virtual 
    rm_procgrp_status_event_e translate (int ev) = 0;
    

private:
    
    rm_procgrp_action_e determine_action ();
    
    //
    // The type of this process group
    //
    rm_procgrp_type_e type;

    //
    // Old and new status. Based on this, we determine
    // state changes and actions.
    //
    rm_procgrp_status_event_e oldstatus;
    rm_procgrp_status_event_e status;

    //
    // Flag to indicate if the status has been updated
    // and if so, what actions are required
    //
    bool updatereq;
    rm_procgrp_action_e reqaction;
};


#endif // SDBG_BASE_RM_PROCGRP_STATUS_HXX
