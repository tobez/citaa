#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "citaa.h"
#include "bsdqueue.h"

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
		if (!c) c = create_component(&connected_components);
		++seen;
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		printf("%c (%d,%d)\n", ch, y, x);
		trace_east(img, status, c, u, y, x+1);
		trace_west(img, status, c, u, y, x-1);
		break;
	case '|':
		if (!c) c = create_component(&connected_components);
		++seen;
		status->d[y][x] = seen;
		u = add_vertex_to_component(c, y, x, ch);
		printf("%c (%d,%d)\n", ch, y, x);
		trace_north(img, status, c, u, y-1, x);
		trace_south(img, status, c, u, y+1, x);
		break;
	}
}

