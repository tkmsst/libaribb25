////////////////////////////////////////////////////////////////////////////////
// B25Decoder.h
////////////////////////////////////////////////////////////////////////////////
#ifndef __B25DECODER_H__
#define __B25DECODER_H__

#include <ctime>
#include <mutex>

#if defined(_WIN32)
	#include <windows.h>
	#include "arib_std_b25.h"
	#include "arib_std_b25_error_code.h"
#else
	#include <aribb25/arib_std_b25.h>
	#include <aribb25/arib_std_b25_error_code.h>
	#include "typedef.h"
#endif

#define RETRY_INTERVAL	10	// 10sec interval

class B25Decoder
{
public:
	B25Decoder();
	~B25Decoder();
	int init();
	void release();
	void decode(BYTE *pSrc, DWORD dwSrcSize, BYTE **ppDst, DWORD *pdwDstSize);

	// libaribb25 wrapper
	int set_strip(int32_t strip);
	int set_emm_proc(int32_t on);
	int set_multi2_round(int32_t round);
	int set_unit_size(int size);
	int reset();
	int flush();
	int put(BYTE *pSrc, DWORD dwSrcSize);
	int get(BYTE **ppDst, DWORD *pdwDstSize);

	// initialize parameter
	static int strip;
	static int emm_proc;
	static int multi2_round;

private:
	std::mutex _mtx;
	B_CAS_CARD *_bcas;
	ARIB_STD_B25 *_b25;
	BYTE *_data;
	time_t _errtime;
};

#endif	// __B25DECODER_H__
