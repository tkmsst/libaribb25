////////////////////////////////////////////////////////////////////////////////
// B25Decoder.h
////////////////////////////////////////////////////////////////////////////////
#ifndef __B25DECODER_H__
#define __B25DECODER_H__
#include <windows.h>
#include <mutex>
#include "arib_std_b25.h"
#include "arib_std_b25_error_code.h"

#define RETRY_INTERVAL	10000	// 10sec interval

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
	std::mutex _mtx;
	B_CAS_CARD *_bcas;
	ARIB_STD_B25 *_b25;
	BYTE *_data;
	DWORD _errtime;
};

#endif	// __B25DECODER_H__
