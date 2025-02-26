#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>

__typeof__( ::write ) *Rwrite;

extern "C" ssize_t write( int fd, const void *buf, size_t count ) {
	ssize_t retcode;
	if ( ! Rwrite)
	    Rwrite = (__typeof__(write) *)dlvsym(RTLD_NEXT, "write", "GLIBC_2.2.5");

	for ( ;; ) {
		retcode = Rwrite( fd, buf, count ); // call the real routine
		if ( retcode != -1 || errno != EINTR ) break;	// timer interrupt ?
	} // for
	return retcode;
} // write

// g++ -shared -ldl -fPIC -g write.cc -o libwrite.so
// http://www.jayconrod.com/cgi/view_post.py?23
