#ifndef LIBRARY_WIN32_H_
#define LIBRARY_WIN32_H_

#include <iostream>
using namespace std;

#include <windows.h>

//#if defined(WIN32_LEAN_AND_MEAN)
//#undef WIN32_LEAN_AND_MEAN
//#include <windows.h>
//#define WIN32_LEAN_AND_MEAN
//#else
//#include <windows.h>
//#endif


//#define WORD Win32::WORD
//#define PTSTR Win32::PTSTR
//#define FILETIME Win32::FILETIME

#include <assert.h>

#include <Containers/Generics.h>
#include <Containers/ObjHandle.h>
#include "OsUtilsExcSet.h"
using namespace Containers;


/// <summary>
/// Dinamik yazılım kütüphanesinden istenilen fonksiyonu veya nesneyi elde etmeyi sağlayan sınıftır.
/// </summary>
template<class T, DEL_TYPE selfDestroy = DT_SELF>
class LibraryWin32
{
public:
	typedef T* ( _stdcall* CRT_FACTORY_PROC )( void );

	LibraryWin32( )
		:ins(NULL),libHnd(NULL),proc(NULL)
	{}

	LibraryWin32(  const wchar_t* relativePath,const wchar_t* _libraryName, const char* _procLiteral, T*& out )
		:ins(NULL),libHnd(NULL),proc(NULL)
	{
		try{
			InitLibrary(relativePath, _libraryName, _procLiteral, out);
		}catch(OsUtils::ExcLibraryLoad& exc){
			wcout << exc.ToString() << endl;
			//MessageBox(NULL, exc.ToString().c_str(), L"Kutuphane Hatasi", MB_OK );
		}
	}

	wstring GenerateExcStr(  DWORD dwError )
	{
		wstring str;
		 HLOCAL hlocal = NULL;// Buffer that gets the error message string

#define ERROR_LANG MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)

		// Get the error code's textual description
		BOOL fOk = FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, 
			NULL, dwError, ERROR_LANG, 
			(PTSTR) &hlocal, 0, NULL);

		if (!fOk) {
			// Is it a network-related error?
			HMODULE hDll = LoadLibraryEx(TEXT("netmsg.dll"), NULL, 
				DONT_RESOLVE_DLL_REFERENCES);

			if (hDll != NULL) {
				FormatMessage(
					FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM,
					hDll, dwError, ERROR_LANG,
					(PTSTR) &hlocal, 0, NULL);
				FreeLibrary(hDll);
			}
		}

		if (hlocal != NULL) {
			str.assign( (PCTSTR) LocalLock(hlocal) );
			LocalFree(hlocal);
		} else {
			str.assign( L"Undefined Error !!" );
		}

		return str;
	}

	void DumpException( DWORD dwError )
	{
		wcout << GenerateExcStr( dwError );
		throw OsUtils::ExcLibraryLoad( libraryName, libraryPath, GenerateExcStr( dwError ) );
	}


	bool InitLibrary( const wchar_t* relativePath,const wchar_t* _libraryName, const char* _procLiteral, T*& out  )
	{
		bool res = InitLibrary(relativePath, _libraryName, _procLiteral);
		if( res == true)
			out = ins;
		return res;
	}

	//InitLibrary
	//Verilen dizin ve dosyaya göre dinamik kütüphane dosyası yüklenir.
	//Dosya ismi belirlenirken derleme ayarlarındaki 32-64 bit ve Debug-Release tercihleri dikkate alınır.
	//Dosya ismi belirlenirken şu format esas alınır; "KütüphaneAdı[_d][_64].uzantı".
	//Debug derleme ise "_d" katarı, 64-bit derleme ise "_64" katarı kütüphane adına eklenir.
	//Elde edilen katara dinamik kütüphane uzantısı eklenir.
	//Bu uzantı Windows sistemler için ".dll" dir.
	//Dinamik kütüphane yüklendikten sonra Factory sınıfını elde etmek için kullanılacak olan fonksiyon ismi sorgulanır.
	//Tüm bunların başarıyla gerçekleşmesi durumunda "true" diğer hatalı durumlarda ise "false" döner.
	bool InitLibrary( const wchar_t* relativePath,const wchar_t* _libraryName, const char* _procLiteral )
	{
		//prevent second time calling
		assert( libHnd == NULL );

		procLiteral.assign(_procLiteral);
		libraryName.assign( _libraryName );

		//prepare library path
		libraryPath.assign(relativePath);
		//if( libraryPath[ libraryPath.length() -1  ] != L'/' )
		//	libraryPath.append( L"/" );
		libraryPath.append( libraryName );

		//for debug mode builds
#ifdef _DEBUG
		libraryPath.append( L"_d" );
#endif

		//for 64 bit builds
#ifdef _M_X64
		libraryPath.append( L"_64" );
#endif

		libraryPath.append( L".dll" );

		libHnd = LoadLibrary( libraryPath.c_str() );
		if( libHnd == NULL){
			wcout << L"Kutuphane yuklenmesi ile ilgili hata olustu: " << libraryPath.c_str() << endl;
			DumpException( GetLastError() );
			return false;
		}

		proc = reinterpret_cast<CRT_FACTORY_PROC> ( GetProcAddress( libHnd, procLiteral.c_str() ) );
		if( proc == NULL ){
			wcout << L"Kutuphane formati ile ilgili hata olustu: " << libraryPath.c_str() << endl;
			DumpException( GetLastError() );
			return false;
		}

		if( CreateFactory() == NULL ){//throw exception
			return false;
		}

		return true;
	}

	T* GetFactory()
	{
		assert( libHnd != NULL && proc != NULL );//InitLibrary must succeed before this call
		return ins;
	}

	//Yüklenen kütüphaneden Factory nesnesi elde edilir.
	//Factory nesnesinin tek örneği vardır (Singleton). Bu tek nesne kütüphane ile birlikte silinir.
private:
	T* CreateFactory()
	{
		ins = (*proc)();//call factory creator proc. the only one exported function from dll
		return ins;
	}

	public:
	void OnDtor( Int2Type<DT_SELF> selfDestroy )
	{
		if( ins != NULL )//destroy unique instance if it is created
			ins->Destroy();
	}

	void OnDtor( Int2Type<DT_OPRT> selfDestroy )
	{
		if( ins != NULL )
			delete ins;
	}

	void OnDtor( Int2Type<DT_NOT> selfDestroy )
	{
		//silinmiyor
	}

	~LibraryWin32()
	{
		OnDtor( Int2Type<selfDestroy>() );

		//free library if it is loaded
		if( libHnd != NULL )
			FreeLibrary( libHnd );
	}
protected:
	HINSTANCE libHnd;///win32 dll handle
	T* ins;//unique instance of factory
	CRT_FACTORY_PROC proc;
	string procLiteral;
	wstring libraryPath;
	wstring libraryName;
};

#endif
