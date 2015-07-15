/*****************************************************************************

				WWIV Version 4
                    Copyright (C) 1988-1995 by Wayne Bell

Distribution of the source code for WWIV, in any form, modified or unmodified,
without PRIOR, WRITTEN APPROVAL by the author, is expressly prohibited.
Distribution of compiled versions of WWIV is limited to copies compiled BY
THE AUTHOR.  Distribution of any copies of WWIV not compiled by the author
is expressly prohibited.


*****************************************************************************/


#ifndef _SUBSXTR_H_
#define _SUBSXTR_H_


/*
 * Info for each network the sub is on.
 *  flags - bitmask
 *  net_num - index into networks.dat
 *  type - numeric sub type = atoi(stype)
 *  host - host system of sub, or 0 if locally hosted
 *  stype - string sub type (up to 7 chars)
 */
typedef struct {
  long flags;
  short net_num;
  unsigned short type;
  short host;
  short category;
  char stype[8];
} xtrasubsnetrec;

#define XTRA_NET_AUTO_ADDDROP 0x00000001    /* can auto add-drop the sub */
#define XTRA_NET_AUTO_INFO    0x00000002    /* sends subs.lst info for sub */


/*
 * Extended info for each sub, relating to network.
 *  flags - bitmask
 *  desc - long description, for auto subs.lst info
 *  num_nets - # records in "nets" field
 *  nets - pointer to network info for sub
 */
typedef struct {
  long flags;
  char desc[61];
  short num_nets;
  short num_nets_max;
  xtrasubsnetrec *nets;
} xtrasubsrec;

#define XTRA_MALLOCED         0x80000000    /* "nets" is malloced */

#define XTRA_MASK             (~(XTRA_MALLOCED))


#ifdef _DEFINE_GLOBALS_

xtrasubsrec huge *xsubs;

#else
#endif

extern xtrasubsrec huge *xsubs;

#endif

