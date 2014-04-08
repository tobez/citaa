#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "citaa.h"

struct image *
copy_image(struct image *src)
{
	struct image *dst;

	dst = malloc(sizeof(*dst));
	if (!dst) croak(1, "copy_image:malloc(dst)");

	dst->h = src->h;
	dst->w = src->w;
	dst->d = malloc(sizeof(CHAR *) * dst->h);
	if (!dst->d) croak(1, "copy_image:malloc(d)");

	for (int i = 0; i < dst->h; i++) {
		CHAR *r = malloc(sizeof(CHAR)*(dst->w + 1));
		if (!r) croak(1, "copy_image:malloc(row %d, %d chars)", i, dst->w+1);
		memset(r, ' ', dst->w);
		memcpy(r, src->d[i], strlen(src->d[i]));
		r[dst->w] = 0;
		dst->d[i] = r;
	}

	return dst;
}

struct image *
create_image(int h, int w, CHAR init)
{
	struct image *dst;

	dst = malloc(sizeof(*dst));
	if (!dst) croak(1, "create_image:malloc(dst)");

	dst->h = h;
	dst->w = w;
	dst->d = malloc(sizeof(CHAR *) * dst->h);
	if (!dst->d) croak(1, "create_image:malloc(d)");

	for (int i = 0; i < dst->h; i++) {
		CHAR *r = malloc(sizeof(CHAR)*(dst->w + 1));
		if (!r) croak(1, "create_image:malloc(row %d, %d chars)", i, dst->w+1);
		memset(r, init, dst->w);
		r[dst->w] = 0;
		dst->d[i] = r;
	}

	return dst;
}

struct image* expand_image(struct image *src, int x, int y)
{
	struct image *dst;

	dst = malloc(sizeof(*dst));
	if (!dst) croak(1, "expand_image:malloc(dst)");

	dst->h = src->h + y*2;
	dst->w = src->w + x*2;
	dst->d = malloc(sizeof(CHAR *) * dst->h);
	if (!dst->d) croak(1, "copy_image:malloc(d)");

	for (int i = 0; i < dst->h; i++) {
		CHAR *r = malloc(sizeof(CHAR)*(dst->w + 1));
		if (!r) croak(1, "copy_image:malloc(row %d, %d chars)", i, dst->w+1);
		memset(r, ' ', dst->w);
		if (i >= y && i-y < src->h)
			memcpy(r+x, src->d[i-y], strlen(src->d[i-y]));
		r[dst->w] = 0;
		dst->d[i] = r;
	}

	return dst;
}

/* LAG freeing function */
void
free_lag( PLAG lag)
{
   int i;

   if (!lag)  return;

   free( lag-> codedLines);
   free( lag-> codedAreas);

   if (lag-> scanLines)
      for ( i = 0; i < lag-> h; i++)
         free( lag-> scanLines[ i]);

   free( lag-> scanLines);
   free( lag-> lineCount);

   free( lag);
}

void
dump_lag( PLAG lag)
{
   for (int i = 0; i < lag-> h; i++) {
	   printf("%03d: ", i);
	   for (int j = 0; j < lag->lineCount[i]; j++) {
		   printf("%d-%d[%d] ",
				  lag->scanLines[i][j].beg, lag->scanLines[i][j].end,
				  lag->scanLines[i][j].code);
	   }
	   printf("\n");
   }
}

void
clean_codes( PLAG lag)
{
   int i, j;

   if ( lag-> codedLines)
      free( lag-> codedLines);
   if ( lag-> codedAreas)
      free( lag-> codedAreas);
   lag-> maxComponentCode = lcNormal;
   lag-> codedCollectionSize = 256;
   lag-> codedLines = calloc( lag-> codedCollectionSize, sizeof( PLAGLine));
   lag-> codedAreas = calloc( lag-> codedCollectionSize, sizeof( int));

   /* Clean next pointer everywere */
   if (lag-> scanLines)
      for ( i = 0; i < lag-> h; i++)
         for ( j = 0; j < lag-> lineCount[ i]; j++)
            lag-> scanLines[ i][ j]. next = NULL;
}


/* LAG building function */

PLAG
build_lag( struct image *im, CHAR c)
{
   PLAG lag;            /* structure to be built */
   int w, h;            /* width and height of an image */
   int i, j, n;         /* indices & counters */
   PLAGLine scan;       /* temporary scan line */
   CHAR *row;  /* a row of an image */

   h = im-> h;
   w = im-> w;

   /* Allocate and initialize main LAG structure */
   lag = malloc( sizeof( LAG));
   if (!lag) croak(1, "build_lag:malloc LAG");
   memset( lag, 0, sizeof( LAG));
   lag-> scanLines = malloc( sizeof( PLAGLine) * h);
   if (!lag-> scanLines) croak(1, "build_lag:malloc scan lines");
   memset( lag-> scanLines, 0, sizeof( PLAGLine) * h);
   lag-> lineCount = malloc( sizeof( int) * h);
   if (!lag-> lineCount) croak(1, "build_lag:malloc line count");
   memset( lag-> lineCount, 0, sizeof( int) * h);
   lag-> h = h;
   lag-> w = w;

   /* Allocate intermediate LAGScanLine of big enough size. */
   /* It is impossible to have more than ( w + 1) / 2 distinct lines in a scan line. */
   scan = malloc( sizeof( LAGLine) * ( w + 1) / 2);
   if (!scan) croak(1, "build_lag:malloc one lagline");

   /* Scan through every line of the image */
   for ( i = 0; i < h; i++)
   {
      n = 0;      /* n holds the number of distinct lines in the current scan line */
      j = 0;      /* start scan from the leftmost pixel */
      row = im->d[i];

      while ( j < w)
      {
         /* Skip from the current position to the first pixel having specified color c */
         while (( j < w) && ( row[ j] != c))    j++;

         /* Store the line, if any */
         if ( j < w)
         {
            scan[ n]. next = NULL;      /* this field is not used in this function at all! */
            scan[ n]. y = i;           /* reference to the scan line */
            scan[ n]. beg = j;         /* The line begins from j */
            scan[ n]. code = lcUnclassified;
            /* Seeking the end of the line... */
            while (( j < w) && ( row[ j] == c))    j++;
            scan[ n]. end = j - 1;     /* The line ends here */
            /* Update the lines counter */
            n++;
         }
      }

      /* Add the scan line description to the LAG (if needed) */
      if ( n > 0)
      {
         lag-> scanLines[ i] = malloc( sizeof( LAGLine) * n);
         lag-> lineCount[ i] = n;
         memcpy( lag-> scanLines[ i], scan, sizeof( LAGLine) * n);
      }
   }

   return lag;
}

/*  Searching the connected components in a LAG */

/*  edgeSize specifies the frame around the border of the */
/*     image;  all the components any part of which is */
/*     inside that frame are marked as lcEdge */

void
find_lag_components( PLAG lag, int edgeSize, int eightConnectivity)
{
   int y, i, j;
   PLAGLine line, prevLine;
   PLAGLine prevScanLine;
   PLAGLine curScanLine = NULL;
   int prevLineCount, curLineCount = 0;

   /* Necessary adjustment! */
   eightConnectivity = ( eightConnectivity) ? 1 : 0;

   clean_codes( lag);
   /* After this call we garanteed to have size of codedLines and */
   /* codedAreas to be at least lcNormal;  hence no checking for */
   /* lcEdge (lcEdge < lcNormal). */

   /* Initial marking of a priori edge-touched lines */
   /*  (not all of them, though;  only bottom border stripe is considered here */
   for ( y = 0; y < edgeSize; y++)
   {
      curScanLine = lag-> scanLines[ y];
      curLineCount = lag-> lineCount[ y];
      for ( i = 0; i < curLineCount; i++)
      {
         line = curScanLine + i;
         line-> code = lcEdge;
         line-> next = lag-> codedLines[ lcEdge];
         lag-> codedLines[ lcEdge] = line;
         lag-> codedAreas[ lcEdge] += line-> end - line-> beg + 1;
      }
   }

   /* Main loop through the rest of the lines */
   for ( y = edgeSize; y < lag-> h; y++)
   {
      prevScanLine = curScanLine;
      curScanLine = lag-> scanLines[ y];
      prevLineCount = curLineCount;
      curLineCount = lag-> lineCount[ y];

      for ( i = 0; i < curLineCount; i++)
      {
         int lastScanned = 0;    /* In the previous scan line! */
         int edgeTouched = 0;
         int overlaps = 0;
         int overlappedWith = 0;
         int oldCode, newCode;
         PLAGLine toChange;

         line = curScanLine + i;

         for ( j = lastScanned; j < prevLineCount; j++)
         {
            prevLine = prevScanLine + j;
            if (( line-> beg <= prevLine-> end + eightConnectivity) &&
                ( line-> end >= prevLine-> beg - eightConnectivity))
            {
               overlaps = 1;
               lastScanned = j + 1;
               overlappedWith = prevLine-> code;
               break;  /* the j loop */
            }
         }

         if ( overlaps)
         {
            line-> code = overlappedWith;
            line-> next = lag-> codedLines[ overlappedWith];
            lag-> codedLines[ overlappedWith] = line;
            lag-> codedAreas[ overlappedWith] += line-> end - line-> beg + 1;

            edgeTouched = ( overlappedWith == lcEdge);

            /* Check for multiple overlapping */
            while ( overlaps)
            {
               overlaps = 0;

               for ( j = lastScanned; j < prevLineCount; j++)
               {
                  prevLine = prevScanLine + j;
                  if (( line-> beg <= prevLine-> end + eightConnectivity) &&
                      ( line-> end >= prevLine-> beg - eightConnectivity))
                  {
                     overlaps = 1;
                     lastScanned = j + 1;
                     overlappedWith = prevLine-> code;
                     break;  /* the j loop */
                  }
               }

               if ( !overlaps)   break;      /* Good boy! */

               if ( line-> code == overlappedWith) continue;      /* Not bad... */

               if ( edgeTouched && ( overlappedWith != lcEdge))
               {
                  oldCode = overlappedWith;
                  newCode = lcEdge;
               }
               else
               {
                  oldCode = line-> code;
                  newCode = overlappedWith;
               }

               /* Perform code adjustment */
               toChange = lag-> codedLines[ oldCode];
               if (toChange)
               {
                  while ( toChange-> next)
                  {
                     toChange-> code = newCode;
                     toChange = toChange-> next;
                  }
                  toChange-> code = newCode;
                  toChange-> next = lag-> codedLines[ newCode];
                  lag-> codedLines[ newCode] = lag-> codedLines[ oldCode];
                  lag-> codedAreas[ newCode] += lag-> codedAreas[ oldCode];
                  lag-> codedLines[ oldCode] = NULL;
                  lag-> codedAreas[ oldCode] = 0;
               }

               edgeTouched = ( overlappedWith == lcEdge) ? 1 : edgeTouched;

            }
         }
         else
         {
            /* Didn't overlap;  assign the unique code here */
            /* Check the collection on overflow (expand if necessary) */
            if ( lag-> maxComponentCode >= lag-> codedCollectionSize)
            {
               PLAGLine *codedLines;
               int *codedAreas;
               int sz;

               sz = lag-> codedCollectionSize * 2;
               codedLines = calloc( sz, sizeof( PLAGLine));
               codedAreas = calloc( sz, sizeof( int));
               memcpy( codedLines, lag-> codedLines, lag-> maxComponentCode * sizeof( PLAGLine));
               memcpy( codedAreas, lag-> codedAreas, lag-> maxComponentCode * sizeof( int));
               free( lag-> codedLines);
               free( lag-> codedAreas);
               lag-> codedLines = codedLines;
               lag-> codedAreas = codedAreas;
               lag-> codedCollectionSize = sz;
            }
            line-> code = lag-> maxComponentCode;
            line-> next = lag-> codedLines[ line-> code];
            lag-> codedLines[ line-> code] = line;
            lag-> codedAreas[ line-> code] = line-> end - line-> beg + 1;
            lag-> maxComponentCode++;
         }

         if (( !edgeTouched) &&
             (( line-> beg < edgeSize) ||
              ( line-> end >= lag-> w - edgeSize) ||
              ( y >= lag-> h - edgeSize)))
         {
            oldCode = line-> code;
            newCode = lcEdge;
            toChange = lag-> codedLines[ oldCode];
            if ( toChange)
            {
               while ( toChange-> next)
               {
                  toChange-> code = newCode;
                  toChange = toChange-> next;
               }
               toChange-> code = newCode;
               toChange-> next = lag-> codedLines[ newCode];
               lag-> codedLines[ newCode] = lag-> codedLines[ oldCode];
               lag-> codedAreas[ newCode] += lag-> codedAreas[ oldCode];
               lag-> codedLines[ oldCode] = NULL;
               lag-> codedAreas[ oldCode] = 0;
            }

         }

      }
   }
}

struct image *
image_fill_holes( struct image *in, int neighborhood)
{
	struct image *out;

	out = copy_image(in);
	int edgeSize = 1;
	CHAR backColor = ' ';
	CHAR foreColor = '+';
	PLAG lag;
	int i;
	PLAGLine line;

	if ( neighborhood != 8 && neighborhood != 4)
		croakx(1, "image_fill_holes: neighborhood must be 4 or 8");

	lag = build_lag(out, backColor);
	find_lag_components(lag, edgeSize, neighborhood == 8);
	for ( i = lcNormal; i < lag-> maxComponentCode; i++) {
		for ( line = lag-> codedLines[ i]; line; line = line-> next) {
			memset( out->d[line->y] + line-> beg,
					foreColor, line-> end - line-> beg + 1);
		}
	}
	free_lag( lag);
	return out;
}

#undef max
#define max(A,B)  ((A) > (B) ? (A) : (B))
#undef min
#define min(A,B)  ((A) < (B) ? (A) : (B))

#define DILATEERODE8(extremum) {                                                            \
   CHAR *p, *c = NULL, *n = inp->d[0], *o;                                                  \
   for (y = 0; y < h; y++) {                                                                \
      o = out->d[y];                                                                        \
      p = c; c = n; n = y == maxy ? NULL : inp->d[y+1];                                     \
      if ( y == 0) {                                                                        \
         o[0] = extremum(extremum(c[0],c[1]),extremum(n[0],n[1]));                          \
         o[maxx] = extremum(extremum(c[maxx-1],c[maxx]),extremum(n[maxx-1],n[maxx]));       \
         for (x=1;x<maxx;x++)                                                               \
            o[x] = extremum(extremum(extremum(c[x-1],c[x]),extremum(n[x-1],n[x])),          \
                   extremum(c[x+1],n[x+1]));                                                \
      } else if ( y == maxy) {                                                              \
         o[0] = extremum(extremum(c[0],c[1]),extremum(p[0],p[1]));                          \
         o[maxx] = extremum(extremum(c[maxx-1],c[maxx]),extremum(p[maxx-1],p[maxx]));       \
         for (x=1;x<maxx;x++)                                                               \
            o[x] = extremum(extremum(extremum(c[x-1],c[x]),extremum(p[x-1],p[x])),          \
                   extremum(p[x+1],p[x+1]));                                                \
      } else {                                                                              \
         o[0] = extremum(extremum(extremum(p[0],p[1]),extremum(c[0],c[1])),                 \
                extremum(n[0],n[1]));                                                       \
         o[maxx] = extremum(extremum(extremum(p[maxx-1],p[maxx]),                           \
                   extremum(c[maxx-1],c[maxx])),extremum(n[maxx-1],n[maxx]));               \
         for (x=1;x<maxx;x++)                                                               \
            o[x] = extremum(extremum(extremum(extremum(p[x-1],p[x]),extremum(c[x-1],c[x])), \
                   extremum(extremum(n[x-1],n[x]),extremum(p[x+1],c[x+1]))),n[x+1]);        \
      }                                                                                     \
   }                                                                                        \
}

#define DILATEERODE4(extremum) {                                                            \
   CHAR *p, *c = NULL, *n = inp->d[0], *o;                                                  \
   for (y = 0; y < h; y++) {                                                                \
      o = out->d[y];                                                                        \
      p = c; c = n; n = y == maxy ? NULL : inp->d[y+1];                                     \
      if ( y == 0) {                                                                        \
         o[0] = extremum(extremum(c[0],c[1]),n[0]);                                         \
         o[maxx] = extremum(extremum(c[maxx-1],c[maxx]),n[maxx]);                           \
         for (x=1;x<maxx;x++)                                                               \
            o[x] = extremum(extremum(c[x-1],c[x]),extremum(c[x+1],n[x]));                   \
      } else if ( y == maxy) {                                                              \
         o[0] = extremum(extremum(c[0],c[1]),p[0]);                                         \
         o[maxx] = extremum(extremum(c[maxx-1],c[maxx]),p[maxx]);                           \
         for (x=1;x<maxx;x++)                                                               \
            o[x] = extremum(extremum(c[x-1],c[x]),extremum(c[x+1],p[x]));                   \
      } else {                                                                              \
         o[0] = extremum(extremum(p[0],n[0]),extremum(c[0],c[1]));                          \
         o[maxx] = extremum(extremum(n[maxx],p[maxx]), extremum(c[maxx-1],c[maxx]));        \
         for (x=1;x<maxx;x++)                                                               \
            o[x] = extremum(extremum(extremum(p[x],n[x]),extremum(c[x-1],c[x+1])),c[x]);    \
      }                                                                                     \
   }                                                                                        \
}

struct image *
image_erode( struct image *inp, int neighborhood)
{
	struct image *out;
	int w, h, x, y, maxy, maxx;

	if ( neighborhood != 8 && neighborhood != 4)
		croakx(1, "image_erode: neighborhood must be 4 or 8");

	out = copy_image(inp);

	w = inp->w;   h = inp->h;
	maxx = w - 1;  maxy = h - 1;

   if ( w < 2 || h < 2)
      croakx(1, "image_erode: input image too small (should be at least 2x2)");

   if ( neighborhood == 8) {
	   DILATEERODE8(min);
   } else if ( neighborhood == 4) {
	   DILATEERODE4(min)
   }

   return out;
}

struct image *
image_dilate( struct image *inp, int neighborhood)
{
	struct image *out;
	int w, h, x, y, maxy, maxx;

	if ( neighborhood != 8 && neighborhood != 4)
		croakx(1, "image_dilate: neighborhood must be 4 or 8");

	out = copy_image(inp);

	w = inp->w;   h = inp->h;
	maxx = w - 1;  maxy = h - 1;

   if ( w < 2 || h < 2)
      croakx(1, "image_dilate: input image too small (should be at least 2x2)");

   if ( neighborhood == 8) {
	   DILATEERODE8(max);
   } else if ( neighborhood == 4) {
	   DILATEERODE4(max)
   }

   return out;
}

