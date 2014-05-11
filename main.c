#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#include "citaa.h"
#include "bsdqueue.h"

struct image* component_marks;

int
main(int argc, char **argv)
{
	struct image *orig, *status;
	struct component *c;
	int x, y;

	TAILQ_INIT(&free_text);

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

	component_marks = create_image(orig->h, orig->w, ' ');
	mark_components(component_marks);

	extract_text(orig);

	paint(orig->h, orig->w);

	return 0;
}
