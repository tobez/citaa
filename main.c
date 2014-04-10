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

void
compactify_component(struct component *c)
{
	struct vertex *v, *v_tmp;

	TAILQ_FOREACH_SAFE(v, &c->vertices, list, v_tmp) {
		switch (v->c) {
		case '-':
		case '=':
			// broken_if(v->e[NORTH], "%c(%d,%d) vertex has a northern edge", v->c, v->y, v->x);
			// broken_if(v->e[SOUTH], "%c(%d,%d) vertex has a southern edge", v->c, v->y, v->x);

			if (v->e[WEST] && v->e[EAST]) {
				v->e[WEST]->e[EAST] = v->e[EAST];
				v->e[EAST]->e[WEST] = v->e[WEST];
				TAILQ_REMOVE(&c->vertices, v, list);
				free(v);
			}
			break;
		case '|':
		case ':':
			// broken_if(v->e[WEST], "%c(%d,%d) vertex has a western edge", v->c, v->y, v->x);
			// broken_if(v->e[EAST], "%c(%d,%d) vertex has an eastern edge", v->c, v->y, v->x);

			if (v->e[NORTH] && v->e[SOUTH]) {
				v->e[NORTH]->e[SOUTH] = v->e[SOUTH];
				v->e[SOUTH]->e[NORTH] = v->e[NORTH];
				TAILQ_REMOVE(&c->vertices, v, list);
				free(v);
			}
			break;
		}
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
void trace_east(struct image *img, struct image *status, struct component *c, struct vertex *v, int y, int x);
void trace_west(struct image *img, struct image *status, struct component *c, struct vertex *v, int y, int x);
void trace_north(struct image *img, struct image *status, struct component *c, struct vertex *v, int y, int x);
void trace_south(struct image *img, struct image *status, struct component *c, struct vertex *v, int y, int x);
void trace_maybe_loop(CHAR ch, struct component *c, struct vertex *v, int y, int x, CHAR *loop_chars);

void
trace_maybe_loop(CHAR ch, struct component *c, struct vertex *v, int y, int x, CHAR *loop_chars)
{
	struct vertex *u;

	if (!strchr(loop_chars, ch)) return;

	u = find_vertex_in_component(c, y, x);
	if (!u) return;

	connect_vertices(v, u);
}

/* To the SOUTH there can be:
 *   '|'  - continuation of the trace
 *   ':'  - continuation of the trace, turn on dashes
 *   'V'  - arrow end
 *   '/'  - turn west
 *   '\'  - turn east
 *   '+'  - join to any direction
 *   '*'  - join to any direction
 *   Anything else terminates the trace in this direction
 */
void
trace_south(struct image *img, struct image *status, struct component *c, struct vertex *v, int y, int x)
{
	struct vertex *u;
	CHAR ch;

	if (x < 0 || x >= img->w)          return;
	if (y < 0 || y >= img->h)          return;

	ch = img->d[y][x];
	if (status->d[y][x] != ST_EMPTY)   return trace_maybe_loop(ch, c, v, y, x, "|:V/\\+*");

	switch (ch) {
	case ':':
		c->dashed = 1;
	case '|':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("V %c (%d,%d)\n", ch, y, x);
		trace_south(img, status, c, u, y+1, x);
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
		printf("V %c (%d,%d) turn west\n", ch, y, x);
		trace_west(img, status, c, u, y, x-1);
		break;
	case '\\':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("V %c (%d,%d) turn east\n", ch, y, x);
		trace_east(img, status, c, u, y, x+1);
		break;
	case '*':
	case '+':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("V %c (%d,%d) possible join point in V><\n", ch, y, x);
		trace_south(img, status, c, u, y+1, x);
		trace_west(img, status, c, u, y, x-1);
		trace_east(img, status, c, u, y, x+1);
		break;
	}
}

/* To the NORTH there can be:
 *   '|'  - continuation of the trace
 *   ':'  - continuation of the trace, turn on dashes
 *   '^'  - arrow end
 *   '/'  - turn east
 *   '\'  - turn west
 *   '+'  - join to any direction
 *   '*'  - join to any direction
 *   Anything else terminates the trace in this direction
 */
void
trace_north(struct image *img, struct image *status, struct component *c, struct vertex *v, int y, int x)
{
	struct vertex *u;
	CHAR ch;

	if (x < 0 || x >= img->w)          return;
	if (y < 0 || y >= img->h)          return;

	ch = img->d[y][x];
	if (status->d[y][x] != ST_EMPTY)   return trace_maybe_loop(ch, c, v, y, x, "|:^/\\+*");

	switch (ch) {
	case ':':
		c->dashed = 1;
	case '|':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("^ %c (%d,%d)\n", ch, y, x);
		trace_north(img, status, c, u, y-1, x);
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
		printf("^ %c (%d,%d) turn east\n", ch, y, x);
		trace_east(img, status, c, u, y, x+1);
		break;
	case '\\':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("^ %c (%d,%d) turn west\n", ch, y, x);
		trace_west(img, status, c, u, y, x-1);
		break;
	case '*':
	case '+':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("^ %c (%d,%d) possible join point in ^><\n", ch, y, x);
		trace_north(img, status, c, u, y-1, x);
		trace_east(img, status, c, u, y, x+1);
		trace_west(img, status, c, u, y, x-1);
		break;
	}
}

/* To the EAST there can be:
 *   '-'  - continuation of the trace
 *   '='  - continuation of the trace, turn on dashes
 *   '>'  - arrow end
 *   '/'  - turn north
 *   '\'  - turn south
 *   '+'  - join to any direction
 *   '*'  - join to any direction
 *   Anything else terminates the trace in this direction
 */
void
trace_east(struct image *img, struct image *status, struct component *c, struct vertex *v, int y, int x)
{
	struct vertex *u;
	CHAR ch;

	if (x < 0 || x >= img->w)          return;
	if (y < 0 || y >= img->h)          return;

	ch = img->d[y][x];
	if (status->d[y][x] != ST_EMPTY)   return trace_maybe_loop(ch, c, v, y, x, "-=>/\\+*");

	switch (ch) {
	case '=':
		c->dashed = 1;
	case '-':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("> %c (%d,%d)\n", ch, y, x);
		trace_east(img, status, c, u, y, x+1);
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
		printf("> %c (%d,%d) turn north\n", ch, y, x);
		trace_north(img, status, c, u, y-1, x);
		break;
	case '\\':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("> %c (%d,%d) turn south\n", ch, y, x);
		trace_south(img, status, c, u, y+1, x);
		break;
	case '*':
	case '+':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("> %c (%d,%d) possible join point in >^V\n", ch, y, x);
		trace_east(img, status, c, u, y, x+1);
		trace_north(img, status, c, u, y-1, x);
		trace_south(img, status, c, u, y+1, x);
		break;
	}
}

/* To the WEST there can be:
 *   '-'  - continuation of the trace
 *   '='  - continuation of the trace, turn on dashes
 *   '<'  - arrow end
 *   '/'  - turn south
 *   '\'  - turn north
 *   '+'  - join to any direction
 *   '*'  - join to any direction
 *   Anything else terminates the trace in this direction
 */
void
trace_west(struct image *img, struct image *status, struct component *c, struct vertex *v, int y, int x)
{
	struct vertex *u;
	CHAR ch;

	if (x < 0 || x >= img->w)          return;
	if (y < 0 || y >= img->h)          return;

	ch = img->d[y][x];
	if (status->d[y][x] != ST_EMPTY)   return trace_maybe_loop(ch, c, v, y, x, "-=</\\+*");

	switch (ch) {
	case '=':
		c->dashed = 1;
	case '-':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("< %c (%d,%d)\n", ch, y, x);
		trace_west(img, status, c, u, y, x-1);
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
		printf("< %c (%d,%d) turn south\n", ch, y, x);
		trace_south(img, status, c, u, y+1, x);
		break;
	case '\\':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("< %c (%d,%d) turn north\n", ch, y, x);
		trace_north(img, status, c, u, y-1, x);
		break;
	case '*':
	case '+':
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		connect_vertices(v, u);
		printf("< %c (%d,%d) possible join point in <^V\n", ch, y, x);
		trace_west(img, status, c, u, y, x-1);
		trace_north(img, status, c, u, y-1, x);
		trace_south(img, status, c, u, y+1, x);
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
		trace_east(img, status, c, u, y, x+1);
		trace_west(img, status, c, u, y, x-1);
		break;
	case '|':
		c = maybe_create_component(c);
		++seen;
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		printf("%c (%d,%d)\n", ch, y, x);
		trace_north(img, status, c, u, y-1, x);
		trace_south(img, status, c, u, y+1, x);
		break;
	}
}

int
main(int argc, char **argv)
{
	struct image *orig, *blob, *no_holes, *eroded, *dilated, *status;
	struct component *c;
	int x, y, i;

	orig = read_image(stdin);

	dump_image("original", orig);

	status = create_image(orig->h, orig->w, ST_EMPTY);
	TAILQ_INIT(&components);
	seen = ST_SEEN;

	for (y = 0; y < orig->h; y++) {
		for (x = 0; x < orig->w; x++) {
			trace_component(orig, status, NULL, y, x);
		}
	}
	dump_image("status", status);

	TAILQ_FOREACH(c, &components, list) {
		compactify_component(c);
		dump_component(c);
	}

	return 0;

	blob = copy_image(orig);
	for (i = 0; i < blob->h; i++) {
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
	int i;
	printf("Image dump of \"%s\", size %dx%d\n", title, img->w, img->h);
	for (i = 0; i < img->h; i++) {
		printf("%03d: |%s|\n", i, img->d[i]);
	}
}
