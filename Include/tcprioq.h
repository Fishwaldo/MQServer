/**
    Copyright (C) 2003  Michael Ahlberg, Måns Rullgård

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use, copy,
    modify, merge, publish, distribute, sublicense, and/or sell copies
    of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
**/

#ifndef _TCPRIOQ_H
#define _TCPRIOQ_H

#if 0
#include <tc.h>
#include <tctypes.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*tccompare_fn) (const void *, const void *);

typedef struct tcprioq tcprioq_t;

extern tcprioq_t *tcprioq_new(int size, int lock, tccompare_fn cmp);
extern int tcprioq_add(tcprioq_t *pq, void *data);
extern int tcprioq_get(tcprioq_t *pq, void **ret);
extern int tcprioq_items(tcprioq_t *pq);
extern void tcprioq_free(tcprioq_t *pq);

#ifdef __cplusplus
}
#endif

#endif
