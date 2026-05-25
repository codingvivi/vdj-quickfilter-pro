#include "MyPlugin8.h"
#include <cstdio>
#include <regex>
#include <set>

static const float ENERGY_OFFSETS[CMyPlugin8::ENERGY_BTN_COUNT] = {
	 3.0f,  2.5f,  2.0f,  1.5f,  1.0f,  0.5f,  0.0f,
	-0.5f, -1.0f, -1.5f, -2.0f, -2.5f, -3.0f
};

static const char* RANGE_LONG_NAMES[CMyPlugin8::ENERGY_BTN_COUNT] = {
	"Energy >=3.0", "Energy >=2.5", "Energy >=2.0", "Energy >=1.5",
	"Energy >=1.0", "Energy >=0.5", "Energy =0",
	"Energy =<0.5", "Energy =<1.0", "Energy =<1.5",
	"Energy =<2.0", "Energy =<2.5", "Energy =<3.0"
};

static const char* RANGE_SHORT_NAMES[CMyPlugin8::ENERGY_BTN_COUNT] = {
	">=3.0", ">=2.5", ">=2.0", ">=1.5", ">=1.0", ">=0.5", "=0",
	"=<0.5", "=<1.0", "=<1.5", "=<2.0", "=<2.5", "=<3.0"
};

static const char* STACK_LONG_NAMES[CMyPlugin8::ENERGY_BTN_COUNT] = {
	"Energy +3.0", "Energy +2.5", "Energy +2.0", "Energy +1.5",
	"Energy +1.0", "Energy +0.5", "Energy 0",
	"Energy -0.5", "Energy -1.0", "Energy -1.5",
	"Energy -2.0", "Energy -2.5", "Energy -3.0"
};

static const char* STACK_SHORT_NAMES[CMyPlugin8::ENERGY_BTN_COUNT] = {
	"+3.0", "+2.5", "+2.0", "+1.5", "+1.0", "+0.5", "0",
	"-0.5", "-1.0", "-1.5", "-2.0", "-2.5", "-3.0"
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
		m_RangeBtns[i] = 0;
		DeclareParameterButton(&m_RangeBtns[i], ID_BUTTON_3 + i,
		                       RANGE_LONG_NAMES[i], RANGE_SHORT_NAMES[i]);
	}

	for (int i = 0; i < ENERGY_BTN_COUNT; ++i) {
		m_StackBtns[i] = 0;
		m_StackEngaged[i] = false;
		DeclareParameterButton(&m_StackBtns[i], ID_BUTTON_16 + i,
		                       STACK_LONG_NAMES[i], STACK_SHORT_NAMES[i]);
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
			ClearRangeState();
			ClearStackState();
		}
		return S_OK;
	}

	if (id >= ID_BUTTON_3 && id <= ID_BUTTON_15) {
		int i = id - ID_BUTTON_3;
		if (m_RangeBtns[i] != 1) return S_OK;

		// pressing a range button while stack is active disables the whole stack
		if (AnyStackEngaged()) ClearStackState();

		float off = ENERGY_OFFSETS[i];
		int hs = (int)((off < 0 ? -off : off) * 2);

		if (off > 0)      m_PosPeakHs   = (m_PosPeakHs == hs) ? 0 : hs;
		else if (off < 0) m_NegPeakHs   = (m_NegPeakHs == hs) ? 0 : hs;
		else              m_ZeroEngaged = !m_ZeroEngaged;

		RebuildEnergyFilter();
		return S_OK;
	}

	if (id >= ID_BUTTON_16 && id <= ID_BUTTON_28) {
		int i = id - ID_BUTTON_16;
		if (m_StackBtns[i] != 1) return S_OK;

		// pressing a stack button while a range is active disables the whole range
		if (AnyRangeEngaged()) ClearRangeState();

		m_StackEngaged[i] = !m_StackEngaged[i];

		RebuildEnergyFilter();
		return S_OK;
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

void CMyPlugin8::ClearRangeState()
{
	m_PosPeakHs = 0;
	m_NegPeakHs = 0;
	m_ZeroEngaged = false;
}

void CMyPlugin8::ClearStackState()
{
	for (int i = 0; i < ENERGY_BTN_COUNT; ++i) m_StackEngaged[i] = false;
}

bool CMyPlugin8::AnyRangeEngaged() const
{
	return m_PosPeakHs > 0 || m_NegPeakHs > 0 || m_ZeroEngaged;
}

bool CMyPlugin8::AnyStackEngaged() const
{
	for (int i = 0; i < ENERGY_BTN_COUNT; ++i)
		if (m_StackEngaged[i]) return true;
	return false;
}

void CMyPlugin8::RebuildEnergyFilter()
{
	std::set<int> deltas;

	// range contribution (mutually exclusive with stack, but unioning is harmless)
	for (int d = 1; d <= m_PosPeakHs; ++d) deltas.insert(d);
	for (int d = 1; d <= m_NegPeakHs; ++d) deltas.insert(-d);
	if (m_ZeroEngaged) deltas.insert(0);

	// stack contribution: each engaged button adds its single half-step value
	for (int i = 0; i < ENERGY_BTN_COUNT; ++i) {
		if (!m_StackEngaged[i]) continue;
		float off = ENERGY_OFFSETS[i];
		int hs = (int)((off < 0 ? -off : off) * 2);
		if      (off > 0) deltas.insert(hs);
		else if (off < 0) deltas.insert(-hs);
		else              deltas.insert(0);
	}

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
