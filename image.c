#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "citaa.h"

struct image *
copy_image(struct image *src)
{
	struct image *dst;
	int i;

	dst = malloc(sizeof(*dst));
	if (!dst) croak(1, "copy_image:malloc(dst)");

	dst->h = src->h;
	dst->w = src->w;
	dst->d = malloc(sizeof(CHAR *) * dst->h);
	if (!dst->d) croak(1, "copy_image:malloc(d)");

	for (i = 0; i < dst->h; i++) {
		CHAR *r = malloc(sizeof(CHAR)*(dst->w + 1));
		if (!r) croak(1, "copy_image:malloc(row %d, %d chars)", i, dst->w+1);
		memset(r, ' ', dst->w);
		memcpy(r, src->d[i], strlen(src->d[i]));
		r[dst->w] = 0;
		dst->d[i] = r;
	}

	return dst;
}

struct image *
create_image(int h, int w, CHAR init)
{
	struct image *dst;
	int i;

	dst = malloc(sizeof(*dst));
	if (!dst) croak(1, "create_image:malloc(dst)");

	dst->h = h;
	dst->w = w;
	dst->d = malloc(sizeof(CHAR *) * dst->h);
	if (!dst->d) croak(1, "create_image:malloc(d)");

	for (i = 0; i < dst->h; i++) {
		CHAR *r = malloc(sizeof(CHAR)*(dst->w + 1));
		if (!r) croak(1, "create_image:malloc(row %d, %d chars)", i, dst->w+1);
		memset(r, init, dst->w);
		r[dst->w] = 0;
		dst->d[i] = r;
	}

	return dst;
}

struct image* expand_image(struct image *src, int x, int y)
{
	struct image *dst;
	int i;

	dst = malloc(sizeof(*dst));
	if (!dst) croak(1, "expand_image:malloc(dst)");

	dst->h = src->h + y*2;
	dst->w = src->w + x*2;
	dst->d = malloc(sizeof(CHAR *) * dst->h);
	if (!dst->d) croak(1, "copy_image:malloc(d)");

	for (i = 0; i < dst->h; i++) {
		CHAR *r = malloc(sizeof(CHAR)*(dst->w + 1));
		if (!r) croak(1, "copy_image:malloc(row %d, %d chars)", i, dst->w+1);
		memset(r, ' ', dst->w);
		if (i >= y && i-y < src->h)
			memcpy(r+x, src->d[i-y], strlen(src->d[i-y]));
		r[dst->w] = 0;
		dst->d[i] = r;
	}

	return dst;
}
