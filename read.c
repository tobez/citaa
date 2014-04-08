#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "citaa.h"

struct image*
read_image(FILE *f)
{
	struct image *img;
	int ch = 100;
	CHAR buf[65536];

	img = malloc(sizeof(*img));
	if (!img) croak(1, "read_image:malloc(img)");

	img->h = 0;
	img->w = 0;
	img->d = malloc(sizeof(CHAR *) * ch);
	if (!img->d) croak(1, "read_image:malloc(d)");

	while (fgets(buf, 65535, f)) {
		int l;
		chomp(buf);

		l = strlen(buf);
		if (l > img->w) {
			img->w = l;
		}
		if (img->h >= ch) {
			ch *= 2;
			img->d = realloc(img->d, sizeof(CHAR *) * ch);
			if (!img->d) croak(1, "read_image:realloc(d 2x[%d])", ch);
		}
		img->d[img->h] = malloc(sizeof(CHAR)*(l+1));
		if (!img->d[img->h]) croak(1, "read_image:malloc(l %d, %d chars)", img->h, l+1);
		strcpy(img->d[img->h], buf);
		img->h++;
	}

	img->d = realloc(img->d, sizeof(CHAR *) * img->h);
	if (!img->d) croak(1, "read_image:realloc(d trunc %d)", img->h);
	for (int i = 0; i < img->h; i++) {
		CHAR *r = malloc(sizeof(CHAR)*(img->w + 1));
		if (!r) croak(1, "read_image:malloc(row %d, %d chars)", i, img->w+1);
		memset(r, ' ', img->w);
		memcpy(r, img->d[i], strlen(img->d[i]));
		r[img->w] = 0;
		free(img->d[i]);
		img->d[i] = r;
	}

	return img;
}

void
chomp(CHAR *s)
{
	while (s && *s) {
		if (*s == '\r' || *s == '\n')
			*s = 0;
		s++;
	}
}
