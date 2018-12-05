#pragma once
#include <string>
#include <Windows.h>
#include <tchar.h>

namespace trigger
{
	namespace tool
	{
		//TODO :: DirectX ���� ������ �ؽ�ó�� Load �� �� �ֵ��� ����
		static inline void make_tex(std::string filename)
		{
			if (filename.find(".png") != std::string::npos)
			{
				STARTUPINFO info = { 0 };
				info.cb = sizeof(STARTUPINFO);
				PROCESS_INFORMATION pinfo;
				std::string com = "../tools/texconv " + filename + " -pmalpha -m 1 -f BC3_UNORM -y";
				std::wstring c(com.begin(), com.end());
				CreateProcess(NULL, (LPTSTR)c.c_str(), NULL, NULL, false, 0, NULL, NULL, &info, &pinfo);
			}
		}
	}
}