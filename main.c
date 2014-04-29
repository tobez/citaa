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

	if (c->dashed)	dashed = ", dashed";
	if (c->type == CT_BOX)
		printf("%s component, area %d%s\n",
			   COMPONENT_TYPE[c->type], c->area,
			   dashed);
	else
		printf("%s component%s\n",
			   COMPONENT_TYPE[c->type],
			   dashed);
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
	TAILQ_INIT(&c->vertices);

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
		if (v == v0)
			v_ = find_vertex_in_component(c, v->y, v->x);
		else
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
	calculate_loop_area(c);
	printf("loop area = %d\n", c->area);
}

void
extract_loops(struct component *o, struct component_head *storage)
{
	struct component_head tmp;
	struct component *c, *c_tmp, *c_max;
	struct vertex *v;
	int dir;

	TAILQ_INIT(&tmp);

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
		}
	}
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
		c->type = CT_LINE;
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
		printf("====\n");
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
		if (c->dashed && c->type == CT_LINE) {
			/* Propagate dashed status for all connected branches */
			TAILQ_FOREACH(v, &c->vertices, list) {
				TAILQ_FOREACH(connected, &components_by_point[v->y][v->x], list_by_point)
					if (connected->type == CT_LINE)
						connected->dashed = 1;
			}
		}
	}
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

void
extract_text(struct image *img)
{
	int y, x, sx;
	CHAR *buf, *s;
	CHAR *special_chars = "|:-=V^<>/\\+* ";
	struct rgb rgb;
	struct component *c;

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
				if (is_color(buf, &rgb) &&
					(c = find_enclosing_component(&components, y, sx)))
				{
					printf("COLOR %x%x%x\n", rgb.r, rgb.g, rgb.b);
				}
			}
		}
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

	TAILQ_FOREACH(c, &components, list) {
		dump_component(c);
	}

	extract_text(orig);

	if (0) {
		find_enclosing_component(&components, 1, 2);
		find_enclosing_component(&components, 4, 6);
		find_enclosing_component(&components, 4, 5);
		find_enclosing_component(&components, 3, 5);
		find_enclosing_component(&components, 3, 6);
		find_enclosing_component(&components, 2, 20);
		find_enclosing_component(&components, 2, 0);
		find_enclosing_component(&components, 6, 0);
		find_enclosing_component(&components, 6, 1);
		find_enclosing_component(&components, 6, 5);
		find_enclosing_component(&components, 6, 10);
		find_enclosing_component(&components, 7, 2);
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
