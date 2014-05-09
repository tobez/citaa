#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#include <cairo.h>

#include "citaa.h"
#include "bsdqueue.h"

struct schema {
	int xscale;
	int yscale;
	int border_left;
	int border_right;
	int border_top;
	int border_bottom;
	double fuzz_x;
	double fuzz_y;
	double dash_spec[2];
} default_paint_schema = {
	.xscale = 10,
	.yscale = 14,
	.border_left = 20,
	.border_right = 20,
	.border_top = 28,
	.border_bottom = 28,
	.fuzz_x = 0.5,
	.fuzz_y = 0.5,
	.dash_spec = { 6.0, 4.0 },
};

struct paint_context {
	cairo_surface_t *surface;
	cairo_t *cr;
	struct schema *s;

	int i_width;
	int i_height;

	int o_width;
	int o_height;
	int o_x;
	int o_y;
};

#define pcx(x) pc->o_x + (x) * pc->s->xscale + pc->s->fuzz_x
#define pcy(y) pc->o_y + (y) * pc->s->yscale + pc->s->fuzz_y

void
paint_box(struct paint_context *pc, struct component *c)
{
	struct vertex *v0, *start, *v1;
	int i, dir, new_dir;
	int min_x = INT_MAX, min_y = INT_MAX;

	cairo_set_line_width(pc->cr, 1);
	cairo_set_line_cap(pc->cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_source_rgb(pc->cr, 0, 0, 0);

	if (c->dashed)
		cairo_set_dash(pc->cr, pc->s->dash_spec, 2, 0);
	else
		cairo_set_dash(pc->cr, NULL, 0, 0);

	start = TAILQ_FIRST(&c->vertices);
	cairo_move_to(pc->cr, pcx(start->x), pcy(start->y));

	v0 = start;
	for (dir = 0; dir < N_DIRECTIONS; dir++)
		if (v0->e[dir])	break;

	while (1) {
		if (v0->x < min_x) min_x = v0->x;
		if (v0->y < min_y) min_y = v0->y;

		v1 = v0->e[dir];

		cairo_line_to(pc->cr, pcx(v1->x), pcy(v1->y));
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

	cairo_close_path(pc->cr);
	cairo_stroke_preserve(pc->cr);
	if (c->has_custom_background)
		cairo_set_source_rgb(pc->cr,
							 c->custom_background.r / 15.0,
							 c->custom_background.g / 15.0,
							 c->custom_background.b / 15.0);
	else
		cairo_set_source_rgb(pc->cr, 1, 1, 1);
	cairo_fill(pc->cr);

	{
	static int boxn = 0;
	char t[40];
	boxn++;
	snprintf(t, 40, "%d", boxn);
	cairo_set_source_rgb(pc->cr, 0, 0, 0);
	cairo_select_font_face(pc->cr, "DejaVu", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(pc->cr, 12);
	cairo_move_to(pc->cr, pcx(min_x+1), pcy(min_y+1));  
	  cairo_show_text(pc->cr, t);
	}
}

void
paint_line(struct paint_context *pc, struct component *c)
{
}

void
paint_component(struct paint_context *pc, struct component *c)
{
	if (c->type == CT_BOX)
		paint_box(pc, c);
	else if (c->type == CT_LINE)
		paint_line(pc, c);
}

void
paint(int i_height, int i_width)
{
	struct paint_context ctx;
	struct paint_context *pc = &ctx;
	struct component *c;

	pc->s = &default_paint_schema;
	pc->i_width = i_width;
	pc->i_height = i_height;
	pc->o_width = pc->s->border_left + pc->i_width * pc->s->xscale + pc->s->border_right;
	pc->o_height = pc->s->border_top + pc->i_height * pc->s->yscale + pc->s->border_bottom;
	pc->o_x = pc->s->border_left + pc->s->xscale / 2;
	pc->o_y = pc->s->border_top + pc->s->yscale / 2;

	pc->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pc->o_width, pc->o_height);
	pc->cr = cairo_create(pc->surface);

	cairo_set_source_rgb(pc->cr, 1, 1, 1);
	cairo_rectangle(pc->cr, 0, 0, pc->o_width, pc->o_height);
	cairo_fill(pc->cr);

	TAILQ_FOREACH(c, &components, list) {
		paint_component(pc, c);
	}
	// paint free text

	cairo_surface_write_to_png(pc->surface, "o.png");

	cairo_destroy(pc->cr);
	cairo_surface_destroy(pc->surface);
}

