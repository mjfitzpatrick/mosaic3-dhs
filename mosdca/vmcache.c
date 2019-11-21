#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <ctype.h>
#include <fcntl.h>
#include "vmcache.h"

#ifdef sun
#ifndef MS_SYNC
#define MS_SYNC 0  /* SunOS */
#else
#include <sys/systeminfo.h>
#endif
#endif

/*
 * Virtual Memory Cache Controller
 *
 * The VM cache controller manages a region of physical memory in the host
 * computer.  Entire files or file segments are loaded into the cache (into
 * memory).  Space to store such files is made available by the cache
 * controller by freeing the least recently used file segments.  This explicit
 * freeing of space immediately before it is reused for new data prevents
 * (in most cases) the kernel reclaim page daemon from running, causing cached
 * data to remain in memory until freed, and preventing the flow of data
 * through the cache from causing the system to page heavily and steal pages
 * away from the region of memory outside the cache.
 *
 *	      vm = vm_initcache (vm|NULL, initstr)
 *		  vm_closecache (vm)
 *
 *		   vm_cachefile (vm, fname, flags)
 *		     vm_cachefd (vm, fd, acmode, flags)
 *		 vm_uncachefile (vm, fname, flags)
 *		   vm_uncachefd (vm, fd, flags)
 *
 *		vm_reservespace (vm, nbytes)
 * 	  addr = vm_cacheregion (vm, fd, offset, nbytes, acmode, flags)
 *	       vm_uncacheregion (vm, fd, offset, nbytes, flags)
 * 	       vm_refreshregion (vm, fd, offset, nbytes)
 *
 *		        vm_sync (vm, fd, offset, nbytes, flags)
 *		       vm_msync (vm, addr, nbytes, flags)
 *
 * Before the VM cache is used it should be initialized with vm_initcache.
 * The string "initstr" may be used to set the size of the cache, enable
 * or disable it (e.g. for performance tests), and set other options.
 *
 * Files or file segments are loaded into the cache with routines such as
 * vm_cachefile and vm_cacheregion.  Normally, cached files or file segments
 * are reused on a least-recently-used basis.  A file can be locked in the
 * cache by setting the VM_LOCKFILE flag when the file is cached.  This is
 * automatic for vm_cacheregion since the address at which the file is
 * mapped is returned to the caller and hence the file is assumed to be in
 * use.  When a file or region which is locked in the cache is no longer
 * needed one of the "uncache" routines should be called to make the space
 * used by the cached file data available for reuse.  Note that "uncaching"
 * a file or file segment does not immediately remove the data from the
 * cache.  Any "uncached" data normally remains in the cache until the
 * space it uses is needed to load other data.
 *
 * The current version of VMcache is a library which is compiled into a
 * process.  In this case the cache controller really only controls the
 * memory utilization by that one process.  To serve a multiprocessing
 * environment, VMcache would need to be broken up into client and server
 * components, with a VMcache server managing the cache globally for a
 * cooperating collection of processes.  Although the current version of
 * VMcache does not support this, the interface is intended to be able to
 * support either type of facility.
 */


#define	DEF_CACHESIZE	"50%"
#define	DEF_PHYSPAGES	32768
#define READAHEAD	32768
#define	SZ_NAME		64
#define	SZ_VALSTR	64

/* Solaris and FreeBSD have a madvise() system call. */
#define HAVE_MADVISE	1

/* Linux provides a madvise call, but it is not implemented and produces
 * a linker warning message.  The madvise call will always fail, but this
 * is harmless (it just means that the cache fails to control paging and
 * everything operates "normally".
 */
#ifdef linux
#undef  HAVE_MADVISE
#define MADV_WILLNEED   3               /* will need these pages */
#define MADV_DONTNEED   4               /* don't need these page */
#endif


/* Describes a segment of memory in the cache. */
struct segment {
	struct segment *next;
	struct segment *prev;
	int refcnt;
	void *addr;
	int fd;
	unsigned long inode;
	unsigned long device;
	unsigned long offset;
	unsigned long nbytes;
};

typedef struct segment Segment;

/* Main VMcache descriptor. */
struct vmcache {
	Segment *segment_head, *segment_tail;
	int cache_initialized;
	int cache_enabled;
	unsigned long cacheused;
	unsigned long cachesize;
	unsigned long physmem;
	int lockpages;
	int pagesize;
};

typedef struct vmcache VMcache;
static VMcache vmcache;
static debug = 0;

static vm_readahead();
static vm_uncache();


/* VM_INITCACHE -- Initialize the VM cache.  A pointer to the cache 
 * descriptor is returned as the function value, or NULL if the cache cannot
 * be initialized.  The argument VM may point to an existing cache which
 * is to be reinitialized, or may be NULL if the cache is being initialized
 * for the first time.
 *
 * The INITSTR argument is used to control all init-time cache options.
 * INITSTR is a sequence of keyword=value substrings.  The recognized options
 * are as follows:
 *
 *	cachesize	total cache size
 *	lockpages	lock pages in memory
 *	enable		enable the cache
 *	debug		turn on debug messages
 *	
 * Other options may be added in the future.
 *
 * Keywords which take a size type value (e.g. cachesize) permit values
 * such as "x" (size in bytes), "x%" (X percent of physical memory), "xK"
 * (X kilobytes), or "xM" (X megabytes).  The "x%" notation may not work
 * correctly on all systems as it is not always easy to determine the total
 * physical memory.
 *
 * If the cache is initialized with "enable=no" then all the cache routines
 * will still be called, the cache controller will be disabled.
 */
void *
vm_initcache (vm, initstr)
register VMcache *vm;
char *initstr;
{
	register char *ip, *op;
	char keyword[SZ_NAME], valstr[SZ_NAME];
	char cachesize[SZ_VALSTR], *modchar;
	int percent, enable = 1, lockpages = 0;
	unsigned long physpages;

	if (debug)
	    fprintf (stderr, "vm_initcache (0x%x, \"%s\")\n", vm, initstr);
	strcpy (cachesize, DEF_CACHESIZE);

	/* Scan the initialization string.  Initstr may be NULL or the empty
	 * string, if only the defaults are desired.
	 */
	for (ip=initstr;  ip && *ip;  ) {
	    /* Advance to the next keyword=value pair. */
	    while (*ip && (isspace(*ip) || *ip == ','))
		ip++;

	    /* Extract the keyword. */
	    for (op=keyword;  *ip && isalnum(*ip);  )
		*op++ = *ip++;
	    *op = '\0';

	    while (*ip && (isspace(*ip) || *ip == '='))
		ip++;

	    /* Extract the value string. */
	    for (op=valstr;  *ip && isalnum(*ip);  )
		*op++ = *ip++;
	    *op = '\0';

	    if (strcmp (keyword, "cachesize") == 0) {
		strcpy (cachesize, valstr);
	    } else if (strcmp (keyword, "lockpages") == 0) {
		int ch = valstr[0];
		lockpages = (ch == 'y' || ch == 'Y');
	    } else if (strcmp (keyword, "enable") == 0) {
		int ch = valstr[0];
		enable = (ch == 'y' || ch == 'Y');
	    } else if (strcmp (keyword, "debug") == 0) {
		int ch = valstr[0];
		debug = (ch == 'y' || ch == 'Y');
	    }
	}

	/* The VM cache needs to be global for a given host, so we just
	 * use a statically allocated cache descriptor here.  In the most
	 * general case the whole VMcache interface needs to be split into
	 * a client-server configuration, with the cache server managing
	 * virtual memory for a collection of processes.
	 */
	if (!vm)
	    vm = &vmcache;

	/* Shut down the old cache if already enabled. */
	vm_closecache (vm);

	/* There is no good way to guess the total physical memory if this
	 * is not available from the system.  But in such a case the user
	 * can just set the value of the cachesize explicitly in the initstr.
	 */
#ifdef _SC_PHYS_PAGES
	physpages = sysconf (_SC_PHYS_PAGES);
	if (debug) {
	    fprintf (stderr, "total physical memory %d (%dm)\n",
		physpages * getpagesize(),
		physpages * getpagesize() / (1024 * 1024));
	}
#else
	physpages = DEF_PHYSPAGES;
#endif

	vm->cacheused = 0;
	vm->cache_enabled = enable;
	vm->cache_initialized = 1;
	vm->segment_head = NULL;
	vm->segment_tail = NULL;
	vm->pagesize = getpagesize();
	vm->physmem = physpages * vm->pagesize;
	vm->lockpages = lockpages;

	vm->cachesize = percent = strtol (cachesize, &modchar, 10);
	if (modchar == cachesize)
	    vm->cachesize = physpages / 2 * vm->pagesize;
	else if (*modchar == '%')
	    vm->cachesize = physpages * percent / 100 * vm->pagesize;
	else if (*modchar == 'k' || *modchar == 'K')
	    vm->cachesize *= 1024;
	else if (*modchar == 'm' || *modchar == 'M')
	    vm->cachesize *= (1024 * 1024);
	else if (*modchar == 'g' || *modchar == 'G')
	    vm->cachesize *= (1024 * 1024 * 1024);

	return ((void *)vm);
}


/* VM_CLOSECACHE -- Forcibly shutdown a cache if it is already open.
 * All segments are freed and returned to the system.  An attempt is made
 * to close any open files (this is the only case where the VM cache code
 * closes files opened by the caller).
 */
vm_closecache (vm)
register VMcache *vm;
{
	register Segment *sp;
	struct stat st;

	if (debug)
	    fprintf (stderr, "vm_closecache (0x%x)\n", vm);
	if (!vm->cache_initialized)
	    return;

	/* Free successive segments at the head of the cache list until the
	 * list is empty.
	 */
	while (sp = vm->segment_head) {
	    vm_uncache (vm, sp, VM_DESTROYREGION | VM_CANCELREFCNT);

	    /* Since we are closing the cache attempt to forcibly close the
	     * associated file descriptor if it refers to an open file.
	     * Make sure that FD refers to the correct file.
	     */
	    if (fstat (sp->fd, &st) == 0)
		if (sp->inode == st.st_ino && sp->device == st.st_dev)
		    close (sp->fd);
	}

	vm->cache_initialized = 0;
}


/* VM_CACHEFILE -- Cache an entire named file in the VM cache.
 */
vm_cachefile (vm, fname, flags)
register VMcache *vm;
char *fname;
int flags;
{
	struct stat st;
	int fd;

	if (debug)
	    fprintf (stderr, "vm_cachefile (0x%x, \"%s\", 0%o)\n",
		vm, fname, flags);
	if (!vm->cache_enabled)
	    return (0);

	if ((fd = open (fname, O_RDONLY)) < 0)
	    return (-1);
	if (fstat (fd, &st) < 0)
	    return (-1);

	if (!vm_cacheregion (vm, fd, 0L, st.st_size, VM_READONLY, 0)) {
	    close (fd);
	    return (-1);
	}

	close (fd);
	if (!(flags & VM_LOCKFILE))
	    vm_uncachefile (vm, fname, 0);

	return (0);
}


/* VM_CACHEFD -- Cache an already open file in the VM cache.
 */
vm_cachefd (vm, fd, acmode, flags)
register VMcache *vm;
int acmode;
int flags;
{
	struct stat st;

	if (debug)
	    fprintf (stderr, "vm_cachefd (0x%x, %d, 0%o, 0%o)\n",
		vm, fd, acmode, flags);
	if (!vm->cache_enabled)
	    return (0);

	if (fstat (fd, &st) < 0)
	    return (-1);

	if (!vm_cacheregion (vm, fd, 0L, st.st_size, acmode, flags))
	    return (-1);

	if (!(flags & VM_LOCKFILE))
	    vm_uncachefd (vm, fd, 0);

	return (0);
}


/* VM_UNCACHEFILE -- Identify a cached file as ready for reuse.  The file
 * remains in the cache, but its space is available for reuse on a least
 * recently used basis.  If it is desired to immediately free the space used
 * by cached file immediately the VM_DESTROYREGION flag may be set in FLAGS.
 */
vm_uncachefile (vm, fname, flags)
register VMcache *vm;
char *fname;
int flags;
{
	register Segment *sp;
	struct stat st;
	int status = 0;

	if (debug)
	    fprintf (stderr, "vm_uncachefile (0x%x, \"%s\", 0%o)\n",
		vm, fname, flags);
	if (!vm->cache_enabled)
	    return (0);

	if (stat (fname, &st) < 0)
	    return (-1);

	for (sp = vm->segment_head;  sp;  sp = sp->next) {
	    if (sp->inode != st.st_ino || sp->device != st.st_dev)
		continue;
	    if (vm_uncache (vm, sp, flags) < 0)
		status = -1;
	}

	return (status);
}


/* VM_UNCACHEFD -- Uncache an entire file identified by its file descriptor.
 * The file remains in the cache, but its space is available for reuse on a
 * least recently used basis.  If it is desired to immediately free the space
 * used by cached file immediately the VM_DESTROYREGION flag may be set in
 * FLAGS.
 */
vm_uncachefd (vm, fd, flags)
register VMcache *vm;
int fd;
int flags;
{
	register Segment *sp;
	struct stat st;
	int status = 0;

	if (debug)
	    fprintf (stderr, "vm_uncachefd (0x%x, %d, 0%o)\n",
		vm, fd, flags);
	if (!vm->cache_enabled)
	    return (0);

	if (fstat (fd, &st) < 0)
	    return (-1);

	for (sp = vm->segment_head;  sp;  sp = sp->next) {
	    if (sp->inode != st.st_ino || sp->device != st.st_dev)
		continue;
	    if (vm_uncache (vm, sp, flags) < 0)
		status = -1;
	}

	return (status);
}


/* VM_CACHEREGION -- Cache a region or segment of a file.  File segments are
 * removed from the tail of the LRU cache list until sufficient space is 
 * available for the new segment.  The new file segment is then mapped and a
 * request is issued to asynchronously read in the file data.  The virtual
 * memory address of the cached and mapped region is returned.
 *
 * File segments may be redundantly cached in which case the existing
 * mapping is refreshed and the segment is moved to the head of the cache.
 * Each cache operation increments the reference count of the region and
 * a matching uncache is required to eventually return the reference count
 * to zero allowing the space to be reused.  vm_refreshregion can be called
 * instead of cacheregion if all that is desired is to refresh the mapping
 * and move the cached region to the head of the cache.  A single file may
 * be cached as multiple segments but the segments must be page aligned
 * and must not overlap.  The virtual memory addresses of independent segments
 * may not be contiguous in virtual memory even though the corresponding
 * file regions are.  If a new segment overlaps an existing segment it must
 * fall within the existing segment as the size of a segment cannot be changed
 * once it is created.  If a file is expected to grow in size after it is
 * cached, the size of the cached region must be at least as large as the
 * expected size of the file.
 *
 * vm_cacheregion can (should) be used instead of MMAP to map files into
 * memory, if the files will be managed by the VM cache controller.  Otherwise
 * the same file may be mapped twice by the same process, which may use
 * extra virtual memory.  Only files can be mapped using vm_cacheregion, and
 * all mappings are for shared data.
 *
 * If the cache is disabled vm_cacheregion will still map file segments into
 * memory, and vm_uncacheregion will unmap them when the reference count goes
 * to zero (regardless of whether the VM_DESTROYREGION flag is set if the
 * cache is disabled).
 *
 * If write access to a segment is desired the file referenced by FD must
 * have already been opened with write permission.
 */
void *
vm_cacheregion (vm, fd, offset, nbytes, acmode, flags)
register VMcache *vm;
int fd;
unsigned long offset;
unsigned long nbytes;
int acmode, flags;
{
	register Segment *sp;
	unsigned long x0, x1, vm_offset, vm_nbytes;
	struct stat st;
	int mode;
	void *addr;

	if (debug)
	    fprintf (stderr, "vm_cacheregion (0x%x, %d, %d, %d, 0%o, 0%o)\n",
		vm, fd, offset, nbytes, acmode, flags);
	if (fstat (fd, &st) < 0)
	    return (NULL);

	/* Align offset,nbytes to fill the referenced memory pages.
	 */
	x0 = offset;
	x0 = (x0 - (x0 % vm->pagesize));

	x1 = offset + nbytes - 1;
	x1 = (x1 - (x1 % vm->pagesize)) + vm->pagesize - 1;

	vm_offset = x0;
	vm_nbytes = x1 - x0 + 1;

	/* Is this a reference to an already cached segment?
	 */
	for (sp = vm->segment_head;  sp;  sp = sp->next) {
	    if (sp->inode != st.st_ino || sp->device != st.st_dev)
		continue;

	    if (x0 >= sp->offset && x0 < (sp->offset + sp->nbytes))
		if (x1 >= sp->offset && x1 < (sp->offset + sp->nbytes)) {
		    /* New segment lies entirely within an existing one. */
		    vm_offset = sp->offset;
		    vm_nbytes = sp->nbytes;
		    goto refresh;
		} else {
		    /* New segment extends an existing one. */
		    return (NULL);
		}
	}

	/* Free sufficient memory pages for the new mapping.  */
	vm_reservespace (vm, vm_nbytes);

	mode = PROT_READ;
	if (acmode == VM_READWRITE)
	    mode |= PROT_WRITE;

	/* Map the new segment, reusing the VM pages freed above. */
	addr = mmap (NULL,
	    (size_t)vm_nbytes, mode, MAP_SHARED, fd, (off_t)vm_offset);
	if (!addr)
	    return (NULL);

	/* Lock segment in memory if indicated. */
	if (vm->lockpages && vm->cache_enabled)
	    mlock (addr, (size_t) vm_nbytes);

	/* Get a segment descriptor for the new segment. */
	if (!(sp = (Segment *) calloc (1, sizeof(Segment)))) {
	    munmap (addr, vm_nbytes);
	    return (NULL);
	}

	sp->fd = fd;
	sp->inode = st.st_ino;
	sp->device = st.st_dev;
	sp->offset = vm_offset;
	sp->nbytes = vm_nbytes;
	sp->addr = addr;
	sp->refcnt = 0;

	/* Set up the new segment at the head of the cache. */
	sp->next = vm->segment_head;
	sp->prev = NULL;
	if (vm->segment_head)
	    vm->segment_head->prev = sp;
	vm->segment_head = sp;
	vm->cacheused += vm_nbytes;

	/* If there is nothing at the tail of the cache yet this element
	 * becomes the tail of the cache list.
	 */
	if (!vm->segment_tail)
	    vm->segment_tail = sp;

refresh:
	/* Move a new or existing segment to the head of the cache and
	 * increment the reference count.  Refresh the segment pages if
	 * indicated.
	 */
	if (vm->segment_head != sp) {
	    /* Unlink the list element. */
	    if (sp->next)
		sp->next->prev = sp->prev;
	    if (sp->prev)
		sp->prev->next = sp->next;

	    /* Link current segment at head of cache. */
	    sp->next = vm->segment_head;
	    sp->prev = NULL;
	    if (vm->segment_head)
		vm->segment_head->prev = sp;
	    vm->segment_head = sp;

	    if (!vm->segment_tail)
		vm->segment_tail = sp;
	}

	/* Preload the referenced segment if indicated. */
	if (vm->cache_enabled)
	    vm_readahead (vm, addr, vm_nbytes);

	sp->refcnt++;
	return ((void *)((char *)addr + (offset - vm_offset)));
}


/* VM_UNCACHEREGION -- Called after a vm_cacheregion to indicate that the
 * cached region is available for reuse.  For every call to vm_cacheregion
 * there must be a corresponding call to vm_uncacheregion before the space
 * used by the region can be reused.  Uncaching a region does not immediately
 * free the space used by the region, it merely decrements a reference 
 * count so that the region can later be freed and reused if its space is
 * needed.  The region remains in the cache and can be immediately reclaimed
 * by a subequent vm_cacheregion.  If it is known that the space will not
 * be reused, it can be freed immediately by setting the VM_DESTROYREGION
 * flag in FLAGS.
 */
vm_uncacheregion (vm, fd, offset, nbytes, flags)
register VMcache *vm;
int fd;
unsigned long offset;
unsigned long nbytes;
int flags;
{
	register Segment *sp;
	unsigned long x0, x1, vm_offset, vm_nbytes;
	struct stat st;
	int mode;

	if (debug)
	    fprintf (stderr, "vm_uncacheregion (0x%x, %d, %d, %d, 0%o)\n",
		vm, fd, offset, nbytes, flags);

	/* Map offset,nbytes to a range of memory pages.
	 */
	x0 = offset;
	x0 = (x0 - (x0 % vm->pagesize));

	x1 = offset + nbytes - 1;
	x1 = (x1 - (x1 % vm->pagesize)) + vm->pagesize - 1;

	vm_offset = x0;
	vm_nbytes = x1 - x0 + 1;

	if (fstat (fd, &st) < 0)
	    return (-1);

	/* Locate the referenced segment.  */
	for (sp = vm->segment_head;  sp;  sp = sp->next)
	    if (sp->inode == st.st_ino && sp->device == st.st_dev)
		if (sp->offset == vm_offset)
		    break;
	if (!sp)
	    return (-1);   /* not found */

	return (vm_uncache (vm, sp, flags));
}


/* VM_UNCACHE -- Internal routine to free a cache segment.
 */
static
vm_uncache (vm, sp, flags)
register VMcache *vm;
register Segment *sp;
int flags;
{
	int status=0, mode;

	if (debug)
	    fprintf (stderr, "vm_uncache (0x%x, 0x%x, 0%o)\n", vm, sp, flags);

	/* Decrement the reference count.  Setting VM_CANCELREFCNT (as in
	 * closecache) causes any references to be ignored.
	 */
	if (--sp->refcnt < 0 || (flags & VM_CANCELREFCNT))
	    sp->refcnt = 0;

	/* If the reference count is zero and the VM_DESTROYREGION flag is
	 * set, try to free up the pages immediately, otherwise merely
	 * decrement the reference count so that it can be reused if it is
	 * referenced before the space it uses is reclaimed by another cache
	 * load.
	 */
	if (!sp->refcnt && ((flags & VM_DESTROYREGION) || !vm->cache_enabled)) {
	    if (vm->cache_enabled)
		madvise (sp->addr, sp->nbytes, MADV_DONTNEED);
	    if (munmap (sp->addr, sp->nbytes) < 0)
		status = -1;
	    vm->cacheused -= sp->nbytes;

	    /* Unlink and free the segment descriptor. */
	    if (sp->next)
		sp->next->prev = sp->prev;
	    if (sp->prev)
		sp->prev->next = sp->next;
	    if (vm->segment_head == sp)
		vm->segment_head = sp->next;
	    if (vm->segment_tail == sp)
		vm->segment_tail = sp->prev;

	    free ((void *)sp);
	}

	return (status);
}


/* VM_REFRESHREGION -- Refresh an already cached file region.  The region is
 * moved to the head of the cache and preloading of any non-memory resident
 * pages is initiated.
 */
vm_refreshregion (vm, fd, offset, nbytes)
register VMcache *vm;
int fd;
unsigned long offset;
unsigned long nbytes;
{
	register Segment *sp;
	unsigned long x0, x1, vm_offset, vm_nbytes;
	struct stat st;
	int mode;
	void *addr;

	if (debug)
	    fprintf (stderr, "vm_refreshregion (0x%x, %d, %d, %d)\n",
		vm, fd, offset, nbytes);

	if (!vm->cache_enabled)
	    return (0);

	/* Map offset,nbytes to a range of memory pages.
	 */
	x0 = offset;
	x0 = (x0 - (x0 % vm->pagesize));

	x1 = offset + nbytes - 1;
	x1 = (x1 - (x1 % vm->pagesize)) + vm->pagesize - 1;

	vm_offset = x0;
	vm_nbytes = x1 - x0 + 1;

	if (fstat (fd, &st) < 0)
	    return (-1);

	/* Locate the referenced segment.  */
	for (sp = vm->segment_head;  sp;  sp = sp->next)
	    if (sp->inode == st.st_ino && sp->device == st.st_dev)
		if (sp->offset == vm_offset)
		    break;
	if (!sp)
	    return (-1);   /* not found */

	/* Relink the segment at the head of the cache.
	 */
	if (vm->segment_head != sp) {
	    /* Unlink the list element. */
	    if (sp->next)
		sp->next->prev = sp->prev;
	    if (sp->prev)
		sp->prev->next = sp->next;

	    /* Link current segment at head of cache. */
	    sp->next = vm->segment_head;
	    sp->prev = NULL;
	    if (vm->segment_head)
		vm->segment_head->prev = sp;
	    vm->segment_head = sp;
	}

	/* Preload any missing pages from the referenced segment. */
	madvise (addr, vm_nbytes, MADV_WILLNEED);

	return (0);
}


/* VM_RESERVESPACE -- Free space in the cache, e.g. to create space to cache
 * a new file or file segment.  File segments are freed at the tail of the
 * cache list until the requested space is available.  Only segments which 
 * have a reference count of zero are freed.
 */
vm_reservespace (vm, nbytes)
register VMcache *vm;
unsigned long nbytes;
{
	register Segment *sp;
	unsigned long freespace;

	if (debug)
	    fprintf (stderr, "vm_reservespace (0x%x, %d)\n", vm, nbytes);

	if (!vm->cache_enabled)
	    return (0);

	freespace = vm->cachesize - vm->cacheused;

	for (sp = vm->segment_tail;  sp;  sp = sp->prev) {
	    if (freespace > nbytes)
		break;
	    if (sp->refcnt)
		continue;

	    if (debug)
		fprintf (stderr, "vm_reservespace: free %d bytes at 0x%x\n",
		    sp->nbytes, sp->addr);

	    madvise (sp->addr, sp->nbytes, MADV_DONTNEED);
	    munmap (sp->addr, sp->nbytes);
	    vm->cacheused -= sp->nbytes;
	    freespace = vm->cachesize - vm->cacheused;

	    /* Unlink and free the segment descriptor. */
	    if (sp->next)
		sp->next->prev = sp->prev;
	    if (sp->prev)
		sp->prev->next = sp->next;
	    if (vm->segment_head == sp)
		vm->segment_head = sp->next;
	    if (vm->segment_tail == sp)
		vm->segment_tail = sp->prev;

	    free ((void *)sp);
	}

	return ((freespace >= nbytes) ? 0 : -1);
}


/* VM_SYNC -- Sync (update on disk) any pages of virtual memory mapped to
 * the given region of the given file.  If nbytes=0, any mapped regions of
 * the given file are synced.  If the VM_ASYNC flag is set the sync operation
 * will be performed asynchronously and vm_sync will return immediately,
 * otherwise vm_sync waits for the synchronization operation to complete.
 */
vm_sync (vm, fd, offset, nbytes, flags)
register VMcache *vm;
int fd;
unsigned long offset;
unsigned long nbytes;
int flags;
{
	register Segment *sp;
	unsigned long x0, x1, vm_offset, vm_nbytes;
	int syncflag, status = 0;
	struct stat st;

	if (debug)
	    fprintf (stderr, "vm_sync (0x%x, %d, %d, %d, 0%o)\n",
		vm, fd, offset, nbytes, flags);
	if (!vm->cache_enabled)
	    return (0);

	/* Map offset,nbytes to a range of memory pages.
	 */
	x0 = offset;
	x0 = (x0 - (x0 % vm->pagesize));

	x1 = offset + nbytes - 1;
	x1 = (x1 - (x1 % vm->pagesize)) + vm->pagesize - 1;

	vm_offset = x0;
	vm_nbytes = x1 - x0 + 1;

#ifdef sun
#ifdef _SYS_SYSTEMINFO_H
	/* This is a mess.  The values of MS_SYNC,MS_ASYNC changed between
	 * Solaris 2.6 and 2.7.  This code assumes that the system is
	 * being built on a Solaris 2.7 or greater system, but the wired-in
	 * values below allow the executable to be run on earlier versions.
	 */
	{
	    char buf[SZ_NAME];   /* e.g. "5.7" */

	    sysinfo (SI_RELEASE, buf, SZ_NAME);
	    if (buf[0] >= '5' && buf[2] >= '7')
		syncflag = (flags & VM_ASYNC) ? MS_ASYNC : MS_SYNC;
	    else
		syncflag = (flags & VM_ASYNC) ? 0x1 : 0x0;
	}
#else
	syncflag = (flags & VM_ASYNC) ? MS_ASYNC : MS_SYNC;
#endif
#else
	syncflag = (flags & VM_ASYNC) ? MS_ASYNC : MS_SYNC;
#endif

	if (fstat (fd, &st) < 0)
	    return (-1);

	/* Locate the referenced segment.  */
	for (sp = vm->segment_head;  sp;  sp = sp->next) {
	    if (sp->inode != st.st_ino || sp->device != st.st_dev)
		continue;

	    if (!nbytes || sp->offset == vm_offset)
		if (msync (sp->addr, sp->nbytes, syncflag))
		    status = -1;
	}

	return (status);
}


/* VM_MSYNC -- Sync the given region of virtual memory.  This routine does
 * not require that the caller know the file to which the memory is mapped.
 * If the VM_ASYNC flag is set the sync operation will be performed
 * asynchronously and vm_sync will return immediately, therwise vm_sync waits
 * for the synchronization operation to complete.
 */
vm_msync (vm, addr, nbytes, flags)
register VMcache *vm;
void *addr;
unsigned long nbytes;
int flags;
{
	register Segment *sp;
	unsigned long addr1, addr2;
	int syncflag;

	if (debug)
	    fprintf (stderr, "vm_msync (0x%x, 0x%x, %d, 0%o)\n",
		vm, addr, nbytes, flags);

	/* Align the given address region to the page boundaries.
	 */
	addr1 = ((long)addr - ((long)addr % vm->pagesize));
	addr2 = (long)addr + nbytes - 1;
	addr2 = (addr2 - (addr2 % vm->pagesize)) + vm->pagesize - 1;
	syncflag = (flags & VM_ASYNC) ? MS_ASYNC : MS_SYNC;

	return (msync ((void *)addr1, addr2 - addr1 + 1, syncflag));
}


/* VM_READAHEAD -- Internal routine used to request that a segment of file
 * data be preloaded.
 */
static
vm_readahead (vm, addr, nbytes)
register VMcache *vm;
void *addr;
unsigned long nbytes;
{
	register int n, nb;
	int chunk = READAHEAD * vm->pagesize;
	unsigned long buf = (unsigned long) addr;

	/* Break large reads into chunks of READAHEAD memory pages.  This
	 * increases the chance that file access and computation can overlap
	 * the readahead i/o.
	 */
	for (n=0;  n < nbytes;  n += chunk) {
	    nb = nbytes - n;
	    if (nb > chunk)
		nb = chunk;
	    madvise ((void *)(buf + n), nb, MADV_WILLNEED);
	}
}
