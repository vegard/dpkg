/*
 * libdpkg - Debian packaging suite library routines
 * dump.c - code to write in-core database to a file
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

/* fixme: don't write uninteresting packages */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>

#include "config.h"
#include "dpkg.h"
#include "dpkg-db.h"
#include "parsedump.h"

void w_name(struct varbuf *vb,
            const struct pkginfo *pigp, const struct pkginfoperfile *pifp,
            const struct fieldinfo *fip) {
  assert(pigp->name);
  varbufaddstr(vb,"Package: "); varbufaddstr(vb, pigp->name);
  varbufaddc(vb,'\n');
}

void w_version(struct varbuf *vb,
               const struct pkginfo *pigp, const struct pkginfoperfile *pifp,
               const struct fieldinfo *fip) {
  /* Revision information is printed in version field too. */
  if ((!pifp->version || !*pifp->version) &&
      (!pifp->revision || !*pifp->revision)) return;
  varbufaddstr(vb,"Version: ");
  varbufaddstr(vb,pifp->version ? pifp->version : "");
  if (pifp->revision && *pifp->revision) {
    varbufaddc(vb,'-');
    varbufaddstr(vb,pifp->revision);
  }
  varbufaddc(vb,'\n');
}

void w_configversion(struct varbuf *vb,
                     const struct pkginfo *pigp, const struct pkginfoperfile *pifp,
                     const struct fieldinfo *fip) {
  if ((!pigp->configversion || !*pigp->configversion) &&
      (!pigp->configrevision || !*pigp->configrevision)) return;
  if (pifp != &pigp->installed) return;
  if (pigp->status == stat_installed || pigp->status == stat_notinstalled) return;
  varbufaddstr(vb,"Config-Version: ");
  varbufaddstr(vb,pigp->configversion ? pigp->configversion : "");
  if (pigp->configrevision && *pigp->configrevision) {
    varbufaddc(vb,'-');
    varbufaddstr(vb,pigp->configrevision);
  }
  varbufaddc(vb,'\n');
}

void w_null(struct varbuf *vb,
            const struct pkginfo *pigp, const struct pkginfoperfile *pifp,
            const struct fieldinfo *fip) {
}

void w_section(struct varbuf *vb,
               const struct pkginfo *pigp, const struct pkginfoperfile *pifp,
               const struct fieldinfo *fip) {
  const char *value= pigp->section;
  if (!value || !*value) return;
  varbufaddstr(vb,"Section: ");
  varbufaddstr(vb,value);
  varbufaddc(vb,'\n');
}

void w_charfield(struct varbuf *vb,
                 const struct pkginfo *pigp, const struct pkginfoperfile *pifp,
                 const struct fieldinfo *fip) {
  const char *value= pifp->valid ? PKGPFIELD(pifp,fip->integer,char*) : 0;
  if (!value || !*value) return;
  varbufaddstr(vb,fip->name); varbufaddstr(vb, ": "); varbufaddstr(vb,value);
  varbufaddc(vb,'\n');
}

void w_filecharf(struct varbuf *vb,
                 const struct pkginfo *pigp, const struct pkginfoperfile *pifp,
                 const struct fieldinfo *fip) {
  struct filedetails *fdp;
  
  if (pifp != &pigp->available) return;
  fdp= pigp->files;
  if (!fdp || !FILEFFIELD(fdp,fip->integer,char*)) return;
  varbufaddstr(vb,fip->name); varbufaddc(vb,':');
  while (fdp) {
    varbufaddc(vb,' ');
    varbufaddstr(vb,FILEFFIELD(fdp,fip->integer,char*));
    fdp= fdp->next;
  }
  varbufaddc(vb,'\n');
}

void w_booleandefno(struct varbuf *vb,
                    const struct pkginfo *pigp, const struct pkginfoperfile *pifp,
                    const struct fieldinfo *fip) {
  int value= pifp->valid ? PKGPFIELD(pifp,fip->integer,int) : 0;
  if (!value) return;
  assert(value==1);
  varbufaddstr(vb,"Essential: yes\n");
}

void w_priority(struct varbuf *vb,
                const struct pkginfo *pigp, const struct pkginfoperfile *pifp,
                const struct fieldinfo *fip) {
  if (pigp->priority == pri_unknown) return;
  assert(pigp->priority <= pri_unknown);
  varbufaddstr(vb,"Priority: ");
  varbufaddstr(vb,
               pigp->priority == pri_other
               ? pigp->otherpriority
               : priorityinfos[pigp->priority].name);
  varbufaddc(vb,'\n');
}

void w_status(struct varbuf *vb,
              const struct pkginfo *pigp, const struct pkginfoperfile *pifp,
              const struct fieldinfo *fip) {
  if (pifp != &pigp->installed) return;
  assert(pigp->want <= want_purge &&
         pigp->eflag <= eflagv_both &&
         pigp->status <= stat_configfiles);
  varbufaddstr(vb,"Status: ");
  varbufaddstr(vb,wantinfos[pigp->want].name); varbufaddc(vb,' ');
  varbufaddstr(vb,eflaginfos[pigp->eflag].name); varbufaddc(vb,' ');
  varbufaddstr(vb,statusinfos[pigp->status].name); varbufaddc(vb,'\n');
}

void varbufdependency(struct varbuf *vb, struct dependency *dep) {
  struct deppossi *dop;
  const char *possdel;

  possdel= "";
  for (dop= dep->list; dop; dop= dop->next) {
    assert(dop->up == dep);
    varbufaddstr(vb,possdel); possdel= " | ";
    varbufaddstr(vb,dop->ed->name);
    if (dop->verrel != dvr_none) {
      varbufaddstr(vb," (");
      switch (dop->verrel) {
      case dvr_exact: varbufaddc(vb,'='); break;
      case dvr_laterequal: varbufaddstr(vb,">="); break;
      case dvr_earlierequal: varbufaddstr(vb,"<="); break;
      case dvr_laterstrict: varbufaddstr(vb,">>"); break;
      case dvr_earlierstrict: varbufaddstr(vb,"<<"); break;
      default: internerr("unknown verrel");
      }
      if (!isalnum(dop->version[0])) varbufaddc(vb,' ');
      varbufaddstr(vb,dop->version);
      if (dop->revision) { varbufaddc(vb,'-'); varbufaddstr(vb,dop->revision); }
      varbufaddc(vb,')');
    }
  }
}

void w_dependency(struct varbuf *vb,
                  const struct pkginfo *pigp, const struct pkginfoperfile *pifp,
                  const struct fieldinfo *fip) {
  char fnbuf[50];
  const char *depdel;
  struct dependency *dyp;

  if (!pifp->valid) return;
  sprintf(fnbuf,"%s: ",fip->name); depdel= fnbuf;
  for (dyp= pifp->depends; dyp; dyp= dyp->next) {
    if (dyp->type != fip->integer) continue;
    assert(dyp->up == pigp);
    varbufaddstr(vb,depdel); depdel= ", ";
    varbufdependency(vb,dyp);
  }
  if (depdel != fnbuf) varbufaddc(vb,'\n');
}

void w_conffiles(struct varbuf *vb,
                 const struct pkginfo *pigp, const struct pkginfoperfile *pifp,
                 const struct fieldinfo *fip) {
  struct conffile *i;

  if (!pifp->valid || !pifp->conffiles || pifp == &pigp->available) return;
  varbufaddstr(vb,"Conffiles:\n");
  for (i=pifp->conffiles; i; i= i->next) {
    varbufaddc(vb,' '); varbufaddstr(vb,i->name); varbufaddc(vb,' ');
    varbufaddstr(vb,i->hash); varbufaddc(vb,'\n');
  }
}

void varbufrecord(struct varbuf *vb,
                  const struct pkginfo *pigp, const struct pkginfoperfile *pifp) {
  const struct fieldinfo *fip;
  const struct arbitraryfield *afp;

  for (fip= fieldinfos; fip->name; fip++) {
    fip->wcall(vb,pigp,pifp,fip);
  }
  if (pifp->valid) {
    for (afp= pifp->arbs; afp; afp= afp->next) {
      varbufaddstr(vb,afp->name); varbufaddstr(vb,": ");
      varbufaddstr(vb,afp->value); varbufaddc(vb,'\n');
    }
  }
}

void writerecord(FILE *file, const char *filename,
                 const struct pkginfo *pigp, const struct pkginfoperfile *pifp) {
  struct varbuf vb;

  varbufinit(&vb);
  varbufrecord(&vb,pigp,pifp);
  varbufaddc(&vb,'\0');
  if (!fputs(vb.buf,file))
    ohshite("failed to write details of `%.50s' to `%.250s'", pigp->name, filename);
}

void writedb(const char *filename, int available, int mustsync) {
  static char writebuf[8192];
  
  struct pkgiterator *it;
  struct pkginfo *pigp;
  char *oldfn, *newfn;
  const char *which;
  FILE *file;
  struct varbuf vb;

  which= available ? "available" : "status";
  oldfn= m_malloc(strlen(filename)+sizeof(OLDDBEXT));
  strcpy(oldfn,filename); strcat(oldfn,OLDDBEXT);
  newfn= m_malloc(strlen(filename)+sizeof(NEWDBEXT));
  strcpy(newfn,filename); strcat(newfn,NEWDBEXT);
  varbufinit(&vb);

  file= fopen(newfn,"w");
  if (!file) ohshite("failed to open `%s' for writing %s information",filename,which);
  
  if (setvbuf(file,writebuf,_IOFBF,sizeof(writebuf)))
    ohshite("unable to set buffering on status file");

  it= iterpkgstart();
  while ((pigp= iterpkgnext(it)) != 0) {
    if (!(informative(pigp) ||
          informativeperfile(&pigp->available) ||
          informativeperfile(&pigp->installed)))
      /* Don't dump records which have no useful content. */
      continue;
    varbufrecord(&vb, pigp, available ? &pigp->available : &pigp->installed);
    varbufaddc(&vb,'\n'); varbufaddc(&vb,0);
    if (!fputs(vb.buf,file))
      ohshite("failed to write %s record about `%.50s' to `%.250s'",
              which, pigp->name, filename);
    varbufreset(&vb);      
  }
  varbuffree(&vb);
  if (mustsync) {
    if (fflush(file))
      ohshite("failed to flush %s information to `%.250s'", which, filename);
    if (fsync(fileno(file)))
      ohshite("failed to fsync %s information to `%.250s'", which, filename);
  }
  if (fclose(file)) ohshite("failed to close `%.250s' after writing %s information",
                            filename, which);
  unlink(oldfn);
  if (link(filename,oldfn) && errno != ENOENT)
    ohshite("failed to link `%.250s' to `%.250s' for backup of %s info",
            filename, oldfn, which);
  if (rename(newfn,filename))
    ohshite("failed to install `%.250s' as `%.250s' containing %s info",
            newfn, filename, which);
}
