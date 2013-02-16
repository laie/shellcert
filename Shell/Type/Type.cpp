// Type.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{
	if ( argc < 2 ) return -1;
	HANDLE hf = CreateFile(argv[1], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if ( hf == INVALID_HANDLE_VALUE ) return -1;
		
	DWORD size = GetFileSize(hf, 0);
	std::vector<char> buf(size+2);
	DWORD dw = 0;
	ReadFile(hf, buf.data(), buf.size(), &dw, 0);
	buf[dw] = 0;
	std::cout << buf.data();

	CloseHandle(hf);
	return 0;
}

