/*
*	The Altalena Project File
*	Copyright (C) 2009  Boris Ouretskey
*
*	This library is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*
*	This library is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*
*	You should have received a copy of the GNU Lesser General Public
*	License along with this library; if not, write to the Free Software
*	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#pragma once


#ifndef _WIN32_WINNT		//Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						



#include "RtpProxySession.h"
namespace ivrworx
{
	
	class ProcLive555RtpProxy :
		public LightweightProcess
	{
	public:
		
		ProcLive555RtpProxy(LpHandlePair pair, ConfigurationPtr conf);

		virtual ~ProcLive555RtpProxy(void);

		virtual void real_run();

	protected:

		virtual ApiErrorCode InitSockets();

		virtual void UponAllocateReq(IwMessagePtr msg);

		virtual void UponDeallocateReq(IwMessagePtr msg);

		virtual void UponModifyReq(IwMessagePtr msg);

		virtual void UponBridgeReq(IwMessagePtr msg);

	private:

		enum CONNECTION_STATE
		{
			CONNECTION_STATE_AVAILABLE,
			CONNECTION_STATE_ALLOCATED,
			CONNECTION_STATE_INPUT,
			CONNECTION_STATE_OUTPUT
		};

		typedef shared_ptr<Groupsock>
			GroupSockPtr;

		struct RtpConnection;
		typedef shared_ptr<struct RtpConnection> 
		RtpConnectionPtr;

		struct RtpConnection :
			public boost::noncopyable
		{
			RtpConnection();

			~RtpConnection();

			int connection_id;

			GroupSockPtr live_rtp_socket;
			GroupSockPtr live_rtcp_socket;

			RTCPInstance *rtcp_instance;

			CONNECTION_STATE state;

			CnxInfo local_cnx_ino;
			CnxInfo remote_cnx_ino;

			MediaFormat media_format;

			RtpConnectionPtr source_conn;
			SimpleRTPSource* source;

			RtpConnectionPtr destination_conn;
			SimpleRTPSink* sink;



		};

		ApiErrorCode 
		Unbridge(RtpConnectionPtr msg);

		ConfigurationPtr _conf;

		TaskScheduler *_scheduler;

		BasicUsageEnvironment *_env;
		
		typedef std::map<int,RtpConnectionPtr> 
		RtpConnectionsMap;
		RtpConnectionsMap _connectionsMap;

		in_addr _localInAddr;

		char _stopChar;

		friend void processIwMessagesTask(void* clientData);

	};


}

