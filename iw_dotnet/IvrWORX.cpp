#include "StdAfx.h"
#include "IvrWORX.h"


//using namespace std;
using namespace System;
using namespace System::Runtime::InteropServices;
using namespace ivrworx;
using namespace interop;


IvrWORX::IvrWORX():
_threadId(::GetCurrentThreadId()),
_conf(NULL),
_factoriesList(NULL),
_procHandles (NULL),
_shutdownHandles (NULL),
_forking(NULL),
_isDisposed(false)
{
	
	
}

void IvrWORX::Init(String ^dt_confFile)
{
	int res = 0;
	try
	{

		std::string conffile;

		if (System::String::IsNullOrEmpty(dt_confFile) == NULL)
		{
			cerr << "init:g_conf file is NULL using conf.json" << endl;
			dt_confFile = "conf.json";
		} 


		pin_ptr<const wchar_t> wch = PtrToStringChars(dt_confFile);
		std::wstring wconffile(wch);	
		conffile = WStringToString(wconffile);

		cout << "loading " << conffile << "..." << endl;

		ivrworx::ApiErrorCode err_code = ivrworx::API_SUCCESS;
		_conf = new ConfigurationPtr(ConfigurationFactory::CreateJsonConfiguration(conffile,err_code));

		if (IW_FAILURE(err_code))
		{
			cerr << "init:error reading configuration file:" << conffile << endl;
			throw gcnew Exception("Error reading configuration file");
		}


		if (!InitLog(*_conf))
		{
			cerr << "init:error initiating logging infrastructure:" << endl;
			throw gcnew Exception("init:error initiating logging infrastructure");
		}

		// from here you may use generic clean up routine
		LogInfo(">>>>>> IVRWORX (.NET) START <<<<<<");
		Start_CPPCSP();


		LpHandlePair _ctxPair = HANDLE_PAIR;

		char buffer[1024];
		::sprintf_s(buffer, 1024,".NET IvrWORX(%d)",_threadId);
		
		_ctx = new RunningContext(_ctxPair,buffer);

		RegisterContext(_ctx);


		_factoriesList = new FactoryPtrList();
		if (IW_FAILURE(LoadConfiguredModules(
			*_conf,
			*_factoriesList)))
		{
			goto error;
		};

		// this is something THAT IS NOT ADVISED by CSP
		// but no way I can do it any other way
		_procHandles	 = new HandlePairList();
		_shutdownHandles = new HandlesVector();
		_forking		 = new ScopedForking();

		if (IW_FAILURE(BootModulesSimple(
			*_conf,
			*_forking,
			*_factoriesList,
			*_procHandles,
			*_shutdownHandles)))
		{
			goto error;
		};

	} 
	catch (exception &e)
	{
		cerr << endl << "Exception caught during program execution e:" << e.what() << endl;
		goto error;
	}


	return;

error:

	throw gcnew Exception("generic initialization failure");

}


IvrWORX::~IvrWORX()
{
	if (_isDisposed)
		return;
	
	if (_threadId != ::GetCurrentThreadId())
	{
		throw gcnew Exception("Attempt to dispose the object in thread different that it was created in.");
	}

	_isDisposed = true;

	try
	{
		//shutdown known modules
		ShutdownModules(*_procHandles,*_conf);
	
		// shutdown unknown modules
		LocalProcessRegistrar().Instance().UnReliableShutdownAll();

		if (_forking)
			delete _forking;

		if (_shutdownHandles)
			delete _shutdownHandles;

		if (_procHandles)
			delete _procHandles;

		if (_conf)
			delete _conf;

	
		End_CPPCSP();
		LogInfo(">>>>>> IVRWORX END <<<<<<");
		ExitLog();
	}
	catch (exception &e)
	{
		cerr << endl << "~IvrWORX::exception caught e:" << e.what() << endl;
	}
	
	
}

