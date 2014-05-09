#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#include "citaa.h"
#include "bsdqueue.h"

int
is_color(CHAR *color, struct rgb *rgb)
{
	if (strlen(color) != 4 || color[0] != 'c')
		return 0;

	if (strcmp(color, "cRED") == 0) {
		rgb->r = 0xE;
		rgb->g = 0x3;
		rgb->b = 0x2;
		return 1;
	}
	if (strcmp(color, "cBLU") == 0) {
		rgb->r = 0x5;
		rgb->g = 0x5;
		rgb->b = 0xB;
		return 1;
	}
	if (strcmp(color, "cGRE") == 0) {
		rgb->r = 0x9;
		rgb->g = 0xD;
		rgb->b = 0x9;
		return 1;
	}
	if (strcmp(color, "cPNK") == 0) {
		rgb->r = 0xF;
		rgb->g = 0xA;
		rgb->b = 0xA;
		return 1;
	}
	if (strcmp(color, "cBLK") == 0) {
		rgb->r = 0x0;
		rgb->g = 0x0;
		rgb->b = 0x0;
		return 1;
	}
	if (strcmp(color, "cYEL") == 0) {
		rgb->r = 0xF;
		rgb->g = 0xF;
		rgb->b = 0x3;
		return 1;
	}
	if (!isxdigit(color[1]))	return 0;
	if (!isxdigit(color[2]))	return 0;
	if (!isxdigit(color[3]))	return 0;

	if (isdigit(color[1]))
		rgb->r = color[1] - '0';
	else if (color[1] >= 'a')
		rgb->r = color[1] - 'a' + 10;
	else
		rgb->r = color[1] - 'A' + 10;

	if (isdigit(color[2]))
		rgb->g = color[2] - '0';
	else if (color[2] >= 'a')
		rgb->g = color[2] - 'a' + 10;
	else
		rgb->g = color[2] - 'A' + 10;

	if (isdigit(color[3]))
		rgb->b = color[3] - '0';
	else if (color[3] >= 'a')
		rgb->b = color[3] - 'a' + 10;
	else
		rgb->b = color[3] - 'A' + 10;

	return 1;
}

int
is_shape(CHAR *shape_name, int *shape)
{
	if (strcmp(shape_name, "{d}") == 0) {
		*shape = SHAPE_DOCUMENT;
		return 1;
	}
	if (strcmp(shape_name, "{s}") == 0) {
		*shape = SHAPE_STORAGE;
		return 1;
	}
	if (strcmp(shape_name, "{io}") == 0) {
		*shape = SHAPE_IO;
		return 1;
	}

	return 0;
}

void
extract_text(struct image *img)
{
	int y, x, sx;
	CHAR *buf, *s;
	CHAR *special_chars = "|:-=V^<>/\\+* ";
	struct rgb rgb;
	struct component *c;
	int shape;
	struct text *t;

	buf = malloc(sizeof(CHAR)*img->w);
	if (!buf)	croak(1, "extract_text:malloc(buf)");

	for (y = 0; y < img->h; y++) {
		for (x = 0; x < img->w; x++) {
			s = buf;
			sx = x;
			while (!strchr(special_chars, img->d[y][x]) && x < img->w) {
				*s++ = img->d[y][x++];
			}
			*s = '\0';
			if (s != buf) {
				printf("%d,%d: |%s|\n", y, sx, buf);
				c = find_enclosing_component(&components, y, sx);
				if (is_color(buf, &rgb) && c) {
					double percepted_luminance = 1 - ( 0.299 * rgb.r + 0.587 * rgb.g + 0.114 * rgb.b)/0xF;

					c->has_custom_background = 1;
					c->custom_background = rgb;
					if (percepted_luminance >= 0.5)
						c->white_text = 1;
					printf("COLOR %x%x%x\n", rgb.r, rgb.g, rgb.b);
				} else if (is_shape(buf, &shape) && c) {
					c->shape = shape;
				} else {
					t = create_text(y, sx, buf);
					TAILQ_INSERT_TAIL(c ? &c->text : &free_text, t, list);
				}
			}
		}
	}
	free(buf);
}

struct text_head free_text;

struct text *
create_text(int y, int x, CHAR *text)
{
	struct text *t = malloc(sizeof(struct text));
	if (!t) croak(1, "create_text:malloc(text)");

	t->y = y;
	t->x = x;
	t->len = strlen(text);

	t->t = malloc(sizeof(CHAR) * (t->len+1));
	if (!t->t) croak(1, "create_text:malloc(t->t)");
	strcpy(t->t, text);

	return t;
}

int
main(int argc, char **argv)
{
	struct image *orig, *status;
	struct component *c;
	int x, y;

	TAILQ_INIT(&free_text);

	orig = read_image(stdin);

	dump_image("original", orig);

	status = create_image(orig->h, orig->w, ST_EMPTY);
	TAILQ_INIT(&connected_components);
	seen = ST_SEEN;

	for (y = 0; y < orig->h; y++) {
		for (x = 0; x < orig->w; x++) {
			trace_component(orig, status, NULL, y, x);
		}
	}
	dump_image("status", status);

	TAILQ_INIT(&components);
	TAILQ_FOREACH(c, &connected_components, list) {
		compactify_component(c);
		extract_branches(c, &components);
		extract_loops(c, &components);
	}
	sort_components(&components);
	build_components_by_point(&components, orig->h, orig->w);
	determine_dashed_components(&components, orig);

	extract_text(orig);

	paint(orig->h, orig->w);

	return 0;
}
