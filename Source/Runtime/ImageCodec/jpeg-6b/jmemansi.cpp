/* jconfig.vc --- jconfig.h for Microsoft Visual C++ on Windows 95 or NT. */
/* see jconfig.doc for explanations */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jmemsys.h"		/* import the system-dependent declarations */

using namespace image_codec::jpg;

/*
 * Memory allocation and freeing are controlled by the regular library
 * routines malloc() and free().
 */

GLOBAL(void *)
jpeg_get_small (j_common_ptr cinfo, size_t sizeofobject)
{
	return malloc(sizeofobject);
}

GLOBAL(void)
jpeg_free_small (j_common_ptr cinfo, void * object, size_t sizeofobject)
{
  if (object) free(object);
}

/*
 * "Large" objects are treated the same as "small" ones.
 * NB: although we include FAR keywords in the routine declarations,
 * this file won't actually work in 80x86 small/medium model; at least,
 * you probably won't be able to process useful-size images in only 64KB.
 */

GLOBAL(void FAR *)
jpeg_get_large (j_common_ptr cinfo, size_t sizeofobject)
{
  return (void FAR *)malloc(sizeofobject);
}

GLOBAL(void)
jpeg_free_large (j_common_ptr cinfo, void FAR * object, size_t sizeofobject)
{
  if (object) free(object);
}

GLOBAL(int)
jpeg_mem_available (j_common_ptr cinfo, int min_bytes_needed,
		    int max_bytes_needed, int already_allocated)
{
	return 0x7fffffff;
}

/*
 * Backing store (temporary file) management.
 * Backing store objects are only used when the value returned by
 * jpeg_mem_available is less than the total space needed.  You can dispense
 * with these routines if you have plenty of virtual memory; see jmemnobs.c.
 */

static void
read_backing_store (j_common_ptr cinfo, backing_store_ptr info,
		    void FAR * buffer_address,
		    int file_offset, int byte_count)
{
}

static void
write_backing_store (j_common_ptr cinfo, backing_store_ptr info,
		     void FAR * buffer_address,
		     int file_offset, int byte_count)
{
}


static void
close_backing_store (j_common_ptr cinfo, backing_store_ptr info)
{
}


/*
 * Initial opening of a backing-store object.
 *
 * This version uses tmpfile(), which constructs a suitable file name
 * behind the scenes.  We don't have to use info->temp_name[] at all;
 * indeed, we can't even find out the actual name of the temp file.
 */

GLOBAL(void)
jpeg_open_backing_store (j_common_ptr cinfo, backing_store_ptr info,
			 int total_bytes_needed)
{
	RAD_FAIL("Unsupported Function!");
}


/*
 * These routines take care of any system-dependent initialization and
 * cleanup required.
 */

GLOBAL(int)
jpeg_mem_init (j_common_ptr cinfo)
{
  return 0x7fffffff;	/* default for max_memory_to_use */
}

GLOBAL(void)
jpeg_mem_term (j_common_ptr cinfo)
{
  /* no work */
}
