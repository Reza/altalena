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

#include "StdAfx.h"
#include "ProcIms.h"
#include "ImsSession.h"
#include "Ims.h"

using namespace boost;

namespace ivrworx
{

ImsSession::ImsSession(LightweightProcess &facade):
_imsSessionHandle(IX_UNDEFINED),
_facade(facade)
{

}

ImsSession::~ImsSession(void)
{
	if (_imsSessionHandle != IX_UNDEFINED)
	{
		TearDown();
	}
}

IxApiErrorCode
ImsSession::PlayFile(IN const wstring &file_name,
					 IN BOOL sync,
					 IN BOOL loop,
					 IN BOOL provisional)
{

	FUNCTRACKER;

	if (_imsSessionHandle == IX_UNDEFINED)
	{
		return CCU_API_FAILURE;
	}

	IxMsgPtr response = CCU_NULL_MSG;
	
	DECLARE_NAMED_HANDLE(ims_play_txn);
	ims_play_txn->HandleName(L"Ims Play TXN"); // for logging purposes
	ims_play_txn->Direction(CCU_MSG_DIRECTION_INBOUND);

	RegistrationGuard guard(ims_play_txn);

	
	CcuMsgStartPlayReq *msg = new CcuMsgStartPlayReq();
	msg->playback_handle	= _imsSessionHandle;
	msg->file_name			= file_name;
	msg->send_provisional	= provisional;
	msg->loop				= loop;
	msg->source.handle_id	= ims_play_txn->GetObjectUid();
	msg->transaction_id		= GenerateNewTxnId();
	
	IxApiErrorCode res = _facade.SendMessage(IMS_Q,IxMsgPtr(msg));
	if (CCU_FAILURE(res))
	{
		return res;
	}

	if (provisional)
	{
		res = _facade.WaitForTxnResponse(ims_play_txn,response, MilliSeconds(_facade.TransactionTimeout()));
		if (CCU_FAILURE(res))
		{
			return res;
		}

		if (response->message_id != CCU_MSG_START_PLAY_REQ_ACK)
		{
			return CCU_API_FAILURE;
		}

	}
	
	if (!sync)
	{
		return CCU_API_SUCCESS;
	}

	res = _facade.WaitForTxnResponse(ims_play_txn,response,  MilliSeconds(MAXLONG));
	if (CCU_FAILURE(res))
	{
		return res;
	}

	if (response->message_id != CCU_MSG_IMS_PLAY_STOPPED)
	{
		return CCU_API_FAILURE;
	}

	return CCU_API_SUCCESS;
}

IxApiErrorCode
ImsSession::AllocateIMSConnection(IN CnxInfo remote_end, 
								  IN MediaFormat codec)
{
	FUNCTRACKER;

	LogDebug("Allocating IMS session remote end = " <<  remote_end.ipporttows()  << ", codec = "  << codec);
	
	if (_imsSessionHandle != IX_UNDEFINED)
	{
		return CCU_API_SUCCESS;
	}

	IxMsgPtr response = CCU_NULL_MSG;

	CcuMsgAllocateImsSessionReq *msg = new CcuMsgAllocateImsSessionReq();
	msg->remote_media_data = remote_end;
	msg->codec = codec;

	IxApiErrorCode res = _facade.DoRequestResponseTransaction(
		IMS_Q,
		IxMsgPtr(msg),
		response,
		Time(MilliSeconds(_facade.TransactionTimeout())),
		L"Allocate IMS Connection TXN");

	if (res != CCU_API_SUCCESS)
	{
		LogWarn("Error allocating Ims connection " << res);
		return res;
	}

	switch (response->message_id)
	{
	case CCU_MSG_ALLOCATE_PLAYBACK_SESSION_REQUEST_ACK:
		{


			shared_ptr<CcuMsgAllocateImsSessionAck> ack = 
				shared_polymorphic_cast<CcuMsgAllocateImsSessionAck>(response);
			_imsSessionHandle = ack->playback_handle;
			_imsMediaData = ack->ims_media_data;

			LogDebug("IMS session allocated successfully, ims handle=[" << _imsSessionHandle << "]");

			break;

		}
	case CCU_MSG_ALLOCATE_PLAYBACK_SESSION_REQUEST_NACK:
		{
			LogDebug("Error allocating IMS session.");
			res = CCU_API_SERVER_FAILURE;
			break;
		}
	default:
		{
			throw;
		}
	}
	return res;

}



CnxInfo ImsSession::ImsMediaData() const 
{ 
	return _imsMediaData; 
}

void ImsSession::ImsMediaData(IN CnxInfo val) 
{ 
	_imsMediaData = val; 
}

void
ImsSession::TearDown()
{
	if(_imsSessionHandle == IX_UNDEFINED)
	{
		return;
	}

	CcuMsgImsTearDownReq *tear_req = new CcuMsgImsTearDownReq();
	tear_req->handle = _imsSessionHandle;

	// no way back
	_imsSessionHandle = IX_UNDEFINED;

	IxApiErrorCode res = _facade.SendMessage(IMS_Q,IxMsgPtr(tear_req));

}

}