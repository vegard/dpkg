/* -*- c++ -*-
 * dselect - selection of Debian packages
 * pkglist.h - external definitions for package list handling
 *
 * Copyright (C) 1994,1995 Ian Jackson <iwj10@cus.cam.ac.uk>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with dpkg; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef PKGLIST_H
#define PKGLIST_H

enum showpriority {
  dp_none,      // has not been involved in any unsatisfied things
  dp_may,       // has been involved in an unsatisfied Optional
  dp_must       // has been involved in an unsatisfied Recommended/Depends/Conflicts
};

enum selpriority {
  // where did the currently suggested value come from, and how important
  //  is it to display this package ?
  // low
  sp_inherit,     // inherited from our parent list
  sp_selecting,   // propagating a selection
  sp_deselecting, // propagating a deselection
  sp_fixed        // it came from the `status' file and we're not a recursive list
  // high
};

struct perpackagestate {
  struct pkginfo *pkg;
  /* The `heading' entries in the list, for `all packages of type foo',
   * point to a made-up pkginfo, which has pkg->name==0.
   * pkg->priority and pkg->section are set to the values if appropriate, or to
   * pri_unset resp. null if the heading refers to all priorities resp. sections.
   * uprec is used when constructing the list initially and when tearing it
   * down and should not otherwise be used; other fields are undefined.
   */
  pkginfo::pkgwant original;         // set by caller
  pkginfo::pkgwant direct;           // set by caller
  pkginfo::pkgwant suggested;        // set by caller, modified by resolvesuggest
  pkginfo::pkgwant selected;         // not set by caller, will be set by packagelist
  selpriority spriority;             // monotonically increases (used by sublists)
  showpriority dpriority;            // monotonically increases (used by sublists)
  struct perpackagestate *uprec;     // 0 if this is not part of a recursive list
  varbuf relations;

  void free(int recursive);
};

class packagelist : public baselist {
  int status_width, gap_width, section_width, priority_width;
  int package_width, description_width;
  int section_column, priority_column, package_column, description_column;

  // Only used when `verbose' is set
  int status_hold_width, status_status_width, status_want_width;

  // Table of packages
  struct perpackagestate *datatable;
  struct perpackagestate **table;

  // Misc.
  int recursive, nallocated, verbose;
  enum { so_unsorted, so_section, so_priority, so_alpha } sortorder;
  struct perpackagestate *headings;

  // Information displays
  struct infotype {
    int (packagelist::*relevant)(); // null means always relevant
    void (packagelist::*display)(); // null means end of table
  };
  const infotype *currentinfo;
  static const infotype infoinfos[];
  static const infotype *const baseinfo;
  int itr_recursive();
  int itr_nonrecursive();
  void severalinfoblurb(const char *whatinfoline);
  void itd_mainwelcome();
  void itd_explaindisplay();
  void itd_recurwelcome();
  void itd_relations();
  void itd_description();
  void itd_controlfile();
  
  // Dependency and sublist processing
  struct doneent { doneent *next; void *dep; } *depsdone, *unavdone;
  int alreadydone(doneent**, void*);
  int resolvedepcon(dependency*);
  int deselect_one_of(pkginfo *er, pkginfo *ed, dependency *display);
  
  // Define these virtuals
  void redraw1itemsel(int index, int selected);
  void redrawcolheads();
  void redrawthisstate();
  void redrawinfo();
  void redrawtitle();
  void setwidths();
  const char *itemname(int index);
  const struct helpmenuentry *helpmenulist();

  // Miscellaneous internal routines
  
  void redraw1package(int index, int selected);
  int compareentries(struct perpackagestate *a, struct perpackagestate *b);
  friend int qsort_compareentries(const void *a, const void *b);

  void sortmakeheads();
  void movecursorafter(int ncursor);
  void initialsetup();
  void finalsetup();

  // To do with building the list, with heading lines in it
  void discardheadings();
  void addheading(pkginfo::pkgpriority,const char*, const char *section);
  void sortinplace();
  void affectedrange(int *startp, int *endp);
  void setwant(pkginfo::pkgwant nw);
  void sethold(int hold);
  
 public:

  // Keybinding functions */
  void kd_quit_noop();
  void kd_revert_abort();
  void kd_revertsuggest();
  void kd_revertdirect();  
  void kd_morespecific();
  void kd_lessspecific();
  void kd_swaporder();
  void kd_select();
  void kd_deselect();
  void kd_purge();
  void kd_hold();
  void kd_unhold();
  void kd_info();
  void kd_verbose();
  
  packagelist(keybindings *kb); // nonrecursive
  packagelist(keybindings *kb, pkginfo **pkgltab); // recursive
  void add(pkginfo **arry) { while (*arry) add(*arry++); }
  void add(pkginfo*);
  void add(pkginfo*, pkginfo::pkgwant);
  void add(pkginfo*, const char *extrainfo, showpriority displayimportance);
  int add(dependency*, showpriority displayimportance);
  void addunavailable(deppossi*);

  int resolvesuggest();
  int deletelessimp_anyleft(showpriority than);
  pkginfo **display();
  ~packagelist();
};

void repeatedlydisplay(packagelist *sub, showpriority, packagelist *unredisplay =0);

extern const char *const wantstrings[];
extern const char *const holdstrings[];
extern const char *const statusstrings[];
extern const char *const prioritystrings[];
extern const char *const priorityabbrevs[];
extern const char *const relatestrings[];
extern const char statuschars[];
extern const char holdchars[];
extern const char wantchars[];

const struct pkginfoperfile *i2info(struct pkginfo *pkg);
int deppossatisfied(deppossi *possi);

extern modstatdb_rw readwrite;

#endif /* PKGLIST_H */
