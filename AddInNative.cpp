#include "stdafx.h"

//#include <winsock2.h>
//#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include <stdio.h>
#include <wchar.h>
#include "AddInNative.h"
#include <string>



static const wchar_t *g_PropNames[] = { L"Connected",
										L"Listen",
										L"Filter",
										L"RegEx",
										L"Version",
										L"ErrorAsEvent"
};

static const wchar_t *g_PropNamesRu[] = {	L"Подключено",
											L"РежимПрослушивания ",
											L"Фильтр",
											L"РегулярноеВыражение",
											L"Версия",
											L"ОшибкаКакСобытие"
};


static const wchar_t *g_MethodNames[] = {	L"Connect",
											L"Disconnect",
											L"SendCommand",
											L"ListenMode",
											L"SetFilter",
											L"SetRegEx",
											L"SetBufferDepth"
};

static const wchar_t *g_MethodNamesRu[] = { L"Подключиться",
											L"Отключиться",
											L"ВыполнитьКоманду",
											L"РежимПрослушивания",
											L"УстановитьФильтр",
											L"УстановитьРегулярноеВыражение",
											L"УстановитьГлубинуБуфера" 
};




static const wchar_t g_kClassNames[] = L"CAddInNative"; //"|OtherClass1|OtherClass2";
static IAddInDefBase *pAsyncEvent = NULL;

uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, uint32_t len = 0);
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len = 0);
uint32_t getLenShortWcharStr(const WCHAR_T* Source);

static AppCapabilities g_capabilities = eAppCapabilitiesInvalid;
static WcharWrapper s_names(g_kClassNames);







//---------------------------------------------------------------------------//
long GetClassObject(const WCHAR_T* wsName, IComponentBase** pInterface)
{
	if (!*pInterface)
	{
		*pInterface = new CAddInNative;
		return (long)*pInterface;
	}
	return 0;
}
//---------------------------------------------------------------------------//
AppCapabilities SetPlatformCapabilities(const AppCapabilities capabilities)
{
	g_capabilities = capabilities;
	return eAppCapabilitiesLast;
}
//---------------------------------------------------------------------------//
long DestroyObject(IComponentBase** pIntf)
{
	if (!*pIntf)
		return -1;


	
	delete *pIntf;
	*pIntf = 0;
	return 0;
}
//---------------------------------------------------------------------------//
const WCHAR_T* GetClassNames()
{
    return s_names;
}



// код прослушивающего треда
static unsigned int _stdcall RecvInThread(void*p)
{
	//Sleep(500);
	CAddInNative *tcpCl = (CAddInNative*)p;

	int len = 1 << 16;
	//int len = 65536;
	char *buf = new char[len];
	int recived = 0;
	bool disconnect = false;
	
	while (tcpCl->connected == 1 && tcpCl->listen == 1)
	{
		recived = recv(tcpCl->Socket, buf, len, 0);
		if (recived > 0)
		{
			buf[recived] = 0;
			EnterCriticalSection(&(tcpCl->cs));
			wchar_t *res = new wchar_t[recived + 1];
			mbstowcs(res, buf, recived + 1);
			tcpCl->SendEvent(L"Recived", res);
			LeaveCriticalSection(&(tcpCl->cs));
		}
		else
		{
			int nError = WSAGetLastError();
			if (recived == 0 || (nError != WSAEWOULDBLOCK && nError != 0))
			{
				disconnect = true;
				break;
			}
		}
		
		Sleep(100);
		
	}

	if (tcpCl->hTh)
		CloseHandle(tcpCl->hTh);
	
	tcpCl->hTh = 0;
	
	if (disconnect && tcpCl->connected == 1)
		tcpCl->OnDisconnect();
	
	
	return 0;
}



// CAddInNative
//---------------------------------------------------------------------------//
CAddInNative::CAddInNative()
{
	
	m_iMemory = 0;
	m_iConnect = 0;

}
//---------------------------------------------------------------------------//
CAddInNative::~CAddInNative()
{
	
}

//---------------------------------------------------------------------------//
bool CAddInNative::Init(void* pConnection)
{
	m_iConnect = (IAddInDefBase*)pConnection;
	
	if (m_iConnect == NULL)
		return false;

	InitializeCriticalSection(&cs);

	connected = 0;
	listen = 0;
	Socket = 0;
	hTh = 0;
	errorAsEvent = 0;
	filter = L"";
	regEx = 0;

	// Initialise Winsock
	WSADATA WsaDat;
	if (WSAStartup(MAKEWORD(2, 2), &WsaDat) != 0)
	{
		//AddError
		WSACleanup();
		
		return false;
	}




	return true;
}

//---------------------------------------------------------------------------//
long CAddInNative::GetInfo()
{
	// Component should put supported component technology version 
	// This component supports 2.0 version
	return 2000;
}

//---------------------------------------------------------------------------//
void CAddInNative::Done()
{

	Disconnect();

	

}



/////////////////////////////////////////////////////////////////////////////
// ILanguageExtenderBase
//---------------------------------------------------------------------------//
bool CAddInNative::RegisterExtensionAs(WCHAR_T** wsExtensionName)
{
	const wchar_t* wsExtension = L"BROM-Asterisk-Native";
	int iActualSize = ::wcslen(wsExtension) + 1;
	WCHAR_T* dest = 0;

	if (m_iMemory)
	{
		if (m_iMemory->AllocMemory((void**)wsExtensionName, iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(wsExtensionName, wsExtension, iActualSize);
		return true;
	}

	return false;
}

//---------------------------------------------------------------------------//
long CAddInNative::GetNProps()
{
	// You may delete next lines and add your own implementation code here
	return ePropLast;
}

//---------------------------------------------------------------------------//
long CAddInNative::FindProp(const WCHAR_T* wsPropName)
{
	long plPropNum = -1;
	wchar_t* propName = 0;

	::convFromShortWchar(&propName, wsPropName);
	plPropNum = findName(g_PropNames, propName, ePropLast);

	if (plPropNum == -1)
		plPropNum = findName(g_PropNamesRu, propName, ePropLast);

	delete[] propName;

	return plPropNum;
}

//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetPropName(long lPropNum, long lPropAlias)
{
	if (lPropNum >= ePropLast)
		return NULL;

	wchar_t *wsCurrentName = NULL;
	WCHAR_T *wsPropName = NULL;
	int iActualSize = 0;

	switch (lPropAlias)
	{
	case 0: // First language
		wsCurrentName = (wchar_t*)g_PropNames[lPropNum];
		break;
	case 1: // Second language
		wsCurrentName = (wchar_t*)g_PropNamesRu[lPropNum];
		break;
	default:
		return 0;
	}

	iActualSize = wcslen(wsCurrentName) + 1;

	if (m_iMemory && wsCurrentName)
	{
		if (m_iMemory->AllocMemory((void**)&wsPropName, iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(&wsPropName, wsCurrentName, iActualSize);
	}

	return wsPropName;
}

//---------------------------------------------------------------------------//
bool CAddInNative::GetPropVal(const long lPropNum, tVariant* pvarPropVal)
{
	wchar_t* temp;
	size_t tempSize = 0;

	switch (lPropNum)
	{
	
	case ePropConnected:
		TV_VT(pvarPropVal) = VTYPE_I4;
		TV_I4(pvarPropVal) = connected;
		break;
	case ePropListen:
		TV_VT(pvarPropVal) = VTYPE_I4;
		TV_I4(pvarPropVal) = listen;
		break;

	case ePropFilter:

		temp = filter;
		tempSize = ::wcslen(temp);

		if (m_iMemory)
		{
			if (m_iMemory->AllocMemory((void**)&pvarPropVal->pwstrVal, tempSize * sizeof(WCHAR_T)))
			{
				::convToShortWchar(&pvarPropVal->pwstrVal, temp, tempSize);
				pvarPropVal->strLen = tempSize;
				TV_VT(pvarPropVal) = VTYPE_PWSTR;
			}
		}
		break;

	case ePropRegEx:

		temp = regEx;
		tempSize = ::wcslen(temp);

		if (m_iMemory)
		{
			if (m_iMemory->AllocMemory((void**)&pvarPropVal->pwstrVal, tempSize * sizeof(WCHAR_T)))
			{
				::convToShortWchar(&pvarPropVal->pwstrVal, temp, tempSize);
				pvarPropVal->strLen = tempSize;
				TV_VT(pvarPropVal) = VTYPE_PWSTR;
			}
		}
		break;

	
	case ePropVersion:

		temp = L"1.0.0";
		tempSize = ::wcslen(temp);

		if (m_iMemory)
		{
			if (m_iMemory->AllocMemory((void**)&pvarPropVal->pwstrVal, tempSize * sizeof(WCHAR_T)))
			{
				::convToShortWchar(&pvarPropVal->pwstrVal, temp, tempSize);
				pvarPropVal->strLen = tempSize;
				TV_VT(pvarPropVal) = VTYPE_PWSTR;
			}
		}
		break;

	case ePropErrorAsEvent:
		TV_VT(pvarPropVal) = VTYPE_I4;
		TV_I4(pvarPropVal) = errorAsEvent;
		break;

	default:
		return false;
	}

	return true;
		
}






//---------------------------------------------------------------------------//
bool CAddInNative::SetPropVal(const long lPropNum, tVariant *varPropVal)
{
	switch (lPropNum) // не забыть, что некоторые свойства не записываются, а только читаются
	{
	
	case ePropErrorAsEvent:
		if (TV_VT(varPropVal) != VTYPE_I4)
			return false;
		errorAsEvent = TV_I4(varPropVal);
		return true;
		break;
			
	default:
		return false;
	}

	return false;
}

//---------------------------------------------------------------------------//
bool CAddInNative::IsPropReadable(const long lPropNum)
{
	return true; // все свойства можно читать
}
//---------------------------------------------------------------------------//
bool CAddInNative::IsPropWritable(const long lPropNum)
{
	switch (lPropNum)
	{
	
	case ePropErrorAsEvent:

		return true;


	default:
		return false;
	}

	return false;
}

//---------------------------------------------------------------------------//
long CAddInNative::GetNMethods()
{
	return eMethLast;
}
//---------------------------------------------------------------------------//
long CAddInNative::FindMethod(const WCHAR_T* wsMethodName)
{
	long plMethodNum = -1;
	wchar_t* name = 0;

	::convFromShortWchar(&name, wsMethodName);

	plMethodNum = findName(g_MethodNames, name, eMethLast);

	if (plMethodNum == -1)
		plMethodNum = findName(g_MethodNamesRu, name, eMethLast);

	delete[] name;

	return plMethodNum;
}

//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetMethodName(const long lMethodNum, const long lMethodAlias)
{
	if (lMethodNum >= eMethLast)
		return NULL;

	wchar_t *wsCurrentName = NULL;
	WCHAR_T *wsMethodName = NULL;
	int iActualSize = 0;

	switch (lMethodAlias)
	{
	case 0: // First language
		wsCurrentName = (wchar_t*)g_MethodNames[lMethodNum];
		break;
	case 1: // Second language
		wsCurrentName = (wchar_t*)g_MethodNamesRu[lMethodNum];
		break;
	default:
		return 0;
	}

	iActualSize = wcslen(wsCurrentName) + 1;

	if (m_iMemory && wsCurrentName)
	{
		if (m_iMemory->AllocMemory((void**)&wsMethodName, iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(&wsMethodName, wsCurrentName, iActualSize);
	}

	return wsMethodName;
}

//---------------------------------------------------------------------------//
long CAddInNative::GetNParams(const long lMethodNum)
{
	switch (lMethodNum)
	{
	case eMethSendCommand:
	case eMethListenMode:
	case eMethSetFilter:
	case eMethSetRegEx:
	case eMethSetBufferDepth:
		return 1;
	case eMethConnect:
		return 2;
	
	default:
		return 0;
	}

	return 0;
}





//---------------------------------------------------------------------------//
bool CAddInNative::GetParamDefValue(const long lMethodNum, const long lParamNum,
	tVariant *pvarParamDefValue)
{
	TV_VT(pvarParamDefValue) = VTYPE_EMPTY;

	switch (lMethodNum)
	{
	
	case eMethConnect:
		switch (lParamNum)
		{
		case 1: // port 
			
			TV_VT(pvarParamDefValue) = VTYPE_I4;
			TV_I4(pvarParamDefValue) = 5038;
			return true;
		
		}
		
	default:
		return false;
	}

	return false;
}


//---------------------------------------------------------------------------//
bool CAddInNative::HasRetVal(const long lMethodNum)
{
	switch (lMethodNum)
	{
	case eMethConnect:
	case eMethDisconnect:
	case eMethSendCommand:
	case eMethListenMode:
	case eMethSetFilter:
	case eMethSetRegEx:
	case eMethSetBufferDepth:
		return true;
	default:
		return false;
	}

	return false;
}



//---------------------------------------------------------------------------//
bool CAddInNative::CallAsProc(const long lMethodNum,
	tVariant* paParams, const long lSizeArray)
{

	return true; // ни один из методов не вызывается как процедура

}


//---------------------------------------------------------------------------//
bool CAddInNative::CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{

	switch (lMethodNum)
	{
		
		case eMethConnect:
		{
			if (!lSizeArray || !paParams)
				return false; // если нет параметров то ошибка

			if (TV_VT(paParams) != VTYPE_PWSTR) // проверяем тип первого параметра Сервер
				return false;
			
			wchar_t* Server = 0;
			::convFromShortWchar(&Server, TV_WSTR(paParams));
			
			if (TV_VT(paParams + 1) != VTYPE_I4) // проверяем тип второго параметра Порт
				return false;

			int Port = TV_I4(paParams + 1);

			return Connect(Server, Port);
		}

		case eMethDisconnect:
		{
			if (lSizeArray || paParams)
				return false; // если есть параметры то ошибка
			
			return Disconnect();
		}
		
		case eMethSendCommand:
		{
			if(!lSizeArray || !paParams)
				return false; // если нет параметров то ошибка

			if (TV_VT(paParams) != VTYPE_PWSTR) // проверяем тип первого параметра
				return false;
			
			wchar_t* msg = 0;
			::convFromShortWchar(&msg, TV_WSTR(paParams));

			return SendCommand(msg);
		}


		case eMethListenMode:
		{
			if (!lSizeArray || !paParams)
				return false; // если нет параметров то ошибка

			if (TV_VT(paParams) != VTYPE_I4) // проверяем тип первого параметра
				return false;
		
			return ListenMode(TV_I4(paParams));
		}

		case eMethSetFilter:
		{
			if (!lSizeArray || !paParams)
				return false; // если нет параметров то ошибка

			if (TV_VT(paParams) != VTYPE_PWSTR) // проверяем тип первого параметра
				return false;

			wchar_t* filter = 0;
			::convFromShortWchar(&filter, TV_WSTR(paParams));

			return setFilter(filter);
		}

		case eMethSetRegEx:
		{
			if (!lSizeArray || !paParams)
				return false; // если нет параметров то ошибка

			if (TV_VT(paParams) != VTYPE_PWSTR) // проверяем тип первого параметра
				return false;

			wchar_t* regEx = 0;
			::convFromShortWchar(&regEx, TV_WSTR(paParams));

			return setRegEx(regEx);
		}

		case eMethSetBufferDepth:
		{
			if (!lSizeArray || !paParams)
				return false; // если нет параметров то ошибка

			if (TV_VT(paParams) != VTYPE_I4)
				return false;

			int depth = TV_I4(paParams);

			return setBufferDepth(depth);
		}




		default:
			return false;
	
	}
	return false;
}

bool CAddInNative::setBufferDepth(int depth)
{

	if (m_iConnect)
		m_iConnect->SetEventBufferDepth(depth);
	else
		return false;

	return true;


}


//---------------------------------------------------------------------------//
void CAddInNative::SetLocale(const WCHAR_T* loc)
{
	_wsetlocale(LC_ALL, loc);

}

/////////////////////////////////////////////////////////////////////////////
// LocaleBase
//---------------------------------------------------------------------------//
bool CAddInNative::setMemManager(void* mem)
{
	m_iMemory = (IMemoryManager*)mem;
	return m_iMemory != 0;
}


//---------------------------------------------------------------------------//
void CAddInNative::addError(uint32_t wcode, const wchar_t* source,
	const wchar_t* descriptor, long code)
{
	if (m_iConnect)
	{
		WCHAR_T *err = 0;
		WCHAR_T *descr = 0;

		::convToShortWchar(&err, source);
		::convToShortWchar(&descr, descriptor);

		m_iConnect->AddError(wcode, err, descr, code);
		delete[] err;
		delete[] descr;
	}
}


//---------------------------------------------------------------------------//
long CAddInNative::findName(const wchar_t* names[], const wchar_t* name,
	const uint32_t size) const
{
	long ret = -1;
	for (uint32_t i = 0; i < size; i++)
	{
		if (!wcscmp(names[i], name))
		{
			ret = i;
			break;
		}
	}
	return ret;
}

//---------------------------------------------------------------------------//
uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, uint32_t len)
{
	if (!len)
		len = ::wcslen(Source) + 1;

	if (!*Dest)
		*Dest = new WCHAR_T[len];

	WCHAR_T* tmpShort = *Dest;
	wchar_t* tmpWChar = (wchar_t*)Source;
	uint32_t res = 0;

	::memset(*Dest, 0, len * sizeof(WCHAR_T));
	for (; len; --len, ++res, ++tmpWChar, ++tmpShort)
	{
		*tmpShort = (WCHAR_T)*tmpWChar;
	}

	return res;
}

//---------------------------------------------------------------------------//
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len)
{
	if (!len)
		len = getLenShortWcharStr(Source) + 1;

	if (!*Dest)
		*Dest = new wchar_t[len];

	wchar_t* tmpWChar = *Dest;
	WCHAR_T* tmpShort = (WCHAR_T*)Source;
	uint32_t res = 0;

	::memset(*Dest, 0, len * sizeof(wchar_t));
	for (; len; --len, ++res, ++tmpWChar, ++tmpShort)
	{
		*tmpWChar = (wchar_t)*tmpShort;
	}

	return res;
}

//---------------------------------------------------------------------------//
uint32_t getLenShortWcharStr(const WCHAR_T* Source)
{
	uint32_t res = 0;
	WCHAR_T *tmpShort = (WCHAR_T*)Source;

	while (*tmpShort++)
		++res;

	return res;
}


//---------------------------------------------------------------------------//
WcharWrapper::WcharWrapper(const wchar_t* str) :
	m_str_wchar(NULL)
{
	if (str)
	{
		int len = wcslen(str);
		m_str_wchar = new wchar_t[len + 1];
		memset(m_str_wchar, 0, sizeof(wchar_t) * (len + 1));
		memcpy(m_str_wchar, str, sizeof(wchar_t) * len);
	}

}

//---------------------------------------------------------------------------//
WcharWrapper::~WcharWrapper()
{
	if (m_str_wchar)
	{
		delete[] m_str_wchar;
		m_str_wchar = NULL;
	}
}
//---------------------------------------------------------------------------//





wchar_t* CAddInNative::getErrorDescription(DWORD dwErr)
{
	LPVOID lpMsgBuf;
	::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
	LPWSTR str = (LPWSTR)lpMsgBuf;
	return str;
}


char* CAddInNative::WCHAR_2_CHAR(wchar_t *txt)
{
	int len = wcslen(txt);
	char*str = new char[len + 1];
	wcstombs(str, txt, len);
	str[len] = 0;
	return str;
}


//---------------------------------------------------------------------------//
bool CAddInNative::SendEvent(wchar_t* msg, wchar_t* Data)
{
	if (m_iConnect)
	{
		if (regEx != 0)
		{
			if (wcsstr(Data, regEx) == NULL)
				return false;
			
		
		}
		bool res = m_iConnect->ExternalEvent(getName(), msg, Data);
		
		return res;
	}
	
	return false;
}



wchar_t* CAddInNative::getName()
{
	const wchar_t* wsExtension = L"BROM-Asterisk"; // то как нас опознает код внешнего события 1С
	return (wchar_t*)wsExtension;
}



// библиотека WINSOCK традиционно возвращает 0 (ноль) если выполнение функции успешно
bool CAddInNative::Connect(wchar_t* server, int port)
{

	if (connected == 1)
		Disconnect();

	connected = 0;

	// 

	// переменная объявлена в h
	Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Socket == INVALID_SOCKET)
	{
		//AddError
		WSACleanup();
		
		return false;
	}

	



		
	SOCKADDR_IN srvAddr;

	ZeroMemory(&srvAddr, sizeof(srvAddr));

	srvAddr.sin_family = AF_INET;
	srvAddr.sin_port = htons(port);
	
	LPSTR str = WCHAR_2_CHAR(server);

	srvAddr.sin_addr.s_addr = inet_addr(str);

	if (srvAddr.sin_addr.s_addr == INADDR_NONE) //если на входе не IP адрес, а имя хоста, то преобразуем его
	{
		hostent * host = gethostbyname(str);
		if (host != NULL)
		{
			memcpy(&srvAddr.sin_addr, host->h_addr_list[0], host->h_length);
		}
		else
		{
			DWORD dwErr = WSAGetLastError();

			OnError(dwErr, getErrorDescription(dwErr)); 
			return false;
		}
	}

	
	if (connect(Socket, (sockaddr*)&srvAddr, sizeof(srvAddr)) == SOCKET_ERROR)
	{
		DWORD dwErr = WSAGetLastError();
		
		OnError(dwErr, getErrorDescription(dwErr)); 

		closesocket(Socket);
		WSACleanup();

		return false;
	}
	
	// перевели в неблокирующий режим
	u_long iMode = 1;
	int iResult;

	iResult = ioctlsocket(Socket, FIONBIO, &iMode);

	if (iResult != NO_ERROR)
	{

		closesocket(Socket);
		//AddError
		WSACleanup();

		return false;

	}


	connected = 1;
	
	SendEvent(L"Connected", server);
		

	return true; // все ОК
}

bool CAddInNative::Disconnect()
{
	if (connected == 0)
		return true;

	EnterCriticalSection(&cs);
	
	if (connected == 0)
		return true;
	
	connected = 0;

	
	wchar_t str[256];

	int res = 0;
	bool bRes = true;

	try {
		res = shutdown(Socket, 2);
		str[0] = 0;

		linger l;
		l.l_linger = 1;
		l.l_onoff = 1;

		res = setsockopt(Socket, SOL_SOCKET, SO_LINGER, (char*)&l, sizeof(l));
		res = closesocket(Socket);

		if (res == SOCKET_ERROR)
		{
			OnError();
			bRes = false;
			//SendErrorEvent(str);
		}
	}
	catch (...) {
		res = false;
	};

	
	LeaveCriticalSection(&cs);
	
	if(hTh)
	{
		WaitForSingleObject(hTh, INFINITE);
		hTh = 0;
	}
	

	return bRes;
}

bool CAddInNative::ListenMode(int flag)
{
	if (flag == 1)
	{
		hTh = 0;

		unsigned int thID;
		
		listen = 1;

		hTh = (HANDLE)_beginthreadex(NULL, 10, RecvInThread, (LPVOID)this, 0, &thID);

		if (hTh == 0)
			return false;
	} 
	else
	{

		if (hTh)
		{
			listen = 0;
			DWORD stopThRes = WaitForSingleObject(hTh, INFINITE);
			hTh = 0;
			if (stopThRes != WAIT_OBJECT_0)
				return false;
		}

	}
	
	return true;
}

bool CAddInNative::SendCommand(wchar_t* msg)
{
	
	int sendCount = 0;

	if (connected == 0)
	{
		//OnError(L"Send",0,L"No connection");
		return false;
	}
	else
	{

		char* toSend = WCHAR_2_CHAR(msg);
		int l = strlen(toSend);
		sendCount = send(Socket, toSend, l, 0);
		if (sendCount == SOCKET_ERROR)
		{
			
			OnError();
			OnDisconnect();
			return false;
		}

		return true;
	}

}


bool CAddInNative::setFilter(wchar_t* str_filter)
{

	filter = str_filter;
	return true;
}

bool CAddInNative::setRegEx(wchar_t* str_regex)
{
	
	regEx = str_regex;
	if (wcslen(str_regex) == 0)
		regEx = 0;
	
	return true;

}


// обработчики псевдо-событий

void CAddInNative::OnDisconnect()
{
	if (connected == 1)
	{
		SendEvent(L"Disconnect", L"");
		Disconnect();
		connected = 0;
	}
}

void CAddInNative::OnError()
{
	DWORD dwErr = GetLastError();
	if (dwErr)
		OnError(dwErr, getErrorDescription(dwErr));
}

void CAddInNative::OnError(long scode, wchar_t *descr)
{
	//if (ErrorAsEvent)
		

	if (m_iConnect)
		m_iConnect->AddError(ADDIN_E_FAIL, getName(), descr, scode); //Если scode имеет не нулевое значение – будет сгенерировано исключение, которое может быть перехвачено и обработано средствами встроенного языка 1С:Предприятия.
	

}

