#ifndef __ADDINNATIVE_H__
#define __ADDINNATIVE_H__

#include "ComponentBase.h"
#include "AddInDefBase.h"
#include "IMemoryManager.h"






///////////////////////////////////////////////////////////////////////////////
// class CAddInNative
class CAddInNative : public IComponentBase
{
public:
	
	enum Props
	{
		ePropConnected = 0,
		ePropListen,
		ePropFilter,
		ePropRegEx,
		ePropVersion,
		ePropErrorAsEvent,
		ePropPort,
		ePropLast      // Always last
	};

	enum Methods
	{
		eMethConnect = 0,
		eMethDisconnect,
		eMethSendCommand,
		eMethListenMode,
		eMethSetFilter,
		eMethSetRegEx,
		eMethSetBufferDepth,
		eMethLast      // Always last
	};

	CAddInNative(void);
	virtual ~CAddInNative();
	// IInitDoneBase
	virtual bool ADDIN_API Init(void*);
	virtual bool ADDIN_API setMemManager(void* mem);
	virtual long ADDIN_API GetInfo();
	virtual void ADDIN_API Done();
	// ILanguageExtenderBase
	virtual bool ADDIN_API RegisterExtensionAs(WCHAR_T**);
	virtual long ADDIN_API GetNProps();
	virtual long ADDIN_API FindProp(const WCHAR_T* wsPropName);
	virtual const WCHAR_T* ADDIN_API GetPropName(long lPropNum, long lPropAlias);
	virtual bool ADDIN_API GetPropVal(const long lPropNum, tVariant* pvarPropVal);
	virtual bool ADDIN_API SetPropVal(const long lPropNum, tVariant* varPropVal);
	virtual bool ADDIN_API IsPropReadable(const long lPropNum);
	virtual bool ADDIN_API IsPropWritable(const long lPropNum);
	virtual long ADDIN_API GetNMethods();
	virtual long ADDIN_API FindMethod(const WCHAR_T* wsMethodName);
	virtual const WCHAR_T* ADDIN_API GetMethodName(const long lMethodNum, const long lMethodAlias);
	virtual long ADDIN_API GetNParams(const long lMethodNum);
	virtual bool ADDIN_API GetParamDefValue(const long lMethodNum, const long lParamNum, tVariant *pvarParamDefValue);
	virtual bool ADDIN_API HasRetVal(const long lMethodNum);
	virtual bool ADDIN_API CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray);
	virtual bool ADDIN_API CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray);
	// LocaleBase
	virtual void ADDIN_API SetLocale(const WCHAR_T* loc);
	
	//


	

	// они должны быть глобальными, чтобы поток прослушивания мог с ними работать
	// как сделать его "другом" класса? тогда можно было бы спрятать это все в приват

	int connected;
	int listen;
	
	bool		SendEvent(wchar_t *msg, wchar_t *Data);
	void		OnDisconnect();
	void		OnError(long errCode, wchar_t *errDesc);
	void		OnError();

	CRITICAL_SECTION cs;
	SOCKET Socket;
	HANDLE hTh;  // дексриптор прослушивающего треда 

	

	
private:
	
	
	long findName(const wchar_t* names[], const wchar_t* name, const uint32_t size) const;
	void addError(uint32_t wcode, const wchar_t* source, const wchar_t* descriptor, long code);
	
	// Attributes
	IAddInDefBase      *m_iConnect;
	IMemoryManager     *m_iMemory;
	
	
	bool Connect(wchar_t*, int);
	bool Disconnect();
	
	
	bool SendCommand(wchar_t*);
	bool ListenMode(int);
	bool setFilter(wchar_t*);
	bool setRegEx(wchar_t*);
	bool setBufferDepth(int);

	char* WCHAR_2_CHAR(wchar_t *txt);
	wchar_t* getErrorDescription(DWORD dwErr);

	static wchar_t* getName();

	int errorAsEvent;
	wchar_t* filter;
	wchar_t* regEx;

	int port;

	
	
};


class WcharWrapper
{
public:
	WcharWrapper(const wchar_t* str);
	~WcharWrapper();
	operator const wchar_t*() { return m_str_wchar; }
	operator wchar_t*() { return m_str_wchar; }
private:
	WcharWrapper& operator = (const WcharWrapper& other) {};
	WcharWrapper(const WcharWrapper& other) {};
private:
	wchar_t* m_str_wchar;
};
#endif //__ADDINNATIVE_H__