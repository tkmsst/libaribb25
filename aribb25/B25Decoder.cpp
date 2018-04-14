////////////////////////////////////////////////////////////////////////////////
// B25Decoder.cpp
////////////////////////////////////////////////////////////////////////////////
#include "B25Decoder.h"

int B25Decoder::strip        = 1;
int B25Decoder::emm_proc     = 0;
int B25Decoder::multi2_round = 4;

B25Decoder::B25Decoder() : _bcas(nullptr), _b25(nullptr), _data(nullptr)
{
}

B25Decoder::~B25Decoder()
{
	release();
}

int B25Decoder::init()
{
	int rc;

	std::lock_guard<std::mutex> lock(_mtx);

	if (_b25)
		return -2;

	_bcas = create_b_cas_card();
	if (!_bcas)
		return -3;

	rc = _bcas->init(_bcas);
	if (rc < 0) {
		rc = -4;
		goto err;
	}

	_b25 = create_arib_std_b25();
	if (!_b25) {
		rc = -5;
		goto err;
	}

	if (_b25->set_b_cas_card(_b25, _bcas) < 0) {
		rc = -6;
		goto err;
	}

	_b25->set_strip(_b25, strip);
	_b25->set_emm_proc(_b25, emm_proc);
	_b25->set_multi2_round(_b25, multi2_round);

	return 0;	// success

err:
	if (_b25) {
		_b25->release(_b25);
		_b25 = nullptr;
	}

	if (_bcas) {
		_bcas->release(_bcas);
		_bcas = nullptr;
	}

	_errtime = time(nullptr);
	return rc;	// error
}

void B25Decoder::release()
{
	if (_data) {
		::free(_data);
		_data = nullptr;
	}

	std::lock_guard<std::mutex> lock(_mtx);

	if (_b25) {
		_b25->release(_b25);
		_b25 = nullptr;
	}

	if (_bcas) {
		_bcas->release(_bcas);
		_bcas = nullptr;
	}
}

void B25Decoder::decode(BYTE *pSrc, DWORD dwSrcSize, BYTE **ppDst, DWORD *pdwDstSize)
{
	if (!_b25) {
		time_t now = time(nullptr);
		if (difftime(now, _errtime) > RETRY_INTERVAL) {
			if (init() < 0)
				_errtime = now;
		}

		if (!_b25) {
			if (*ppDst != pSrc) {
				*ppDst = pSrc;
				*pdwDstSize = dwSrcSize;
			}
			return;
		}
	}

	if (_data) {
		::free(_data);
		_data = nullptr;
	}

	ARIB_STD_B25_BUFFER buf;
	buf.data = pSrc;
	buf.size = dwSrcSize;
	const int rc = _b25->put(_b25, &buf);
	if (rc < 0) {
		if (rc >= ARIB_STD_B25_ERROR_NO_ECM_IN_HEAD_32M) {
			// pass through
			_b25->release(_b25);
			_b25 = nullptr;
			_bcas->release(_bcas);
			_bcas = nullptr;
			if (*ppDst != pSrc) {
				*ppDst = pSrc;
				*pdwDstSize = dwSrcSize;
			}
		} else {
			BYTE *p = nullptr;
			_b25->withdraw(_b25, &buf);	// withdraw src buffer
			if (buf.size > 0)
				p = (BYTE *)::malloc(buf.size + dwSrcSize);

			if (p) {
				::memcpy(p, buf.data, buf.size);
				::memcpy(p + buf.size, pSrc, dwSrcSize);
				*ppDst = p;
				*pdwDstSize = buf.size + dwSrcSize;
				_data = p;
			} else {
				if (*ppDst != pSrc) {
					*ppDst = pSrc;
					*pdwDstSize = dwSrcSize;
				}
			}

			if (rc == ARIB_STD_B25_ERROR_ECM_PROC_FAILURE) {
				// pass through
				_b25->release(_b25);
				_b25 = nullptr;
				_bcas->release(_bcas);
				_bcas = nullptr;
			}
		}
		_errtime = time(nullptr);
		return;	// error
	}
	_b25->get(_b25, &buf);
	*ppDst = buf.data;
	*pdwDstSize = buf.size;
	return;	// success
}

int B25Decoder::set_strip(int32_t strip)
{
	int rc = 0;
	if (_b25)
		rc = _b25->set_strip(_b25, strip);
	return rc;
}

int B25Decoder::set_emm_proc(int32_t on)
{
	int rc = 0;
	if (_b25)
		rc = _b25->set_emm_proc(_b25, on);
	return rc;
}

int B25Decoder::set_multi2_round(int32_t round)
{
	int rc = 0;
	if (_b25)
		rc = _b25->set_multi2_round(_b25, round);
	return rc;
}

int B25Decoder::set_unit_size(int size)
{
	int rc = 0;
	if (_b25)
		rc = _b25->set_unit_size(_b25, size);
	return rc;
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
