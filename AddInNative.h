
#ifndef __ADDINNATIVE_H__
#define __ADDINNATIVE_H__

#ifndef __linux__
#include <wtypes.h>
#endif //__linux__

#include "ComponentBase.h"
#include "AddInDefBase.h"
#include "IMemoryManager.h"


std::vector<std::string> resplit(const std::string & s, std::string rgx_str);
char* WCHAR_2_CHAR(wchar_t *in_str);
wchar_t* CHAR_2_WCHAR(char *in_str);

///////////////////////////////////////////////////////////////////////////////
// class CAddInNative
class CAddInNative : public IComponentBase
{
public:
	
	enum Props
	{
		ePropConnected = 0,
		ePropListen,
		ePropRegEx,
		ePropVersion,
		ePropErrorAsEvent,
		ePropIsDemo,
		ePropID,
		ePropKey,
		ePropLast      // Always last
	};

	enum Methods
	{
		eMethodConnect = 0,
		eMethodDisconnect,
		eMethodSendCommand,
		eMethodListenMode,
		eMethodSetRegEx,
		eMethodLast      // Always last
	};
	//*Innova-it

	// --------------------------------------------------Стандартные объявления 1С
	CAddInNative(void);
	virtual ~CAddInNative();
	// IInitDoneBase
	virtual bool ADDIN_API Init(void*);
	virtual bool ADDIN_API setMemManager(void* mem);
	virtual long ADDIN_API GetInfo();
	virtual void ADDIN_API Done();
	// ILanguageExtenderBase
	virtual bool ADDIN_API RegisterExtensionAs(WCHAR_T** wsLanguageExt);
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
	operator IComponentBase*() { return (IComponentBase*)this; }
	// LocaleBase
	virtual void ADDIN_API SetLocale(const WCHAR_T* loc);
	//  -----------------------------------Конец стандартные объявления 1С


	
	bool connected;
	bool listen;
	bool isDemo;                      // Демо режим
	int count_event = 0;                  // Кол-во сообщений до ДЕМО режима

	bool		SendEvent(wchar_t *msg, wchar_t *Data);
	
	void		OnDisconnect();
	void		OnError(long errCode, wchar_t *errDesc);
	void		OnError();

	SOCKET ConnectSocket;
	HANDLE hTh;

	
private:
	wchar_t* wsName = L"InnovaIT-Asterisk-Native";

	long findName(const wchar_t* names[], const wchar_t* name, const uint32_t size) const;
	void addError(uint32_t wcode, const wchar_t* source, const wchar_t* descriptor, long code);

	IAddInDefBase      *m_iConnect;
	IMemoryManager     *m_iMemory;


	bool Connect(wchar_t*, wchar_t*);
	bool Disconnect();
	
	bool SendCommand(wchar_t*);
	bool ListenMode(int);
	bool setRegEx(wchar_t*);
	
	wchar_t* getErrorDescription(DWORD dwErr);
		
	bool errorAsEvent;
	wchar_t* regEx =L"";
		
	wchar_t* key = L"";
	// Защита
	wchar_t  computer_id[35] = { 0 }; // ID компьютера
	wchar_t  valid_key[35] = { 0 };   // Ответ
	
	
	bool GetValueFromKey(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValue, LPVOID pBuffer, ULONG uSize);
	void SetComputerID(wchar_t* id, DWORD date);
	// *Защита
	
};
#endif //__ADDINNATIVE_H__
