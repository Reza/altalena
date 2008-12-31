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
#include "ProcVcs.h"
#include "ProcPipeIPCDispatcher.h"
#include "CallWithRTPManagment.h"
#include "SipStackFactory.h"
#include "LuaScriptRunnerFactory.h"


ProcVcs::ProcVcs(IN LpHandlePair pair, IN CnxInfo sip_stack_media)
:LightweightProcess(pair,VCS_Q,	__FUNCTIONW__),
_sipStackData(sip_stack_media),
_conf(NULL)
{
}

ProcVcs::ProcVcs(IN LpHandlePair pair, IN CcuConfiguration &conf)
:LightweightProcess(pair,VCS_Q,	__FUNCTIONW__),
_conf(&conf),
_sipStackData(conf.VcsCnxInfo())
{
}



ProcVcs::~ProcVcs(void)
{
}

void
ProcVcs::real_run()
{

	START_FORKING_REGION;

	_vm.InitialiseVM();
	
	//
	// Start IPC for VCS queue
	//
	DECLARE_NAMED_HANDLE_PAIR(ipc_pair);
	FORK(new ProcPipeIPCDispatcher(ipc_pair,VCS_Q));
	if (CCU_FAILURE(WaitTillReady(Seconds(5),ipc_pair)))
	{
		return;
	};

	//
	// Start SIP stack process
	//
	DECLARE_NAMED_HANDLE_PAIR(stack_pair);
	_stackPair = stack_pair;
	FORK(SipStackFactory::CreateSipStack(stack_pair,_sipStackData));
	if (CCU_FAILURE(WaitTillReady(Seconds(5),stack_pair)))
	{
		Shutdown(Seconds(5), ipc_pair);
		return;
	};

	//
	// Start Script Runner
	//
	DECLARE_NAMED_HANDLE_PAIR(script_runner_pair);
	FORK(LuaScriptRunnerFactory::CreateScriptRunner(script_runner_pair, stack_pair));
	if (CCU_FAILURE(WaitTillReady(Seconds(5),script_runner_pair)))
	{
		Shutdown(Seconds(5), stack_pair);
		Shutdown(Seconds(5), ipc_pair);
		return;
	};

	I_AM_READY;

	HandlesList list;
	int index = 0;
	
	list.push_back(ipc_pair.outbound);
	const int ipc_index = index++;

	list.push_back(stack_pair.outbound);
	const int stack_index = index++;

	list.push_back(_inbound);
	const int inbound_index = index++;

	
	//
	// Message Loop
	// 
	BOOL shutdown_flag = FALSE;
	CcuMsgPtr event;
	while(shutdown_flag == FALSE)
	{
		
		int index = -1;
		CcuApiErrorCode err_code = 
			SelectFromChannels(
			list,
			Seconds(60), 
			index, 
			event);

		if (err_code == CCU_API_TIMEOUT)
		{
			LogInfo(">>VCS Keep Alive<<");
			continue;
		}

		if (index == stack_index)
		{
			shutdown_flag = ProcessStackMessage(event, forking);
		} 
		else if (index == inbound_index)
		{
			shutdown_flag = ProcessInboundMessage(event,forking);
		}

	}

	Shutdown(Time(Seconds(5)),script_runner_pair);
	Shutdown(Time(Seconds(5)),stack_pair);
	Shutdown(Time(Seconds(5)),ipc_pair);

	if (event != CCU_NULL_MSG && 
		event->message_id == CCU_MSG_PROC_SHUTDOWN_REQ)
	{

		SendResponse(event,new CcuMsgShutdownAck());
	}

	END_FORKING_REGION;

}

BOOL 
ProcVcs::ProcessInboundMessage(IN CcuMsgPtr event, IN ScopedForking &forking)
{
	FUNCTRACKER;
	switch (event->message_id)
	{
	case CCU_MSG_PROC_SHUTDOWN_REQ:
		{
			return TRUE;
		}
	default:
		{
			BOOL oob_res = HandleOOBMessage(event);
			if (oob_res = FALSE)
			{
				LogWarn("Unknown message received id=[" << event->message_id_str << "]");
				_ASSERT(false);
			}
			
		}
	}

	return FALSE;

}

BOOL 
ProcVcs::ProcessStackMessage(IN CcuMsgPtr ptr, IN ScopedForking &forking)
{
	FUNCTRACKER;

	switch (ptr->message_id)
	{
	case CCU_MSG_CALL_OFFERED:
		{
			
			shared_ptr<CcuMsgCallOffered> call_offered_msg = 
				dynamic_pointer_cast<CcuMsgCallOffered>(ptr);

			DECLARE_NAMED_HANDLE_PAIR(script_pair);
			FORK_IN_THIS_THREAD(
				new ProcVoidFuncRunner<ProcVcs>(
				script_pair,
				bind<void>(&ProcVcs::StartScript, _1, ptr),
				this,
				L"Script Runner"));

		}
	case CCU_MSG_PROC_SHUTDOWN_REQ:
		{
			
			return TRUE;
		}
	default:
		{

		}
	}

	return FALSE;

}

void ProcVcs::StartScript(IN CcuMsgPtr msg)
{
	shared_ptr<CcuMsgCallOffered> start_script_msg  = 
		dynamic_pointer_cast<CcuMsgCallOffered>(msg);

	CallWithRTPManagment call_session(
		_stackPair,
		start_script_msg->stack_call_handle,
		start_script_msg->remote_media,
		*this);

	CallFlowScript script( _vm, call_session);

	string s = WStringToString(L"hello.cvs");

	if (!script.CompileBuffer((unsigned char*)s.c_str(), s.length()))
	{
		SendResponse(msg, new CcuMsgNack());
	}

	script.Go();
}

CallFlowScript::CallFlowScript(IN CLuaVirtualMachine &vm, IN CallWithRTPManagment &call_session)
:CLuaScript(vm),
_callSession(call_session)
{
	_methodBase = RegisterFunction("answer");

}

CallFlowScript::~CallFlowScript()
{

}

int 
CallFlowScript::ScriptCalling (CLuaVirtualMachine& vm, int iFunctionNumber) 
{
	switch (iFunctionNumber - _methodBase)
	{
	case 0:
		return AnswerCall(vm);
	}

	return 0;

}


void 
CallFlowScript::HandleReturns (CLuaVirtualMachine& vm, const char *strFunc)
{

}

int
CallFlowScript::AnswerCall(CLuaVirtualMachine& vm)
{
	FUNCTRACKER;

	CcuApiErrorCode res = _callSession.AcceptCall();

	if (CCU_SUCCESS(res))
	{
		return 0;
	} else 
	{
		return -1;
	}

}

