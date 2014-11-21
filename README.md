Use with caution!

In CXX you can use with below code:

```
extern LibraryWin32<HwFactory> hwLib;
	
assert( HlaWrapper::g_HwFactory == NULL );
	try{
		hwLib.InitLibrary( L"./", L"HW_Mak", "CrtHwFactory", HlaWrapper::g_HwFactory );
		return true;
	}catch( OsUtils::ExcLibraryLoad& exc ){
		HlaWrapper::g_HwFactory = NULL;
		XMessageBox::Show( ConvertStr( exc.ToString() ), "Hata",
			MessageBoxButtons::OK, MessageBoxIcon::Error );
		return false;
	}
```

In C# you can use with `__cdecl` calling convention on the above code
