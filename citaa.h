#ifndef _CITAA_H
#define _CITAA_H

#include "bsdqueue.h"

#define EAST  0
#define NORTH 1
#define WEST  2
#define SOUTH 3

#define COMPASS_FIRST EAST
#define COMPASS_LAST  SOUTH
#define N_DIRECTIONS  4

#define CLOCKWISE -1
#define COUNTERCLOCKWISE 1

#define ST_EMPTY ' '
#define ST_SEEN '0'

#define CT_UNKNOWN 0
#define CT_BRANCH  1
#define CT_BOX     2

#define SHAPE_NORMAL   0
#define SHAPE_DOCUMENT 1
#define SHAPE_STORAGE  2
#define SHAPE_IO       3

typedef char CHAR;

extern CHAR seen;
extern char *DIR[];
extern char *COMPONENT_TYPE[];

struct image
{
	int w, h;
	CHAR **d;
};

void chomp(CHAR *s);
void croak(int exit_code, const char *fmt, ...);
void croakx(int exit_code, const char *fmt, ...);
struct image* read_image(FILE *f);
const char *thisprogname(void);
void dump_image(const char *, struct image *);
struct image * create_image(int h, int w, CHAR init);
struct image* copy_image(struct image *);
struct image* expand_image(struct image *, int x, int y);

TAILQ_HEAD(vertex_head, vertex);

struct vertex
{
	TAILQ_ENTRY(vertex) list;
	int x, y, aux;
	CHAR c;
	struct vertex *e[4];  /* edges in four directions */
};

struct rgb
{
	int r;
	int g;
	int b;
};

TAILQ_HEAD(text_head, text);

struct text
{
	TAILQ_ENTRY(text) list;
	int x, y;
	int len;
	CHAR *t;
};
extern struct text_head free_text;

struct component
{
	TAILQ_ENTRY(component) list;
	TAILQ_ENTRY(component) list_by_point;

	int type;
	int dashed;
	int area;

	int has_custom_background;
	struct rgb custom_background;
	int white_text;
	int shape;

	struct vertex_head vertices;
	struct text_head text;
};
TAILQ_HEAD(component_head, component);

extern struct component_head connected_components;
extern struct component_head **components_by_point;
extern struct component_head components;
extern struct image* component_marks;

struct vertex *add_vertex(struct vertex_head *vs, int y, int x, CHAR c);
struct vertex *add_vertex_to_component(struct component *c, int y, int x, CHAR ch);
void connect_vertices(struct vertex *v1, struct vertex *v2);
void dump_vertex(struct vertex *v);
struct vertex *find_vertex_in_component(struct component *c, int y, int x);
void trace_component(struct image *img, struct image *status, struct component *c, int y, int x);
struct component *create_component(struct component_head *storage);
struct text *create_text(int y, int x, CHAR *text);
void paint(int i_height, int i_width);
struct component *find_enclosing_component(struct component_head *components, int y, int x);
void compactify_component(struct component *c);
void extract_branches(struct component *o, struct component_head *storage);
void extract_loops(struct component *o, struct component_head *storage);
void sort_components(struct component_head *storage);
void build_components_by_point(struct component_head *storage, int h, int w);
void determine_dashed_components(struct component_head *storage, struct image *img);
void extract_text(struct image *img);
void mark_components(struct image *img);

#endif
