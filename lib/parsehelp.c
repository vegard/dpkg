/*
 * libdpkg - Debian packaging suite library routines
 * parsehelp.c - helpful routines for parsing and writing
 *
 * Copyright (C) 1995 Ian Jackson <iwj10@cus.cam.ac.uk>
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

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "config.h"
#include "dpkg.h"
#include "dpkg-db.h"
#include "parsedump.h"

void parseerr(FILE *file, const char *filename, int lno, FILE *warnto, int *warncount,
              const struct pkginfo *pigp, int warnonly,
              const char *fmt, ...) {
  va_list al;
  char buf1[768], buf2[1000], *p, *q;
  if (file && ferror(file)) ohshite("failed to read `%s' at line %d",filename,lno);
  sprintf(buf1, "%s, in file `%.255s' near line %d",
          warnonly ? "warning" : "parse error", filename, lno);
  if (pigp && pigp->name) {
    sprintf(buf2, " package `%.255s'", pigp->name);
    strcat(buf1,buf2);
  }
  for (p=buf1, q=buf2; *p; *q++= *p++) if (*p == '%') *q++= '%';
  strcpy(q,":\n "); strcat(q,fmt);
  va_start(al,fmt);
  if (!warnonly) ohshitv(buf2,al);
  if (warncount) (*warncount)++;
  if (warnto) {
    strcat(q,"\n");
    if (vfprintf(warnto,buf2,al) == EOF)
      ohshite("failed to write parsing warning");
  }
  va_end(al);
}

const struct namevalue booleaninfos[]= {  /* Note !  These must be in order ! */
  { "no",                             0                  },
  { "yes",                            1                  },
  {  0                                                   }
};

const struct namevalue priorityinfos[]= {  /* Note !  These must be in order ! */
  { "required",                       pri_required     },
  { "important",                      pri_important    },
  { "standard",                       pri_standard     },
  { "recommended",                    pri_recommended  }, /* fixme: obsolete */
  { "optional",                       pri_optional     },
  { "extra",                          pri_extra        },
  { "contrib",                        pri_contrib      }, /* fixme: keep? */
  { "this is a bug - please report",  pri_other        },
  { "unknown",                        pri_unknown      },
  
  { "base",                           pri_required     }, /* fixme: alias, remove */
  {  0                                                 }
};

const struct namevalue statusinfos[]= {  /* Note !  These must be in order ! */
  { "not-installed",   stat_notinstalled    },
  { "unpacked",        stat_unpacked        },
  { "half-configured", stat_halfconfigured  },
  { "installed",       stat_installed       },
  { "half-installed",  stat_halfinstalled   },
  { "config-files",    stat_configfiles     },
  /* These are additional entries for reading only, in any order ... */
  { "postinst-failed", stat_halfconfigured  }, /* fixme: backwards compat., remove */
  { "removal-failed",  stat_halfinstalled   }, /* fixme: backwards compat., remove */
  {  0                                      }
};

const struct namevalue eflaginfos[]= {  /* Note !  These must be in order ! */
  { "ok",             eflagv_ok         },
  { "hold",           eflagv_hold       },
  { "reinstreq",      eflagv_reinstreq  },
  { "hold-reinstreq", eflagv_both       },
  {  0                                  }
};

const struct namevalue wantinfos[]= {  /* Note !  These must be in order ! */
  { "unknown",   want_unknown    },
  { "install",   want_install    },
  { "deinstall", want_deinstall  },
  { "purge",     want_purge      },
  {  0                           }
};

const char *illegal_packagename(const char *p, const char **ep) {
  static const char alsoallowed[]= "-_+.@:=%";
  static char buf[150];
  int c;
  
  if (!*p) return "may not be empty string";
  if (!isalnum(*p)) return "must start with an alphanumeric";
  if (!*++p) return "must be at least two characters";
  while ((c= *p++)!=0)
    if (!isalnum(c) && !strchr(alsoallowed,c)) break;
  if (!c) return 0;
  if (isspace(c) && ep) {
    while (isspace(*p)) p++;
    *ep= p; return 0;
  }
  sprintf(buf,
          "character `%c' not allowed - only letters, digits and %s allowed",
          c, alsoallowed);
  return buf;
}

const struct nickname nicknames[]= {
  /* NB: capitalisation of these strings is important. */
  { "Recommended",       "Recommends"  },
  { "Optional",          "Suggests"    },
  { "Class",             "Priority"    },
  { "Package-Revision",  "Revision"    },
  { "Package_Revision",  "Revision"    },
  {  0                                 }
};

void parsemustfield(FILE *file, const char *filename, int lno,
                    FILE *warnto, int *warncount,
                    const struct pkginfo *pigp, int warnonly,
                    char **value, const char *what) {
  if (!*value) {
    parseerr(file,filename,lno, warnto,warncount,pigp,warnonly, "missing %s",what);
    *value= nfstrsave("");
  } else if (!**value) {
    parseerr(file,filename,lno, warnto,warncount,pigp,warnonly,
             "empty value for %s",what);
  }
}

const char *skip_slash_dotslash(const char *p) {
  while (p[0] == '/' || (p[0] == '.' && p[1] == '/')) p++;
  return p;
}
