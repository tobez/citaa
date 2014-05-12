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
extend_text(struct text *t, int n_spaces, CHAR *suffix)
{
	CHAR *new_t;

	new_t = malloc(sizeof(CHAR) * (t->len + 1 + n_spaces + strlen(suffix)));
	if (!new_t) croak(1, "extend_text:malloc(new_t)");

	strcpy(new_t, t->t);
	while (n_spaces--) strcat(new_t, " ");
	strcat(new_t, suffix);
	free(t->t);
	t->t = new_t;
	t->len = strlen(new_t);
}

void
extract_text(struct image *img)
{
	int y, x, sx;
	CHAR *buf, *s;
	struct rgb rgb;
	struct component *c, *last_c = NULL;
	int shape;
	struct text *t, *last_t = NULL;

	buf = malloc(sizeof(CHAR)*img->w);
	if (!buf)	croak(1, "extract_text:malloc(buf)");

	for (y = 0; y < img->h; y++) {
		last_c = NULL;
		last_t = NULL;
		for (x = 0; x < img->w; x++) {
			s = buf;
			sx = x;

			while (img->d[y][x] != ' ' && component_marks->d[y][x] == ' ') {
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
					if (last_t && last_c == c &&
						sx > 0 && img->d[y][sx-1] == ' ' &&
						last_t->x + last_t->len + 1 == sx)
					{
						extend_text(last_t, 1, buf);
					} else {
						t = create_text(y, sx, buf);
						TAILQ_INSERT_TAIL(c ? &c->text : &free_text, t, list);
						last_c = c;
						last_t = t;
					}
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

