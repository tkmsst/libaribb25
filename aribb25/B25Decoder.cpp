////////////////////////////////////////////////////////////////////////////////
// B25Decoder.cpp
////////////////////////////////////////////////////////////////////////////////
#include "B25Decoder.h"

int B25Decoder::strip;
int B25Decoder::emm_proc;
int B25Decoder::multi2_round;

B25Decoder::B25Decoder() : _bcas(nullptr), _b25(nullptr), _data(nullptr)
{
}

B25Decoder::~B25Decoder()
{
	if (_data)
		::free(_data);

	std::lock_guard<std::mutex> lock(_mtx);

	if (_b25)
		_b25->release(_b25);

	if (_bcas)
		_bcas->release(_bcas);

	char log[64];
	::wsprintfA(log, "B25Decoder::dtor() : end");
	::OutputDebugStringA(log);
}

int B25Decoder::init()
{
	int rc;

	std::lock_guard<std::mutex> lock(_mtx);

	if (_b25)
		return -2;

	char log[64];

	_bcas = create_b_cas_card();
	if (!_bcas)
		return -3;

	rc = _bcas->init(_bcas);
	if (rc < 0)
	{
		::wsprintfA(log, "B25Decoder::init() :  bcas init error / rc(%d)", rc);
		::OutputDebugStringA(log);
		rc = -4;
		goto err;
	}

	_b25 = create_arib_std_b25();
	if (!_b25)
	{
		rc = -5;
		goto err;
	}

	if (_b25->set_b_cas_card(_b25, _bcas) < 0)
	{
		rc = -6;
		goto err;
	}

	_b25->set_strip(_b25, strip);
	_b25->set_emm_proc(_b25, emm_proc);
	_b25->set_multi2_round(_b25, multi2_round);

	::wsprintfA(log, "B25Decoder::init() : success");
	::OutputDebugStringA(log);
	return 0;	// success

err:
	if (_b25)
	{
		_b25->release(_b25);
		_b25 = nullptr;
	}

	if (_bcas)
	{
		_bcas->release(_bcas);
		_bcas = nullptr;
	}

	::wsprintfA(log, "B25Decoder::init() : error / rc(%d)", rc);
	::OutputDebugStringA(log);
	_errtime = ::GetTickCount();
	return rc;	// error
}

void B25Decoder::decode(BYTE *pSrc, DWORD dwSrcSize, BYTE **ppDst, DWORD *pdwDstSize)
{
	if (!_b25)
	{
		DWORD now = ::GetTickCount();
		if ((now - _errtime) > RETRY_INTERVAL)
		{
			if (init() < 0)
				_errtime = now;
		}

		if (!_b25)
		{
			if (*ppDst != pSrc)
			{
				*ppDst = pSrc;
				*pdwDstSize = dwSrcSize;
			}
			return;
		}
	}

	if (_data)
	{
		::free(_data);
		_data = nullptr;
	}

	char log[64];

	ARIB_STD_B25_BUFFER buf;
	buf.data = pSrc;
	buf.size = dwSrcSize;
	const int rc = _b25->put(_b25, &buf);
	if (rc < 0)
	{
		if (rc >= ARIB_STD_B25_ERROR_NO_ECM_IN_HEAD_32M)
		{
			// pass through
			_b25->release(_b25);
			_b25 = nullptr;
			_bcas->release(_bcas);
			_bcas = nullptr;
			if (*ppDst != pSrc)
			{
				*ppDst = pSrc;
				*pdwDstSize = dwSrcSize;
			}
		}
		else
		{
			BYTE *p = nullptr;
			_b25->withdraw(_b25, &buf);	// withdraw src buffer
			if (buf.size > 0)
			{
				::wsprintfA(log, "B25Decoder::decode() : error / withdraw size(%u)", buf.size);
				::OutputDebugStringA(log);
				p = (BYTE *)::malloc(buf.size + dwSrcSize);
			}

			if (p)
			{
				::memcpy(p, buf.data, buf.size);
				::memcpy(p + buf.size, pSrc, dwSrcSize);
				*ppDst = p;
				*pdwDstSize = buf.size + dwSrcSize;
				_data = p;
			}
			else
			{
				if (*ppDst != pSrc)
				{
					*ppDst = pSrc;
					*pdwDstSize = dwSrcSize;
				}
			}

			if (rc == ARIB_STD_B25_ERROR_ECM_PROC_FAILURE)
			{
				// pass through
				_b25->release(_b25);
				_b25 = nullptr;
				_bcas->release(_bcas);
				_bcas = nullptr;
			}
		}
		::wsprintfA(log, "B25Decoder::decode() : error / rc(%d)", rc);
		::OutputDebugStringA(log);
		_errtime = ::GetTickCount();
		return;	// error
	}
	_b25->get(_b25, &buf);
	*ppDst = buf.data;
	*pdwDstSize = buf.size;
	return;	// success
}

void B25Decoder::setemm(bool flag)
{
	if (_b25)
		_b25->set_emm_proc(_b25, flag ? 1 : 0);
}
