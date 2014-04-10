#include <stdio.h>
#include <stdlib.h>

#include "citaa.h"
#include "bsdqueue.h"

void
dump_vertex(struct vertex *v)
{
	int dir;

	printf("vertex %c (%d,%d), connected to ", v->c, v->y, v->x);
	for (dir = COMPASS_FIRST; dir <= COMPASS_LAST; dir++) {
		if (v->e[dir])
			printf("[%c (%d,%d)] ", v->e[dir]->c, v->e[dir]->y, v->e[dir]->x);
	}
	printf("\n");
}

struct vertex *
find_vertex_in_component(struct component *c, int y, int x)
{
	struct vertex *v;

	TAILQ_FOREACH(v, &c->vertices, list) {
		if (v->y == y && v->x == x)
			return v;
	}
	return NULL;
}

struct vertex*
add_vertex(struct vertex_head *vs, int y, int x, CHAR c)
{
	struct vertex *v;
	int dir;

	v = malloc(sizeof(struct vertex));
	if (!v) croak(1, "add_vertex:malloc(vertex)");

	v->x = x;
	v->y = y;
	v->c = c;
	for (dir = COMPASS_FIRST; dir <= COMPASS_LAST; dir++) {
		v->e[dir] = NULL;
	}
	TAILQ_INSERT_TAIL(vs, v, list);

	return v;
}

struct vertex *
add_vertex_to_component(struct component *c, int y, int x, CHAR ch)
{
	return add_vertex(&c->vertices, y, x, ch);
}

void
connect_vertices(struct vertex *v1, struct vertex *v2)
{
	if (v1->y == v2->y) {
		if (v1->x < v2->x) {
			v1->e[EAST] = v2;
			v2->e[WEST] = v1;
		} else if (v1->x > v2->x) {
			v1->e[WEST] = v2;
			v2->e[EAST] = v1;
		} else {
			croakx(1, "internal: connect_vertices: connecting (%d,%d) to self",
				   v1->y, v1->x);
		}
	} else if (v1->x == v2->x) {
		if (v1->y < v2->y) {
			v1->e[SOUTH] = v2;
			v2->e[NORTH] = v1;
		} else if (v1->y > v2->y) {
			v1->e[NORTH] = v2;
			v2->e[SOUTH] = v1;
		} else {
			/* unreach */
			croakx(1, "internal unreach: connect_vertices: connecting (%d,%d) to self",
				   v1->y, v1->x);
		}
	} else {
		croakx(1, "internal: connect_vertices: bad direction between (%d,%d) and (%d,%d)",
			   v1->y, v1->x, v2->y, v2->x);
	}
}
