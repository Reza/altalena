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

#include "LpHandle.h"
#include "LightweightProcess.h"

using namespace csp;
using namespace std;



namespace ivrworx
{
	typedef map<int,LpHandlePtr> EventListenersMap;

	class ActiveObject
	{
	public:

		ActiveObject();

		virtual void Start(IN ScopedForking &forking, IN LpHandlePair pair, IN const wstring &name);

		virtual void UponActiveObjectEvent(IxMsgPtr ptr);

		virtual void SetEventListener(int ccu_msg_id, LpHandlePtr listener_handle);

		virtual ~ActiveObject(void);

	private:

		LpHandlePair _handlePair;

		EventListenersMap _listenersMap;

		BucketPtr _listenerBucket;

		BOOL volatile _shutdownFlag;

		friend class ProcEventListener;
	};

	class ProcEventListener:
		public LightweightProcess
	{
	public:

		ProcEventListener(ActiveObject &object,LpHandlePair pair, wstring name);


		void real_run();

	private:

		ActiveObject &_activeObject;
		
	};


}
