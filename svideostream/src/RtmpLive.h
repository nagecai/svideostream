#ifndef RTMP_LIVE_H_
#define RTMP_LIVE_H_
#include <stdint.h>
#include <string>
#include "Common.h"
#include "rtmp.h"
#include "PlatformThreadEx.h"
#include "BufferQuene.h"
#include"RtmpCommon.h"

class CRtmpLive
{
public:
	enum LiveState
	{
		LS_NONE,
		LS_CONNECTING,
		LS_CONNECTED,
		LS_CONNECTE_ERROR
	};
	enum LiveEvent
	{
		LE_CONNECTING,
		LE_CONNECT_ERROR,
		LE_CONNECTED_SUCCESS,
		LE_LIVE_ERROR,
		LE_LIVE_STOP,
		LE_NEED_KEYFRAME
	};
	class ILiveEventObserver
	{
	public:
		virtual void NotifyEvent(LiveEvent event, int nerror) = 0;
	};
private:
	class RtmpPacket
	{
	public:
		static void FreeFun(void* data)
		{
			RtmpPacket *ins = (RtmpPacket*)data;
			delete ins;
		}
		static bool IsKeyVideoFrame(RtmpPacket* packet)
		{
			if (packet == nullptr)
			{
				return false;
			}
			return packet->m_bKeyFrame;
		}
		enum PacketType
		{
			RP_AUDIO,
			RP_VIDEO
		};
		PacketType m_eType;
		bool m_bKeyFrame;
		int64_t m_pts;
		int m_nSize;
		uint8_t* m_pData;
		RtmpPacket(PacketType type, int64_t pts, int nsize, bool keyframe)
		{
			m_pData = nullptr;
			m_eType = type;
			m_nSize = nsize;
			m_bKeyFrame = keyframe;
		}
		void FillData(uint8_t* data, int nsize)
		{
			if (m_pData == nullptr)
			{
				m_pData = new uint8_t[m_nSize];
			}
			memcpy(m_pData, data, nsize);
		}
		~RtmpPacket()
		{
			if (m_pData != nullptr)
			{
				delete m_pData;
			}
		}
	};
	bool m_bAudioEnable = false;
	std::string m_strPublishUrl;
	std::string m_strCopyRight;

	sH264CodecInfo m_H264Info;
	sAacCodecInfo m_AacInfo;

	LiveState m_eState = LS_NONE;
	bool m_bStarted = false;
	//rtmp
	RTMP* m_pRtmp;
	CPlatformThreadEx<CRtmpLive> m_hSendThread;
	ILiveEventObserver *m_pEventObserv = nullptr;
	bool m_bNeedWaitKeyFrame = false;
	CBufferQuene<RtmpPacket*, &RtmpPacket::FreeFun> m_qAudio;
	CBufferQuene<RtmpPacket*, &RtmpPacket::FreeFun> m_qVideo;
	uint8_t m_VideoSPSData[RTMP_VIDEO_SPECDATA_SIZE];
	int m_nVideoSPSDataLen = 0;
	uint8_t m_VideoPPSData[RTMP_VIDEO_SPECDATA_SIZE];
	int m_nVideoPPSDataLen = 0;

	uint8_t m_AudioSpecdata[RTMP_AUDIO_SPECDATA_SIZE];
	int m_nAudioSpecdataLen = 0;

	int OnAVSpecdata(uint8_t* data, int nsize, RtmpPacket::PacketType type);
	int OnAudioQueneFull();
	int OnVideoQueneFull();
	int WaitForAVSpecData();
	int SendRTMPConfigInfo();
	//send chunk size info
	int SendChunkSizeInfo();
	//send metadata
	int SendMetaData();
	//send video spec data
	int SendVideoSpecData();
	//send audio spec data
	int SendAudioSpecData();

	//send avpacket
	int SendAVPacket(RtmpPacket* packet);
	//send RtmpPacket
	int SendRtmpData(RTMPPacket *pkt);
	void NotifyEvent(LiveEvent event, int nerror)
	{
		if (m_pEventObserv != nullptr)
		{
			m_pEventObserv->NotifyEvent(event, nerror);
		}
	}
	void SetState(LiveState state)
	{
		m_eState = state;
	}
	bool OnSendThread();
	int ConnectServer();
public:
	CRtmpLive();
	void SetH264CodecInfo(sH264CodecInfo vinfo)
	{
		m_H264Info = vinfo;
	}
	void SetAacCodecInfo(sAacCodecInfo ainfo)
	{
		m_AacInfo = ainfo;
	}
	void SetEnableAudio(bool enable)
	{
		m_bAudioEnable = enable;
	}
	void SetUrl(const std::string& url)
	{
		m_strPublishUrl = url;
	}
	void SetCopyRight(const std::string& copyright)
	{
		m_strCopyRight = copyright;
	}
	int StartLive();
	void StopLive();
	void SetEventObserver(ILiveEventObserver *observer)
	{
		m_pEventObserv = observer;
	}
	int SendVideoData(uint8_t* vdata, int nsize, int64_t npts);
	int SendAudioData(uint8_t* vdata, int nsize, int64_t npts);
};
#endif 