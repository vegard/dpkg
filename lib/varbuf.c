/*
 * libdpkg - Debian packaging suite library routines
 * varbuf.c - variable length expandable buffer handling
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

#include <stdlib.h>

#include "config.h"
#include "dpkg.h"
#include "dpkg-db.h"

void varbufaddc(struct varbuf *v, int c) {
  if (v->used >= v->size) varbufextend(v);
  v->buf[v->used++]= c;
}

void varbufaddstr(struct varbuf *v, const char *s) {
  int l, ou;
  l= strlen(s);
  ou= v->used;
  v->used += l;
  if (v->used >= v->size) varbufextend(v);
  memcpy(v->buf + ou, s, l);
}

void varbufinit(struct varbuf *v) {
  /* NB: dbmodify.c does its own init to get a big buffer */
  v->size= v->used= 0;
  v->buf= 0;
}

void varbufreset(struct varbuf *v) {
  v->used= 0;
}

void varbufextend(struct varbuf *v) {
  int newsize;
  char *newbuf;

  newsize= v->size + 80 + v->used;
  newbuf= realloc(v->buf,newsize);
  if (!newbuf) ohshite("failed to realloc for variable buffer");
  v->size= newsize;
  v->buf= newbuf;
}

void varbuffree(struct varbuf *v) {
  free(v->buf); v->buf=0; v->size=0; v->used=0;
}
