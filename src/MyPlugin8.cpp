#include "MyPlugin8.h"
#include <cstdio>
#include <regex>

static const float ENERGY_OFFSETS[CMyPlugin8::ENERGY_BTN_COUNT] = {
	 3.0f,  2.5f,  2.0f,  1.5f,  1.0f,  0.5f,  0.0f,
	-0.5f, -1.0f, -1.5f, -2.0f, -2.5f, -3.0f
};

static const char* ENERGY_LONG_NAMES[CMyPlugin8::ENERGY_BTN_COUNT] = {
	"Energy +3.0", "Energy +2.5", "Energy +2.0", "Energy +1.5",
	"Energy +1.0", "Energy +0.5", "Energy 0.0",
	"Energy -0.5", "Energy -1.0", "Energy -1.5",
	"Energy -2.0", "Energy -2.5", "Energy -3.0"
};

static const char* ENERGY_SHORT_NAMES[CMyPlugin8::ENERGY_BTN_COUNT] = {
	"E+3.0", "E+2.5", "E+2.0", "E+1.5", "E+1.0", "E+0.5", "E0",
	"E-0.5", "E-1.0", "E-1.5", "E-2.0", "E-2.5", "E-3.0"
};

//-----------------------------------------------------------------------------
HRESULT VDJ_API CMyPlugin8::OnLoad()
{
	m_Btn1State = 0;
	DeclareParameterButton(&m_Btn1State, ID_BUTTON_1, "Refresh Quick Filters", "RFR");

	for (int i = 0; i < ENERGY_BTN_COUNT; ++i) {
		m_EnergyBtns[i] = 0;
		DeclareParameterButton(&m_EnergyBtns[i], ID_BUTTON_2 + i,
		                       ENERGY_LONG_NAMES[i], ENERGY_SHORT_NAMES[i]);
	}

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
	if (id == ID_BUTTON_1) {
		if (m_Btn1State == 1) {
			// TODO: send the disengage + re-engage command sequence here
			// e.g. SendCommand("quick_filter 1 & quick_filter 1");
		}
		return S_OK;
	}

	if (id >= ID_BUTTON_2 && id <= ID_BUTTON_14) {
		int i = id - ID_BUTTON_2;
		if (m_EnergyBtns[i] == 1) MatchNumericTag("Energy", ENERGY_OFFSETS[i]);
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
void CMyPlugin8::SendCommentTagFilter(const std::string& name, const std::string& value)
{
	std::string cmd = "quick_filter 'Comment has tag #" + value + name + "'";
	SendCommand(cmd.c_str());
}

//---------------------------------------------------------------------------
void CMyPlugin8::MatchNumericTag(const std::string& name, float offset)
{
	std::string value = GetNumericTag(name);
	if (value.empty()) return;

	double n = std::stod(value) + offset;

	char buf[32];
	if (value.find('.') == std::string::npos && n == (int)n)
		snprintf(buf, sizeof buf, "%0*d", (int)value.size(), (int)n);
	else
		snprintf(buf, sizeof buf, "%g", n);

	SendCommentTagFilter(name, buf);
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
