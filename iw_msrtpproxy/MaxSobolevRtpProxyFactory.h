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

#ifdef _DLL_IW_MSRTPPROXY // assume this is defined when we build the DLL
#define IW_MSRTPPROXY_API  __declspec( dllexport)
#else
#define IW_MSRTPPROXY_API  __declspec( dllimport)
#endif

namespace ivrworx
{
	class MaxSobolevRtpProxyFactory:
		public IProcFactory
	{
	public:
		MaxSobolevRtpProxyFactory(void);
		virtual ~MaxSobolevRtpProxyFactory(void);

		LightweightProcess *Create(LpHandlePair pair, ConfigurationPtr conf);
	};

	IW_MSRTPPROXY_API IProcFactory* GetIwFactory();


}


