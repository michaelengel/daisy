/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <sys/errno.h>

#define NUM_ERRNO_VALS		125

char *errno_text[NUM_ERRNO_VALS];

/************************************************************************
 *									*
 *				init_errno_text				*
 *				---------------				*
 *									*
 ************************************************************************/

init_errno_text ()
{
   int i;

   /* Some values are not defined in AIX.  Give them NULL strings */
   for (i = 0; i < NUM_ERRNO_VALS; i++)
      errno_text[i] = "";

   errno_text[EPERM]		= "Operation not permitted";
   errno_text[ENOENT]		= "No such file or directory";
   errno_text[ESRCH]		= "No such process";
   errno_text[EINTR]		= "interrupted system call";
   errno_text[EIO]		= "I/O error";
   errno_text[ENXIO]		= "No such device or address";
   errno_text[E2BIG]		= "Arg list too long";
   errno_text[ENOEXEC]		= "Exec format error";
   errno_text[EBADF]		= "Bad file descriptor";
   errno_text[ECHILD]		= "No child processes";
   errno_text[EAGAIN]		= "Resource temporarily unavailable";
   errno_text[ENOMEM]		= "Not enough space";
   errno_text[EACCES]		= "Permission denied";
   errno_text[EFAULT]		= "Bad address";
   errno_text[ENOTBLK]		= "Block device required";
   errno_text[EBUSY]		= "Resource busy";
   errno_text[EEXIST]		= "File exists";
   errno_text[EXDEV]		= "Improper link";
   errno_text[ENODEV]		= "No such device";
   errno_text[ENOTDIR]		= "Not a directory";
   errno_text[EISDIR]		= "Is a directory";
   errno_text[EINVAL]		= "Invalid argument";
   errno_text[ENFILE]		= "Too many open files in system";
   errno_text[EMFILE]		= "Too many open files";
   errno_text[ENOTTY]		= "Inappropriate I/O control operation";
   errno_text[ETXTBSY]		= "Text file busy";
   errno_text[EFBIG]		= "File too large";
   errno_text[ENOSPC]		= "No space left on device";
   errno_text[ESPIPE]		= "Invalid seek";
   errno_text[EROFS]		= "Read only file system";
   errno_text[EMLINK]		= "Too many links";
   errno_text[EPIPE]		= "Broken pipe";
   errno_text[EDOM]		= "Domain error within math function";
   errno_text[ERANGE]		= "Result too large";
   errno_text[ENOMSG]		= "No message of desired type";
   errno_text[EIDRM]		= "Identifier removed";
   errno_text[ECHRNG]		= "Channel number out of range";
   errno_text[EL2NSYNC]		= "Level 2 not synchronized";
   errno_text[EL3HLT]		= "Level 3 halted";
   errno_text[EL3RST]		= "Level 3 reset";
   errno_text[ELNRNG]		= "Link number out of range";
   errno_text[EUNATCH]		= "Protocol driver not attached";
   errno_text[ENOCSI]		= "No CSI structure available";
   errno_text[EL2HLT]		= "Level 2 halted";
   errno_text[EDEADLK]		= "Resource deadlock avoided";
   errno_text[ENOTREADY]	= "Device not ready";
   errno_text[EWRPROTECT]	= "Write-protected media";
   errno_text[EFORMAT]		= "Unformatted media";
   errno_text[ENOLCK]		= "No locks available";
   errno_text[ENOCONNECT]	= "no connection";
   errno_text[ESTALE]		= "no filesystem";
   errno_text[EDIST]		= "old, currently unused AIX errno";
   errno_text[EWOULDBLOCK]	= "Operation would block";
   errno_text[EINPROGRESS]	= "Operation now in progress";
   errno_text[EALREADY]		= "Operation already in progress";
   errno_text[ENOTSOCK]		= "Socket operation on non-socket";
   errno_text[EDESTADDRREQ]	= "Destination address required";
   errno_text[EMSGSIZE]		= "Message too long";
   errno_text[EPROTOTYPE]	= "Protocol wrong type for socket";
   errno_text[ENOPROTOOPT]	= "Protocol not available";
   errno_text[EPROTONOSUPPORT]	= "Protocol not supported";
   errno_text[ESOCKTNOSUPPORT]	= "Socket type not supported";
   errno_text[EOPNOTSUPP]	= "Operation not supported on socket";
   errno_text[EPFNOSUPPORT]	= "Protocol family not supported";
   errno_text[EAFNOSUPPORT]	= "Address family not supported by protocol family";
   errno_text[EADDRINUSE]	= "Address already in use";
   errno_text[EADDRNOTAVAIL]	= "Can't assign requested address";
   errno_text[ENETDOWN]		= "Network is down";
   errno_text[ENETUNREACH]	= "Network is unreachable";
   errno_text[ENETRESET]	= "Network dropped connection on reset";
   errno_text[ECONNABORTED]	= "Software caused connection abort";
   errno_text[ECONNRESET]	= "Connection reset by peer";
   errno_text[ENOBUFS]		= "No buffer space available";
   errno_text[EISCONN]		= "Socket is already connected";
   errno_text[ENOTCONN]		= "Socket is not connected";
   errno_text[ESHUTDOWN]	= "Can't send after socket shutdown";
   errno_text[ETIMEDOUT]	= "Connection timed out";
   errno_text[ECONNREFUSED]	= "Connection refused";
   errno_text[EHOSTDOWN]	= "Host is down";
   errno_text[EHOSTUNREACH]	= "No route to host";
   errno_text[ERESTART]		= "restart the system call";
   errno_text[EPROCLIM]		= "Too many processes";
   errno_text[EUSERS]		= "Too many users";
   errno_text[ELOOP]		= "Too many levels of symbolic links";
   errno_text[ENAMETOOLONG]	= "File name too long";
   errno_text[ENOTEMPTY]	= "Directory not empty";
   errno_text[EDQUOT]		= "Disc quota exceeded";
   errno_text[EREMOTE]		= "Item is not local to host";
   errno_text[ENOSYS]		= "Function not implemented  POSIX";
   errno_text[EMEDIA]		= "media surface error";
   errno_text[ESOFT]		= "I/O completed, but needs relocation";
   errno_text[ENOATTR]		= "no attribute found";
   errno_text[ESAD]		= "security authentication denied";
   errno_text[ENOTRUST]		= "not a trusted program *";
   errno_text[ETOOMANYREFS]	= "Too many references: can't splice";
   errno_text[EILSEQ]		= "Invalid wide character";
   errno_text[ECANCELED]	= "asynchronous i/o cancelled";
   errno_text[ENOSR]		= "temp out of streams resources";
   errno_text[ETIME]		= "I_STR ioctl timed out";
   errno_text[EBADMSG]		= "wrong message type at stream head";
   errno_text[EPROTO]		= "STREAMS protocol error";
   errno_text[ENODATA]		= "no message ready at stream head";
   errno_text[ENOSTR]		= "fd is not a stream";
   errno_text[ECLONEME]		= "this is the way we clone a stream ...";
}
