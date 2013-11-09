## $Header: $
##
## Makefile.am -- Process this file with automake to produce Makefile.in 
##
##--------------------------------------------------------------------------------
## Copyright (c) 2008, Lawrence Livermore National Security, LLC. Produced at
## the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
## LLNL-CODE-401141. All rights reserved.
##
## This file is part of LaunchMON. For details, see
## https://computing.llnl.gov/?set=resources&page=os_projects
##
## Please also read LICENSE -- Our Notice and GNU Lesser General Public License.
##
##
## This program is free software; you can redistribute it and/or modify it under the
## terms of the GNU General Public License (as published by the Free Software
## Foundation) version 2.1 dated February 1999.
##
## This program is distributed in the hope that it will be useful, but WITHOUT ANY
## WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or
## FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public License along
## with this program; if not, write to the Free Software Foundation, Inc., 59 Temple
## Place, Suite 330, Boston, MA 02111-1307 USA
##--------------------------------------------------------------------------------
##
##  Update Log:
##        Oct 1 2013 DHA: Made for COBO/CMB
##


INCLUDES               = -I@RMAPI_COMMINC_PATH@

lib_LTLIBRARIES        = libcobo_fen.la libcobo_be.la

libcobo_be_la_SOURCES  = pmgr_collective_common.c \
                         pmgr_collective_ranges.c \
                         pmgr_collective_client_common.c \
                         pmgr_collective_client_mpirun.c \
                         pmgr_collective_client_tree.c \
                         pmgr_collective_client_slurm.c \
                         pmgr_collective_client.c \
                         pmgr_collective_mpirun.c \
                         cobo_be.c \
                         cobo_be.h

libcobo_fen_la_SOURCES = pmgr_collective_common.c \
                         pmgr_collective_ranges.c \
                         pmgr_collective_client_common.c \
                         pmgr_collective_client_mpirun.c \
                         pmgr_collective_client_tree.c \
                         pmgr_collective_client_slurm.c \
                         pmgr_collective_client.c \
                         pmgr_collective_mpirun.c \
                         cobo_fen.c \
                         cobo_fen.h

include_HEADERS        = cobo_be.h cobo_fen.h 

libcobo_be_la_CFLAGS   = -DHAVE_FLUX_CMB=1 -DMPIRUN_FLUX_CMB_ENABLE=1 
libcobo_be_la_CXXFLAGS = -DHAVE_FLUX_CMB=1 -DMPIRUN_FLUX_CMB_ENABLE=1 

libcobo_be_la_LDFLAGS  = -L@RMAPI_COMMLIB_PATH@ \
                         -Wl,-rpath=@RMAPI_COMMLIB_PATH@ \
                         -version-info @LMON_CURRENT@:@LMON_REVISION@:@LMON_AGE@

libcobo_fen_la_LDFLAGS = -L@RMAPI_COMMLIB_PATH@ \
                         -Wl,-rpath=@RMAPI_COMMLIB_PATH@ \
                         -version-info @LMON_CURRENT@:@LMON_REVISION@:@LMON_AGE@

libcobo_be_la_LIBADD   = -lcmb -ljson -luuid -lzmq -lczmq -lssl 
libcobo_fen_la_LIBADD  = -lcmb -ljson -luuid -lzmq -lczmq -lssl

