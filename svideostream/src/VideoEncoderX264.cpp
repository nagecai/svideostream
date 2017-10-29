#include "VideoEncoderX264.h"



bool CVideoEncoderX264::OnEncodeThread()
{
	x264_nal_t *pNals = NULL;
	int nNals = -1;
	x264_picture_t picIn;
	x264_picture_t picOut;
	x264_picture_init(&picIn);
	x264_picture_init(&picOut);
	picIn.img.i_csp = X264_CSP_I420;
	picIn.img.i_plane = 3;
	picIn.img.i_stride[0] = m_nWidth;
	picIn.img.i_stride[1] = m_nWidth / 2;
	picIn.img.i_stride[2] = m_nWidth / 2;

	//encode header
	x264_nal_t *pHeaderNals = NULL;
	int nHeaderNals = -1;
	int nHeaderNalsSize = x264_encoder_headers(m_pX264, &pHeaderNals, &nHeaderNals);
	LOGI << "header nals is " << nHeaderNals;
	if (nHeaderNalsSize < 0 || nHeaderNals < 2) {
		LOGE<<"X264 get headers failed.";
		return false;
	}
	if (m_pCallBack == nullptr)
	{
		LOGE << "x264 callback is null";
		return false;
	}
	m_pCallBack->OnVideoEncodedData(pHeaderNals[0].p_payload, pHeaderNals[0].i_payload + pHeaderNals[1].i_payload, -1);

	int nUOffset = m_nWidth * m_nHeight;
	int nVOffset = m_nWidth * m_nHeight + m_nWidth * m_nHeight / 4;
	while (!m_bAbort)
	{
		VideoFrame * imagedata = nullptr;
		if (!m_qVideoFrameQ.PullData(&imagedata, true))
		{
			usleep(20000);
			continue;
		}
		picIn.i_pts = imagedata->GetPts();
		picIn.img.plane[0] = imagedata->GetFrameData();
		picIn.img.plane[1] = imagedata->GetFrameData() + nUOffset;
		picIn.img.plane[2] = imagedata->GetFrameData() + nVOffset;
		picIn.i_type = X264_TYPE_AUTO;
		if (m_bReqKeyFrame)
		{
			m_bReqKeyFrame = false;
			picIn.i_type = X264_TYPE_KEYFRAME;
		}
		// Encode
		int nNalsSize = x264_encoder_encode(m_pX264, &pNals, &nNals, &picIn, &picOut);
		if (nNalsSize < 0) {
			LOGE<<"Error encoding frame.";
			break;
		}
		
		if (nNalsSize > 0) {
			m_pCallBack->OnVideoEncodedData(pNals[0].p_payload, nNalsSize, picOut.i_pts);
		}
		delete imagedata;
	}
	x264_encoder_close(m_pX264);
	LOGI<<"Out videoEncodeThread.";
	return false;
}

CVideoEncoderX264::CVideoEncoderX264():
m_hEncodeThread(this, &CVideoEncoderX264::OnEncodeThread, "x264encodethread")
{
	m_qVideoFrameQ.SetMaxCount(3);
}

int CVideoEncoderX264::OpenEncoder()
{
	x264_param_t x264param;

	//Init x264
	x264_param_default_preset(&x264param, "superfast", "zerolatency");
	x264_param_apply_profile(&x264param, "baseline");

	x264param.rc.i_rc_method = X264_RC_ABR;
	x264param.rc.i_bitrate = m_nBitrate / 1000;
	x264param.i_timebase_num = 1;
	x264param.i_timebase_den = m_nFps;
	x264param.i_fps_num = x264param.i_timebase_den;
	x264param.i_fps_den = x264param.i_timebase_num * 1;
	x264param.i_keyint_min = m_nGop / 2;
	x264param.i_keyint_max = m_nGop;
	x264param.i_bframe = 0;

	x264param.b_repeat_headers = 0;
	//x264param.b_annexb = pCtx->bVideoAnnexb ? 1 : 0;
	x264param.b_annexb = 1;
	x264param.i_width = m_nWidth;
	x264param.i_height = m_nHeight;

	x264param.i_csp = X264_CSP_I420;
	x264param.i_log_level = X264_LOG_NONE;

	m_pX264 = x264_encoder_open(&x264param);
	return 0;
}

int CVideoEncoderX264::CloseEncoder()
{
	m_bAbort = true;
	m_qVideoFrameQ.SetEnable(false);
	m_hEncodeThread.Stop();
	return 0;
}

int CVideoEncoderX264::PushData(uint8_t *imagedata, int nsize, int64_t pts)
{
	//LOGI << " video data size = " << nsize << " pts = " << pts;
	VideoFrame *frame = new VideoFrame(m_nWidth, m_nHeight);
	frame->FillData(imagedata, nsize, pts);
	if (!m_qVideoFrameQ.PushData(frame, true))
	{
		delete frame;
	}
	return 0;
}

sH264CodecInfo CVideoEncoderX264::GetCodecInfo()
{
	sH264CodecInfo info;
	info.m_bAnnexB = true;
	info.m_nBitrate = m_nBitrate;
	info.m_nFps = m_nFps;
	info.m_nGop = m_nGop;
	info.m_nHeight = m_nHeight;
	info.m_nWidth = m_nWidth;
	info.m_nLevel = 21;
	info.m_nProfile = 66;
	return info;
}

int CVideoEncoderX264::StartEncode()
{
	m_bAbort = false;
	m_qVideoFrameQ.SetEnable(true);
	m_hEncodeThread.Start();
	return 0;
}
