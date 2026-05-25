#include "MyPlugin8.h"
#include <cstdio>
#include <regex>


//-----------------------------------------------------------------------------
HRESULT VDJ_API CMyPlugin8::OnLoad()
{
	m_Btn1State = 0;
	m_Btn2State = 0;

	DeclareParameterButton(&m_Btn1State, ID_BUTTON_1, "Refresh Quick Filters", "RFR");
	DeclareParameterButton(&m_Btn2State, ID_BUTTON_2, "Toggle Key Difference Filter", "KeyDiff");

	return S_OK;
}
//-----------------------------------------------------------------------------
HRESULT VDJ_API CMyPlugin8::OnGetPluginInfo(TVdjPluginInfo8 *infos)
{
	infos->PluginName = "QFPro";
	infos->Author = "musicvivireal";
	infos->Description = "My first VirtualDJ 8 plugin";
	infos->Version = "1.0";
	infos->Flags = 0x00;
	infos->Bitmap = nullptr;

	return S_OK;
}
//---------------------------------------------------------------------------
ULONG VDJ_API CMyPlugin8::Release()
{
	// ADD YOUR CODE HERE WHEN THE PLUGIN IS RELEASED

	delete this;
	return 0;
}
//---------------------------------------------------------------------------
HRESULT VDJ_API CMyPlugin8::OnGetUserInterface(TVdjPluginInterface8 *pluginInterface)
{
	pluginInterface->Type = VDJINTERFACE_DEFAULT;

	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT VDJ_API CMyPlugin8::OnParameter(int id)
{
	switch (id) {
		case ID_BUTTON_1:
			if (m_Btn1State == 1) {
				// TODO: send the disengage + re-engage command sequence here
				// e.g. SendCommand("quick_filter 1 & quick_filter 1");
			}
			break;

		case ID_BUTTON_2:
			if (m_Btn2State == 1 && !GetNumericTag("Pop").empty())
				SendCommand("quick_filter 'Comment has tag #00Pop'");
			break;
	}

	return S_OK;
}

//---------------------------------------------------------------------------
int CMyPlugin8::GetMasterDeck()
{
	double n = 0.0;
	GetInfo("get_activedeck", &n);
	return (int)n;
}

//---------------------------------------------------------------------------
std::string CMyPlugin8::GetMasterDeckComment()
{
	int deck = GetMasterDeck();
	if (deck < 1) return std::string();

	char query[32];
	snprintf(query, sizeof query, "deck %d get_comment", deck);

	char buf[1024] = {0};
	GetStringInfo(query, buf, sizeof buf);
	return std::string(buf);
}

//---------------------------------------------------------------------------
std::string CMyPlugin8::GetNumericTag(const std::string& name)
{
	std::string comment = GetMasterDeckComment();
	if (comment.empty()) return std::string();

	std::regex pattern("#(\\d+(?:\\.\\d+)?)" + name);
	std::smatch match;
	if (std::regex_search(comment, match, pattern))
		return match[1].str();
	return std::string();
}

//-------------------------------------------------------------------------------------------------------------------------------------
// BELOW, ADDITIONAL FUNCTIONS ONLY TO EXPLAIN SOME FEATURES (CAN BE REMOVED)
//-------------------------------------------------------------------------------------------------------------------------------------
bool CMyPlugin8::isMasterFX()
{
	double qRes;
	HRESULT hr = S_FALSE;

	hr = GetInfo("get_deck 'master' ? true : false", &qRes);

	if(qRes==1.0f) return true;
	else return false;
}
