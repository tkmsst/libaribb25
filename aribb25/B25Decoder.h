////////////////////////////////////////////////////////////////////////////////
// B25Decoder.h
////////////////////////////////////////////////////////////////////////////////
#ifndef __B25DECODER_H__
#define __B25DECODER_H__

#if defined(_WIN32)
	#include <windows.h>
	#include <mutex>
#else
	#include <stdlib.h>
	#include <string.h>
	#include <time.h>
	#include <pthread.h>
#endif

#include "portable.h"
#include "arib_std_b25.h"
#include "arib_std_b25_error_code.h"

#define RETRY_INTERVAL	10	// 10sec interval

class B25Decoder
{
public:
	B25Decoder();
	~B25Decoder();
	int init();
	void setemm(bool flag);
	void decode(BYTE *pSrc, DWORD dwSrcSize, BYTE **ppDst, DWORD *pdwDstSize);

	// libaribb25 wrapper
	int reset();
	int flush();
	int put(BYTE *pSrc, DWORD dwSrcSize);
	int get(BYTE **ppDst, DWORD *pdwDstSize);

	// initialize parameter
	static int strip;
	static int emm_proc;
	static int multi2_round;

private:
#if defined(_WIN32)
	std::mutex _mtx;
#else
	pthread_mutex_t _mtx;
#endif
	B_CAS_CARD *_bcas;
	ARIB_STD_B25 *_b25;
	BYTE *_data;
#if defined(_WIN32)
	DWORD _errtime;
#else
	struct timespec _errtime;
#endif
};

#endif	// __B25DECODER_H__
