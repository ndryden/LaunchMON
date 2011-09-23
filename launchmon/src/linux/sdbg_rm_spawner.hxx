/*
 * $Header: $
 *--------------------------------------------------------------------------------
 * Copyright (c) 2008 ~ 2010, Lawrence Livermore National Security, LLC. Produced at 
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
 *        Jul 02 2010 DHA: Created file.
 */

#ifndef SDBG_RM_SPAWNER_HXX
#define SDBG_RM_SPAWNER_HXX 1

#if HAVE_STRING
# include <string>
#else
# error string is required
#endif 


#if HAVE_VECTOR
# include <vector>
#else
# error vector is required
#endif

#include "sdbg_std.hxx"
#include "sdbg_base_spawner.hxx"

class spawner_rm_t : public spawner_base_t
{
public:
  ////////////////////////////////////////////////////////////
  //
  //  Public Interfaces
  //
  spawner_rm_t()
  {
      
  }

  spawner_rm_t(const std::string &dpath,
            const std::vector<std::string> &dmonopts,
            const std::vector<std::string> &hosts)
            : spawner_base_t(std::string(TARGET_JOB_LAUNCHER_PATH), std::vector<std::string>(), dpath, dmonopts, hosts)
    {

    }

  virtual ~spawner_rm_t()
  {

  }

  virtual bool spawn();

  virtual bool combineHosts(std::vector<std::string> &combHosts);

private:
  explicit spawner_rm_t (const spawner_rm_t & s)
   {
     // does nothing
   }
  
  bool create_launch_args();
  bool create_slurm_nodelist_arbitrary();
  bool create_slurm_nodelist_blocks();
  
  bool execute_rm_bulk_launch(int i);
  
  std::vector<pid_t> execpid;
};

#endif // SDBG_RM_SPAWNER_HXX
