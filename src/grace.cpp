/* Copyright (C) 2003 Massachusetts Institute of Technology
%
%  This program is free software; you can redistribute it and/or modify
%  it under the terms of the GNU General Public License as published by
%  the Free Software Foundation; either version 2, or (at your option)
%  any later version.
%
%  This program is distributed in the hope that it will be useful,
%  but WITHOUT ANY WARRANTY; without even the implied warranty of
%  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
%  GNU General Public License for more details.
%
%  You should have received a copy of the GNU General Public License
%  along with this program; if not, write to the Free Software Foundation,
%  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "meep.h"
#include "meep_internals.h"

/* Below are some routines to output to a grace file. */

const char grace_header[] = "# Grace project file\
#\
@page size 792, 612\
@default symbol size 0.330000\
@g0 on\
@with g0\
";

class grace_point {
public:
  int n;
  double x, y, dy, extra;
  grace_point *next;
};

grace::grace(const char *fname, const char *dirname) {
  fn = new char[strlen(fname)+1];
  strcpy(fn, fname);
  dn = new char[strlen(dirname)+1];
  strcpy(dn, dirname);
  char buf[300];
  snprintf(buf,300,"%s/%s", dirname, fname);
  if (!strcmp(fname+strlen(fname)-4,".eps") && !strcmp(dirname,".")) {
    snprintf(buf,300,"%s", fn);
    buf[strlen(buf)-4] = 0;
    fn[strlen(fn)-4] = 0;
  }
  f = everyone_open_write(buf);
  if (!f) abort("Unable to open file %s\n", buf);
  set_num = -1;
  sn = -1;
  pts = NULL;
  master_fprintf(f,"%s", grace_header);
}

grace::~grace() {
  flush_pts();
  everyone_close(f);
  char gracecmd[500];
  snprintf(gracecmd, 500, "gracebat -hdevice EPS -printfile %s/%s.eps -hardcopy %s/%s",
           dn, fn, dn, fn);
  if (my_rank() == 0) system(gracecmd);
  delete[] dn;
  delete[] fn;
  all_wait();
}

void grace::new_set(grace_type pt) {
  flush_pts();
  set_num++;
  sn++;
  master_fprintf(f, "@    s%d line color %d\n", sn, set_num+1);
  master_fprintf(f, "@    s%d symbol color %d\n", sn, set_num+1);
  master_fprintf(f, "@    s%d errorbar color %d\n", sn, set_num+1);
  master_fprintf(f, "@    target G0.S%d\n", sn);
  if (pt == ERROR_BARS) master_fprintf(f, "@    type xydy\n");
  else master_fprintf(f, "@    type xy\n");
}

void grace::set_range(double xmin, double xmax, double ymin, double ymax) {
  master_fprintf(f, "@ version 1\n"); // Stupid nasty hack to make grace recognize the range.
  master_fprintf(f, "@    world xmin %lg\n", xmin);
  master_fprintf(f, "@    world xmax %lg\n", xmax);
  master_fprintf(f, "@    world ymin %lg\n", ymin);
  master_fprintf(f, "@    world ymax %lg\n", ymax);
  master_fprintf(f, "@    view xmin 0.15\n");
  master_fprintf(f, "@    view xmax 0.95\n");
  master_fprintf(f, "@    view ymin 0.15\n");
  master_fprintf(f, "@    view ymax 0.85\n");
}

void grace::set_legend(const char *l) {
  master_fprintf(f, "@    s%d legend  \"%s\"\n", sn, l);
}

void grace::new_curve() {
  if (set_num == -1) new_set();
  else sn++;
  master_fprintf(f, "@    s%d line color %d\n", sn, set_num+1);
  master_fprintf(f, "@    s%d symbol color %d\n", sn, set_num+1);
  master_fprintf(f, "@    s%d errorbar color %d\n", sn, set_num+1);
  master_fprintf(f, "\n");
}

void grace::output_point(double x, double y, double dy, double extra) {
  if (dy >= 0 && extra != -1) {
    master_fprintf(f, "%lg\t%lg\t%lg\t%lg\n", x, y, dy, extra);
  } else if (dy >= 0) {
    master_fprintf(f, "%lg\t%lg\t%lg\n", x, y, dy);
  } else {
    master_fprintf(f, "%lg\t%lg\n", x, y);
  }
}

void grace::output_out_of_order(int n, double x, double y, double dy, double extra) {
  if (set_num == -1) new_set();
  grace_point *gp = new grace_point;
  gp->n = n;
  gp->x = x;
  gp->y = y;
  gp->dy = dy;
  gp->extra = extra;
  gp->next = pts;
  pts = gp;
}

void grace::flush_pts() {
  int first_time = 1;
  while (pts) {
    grace_point *p = pts;
    int num_seen = 0;
    while (p) {
      if (p->n <= 0) num_seen++;
      p = p->next;
    }
    if (num_seen && !first_time) new_curve();
    first_time = 0;
    p = pts;
    grace_point **last = &pts;
    while (p) {
      if (p->n <= 0) {
        *last = p->next;
        output_point(p->x,p->y,p->dy,p->extra);
        delete p;
        p = *last;
      } else {
        p->n -= 1;
        last = &p->next;
        p = p->next;
      }
    }
  }
}

