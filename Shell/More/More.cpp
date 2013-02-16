// More.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

int _tmain(int argc, _TCHAR* argv[])
{
	GetAsyncKeyState(VK_ESCAPE);
	std::list<std::string> listline;
	while ( !std::cin.eof() )
	{
		std::string line;
		std::getline(std::cin, line);
		listline.push_back(line);
	}

	DWORD linecnt = 0;
	for ( auto i=listline.begin(); i != listline.end(); i++ )
	{
		std::cout << *i << std::endl;
		linecnt++;
		if ( linecnt >= 20 )
		{
			linecnt = 0;
			std::cout << "\t ** press esc to see more" << std::endl;
			while ( 0 == (GetAsyncKeyState(VK_ESCAPE) & 0x01) ) Sleep(1);
		}
	}
	return 0;
}

