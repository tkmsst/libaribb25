#ifndef PORTABLE_H
#define PORTABLE_H

#include <inttypes.h>

#if !defined(_WIN32)
	#define _open			open
	#define _topen 			_open
	#define _close			close
	#define _read			read
	#define _write			write
	#define _lseeki64		lseek
	#define _telli64(fd)	(lseek(fd,0,SEEK_CUR))
	#define _O_BINARY		(0)
	#define _O_RDONLY		(O_RDONLY)
	#define _O_WRONLY		(O_WRONLY)
	#define _O_SEQUENTIAL	(0)
	#define _O_CREAT		(O_CREAT)
	#define _O_TRUNC		(O_TRUNC)
	#define _S_IREAD		(S_IRUSR|S_IRGRP|S_IROTH)
	#define _S_IWRITE		(S_IWUSR|S_IWGRP|S_IWOTH)
	#define _S_IWRITE		(S_IWUSR|S_IWGRP|S_IWOTH)
	#define TCHAR			char
	#define _T(X) 			X
	#define _ftprintf 		fprintf
	#define _ttoi 			atoi
	#define _tmain 			main
	#define _tcslen			strlen
	#define __inline		inline
	#define __forceinline	inline
#endif

#if !defined(nullptr)
	#define nullptr NULL
#endif

#endif /* PORTABLE_H */
