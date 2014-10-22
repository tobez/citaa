#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#include "citaa.h"
#include "bsdqueue.h"

struct component_head connected_components;
struct component_head components;
CHAR seen;

char *DIR[] = {"EAST", "NORTH", "WEST", "SOUTH"};
char *COMPONENT_TYPE[] = {"UNKNOWN", "LINE", "BOX"};

void
dump_component(struct component *c)
{
	struct vertex *v;
	char *dashed = "";
	char color[256];
	char *white_text = "";
	char *shape_text = "";

	color[0] = '\0';

	if (c->dashed)	dashed = ", dashed";
	if (c->has_custom_background) {
		snprintf(color, 256, ", color %x%x%x",
				 c->custom_background.r,
				 c->custom_background.g,
				 c->custom_background.b);
	}
	if (c->white_text)	white_text = ", white text";
	switch (c->shape) {
	case SHAPE_DOCUMENT:
		shape_text = ", document";
		break;
	case SHAPE_STORAGE:
		shape_text = ", storage";
		break;
	case SHAPE_IO:
		shape_text = ", I/O box";
		break;
	}

	if (c->type == CT_BOX)
		printf("%s component, area %d%s%s%s%s\n",
			   COMPONENT_TYPE[c->type], c->area,
			   dashed, color, white_text, shape_text);
	else
		printf("%s component%s%s%s%s\n",
			   COMPONENT_TYPE[c->type],
			   dashed, color, white_text, shape_text);
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
create_component(struct component_head *storage)
{
	struct component *c = malloc(sizeof(struct component));
	if (!c) croak(1, "create_component:malloc(component)");

	c->type = CT_UNKNOWN;
	c->dashed = 0;
	c->area = 0;

	c->has_custom_background = 0;
	c->white_text = 0;
	c->shape = SHAPE_NORMAL;

	TAILQ_INIT(&c->vertices);
	TAILQ_INIT(&c->text);

	if (storage)
		TAILQ_INSERT_TAIL(storage, c, list);
	return c;
}

void
calculate_loop_area(struct component *c)
{
	int min_x = INT_MAX;
	int min_y = INT_MAX;
	int area = 0;
	struct vertex *v, *min_v, *v0, *v1;
	int dir, new_dir, i;

	min_v = NULL;
	TAILQ_FOREACH(v, &c->vertices, list) {
		if (v->x < min_x) {
			min_x = v->x;
			min_y = v->y;
			min_v = v;
		} else if (v->x == min_x && v->y < min_y) {
			min_y = v->y;
			min_v = v;
		}
	}

	/* The min_v vertex is now the topmost of the leftmost vertices.
	 * Moreover, there MUST be a way to the EAST from here. */
	v0 = min_v;
	dir = EAST;
	while (1) {
		v1 = v0->e[dir];
		area += (v0->x - v1->x) * v1->y;

		if (v1 == min_v)
			break;

		for (i = 1; i >= -1; i--) {
			new_dir = (dir + i + 4) % N_DIRECTIONS;
			if (v1->e[new_dir]) {
				dir = new_dir;
				v0 = v1;
				break;
			}
		}
	}
	c->area = area;
}

void
extract_one_loop(struct vertex *v0, int dir, struct component_head *storage)
{
	struct vertex *u, *v, *u_, *v_;
	struct component *c;
	int i, new_dir;

	printf("==LOOP\n");

	u = v0;
	c = create_component(storage);
	u_ = add_vertex_to_component(c, u->y, u->x, u->c);

	while (1) {
		v = u->e[dir];
		printf("coming from (%d,%d) to (%d,%d) due %s\n",
			   u->y, u->x, v->y, v->x, DIR[dir]);
		v_ = find_vertex_in_component(c, v->y, v->x);
		if (!v_)
			v_ = add_vertex_to_component(c, v->y, v->x, v->c);
		connect_vertices(u_, v_);
		u->e[dir] = NULL;  /* remove the edge we just followed */

		if (v == v0) break;

		u = NULL;
		for (i = 1; i >= -1; i--) {
			new_dir = (dir + i + 4) % N_DIRECTIONS;
			if (v->e[new_dir]) {
				u = v;
				u_ = v_;
				dir = new_dir;
				break;
			}
		}

		if (!u)	croakx(1, "extract_one_loop: cannot decide where to go from (%d,%d)\"%c\" -> %s",
					   v->y, v->x, v->c, DIR[dir]);
	}
	extract_branches(c, storage);
	printf("before loop calculation\n");
	calculate_loop_area(c);
	printf("loop area = %d\n", c->area);
}

struct component*
copy_component(struct component *o)
{
	struct vertex *v, **vo, **vr;
	struct component *r;
	int dir, nv, i;

	nv = 0;
	TAILQ_FOREACH(v, &o->vertices, list) {
		nv++;
	}

	vo = malloc(sizeof(struct vertex *) * nv);
	if (!vo) croak(1, "copy_component:malloc(%d vertices #1)", nv);
	vr = malloc(sizeof(struct vertex *) * nv);
	if (!vr) croak(1, "copy_component:malloc(%d vertices #2)", nv);

	r = create_component(NULL);

	i = 0;
	TAILQ_FOREACH(v, &o->vertices, list) {
		v->aux = i;
		vo[i] = v;
		vr[i] = add_vertex_to_component(r, v->y, v->x, v->c);
		i++;
	}

	i = 0;
	TAILQ_FOREACH(v, &o->vertices, list) {
		for (dir = 0; dir < N_DIRECTIONS; dir++) {
			if (v->e[dir]) {
				vr[i]->e[dir] = vr[v->e[dir]->aux];
			}
		}
		i++;
	}

	free(vo);
	free(vr);

	return r;
}

/* subtract component c from component C */
void
subtract_component(struct component *C, struct component *c)
{
	struct vertex *v, *V;
	int dir;

	TAILQ_FOREACH(v, &c->vertices, list) {
		V = find_vertex_in_component(C, v->y, v->x);
		if (V) {
			for (dir = 0; dir < N_DIRECTIONS; dir++) {
				if (v->e[dir]) {
					V->e[dir] = NULL;
				}
			}
		}
	}
}

void
extract_loops(struct component *o, struct component_head *storage)
{
	struct component_head tmp;
	struct component *c, *c_tmp, *c_max, *orig;
	struct vertex *v;
	int dir;

	TAILQ_INIT(&tmp);

	orig = copy_component(o);

	TAILQ_FOREACH(v, &o->vertices, list) {
		for (dir = COMPASS_FIRST; dir <= COMPASS_LAST; dir++) {
			if (v->e[dir]) {
				extract_one_loop(v, dir, &tmp);
			}
		}
	}

	c_max = NULL;
	TAILQ_FOREACH(c, &tmp, list) {
		if (!c_max || c->area > c_max->area)
			c_max = c;
	}
	TAILQ_FOREACH_SAFE(c, &tmp, list, c_tmp) {
		TAILQ_REMOVE(&tmp, c, list);
		if (c != c_max) {
			c->type = CT_BOX;
			TAILQ_INSERT_TAIL(storage, c, list);
			subtract_component(orig, c);
		}
	}

	extract_branches(orig, storage);
}

void
extract_one_branch(struct vertex *u, struct component_head *storage)
{
	struct vertex *v = NULL, *u_, *v_;
	struct component *c;
	int dir, branch_dir, cnt;

	c = create_component(storage);
	u_ = add_vertex_to_component(c, u->y, u->x, u->c);

	printf("==BRANCH\n");

	while (1) {
		cnt = 0;
		for (dir = COMPASS_FIRST; dir <= COMPASS_LAST; dir++)
			if (u->e[dir]) {
				cnt++;
				v = u->e[dir];
				branch_dir = dir;
			}

		if (cnt == 1) {
			v_ = add_vertex_to_component(c, v->y, v->x, v->c);

			printf("coming from (%d,%d) to (%d,%d) due %s\n",
				   u->y, u->x, v->y, v->x, DIR[branch_dir]);

			connect_vertices(u_, v_);

			u->e[branch_dir] = NULL;
			v->e[(branch_dir + 2) % N_DIRECTIONS] = NULL;

			u = v;
			u_ = v_;
			continue;
		}

		break;
	}
}

void
extract_branches(struct component *o, struct component_head *storage)
{
	struct vertex *v, *v_tmp;
	struct component_head tmp;
	struct component *c, *c_tmp;
	int dir;

	TAILQ_INIT(&tmp);

again:
	TAILQ_FOREACH_SAFE(v, &o->vertices, list, v_tmp) {
		int cnt = 0;

		for (dir = COMPASS_FIRST; dir <= COMPASS_LAST; dir++)
			if (v->e[dir])
				cnt++;

		if (cnt == 1) {
			extract_one_branch(v, &tmp);
			TAILQ_REMOVE(&o->vertices, v, list);
			goto again;
		}

		if (cnt == 0)
			TAILQ_REMOVE(&o->vertices, v, list);
	}

	TAILQ_FOREACH_SAFE(c, &tmp, list, c_tmp) {
		TAILQ_REMOVE(&tmp, c, list);
		c->type = CT_BRANCH;
		TAILQ_INSERT_TAIL(storage, c, list);
	}
}

static int
func_component_compare(const void *ap, const void *bp)
{
	const struct component *a = *(const struct component **)ap;
	const struct component *b = *(const struct component **)bp;
	int cp;

	cp = b->area - a->area;
	if (cp)	return cp;

	if (ap < bp)	return -1;
	if (ap > bp)	return 1;
	return 0;
}

void
sort_components(struct component_head *storage)
{
	struct component *c, **all, *c_tmp;
	int cnt = 0, i = 0;

	TAILQ_FOREACH(c, storage, list) {
		cnt++;
	}

	all = malloc(sizeof(c) * cnt);
	if (!all) croak(1, "sort_components:malloc(all)");

	TAILQ_FOREACH_SAFE(c, storage, list, c_tmp) {
		all[i++] = c;
		TAILQ_REMOVE(storage, c, list);
	}

	qsort(all, cnt, sizeof(c), func_component_compare);

	for (i = 0; i < cnt; i++)
		TAILQ_INSERT_TAIL(storage, all[i], list);
	free(all);
}

struct component_head **components_by_point;

void
build_components_by_point(struct component_head *storage, int h, int w)
{
	struct component *c;
	struct vertex *v;
	int x, y;

	components_by_point = malloc(sizeof(struct component_head *)*h);
	if (!components_by_point)	croak(1, "build_components_by_point:malloc(components_by_point)");
	for (y = 0; y < h; y++) {
		components_by_point[y] = malloc(sizeof(struct component_head)*w);
		if (!components_by_point[y])
			croak(1, "build_components_by_point:malloc(components_by_point[%d])", y);
		for (x = 0; x < w; x++)
			TAILQ_INIT(&components_by_point[y][x]);
	}

	TAILQ_FOREACH(c, storage, list) {
		TAILQ_FOREACH(v, &c->vertices, list) {
			TAILQ_INSERT_TAIL(&components_by_point[v->y][v->x], c, list_by_point);
		}
	}
}

void
determine_dashed_components(struct component_head *storage, struct image *img)
{
	struct component *c, *connected;
	struct vertex *u, *v;
	int x, y;

	TAILQ_FOREACH(c, storage, list) {
		printf("dashed? ====\n");
		TAILQ_FOREACH(u, &c->vertices, list) {
			if ((v = u->e[EAST])) {
				y = u->y;
				for (x = u->x; x <= v->x; x++) {
					if (img->d[y][x] == '=') {
						c->dashed = 1;
						break;
					}
				}
			}
			if (c->dashed)
				break;
			if ((v = u->e[SOUTH])) {
				x = u->x;
				for (y = u->y; y <= v->y; y++) {
					if (img->d[y][x] == ':') {
						c->dashed = 1;
						break;
					}
				}
			}
			if (c->dashed)
				break;
		}
		if (c->dashed && c->type == CT_BRANCH) {
			/* Propagate dashed status for all connected branches */
			TAILQ_FOREACH(v, &c->vertices, list) {
				TAILQ_FOREACH(connected, &components_by_point[v->y][v->x], list_by_point)
					if (connected->type == CT_BRANCH)
						connected->dashed = 1;
			}
		}
	}
	printf("end of dashed check\n");
}

struct component *
find_enclosing_component(struct component_head *components, int y, int x)
{
	struct component *c;
	struct vertex *u, *v;
	int cnt;

	TAILQ_FOREACH_REVERSE(c, components, component_head, list) {
		if (c->type != CT_BOX)	continue;

		cnt = 0;

		TAILQ_FOREACH(u, &c->vertices, list) {
			if ((v = u->e[SOUTH])) {
				if (u->x == x && y >= u->y && y <= v->y) {
					/* point ON the edge, consider it to be inside */
					cnt = 1;
					break;
				}

				if (x < u->x && y >= u->y && y < v->y)
					cnt++;
			}

			if ((v = u->e[EAST])) {
				if (u->y == y && x >= u->x && x <= v->x) {
					/* point ON the edge, consider it to be inside */
					cnt = 1;
					break;
				}
			}
		}

		if (cnt % 2 == 1) {
			printf("Point %d,%d is inside the following component:\n", y, x);
			dump_component(c);
			return c;
		}
	}
	printf("Point %d,%d is outside of everything\n", y, x);
	return NULL;
}

void
mark_points(struct image *img, int y0, int x0, int y1, int x1)
{
	int exch;

	if (y0 == y1) {
		if (x0 > x1) {
			exch = x0;
			x0 = x1;
			x1 = exch;
		}
		while (x0 <= x1) {
			img->d[y0][x0] = '*';
			x0++;
		}
	} else if (x0 == x1) {
		if (y0 > y1) {
			exch = y0;
			y0 = y1;
			y1 = exch;
		}
		while (y0 <= y1) {
			img->d[y0][x0] = '*';
			y0++;
		}
	}
}

void
mark_box(struct image *img, struct component *c)
{
	struct vertex *v0, *start, *v1;
	int i, dir, new_dir;

	start = TAILQ_FIRST(&c->vertices);

	v0 = start;
	for (dir = 0; dir < N_DIRECTIONS; dir++)
		if (v0->e[dir])	break;

	while (1) {
		v1 = v0->e[dir];

		mark_points(img, v0->y, v0->x, v1->y, v1->x);

		if (v1 == start)
			break;

		for (i = 1; i >= -1; i--) {
			new_dir = (dir + i + 4) % N_DIRECTIONS;
			if (v1->e[new_dir]) {
				dir = new_dir;
				v0 = v1;
				break;
			}
		}
	}
}

void
mark_line(struct image *img, struct component *c)
{
	struct vertex *v0, *start = NULL, *v1, *v;
	int i, dir, new_dir;

	TAILQ_FOREACH(v, &c->vertices, list) {
		int cnt = 0;

		for (dir = 0; dir < N_DIRECTIONS; dir++) {
			if (v->e[dir]) {
				cnt++;
				start = v;
			}
		}
		if (cnt == 1)
			break;
	}
	if (!start) return;

	v0 = start;
	for (dir = 0; dir < N_DIRECTIONS; dir++)
		if (v0->e[dir])	break;

	while (v0) {
		v1 = v0->e[dir];

		mark_points(img, v0->y, v0->x, v1->y, v1->x);

		v0 = NULL;
		for (i = 1; i >= -1; i--) {
			new_dir = (dir + i + 4) % N_DIRECTIONS;
			if (v1->e[new_dir]) {
				dir = new_dir;
				v0 = v1;
				break;
			}
		}
	}
}

void
mark_component(struct image *img, struct component *c)
{
	dump_component(c);
	printf("marking...\n");
	if (c->type == CT_BOX && c->area == 0)
		c->type = CT_BRANCH;
	if (c->type == CT_BOX)
		mark_box(img, c);
	else if (c->type == CT_BRANCH)
		mark_line(img, c);
}

void
mark_components(struct image *img)
{
	struct component *c;

	TAILQ_FOREACH(c, &components, list) {
		mark_component(img, c);
	}
}

/* TODO: implement me */
static int
component_direction(struct component *c)
{
	/* count number of left and right turns */
	/* return COUNTERCLOCKWISE if left > right */
	/* return CLOCKWISE if right > left */
	/* bitch and die if right == left */

	struct vertex *v0, *start, *v1;
	int i, dir, new_dir, left = 0, right = 0;

	start = TAILQ_FIRST(&c->vertices);

	v0 = start;
	for (dir = 0; dir < N_DIRECTIONS; dir++)
		if (v0->e[dir])	break;

	while (1) {
		v1 = v0->e[dir];

		if (v1 == start)
			break;

		for (i = 1; i >= -1; i--) {
			new_dir = (dir + i + 4) % N_DIRECTIONS;
			if (v1->e[new_dir]) {

				/* XXX: find out dir-new_dir is left or right here */
				dir = new_dir;
				v0 = v1;
				break;
			}
		}
	}

	if (left > right)
		return COUNTERCLOCKWISE;
	if (right > left)
		return CLOCKWISE;
	croakx(1, "internal: component_direction: left turns equals right turns");
	return 0;
}

/* TODO: implement me */
static void
flip_component(struct component_head *components, struct component *c)
{
	/* Allocate new component, copy the given component into it,
	 * with and OPPOSITE traversal direction (reversed order of
	 * nodes, flipped direction of traversal).
	 * Then replace the given component with the new one,
	 * in-place in the components list, and free the old one.
	 *
	 * See whether TAILQ_SWAP() might be useful for making the
	 * sbstitution.
	 */
}

/* XXX: careful;  since it changes the component list,
 * it should be called before other functions which store
 * pointers to the components in other places!
 */
void
make_all_loops_going_counterclockwise(struct component_head *components)
{
	struct component *c;

	TAILQ_FOREACH(c, components, list) {
		if (!c->type == CT_BOX)
			continue;
		if (component_direction(c) == CLOCKWISE)
			flip_component(components, c);
	}
}

