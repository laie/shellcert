// Shell.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

class Shell
{
private:
	std::vector<std::wstring> SplitToken(const std::wstring& Str)
	{
		std::vector<std::wstring> tokens;

		std::wstring curtoken;
		for ( DWORD i=0; i < Str.size(); i++ )
		{
			if ( Str[i] == L' ' )
			{
				tokens.push_back(curtoken);
				curtoken = L"";
			} else if ( Str[i] == L'"' )
			{
				i++;
				for ( ; i < Str.size(); i++ )
				{
					if ( Str[i] == L'"' ) break;
					curtoken.push_back(Str[i]);
				}
				continue;
			} else curtoken.push_back(Str[i]);
		}

		tokens.push_back(curtoken);

		return tokens;
	}

	void CommandDir()
	{
		WIN32_FIND_DATA fd;
		HANDLE hff = FindFirstFile(L"*", &fd); 

		if ( hff != INVALID_HANDLE_VALUE )
		{
			do
			{
				std::wcout << fd.cFileName << std::endl;
			} while ( FindNextFile(hff, &fd) );
			FindClose(hff);
		}
	}

	void CommandMd(const std::wstring& Directory)
	{
		if ( !CreateDirectory(Directory.c_str(), NULL) ) std::wcout << L"Failed to make a directory " + Directory << std::endl;
	}

	void CommandRd(const std::wstring& RemovingDir)
	{
		DWORD ret = FILE_ATTRIBUTE_DIRECTORY | GetFileAttributes(RemovingDir.c_str());
		if ( ret != INVALID_FILE_ATTRIBUTES
			&& (ret & FILE_ATTRIBUTE_DIRECTORY)
			&& !(ret & FILE_ATTRIBUTE_ARCHIVE) )
		{
			// directory
			SetLastError(0);
			if ( !RemoveDirectory(RemovingDir.c_str()) || GetLastError() ) std::wcout << L"Failed to delete the directory " + RemovingDir << std::endl;
		} else
		{
			// file or somewhat failed
			std::wcout << L"no directory's there" << std::endl;
		}
	}

	void CommandCd(const std::wstring& Directory)
	{
		if ( !SetCurrentDirectory(Directory.c_str()) ) std::wcout << L"Failed to set current directory to " + Directory << std::endl;
	}

	void CommandDel(const std::wstring& File)
	{
		DWORD ret = FILE_ATTRIBUTE_DIRECTORY | GetFileAttributes(File.c_str());
		if ( ret != INVALID_FILE_ATTRIBUTES
			&& (ret & FILE_ATTRIBUTE_ARCHIVE) )
		{
			// file
			if ( !DeleteFile(File.c_str()) ) std::wcout << L"Failed to delete the file " + File << std::endl;
		} else
		{
			// directory or somewhat failed
			std::wcout << L"no file's there" << std::endl;
		}
	}

	void CommandRen(const std::wstring& FileFrom, const std::wstring& FileTo)
	{
		if ( !MoveFile(FileFrom.c_str(), FileTo.c_str()) ) std::wcout << L"Failed to rename the file " + FileFrom + L" to " + FileTo << std::endl;
	}

	void CommandXCopy(const std::wstring& PathFrom, const std::wstring& PathTo)
	{
		DWORD ret = FILE_ATTRIBUTE_DIRECTORY | GetFileAttributes(PathFrom.c_str());
		if ( ret != INVALID_FILE_ATTRIBUTES
			&& (ret & FILE_ATTRIBUTE_DIRECTORY)
			&& !(ret & FILE_ATTRIBUTE_ARCHIVE) )
		{
			//directory

			std::vector<std::wstring> arrfilename;

			std::tr1::function<void (const std::wstring& CurDir)> addfilelist;

			addfilelist =
				[&](const std::wstring& CurDir) mutable
				{
					CreateDirectory((PathTo + L"\\" + CurDir).c_str(), 0);
					WIN32_FIND_DATA fd;
					HANDLE hff = FindFirstFile((PathFrom + L"\\" + CurDir + L"\\*").c_str(), &fd); 
					if ( hff != INVALID_HANDLE_VALUE )
					{
						do
						{
							if (
								!(fd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
								&& (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
								)
							{
								if ( std::wstring(fd.cFileName) == L"."
									|| std::wstring(fd.cFileName) == L".." ) continue;

								// directory
								addfilelist(CurDir + L"\\" + fd.cFileName);
							} else
							{
								arrfilename.push_back(CurDir + L"\\" + fd.cFileName);
							}
						} while ( FindNextFile(hff, &fd) );
						FindClose(hff);
					}
				};
			addfilelist(L".");

			SYSTEM_INFO si;
			GetSystemInfo(&si);
			
			DWORD threadno = si.dwNumberOfProcessors*2;

			std::list<std::tr1::shared_ptr<std::thread>> listthread;

			for ( DWORD i=0; i < threadno; i++ )
			{
				listthread.push_back(
					std::tr1::shared_ptr<std::thread>(
						new std::thread(
							[i, threadno, &arrfilename, &PathFrom, &PathTo]()
							{
								DWORD index = i;
								while ( index < arrfilename.size() )
								{
									if ( arrfilename[index] != L"."
										&& arrfilename[index] != L".." )
									{
										CopyFile((PathFrom + L"\\" + arrfilename[index]).c_str(), (PathTo + L"\\" + arrfilename[index]).c_str(), true);
									}
									index += threadno;
								}
							}
						)
					)
				);
			}

			for ( auto i=listthread.begin(); i != listthread.end(); i++ )
				i->get()->join();
		} else
		{
			std::wcout << L"no directory's there" << std::endl;
		}

	}

	void CommandRun(const std::vector<std::wstring>& Tokens)
	{
		HANDLE hpipereadin = 0, hpipewritein = 0,
			hpipereadout = 0, hpipewriteout = 0;

		bool issucceeded = true;

		SECURITY_ATTRIBUTES sa = { };
		sa.nLength = sizeof(SECURITY_ATTRIBUTES); 
		sa.bInheritHandle = TRUE; 
		sa.lpSecurityDescriptor = NULL; 
		if ( !CreatePipe(&hpipereadin, &hpipewritein, &sa, 0xFFFFFFFF/2) ) issucceeded = false;
		if ( !CreatePipe(&hpipereadout, &hpipewriteout, &sa, 0xFFFFFFFF/2) ) issucceeded = false;

		//SetHandleInformation(hpipereadout, HANDLE_FLAG_INHERIT, 0);
		//SetHandleInformation(hpipewritein, HANDLE_FLAG_INHERIT, 0);


		if ( !issucceeded )
		{
			CloseHandle(hpipereadin);
			CloseHandle(hpipewritein);
			CloseHandle(hpipereadout);
			CloseHandle(hpipewriteout);
			std::wcout << L"pipe creation failed" << std::endl;
			return;
		}

		std::list<std::wstring> listcmdlines;
		listcmdlines.push_back(std::wstring());

		for ( DWORD idxtok = 0; idxtok < Tokens.size(); idxtok++ )
		{
			if ( Tokens[idxtok] == L"|" )
			{
				listcmdlines.push_back(std::wstring());
				continue;
			}

			if ( !listcmdlines.back().size() ) listcmdlines.back().append(Tokens[idxtok]);
			else listcmdlines.back().append(L" " + Tokens[idxtok]);

		}

		for ( auto iprocess=listcmdlines.begin(); iprocess != listcmdlines.end(); iprocess++ )
		{
			PROCESS_INFORMATION pi = { };
			STARTUPINFO si = { };

			si.cb = sizeof(STARTUPINFO); 
			si.hStdError = hpipewriteout;
			si.hStdOutput = hpipewriteout;
			si.hStdInput = hpipereadin;
			si.dwFlags |= STARTF_USESTDHANDLES;

			if ( iprocess == listcmdlines.begin() )
			{
				si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
			}

			if ( iprocess == --listcmdlines.end() )
			{
				si.hStdError	= GetStdHandle(STD_OUTPUT_HANDLE);
				si.hStdOutput	= GetStdHandle(STD_OUTPUT_HANDLE);
			}


			// Create the child process. 

			issucceeded = 0 != CreateProcess(
				NULL,
				const_cast<wchar_t*>(iprocess->c_str()),
				NULL,
				NULL,
				TRUE,
				0,
				NULL,
				NULL,
				&si,
				&pi
				);

			if ( !issucceeded ) break;
			WaitForSingleObject(pi.hProcess, INFINITE);

			DWORD written;
			WriteFile(hpipewriteout, "\r\n\x1A", 3, &written, 0);

			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);

			std::swap(hpipewritein, hpipewriteout);
			std::swap(hpipereadin, hpipereadout);
		}

		CloseHandle(hpipereadin);
		CloseHandle(hpipewritein);
		CloseHandle(hpipereadout);
		CloseHandle(hpipewriteout);
		
		if ( !issucceeded )
		{
			std::wcout << L"failed to run pipeline." << std::endl;
		}
	}

public:
	void Start()
	{
		for ( ;; )
		{
			std::vector<wchar_t> buf(MAX_PATH+1);
			GetCurrentDirectory(buf.size(), buf.data());
			std::wstring dirstr(buf.data());
			
			std::wcout << dirstr + L">";

			std::wstring full;
			std::getline(std::wcin, full);

			auto& tokens = SplitToken(full);

			std::wstring const & command = tokens[0];

			if ( command == L"dir" )
				CommandDir();
			else if ( command == L"md" )
				tokens.size() > 1 ? CommandMd(tokens[1]):void();
			else if ( command == L"rd" )
				tokens.size() > 1 ? CommandRd(tokens[1]):void();
			else if ( command == L"cd" )
				tokens.size() > 1 ? CommandCd(tokens[1]):void();
			else if ( command == L"del" )
				tokens.size() > 1 ? CommandDel(tokens[1]):void();
			else if ( command == L"ren" )
				tokens.size() > 2 ? CommandRen(tokens[1], tokens[2]):void();
			else if ( command == L"xcopy" )
				tokens.size() > 2 ? CommandXCopy(tokens[1], tokens[2]):void();
			else if ( command == L"exit" ) exit(0);
			else if ( command == L"" ) continue;
			else
			{
				// program
				CommandRun(tokens);
			}


		}
	}
};

int _tmain(int argc, _TCHAR* argv[])
{
	std::locale::global(std::locale("kor"));

	Shell shell;
	shell.Start();
	return 0;
}

