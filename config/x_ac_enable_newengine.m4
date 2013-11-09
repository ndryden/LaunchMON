der: $
#
# x_ac_newengine.m4
#
# --------------------------------------------------------------------------------
# Copyright (c) 2013, Lawrence Livermore National Security, LLC. Produced at
# the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
# LLNL-CODE-409469. All rights reserved.
#
#  This file is part of LaunchMON. For details, see
#  https://computing.llnl.gov/?set=resources&page=os_projects
#
#  Please also read LICENSE -- Our Notice and GNU Lesser General Public License.
#
#
#  This program is free software; you can redistribute it and/or modify it under the
#  terms of the GNU General Public License (as published by the Free Software
#  Foundation) version 2.1 dated February 1999.
#
#  This program is distributed in the hope that it will be useful, but WITHOUT ANY
#  WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or
#  FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU
#  General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License along
#  with this program; if not, write to the Free Software Foundation, Inc., 59 Temple
#  Place, Suite 330, Boston, MA 02111-1307 USA
# --------------------------------------------------------------------------------
#
#   Update Log:
#         Oct 30 2013 DHA: File created.
#

AC_DEFUN([X_AC_NEWENGINE], [
  AC_MSG_CHECKING([enable new launchmon engine])
  AC_ARG_ENABLE([newengine],
    AS_HELP_STRING([--enable-newengine@<:@=RMAPIPath@:>@],
        [specify a root path of the resource manager API \
         that new engine depends on @<:@default=/usr@:>@]),
    [newengine_enabled=$enableval],
    [newengine_enabled="check"])

  new_engine_needed="no"
  if test "x$newengine_enabled" != "xcheck"; then 
    rmapi_root=""
    if test "x$newengine_enabled" = "xyes"; then
      rmapi_root="/usr"
    else
      rmapi_root=$newengine_enabled
    fi  
    rmapi_lib_path="$rmapi_root/lib"
    rmapi_commlib_path="$rmapi_root/../../lib"
    rmapi_util_path="$rmapi_root/../../util"
    rmapi_inc_path="$rmapi_root/include"
    rmapi_comminc_path="$rmapi_root/../../include"
    rmapi_link="-lfluxapi" 
    AC_SUBST(RMAPI_LIB_PATH, [$rmapi_lib_path])
    AC_SUBST(RMAPI_COMMLIB_PATH, [$rmapi_commlib_path])
    AC_SUBST(RMAPI_UTIL_PATH, [$rmapi_util_path])
    AC_SUBST(RMAPI_INC_PATH, [$rmapi_inc_path])
    AC_SUBST(RMAPI_COMMINC_PATH, [$rmapi_comminc_path])
    AC_SUBST(RMAPI_LINK, [$rmapi_link])
    new_engine_needed="yes"
  fi

  AM_CONDITIONAL([WITH_NEWENGINE], [test "x$new_engine_needed" = "xyes"])
  AC_MSG_RESULT($new_engine_needed)
])

