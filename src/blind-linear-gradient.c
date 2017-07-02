/* See LICENSE file for copyright and license details. */
#include "common.h"

USAGE("[-b] -w width -h height")

static int bilinear = 0;
static size_t width = 0;
static size_t height = 0;


#define PROCESS(TYPE, SUFFIX)\
	static void\
	process_##SUFFIX(struct stream *stream)\
	{\
		typedef TYPE pixel_t[4];\
		pixel_t buf[BUFSIZ / sizeof(pixel_t)];\
		TYPE *params, x1, y1, x2, y2, norm2;\
		TYPE x, y;\
		size_t ix, iy, ptr = 0;\
		for (;;) {\
			while (stream->ptr < stream->frame_size) {\
				if (!eread_stream(stream, stream->frame_size - stream->ptr)) {\
					ewriteall(STDOUT_FILENO, buf, ptr * sizeof(*buf), "<stdout>");\
					return;\
				}\
			}\
			params = (TYPE *)stream->buf;\
			x1 = (params)[0];\
			y1 = (params)[1];\
			x2 = (params)[4];\
			y2 = (params)[5];\
			memmove(stream->buf, stream->buf + stream->frame_size,\
				stream->ptr -= stream->frame_size);\
			\
			x2 -= x1;\
			y2 -= y1;\
			norm2 = x2 * x2 + y2 * y2;\
			\
			for (iy = 0; iy < height; iy++) {\
				y = (TYPE)iy - y1;\
				for (ix = 0; ix < width; ix++) {\
					x = (TYPE)ix - x1;\
					x = (x * x2 + y * y2) / norm2;\
					if (bilinear)\
						x = abs(x);\
					buf[ptr][0] = buf[ptr][1] = buf[ptr][2] = buf[ptr][3] = x;\
					if (++ptr == ELEMENTSOF(buf)) {\
						ewriteall(STDOUT_FILENO, buf, sizeof(buf), "<stdout>");\
						ptr = 0;\
					}\
				}\
			}\
		}\
	}

PROCESS(double, lf)
PROCESS(float, f)


int
main(int argc, char *argv[])
{
	struct stream stream;
	void (*process)(struct stream *stream);

	ARGBEGIN {
	case 'b':
		bilinear = 1;
		break;
	case 'w':
		width = etozu_flag('w', UARGF(), 1, SIZE_MAX);
		break;
	case 'h':
		height = etozu_flag('h', UARGF(), 1, SIZE_MAX);
		break;
	default:
		usage();
	} ARGEND;

	if (!width || !height || argc)
		usage();

	eopen_stream(&stream, NULL);

	if (!strcmp(stream.pixfmt, "xyza"))
		process = process_lf;
	else if (!strcmp(stream.pixfmt, "xyza f"))
		process = process_f;
	else
		eprintf("pixel format %s is not supported, try xyza\n", stream.pixfmt);

	if (stream.width > 2 || stream.height > 2 || stream.width * stream.height != 2)
		eprintf("<stdin>: each frame must contain exactly 2 pixels\n");

	stream.width = width;
	stream.height = height;
	fprint_stream_head(stdout, &stream);
	efflush(stdout, "<stdout>");
	process(&stream);
	return 0;
}
