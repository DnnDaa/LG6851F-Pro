==========================================================================
WO firmware layout
==========================================================================
   Jedi/
    |
warp_driver/
    |
    +-- wocpu/
        |
        +-- Makefile
	|
	+-- wo_main.c
	|       |-- wo_basic.h
	|
	+-- wo_pre_rro.c
	|       |-- wo_mid_mod.h 
	|
	+-- wo_ring.c
	|
	+-- wo_fwcmd.c
	|      |-- wo_fwcmd.h
	|
	+-- wo_fbcmd.c
	|      |-- wo_fbcmd.h
	|
	+-- rro.c
	       |-- rro.h

===========================================================================
Source File descriptions
===========================================================================
- Makefile:
	build system usage

- wo_main.c:
	kernel module entry point with init/exit, interrupt service routine registration

- wo_pre_rro.c:
	MID parsing and MOD formation, mid_mod_ring operations

- wo_ring.c:
	common ring operation APIs definition, for both fbcmd_ring and mid_mod_ring

- wo_fwcmd.c:
	formating warp to wo-cpu fw command and event, commands and events parsing 

- wo_fbcmd.c:
	formating fb_cmd, fbcmd_ring operations, mpdu_list manipulated

- rro.c:
	rx reordering algorithm

===========================================================================
Head File descriptions
===========================================================================
- wo_basic.h:
	basic header files, and some internal function identify

- wo_mid_mod.h:
	definition of MID and MOD HW format

- wo_fwcmd_.h:
	definition of warp to host command and event format

- wo_fbcmd.h:
	mpdu_list definition, feedback command definition

- rro.h:
	rx reordering related data structures, ba_ctrl, rx_blk
