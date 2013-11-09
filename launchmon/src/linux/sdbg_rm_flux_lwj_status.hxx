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

#ifndef SDBG_RM_FLUX_LWJ_STATUS_HXX
#define SDBG_RM_FLUX_LWJ_STATUS_HXX 1

#include "sdbg_base_rm_procgrp_status.hxx"
#include "flux_api.h"


//! flux_lwj_status_pair_t 
/*!
    This is the FLUX LWJ implementation of job_procgrp_status_t.
*/
class flux_lwj_status_pair_t : public job_procgrp_status_pair_t 
{

public:
    flux_lwj_status_pair_t ();

    flux_lwj_status_pair_t (const flux_lwj_status_pair_t &o);

    virtual ~flux_lwj_status_pair_t ();

    virtual const void * get_procgrp_id ();

    virtual void set_procgrp_id (void * id);

    virtual rm_procgrp_status_event_e translate (int status);

private:
    flux_lwj_id_t lwj;
};


#endif // SDBG_RM_FLUX_LWJ_STATUS_HXX
