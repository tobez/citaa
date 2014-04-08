#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "citaa.h"
#include "bsdqueue.h"

#define ST_EMPTY ' '
#define ST_SEEN '0'

#define CT_UNKNOWN 0
#define CT_LINE    1
#define CT_BOX     2

struct component_head components;
CHAR seen;

void
dump_component(struct component *c)
{
	struct vertex *v;

	printf("components type %d, dashed %d\n", c->type, c->dashed);
	TAILQ_FOREACH(v, &c->vertices, list) {
		dump_vertex(v);
	}
}

struct component *
maybe_create_component(struct component *c)
{
	if (c) return c;

	c = malloc(sizeof(struct component));
	if (!c) croak(1, "maybe_create_component:malloc(component)");

	c->type = CT_UNKNOWN;
	c->dashed = 0;
	TAILQ_INIT(&c->vertices);

	TAILQ_INSERT_TAIL(&components, c, list);
	return c;
}

void trace_component(struct image *img, struct image *status, struct component *c, int y, int x);
void trace_from_left(struct image *img, struct image *status, struct component *c, struct vertex *v, int y, int x);
void trace_from_right(struct image *img, struct image *status, struct component *c, struct vertex *v, int y, int x);
void trace_from_bottom(struct image *img, struct image *status, struct component *c, struct vertex *v, int y, int x);
void trace_from_top(struct image *img, struct image *status, struct component *c, struct vertex *v, int y, int x);
void trace_maybe_loop(CHAR ch, struct component *c, struct vertex *v, int y, int x, CHAR *loop_chars);

void
trace_maybe_loop(CHAR ch, struct component *c, struct vertex *v, int y, int x, CHAR *loop_chars)
{
	struct vertex *u;

	if (!strchr(loop_chars, ch)) return;

	u = find_vertex_in_component(c, y, x);
	if (!u) return;

	maybe_connect_vertices(v, u);
}

/* To the BOTTOM there can be:
 *   '|'  - continuation of the trace
 *   ':'  - continuation of the trace, turn on dashes
 *   'V'  - arrow end
 *   '/'  - turn towards left
 *   '\'  - turn towards right
 *   '+'  - join to any direction
 *   '*'  - join to any direction
 *   Anything else terminates the trace in this direction
 */
void
trace_from_top(struct image *img, struct image *status, struct component *c, struct vertex *v, int y, int x)
{
	struct vertex *u;
	CHAR ch;

	if (x < 0 || x >= img->w)          return;
	if (y < 0 || y >= img->h)          return;

	ch = img->d[y][x];
	if (status->d[y][x] != ST_EMPTY)   return trace_maybe_loop(ch, c, v, y, x, "|:V/\\+*");

	switch (ch) {
	case '|':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("V %c (%d,%d)\n", ch, y, x);
		trace_from_top(img, status, c, u, y+1, x);
		break;
	case ':':
		status->d[y][x] = seen;
		c->dashed = 1;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("V %c (%d,%d)\n", ch, y, x);
		trace_from_top(img, status, c, u, y+1, x);
		break;
	case 'V':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("V %c (%d,%d) arrow end\n", ch, y, x);
		c->type = CT_LINE;
		break;
	case '/':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("V %c (%d,%d) turn towards left\n", ch, y, x);
		trace_from_right(img, status, c, u, y, x-1);
		break;
	case '\\':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("V %c (%d,%d) turn towards right\n", ch, y, x);
		trace_from_left(img, status, c, u, y, x+1);
		break;
	case '+':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("V %c (%d,%d) possible join point in V><\n", ch, y, x);
		trace_from_top(img, status, c, u, y+1, x);
		trace_from_right(img, status, c, u, y, x-1);
		trace_from_left(img, status, c, u, y, x+1);
		break;
	case '*':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("V %c (%d,%d) possible join point in V><\n", ch, y, x);
		trace_from_top(img, status, c, u, y+1, x);
		trace_from_right(img, status, c, u, y, x-1);
		trace_from_left(img, status, c, u, y, x+1);
		break;
	default:
		break;
	}
}

/* To the TOP there can be:
 *   '|'  - continuation of the trace
 *   ':'  - continuation of the trace, turn on dashes
 *   '^'  - arrow end
 *   '/'  - turn towards right
 *   '\'  - turn towards left
 *   '+'  - join to any direction
 *   '*'  - join to any direction
 *   Anything else terminates the trace in this direction
 */
void
trace_from_bottom(struct image *img, struct image *status, struct component *c, struct vertex *v, int y, int x)
{
	struct vertex *u;
	CHAR ch;

	if (x < 0 || x >= img->w)          return;
	if (y < 0 || y >= img->h)          return;

	ch = img->d[y][x];
	if (status->d[y][x] != ST_EMPTY)   return trace_maybe_loop(ch, c, v, y, x, "|:^/\\+*");

	switch (ch) {
	case '|':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("^ %c (%d,%d)\n", ch, y, x);
		trace_from_bottom(img, status, c, u, y-1, x);
		break;
	case ':':
		status->d[y][x] = seen;
		c->dashed = 1;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("^ %c (%d,%d)\n", ch, y, x);
		trace_from_bottom(img, status, c, u, y-1, x);
		break;
	case '^':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("^ %c (%d,%d) arrow end\n", ch, y, x);
		c->type = CT_LINE;
		break;
	case '/':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("^ %c (%d,%d) turn towards right\n", ch, y, x);
		trace_from_left(img, status, c, u, y, x+1);
		break;
	case '\\':
		// TODO
		break;
	case '+':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("^ %c (%d,%d) possible join point in ^><\n", ch, y, x);
		trace_from_bottom(img, status, c, u, y-1, x);
		trace_from_left(img, status, c, u, y, x+1);
		trace_from_right(img, status, c, u, y, x-1);
		break;
	case '*':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("^ %c (%d,%d) possible join point in ^><\n", ch, y, x);
		trace_from_bottom(img, status, c, u, y-1, x);
		trace_from_left(img, status, c, u, y, x+1);
		trace_from_right(img, status, c, u, y, x-1);
		break;
	default:
		break;
	}
}

/* To the RIGHT there can be:
 *   '-'  - continuation of the trace
 *   '='  - continuation of the trace, turn on dashes
 *   '>'  - arrow end
 *   '/'  - turn towards top
 *   '\'  - turn towards bottom
 *   '+'  - join to any direction
 *   '*'  - join to any direction
 *   Anything else terminates the trace in this direction
 */
void
trace_from_left(struct image *img, struct image *status, struct component *c, struct vertex *v, int y, int x)
{
	struct vertex *u;
	CHAR ch;

	if (x < 0 || x >= img->w)          return;
	if (y < 0 || y >= img->h)          return;

	ch = img->d[y][x];
	if (status->d[y][x] != ST_EMPTY)   return trace_maybe_loop(ch, c, v, y, x, "-=>/\\+*");

	switch (ch) {
	case '-':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("> %c (%d,%d)\n", ch, y, x);
		trace_from_left(img, status, c, u, y, x+1);
		break;
	case '=':
		status->d[y][x] = seen;
		c->dashed = 1;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("> %c (%d,%d)\n", ch, y, x);
		trace_from_left(img, status, c, u, y, x+1);
		break;
	case '>':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("> %c (%d,%d) arrow end\n", ch, y, x);
		c->type = CT_LINE;
		break;
	case '/':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("> %c (%d,%d) turn towards top\n", ch, y, x);
		trace_from_bottom(img, status, c, u, y-1, x);
		break;
	case '\\':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("> %c (%d,%d) turn towards bottom\n", ch, y, x);
		trace_from_top(img, status, c, u, y+1, x);
		break;
	case '+':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("> %c (%d,%d) possible join point in >^V\n", ch, y, x);
		trace_from_left(img, status, c, u, y, x+1);
		trace_from_bottom(img, status, c, u, y-1, x);
		trace_from_top(img, status, c, u, y+1, x);
		break;
	case '*':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("> %c (%d,%d) possible join point in >^V\n", ch, y, x);
		trace_from_left(img, status, c, u, y, x+1);
		trace_from_bottom(img, status, c, u, y-1, x);
		trace_from_top(img, status, c, u, y+1, x);
		break;
	default:
		break;
	}
}

/* To the LEFT there can be:
 *   '-'  - continuation of the trace
 *   '='  - continuation of the trace, turn on dashes
 *   '<'  - arrow end
 *   '/'  - turn towards bottom
 *   '\'  - turn towards top
 *   '+'  - join to any direction
 *   '*'  - join to any direction
 *   Anything else terminates the trace in this direction
 */
void
trace_from_right(struct image *img, struct image *status, struct component *c, struct vertex *v, int y, int x)
{
	struct vertex *u;
	CHAR ch;

	if (x < 0 || x >= img->w)          return;
	if (y < 0 || y >= img->h)          return;

	ch = img->d[y][x];
	if (status->d[y][x] != ST_EMPTY)   return trace_maybe_loop(ch, c, v, y, x, "-=</\\+*");

	switch (ch) {
	case '-':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("< %c (%d,%d)\n", ch, y, x);
		trace_from_right(img, status, c, u, y, x-1);
		break;
	case '=':
		status->d[y][x] = seen;
		c->dashed = 1;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("< %c (%d,%d)\n", ch, y, x);
		trace_from_right(img, status, c, u, y, x-1);
		break;
	case '<':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("< %c (%d,%d) arrow end\n", ch, y, x);
		c->type = CT_LINE;
		break;
	case '/':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("< %c (%d,%d) turn towards bottom\n", ch, y, x);
		trace_from_top(img, status, c, u, y+1, x);
		break;
	case '\\':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("< %c (%d,%d) turn towards top\n", ch, y, x);
		trace_from_bottom(img, status, c, u, y-1, x);
		break;
	case '+':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("< %c (%d,%d) possible join point in <^V\n", ch, y, x);
		trace_from_right(img, status, c, u, y, x-1);
		trace_from_bottom(img, status, c, u, y-1, x);
		trace_from_top(img, status, c, u, y+1, x);
		break;
	case '*':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("< %c (%d,%d) possible join point in <^V\n", ch, y, x);
		trace_from_right(img, status, c, u, y, x-1);
		trace_from_bottom(img, status, c, u, y-1, x);
		trace_from_top(img, status, c, u, y+1, x);
		break;
	default:
		break;
	}
}

void
trace_component(struct image *img, struct image *status, struct component *c, int y, int x)
{
	struct vertex *u;
	CHAR ch;

	if (x < 0 || x >= img->w)          return;
	if (y < 0 || y >= img->h)          return;

	ch = img->d[y][x];
	if (status->d[y][x] != ST_EMPTY)   return;

	switch (ch) {
	case '-':
		c = maybe_create_component(c);
		++seen;
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		printf("%c (%d,%d)\n", ch, y, x);
		trace_from_left(img, status, c, u, y, x+1);
		trace_from_right(img, status, c, u, y, x-1);
		break;
	case '|':
		c = maybe_create_component(c);
		++seen;
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		printf("%c (%d,%d)\n", ch, y, x);
		trace_from_bottom(img, status, c, u, y-1, x);
		trace_from_top(img, status, c, u, y+1, x);
		break;
	default:
		/* do nothing */
		break;
	}
}

int
main(int argc, char **argv)
{
	struct image *orig, *blob, *no_holes, *eroded, *dilated, *status;
	PLAG lag;
	struct component *c;

	orig = read_image(stdin);

	dump_image("original", orig);

	status = create_image(orig->h, orig->w, ST_EMPTY);
	TAILQ_INIT(&components);
	seen = ST_SEEN;

	for (int y = 0; y < orig->h; y++) {
		for (int x = 0; x < orig->w; x++) {
			trace_component(orig, status, NULL, y, x);
		}
	}
	dump_image("status", status);

	TAILQ_FOREACH(c, &components, list) {
		dump_component(c);
	}

	return 0;

	blob = copy_image(orig);
	for (int i = 0; i < blob->h; i++) {
		CHAR *s = blob->d[i];
		while (*s) {
			if (*s == '+' ||
				*s == '-' ||
				*s == '|' ||
				*s == ':' ||
				*s == '=' ||
				*s == '*' ||
				*s == '/' ||
				*s == '\\' ||
				*s == '>' ||
				*s == '<' ||
				*s == '^' ||
				*s == 'V')
			{
				*s = '+';
			} else {
				*s = ' ';
			}
			s++;
		}
	}

	dump_image("blob", blob);

	// lag = build_lag(blob, '+');
	// find_lag_components(lag, 1, 1);
	// dump_lag(lag);

	no_holes = image_fill_holes(blob, 4);
	dump_image("no holes 4", no_holes);

	eroded = image_erode(no_holes, 8);
	dump_image("eroded 8", eroded);

	dilated = image_dilate(eroded, 8);
	dump_image("dilated 8", dilated);

	return 0;
}

void
dump_image(const char *title, struct image *img)
{
	printf("Image dump of \"%s\", size %dx%d\n", title, img->w, img->h);
	for (int i = 0; i < img->h; i++) {
		printf("%03d: |%s|\n", i, img->d[i]);
	}
}
