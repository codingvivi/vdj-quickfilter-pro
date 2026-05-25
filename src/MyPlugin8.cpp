#include "MyPlugin8.h"
#include <algorithm>
#include <cstdio>
#include <regex>
#include <set>

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
	m_KillState = 0;
	DeclareParameterButton(&m_Btn1State, ID_BUTTON_1, "Refresh Quick Filters", "RFR");
	DeclareParameterButton(&m_KillState, ID_BUTTON_2, "Kill Quick Filter", "Kill");

	m_PosPeakHs = 0;
	m_NegPeakHs = 0;
	m_ZeroEngaged = false;

	for (int i = 0; i < ENERGY_BTN_COUNT; ++i) {
		m_EnergyBtns[i] = 0;
		DeclareParameterButton(&m_EnergyBtns[i], ID_BUTTON_3 + i,
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

	if (id == ID_BUTTON_2) {
		if (m_KillState == 1) {
			SendCommand("quick_filter off");
			m_ActiveFilter.clear();
			m_PosPeakHs = 0;
			m_NegPeakHs = 0;
			m_ZeroEngaged = false;
		}
		return S_OK;
	}

	if (id >= ID_BUTTON_3 && id <= ID_BUTTON_15) {
		int i = id - ID_BUTTON_3;
		if (m_EnergyBtns[i] != 1) return S_OK;

		float off = ENERGY_OFFSETS[i];
		int hs = (int)((off < 0 ? -off : off) * 2);

		if (off > 0)      m_PosPeakHs   = (m_PosPeakHs == hs) ? 0 : hs;
		else if (off < 0) m_NegPeakHs   = (m_NegPeakHs == hs) ? 0 : hs;
		else              m_ZeroEngaged = !m_ZeroEngaged;

		RebuildEnergyFilter();
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
void CMyPlugin8::SendFilterExpression(const std::string& expr)
{
	std::string cmd = "quick_filter '" + expr + "'";

	// re-sending the same expression toggles VDJ's filter off — use that to clear the previous one
	if (!m_ActiveFilter.empty()) SendCommand(m_ActiveFilter.c_str());

	if (cmd == m_ActiveFilter) {
		// caller asked for the exact same filter — we just disengaged it, leave it off
		m_ActiveFilter.clear();
		return;
	}

	SendCommand(cmd.c_str());
	m_ActiveFilter = cmd;
}

//---------------------------------------------------------------------------
std::string CMyPlugin8::BuildOrExpression(const std::vector<std::string>& clauses)
{
	if (clauses.empty()) return std::string();
	if (clauses.size() == 1) return clauses[0];

	std::string out;
	for (size_t i = 0; i < clauses.size(); ++i) {
		if (i > 0) out += " or ";
		out += "(" + clauses[i] + ")";
	}
	return out;
}

//---------------------------------------------------------------------------
std::string CMyPlugin8::CommentTagClause(const std::string& name, const std::string& value)
{
	return "Comment has tag #" + value + name;
}

//---------------------------------------------------------------------------
void CMyPlugin8::SendCommentTagFilter(const std::string& name, const std::string& value)
{
	SendFilterExpression(CommentTagClause(name, value));
}

//---------------------------------------------------------------------------
static std::string formatTagValue(double n, int width, bool sourceHasDecimal)
{
	char buf[32];
	if (!sourceHasDecimal && n == (int)n)
		snprintf(buf, sizeof buf, "%0*d", width, (int)n);
	else
		snprintf(buf, sizeof buf, "%g", n);
	return std::string(buf);
}

void CMyPlugin8::RebuildEnergyFilter()
{
	std::set<int> deltas;
	for (int d = 1; d <= m_PosPeakHs; ++d) deltas.insert(d);
	for (int d = 1; d <= m_NegPeakHs; ++d) deltas.insert(-d);
	// 0 is implicit whenever either side has a range; only the explicit zero button can hold it alone
	if (m_ZeroEngaged || m_PosPeakHs > 0 || m_NegPeakHs > 0) deltas.insert(0);

	// nothing engaged → disengage VDJ-side too
	if (deltas.empty()) {
		if (!m_ActiveFilter.empty()) {
			SendCommand(m_ActiveFilter.c_str());
			m_ActiveFilter.clear();
		}
		return;
	}

	std::string master = GetNumericTag("Energy");
	if (master.empty()) return;

	double base = std::stod(master);
	int width = (int)master.size();
	bool hasDecimal = master.find('.') != std::string::npos;

	std::vector<std::string> clauses;
	for (int d : deltas) {
		double n = base + d * 0.5;
		std::string val = formatTagValue(n, width, hasDecimal);
		clauses.push_back(CommentTagClause("Energy", val));
	}

	SendFilterExpression(BuildOrExpression(clauses));
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
