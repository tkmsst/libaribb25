////////////////////////////////////////////////////////////////////////////////
// B25Decoder.cpp
////////////////////////////////////////////////////////////////////////////////
#include "B25Decoder.h"

int B25Decoder::strip        = 1;
int B25Decoder::emm_proc     = 0;
int B25Decoder::multi2_round = 4;

B25Decoder::B25Decoder() : _bcas(nullptr), _b25(nullptr), _data(nullptr)
{
#if !defined(_WIN32)
	pthread_mutex_init(&_mtx, nullptr);
#endif
}

B25Decoder::~B25Decoder()
{
	if (_data)
		::free(_data);

#if defined(_WIN32)
	_mtx.lock();
#else
	pthread_mutex_lock(&_mtx);
#endif

	if (_b25)
		_b25->release(_b25);

	if (_bcas)
		_bcas->release(_bcas);

#if defined(_WIN32)
	_mtx.unlock();
#else
	pthread_mutex_unlock(&_mtx);
	pthread_mutex_destroy(&_mtx);
#endif
}

int B25Decoder::init()
{
	int rc;

#if defined(_WIN32)
	_mtx.lock();;
#else
	pthread_mutex_lock(&_mtx);
#endif

	if (_b25)
	{
		rc = -2;
		goto unlock;
	}

	_bcas = create_b_cas_card();
	if (!_bcas)
	{
		rc = -3;
		goto unlock;
	}

	rc = _bcas->init(_bcas);
	if (rc < 0)
	{
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

	// success
	_b25->set_strip(_b25, strip);
	_b25->set_emm_proc(_b25, emm_proc);
	_b25->set_multi2_round(_b25, multi2_round);

	rc = 0;
	goto unlock;

err:
	// error
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

#if defined(_WIN32)
	_errtime = ::GetTickCount();
#else
	clock_gettime(CLOCK_MONOTONIC_COARSE, &_errtime);
#endif

unlock:
#if defined(_WIN32)
	_mtx.unlock();
#else
	pthread_mutex_unlock(&_mtx);
#endif
	return rc;
}

void B25Decoder::setemm(bool flag)
{
	if (_b25)
		_b25->set_emm_proc(_b25, flag ? 1 : 0);
}

void B25Decoder::decode(BYTE *pSrc, DWORD dwSrcSize, BYTE **ppDst, DWORD *pdwDstSize)
{
	if (!_b25)
	{
#if defined(_WIN32)
		DWORD now = ::GetTickCount();
		DWORD interval = (now - _errtime) / 1000;
#else
		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC_COARSE, &now);
		time_t interval = now.tv_sec - _errtime.tv_sec;
#endif
		if (interval > RETRY_INTERVAL)
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
				p = (BYTE *)::malloc(buf.size + dwSrcSize);

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
#if defined(_WIN32)
		_errtime = ::GetTickCount();
#else
		clock_gettime(CLOCK_MONOTONIC_COARSE, &_errtime);
#endif
		return;	// error
	}
	_b25->get(_b25, &buf);
	*ppDst = buf.data;
	*pdwDstSize = buf.size;
	return;	// success
}

int B25Decoder::reset()
{
	int rc = 0;
	
	if (_b25)
		rc = _b25->reset(_b25);
	return rc;
}

int B25Decoder::flush()
{
	int rc = 0;
	
	if (_b25)
		rc = _b25->flush(_b25);
	return rc;
}

int B25Decoder::put(BYTE *pSrc, DWORD dwSrcSize)
{
	ARIB_STD_B25_BUFFER buf;
	buf.data = pSrc;
	buf.size = dwSrcSize;
	return _b25->put(_b25, &buf);
}

int B25Decoder::get(BYTE **ppDst, DWORD *pdwDstSize)
{
	ARIB_STD_B25_BUFFER buf;
	int rc = _b25->get(_b25, &buf);
	*ppDst = buf.data;
	*pdwDstSize = buf.size;
	return rc;
}
