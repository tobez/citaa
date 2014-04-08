#include <stdio.h>
#include <stdlib.h>

#include "citaa.h"
#include "bsdqueue.h"

void
dump_vertex(struct vertex *v)
{
	struct adjacent *a;

	printf("vertex %c (%d,%d), connected to ", v->c, v->y, v->x);
	TAILQ_FOREACH(a, &v->adjacency_list, list) {
		printf("[%c (%d,%d)] ", a->v->c, a->v->y, a->v->x);
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

	v = malloc(sizeof(struct vertex));
	if (!v) croak(1, "add_vertex:malloc(vertex)");

	v->x = x;
	v->y = y;
	v->c = c;
	TAILQ_INIT(&v->adjacency_list);
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
	struct adjacent *a1, *a2;

	a1 = malloc(sizeof(struct adjacent));
	if (!a1) croak(1, "connect_vertices:malloc(adjacent)");
	a1->v = v1;

	a2 = malloc(sizeof(struct adjacent));
	if (!a2) croak(1, "connect_vertices:malloc(adjacent)");
	a2->v = v2;

	TAILQ_INSERT_TAIL(&v1->adjacency_list, a2, list);
	TAILQ_INSERT_TAIL(&v2->adjacency_list, a1, list);
}

void
maybe_connect_vertices(struct vertex *v1, struct vertex *v2)
{
	struct adjacent *a1, *a2, *a;

	a1 = NULL;
	TAILQ_FOREACH(a, &v2->adjacency_list, list) {
		if (a->v->y == v1->y && a->v->x == v1->x)
			a1 = a;
	}
	if (!a1) {
		a1 = malloc(sizeof(struct adjacent));
		if (!a1) croak(1, "connect_vertices:malloc(adjacent)");
		a1->v = v1;
	} else {
		a1 = NULL;
	}

	a2 = NULL;
	TAILQ_FOREACH(a, &v1->adjacency_list, list) {
		if (a->v->y == v2->y && a->v->x == v2->x)
			a2 = a;
	}
	if (!a2) {
		a2 = malloc(sizeof(struct adjacent));
		if (!a2) croak(1, "connect_vertices:malloc(adjacent)");
		a2->v = v2;
	} else {
		a2 = NULL;
	}

	if (a2) TAILQ_INSERT_TAIL(&v1->adjacency_list, a2, list);
	if (a1) TAILQ_INSERT_TAIL(&v2->adjacency_list, a1, list);
}

