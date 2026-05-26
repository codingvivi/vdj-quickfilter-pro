#include "MyPlugin8.h"
#include <cctype>
#include <cstdio>
#include <regex>

// Range offsets (13 entries — index 6 is the standalone "0"); index 0 = +3.0 peak, index 12 = -3.0.
static const float RANGE_OFFSETS[TagBank::RANGE_BTN_COUNT] = {
	 3.0f,  2.5f,  2.0f,  1.5f,  1.0f,  0.5f,  0.0f,
	-0.5f, -1.0f, -1.5f, -2.0f, -2.5f, -3.0f
};

// Stack offsets (13 entries — includes 0); index 0 = +3.0, index 6 = 0, index 12 = -3.0.
static const float STACK_OFFSETS[TagBank::STACK_BTN_COUNT] = {
	 3.0f,  2.5f,  2.0f,  1.5f,  1.0f,  0.5f,  0.0f,
	-0.5f, -1.0f, -1.5f, -2.0f, -2.5f, -3.0f
};

static const char* RANGE_SUFFIX[TagBank::RANGE_BTN_COUNT] = {
	">=3.0", ">=2.5", ">=2.0", ">=1.5", ">=1.0", ">=0.5", "0",
	"=<0.5", "=<1.0", "=<1.5", "=<2.0", "=<2.5", "=<3.0"
};

static const char* STACK_SUFFIX[TagBank::STACK_BTN_COUNT] = {
	"+3.0", "+2.5", "+2.0", "+1.5", "+1.0", "+0.5", "0",
	"-0.5", "-1.0", "-1.5", "-2.0", "-2.5", "-3.0"
};

// VDJ-var-name-safe suffixes for the 13 stack buttons (no '+'/'-'/'.').
static const char* STACK_VAR_SUFFIX[TagBank::STACK_BTN_COUNT] = {
	"p30", "p25", "p20", "p15", "p10", "p05", "0",
	"n05", "n10", "n15", "n20", "n25", "n30"
};

// Bank tag names, in display order.
static const char* BANK_TAG_NAMES[CMyPlugin8::BANK_COUNT] = {
	"Energy",
	"Happy",
	"Dance",
	"Pop"
};

// First char of the tag name is used as the label prefix (e.g. "E>=3.0", "H+1.5").
// Storage for the per-button labels — must outlive the SDK call, hence file-scope static.
static char g_rangeLongNames [CMyPlugin8::BANK_COUNT][TagBank::RANGE_BTN_COUNT][16];
static char g_rangeShortNames[CMyPlugin8::BANK_COUNT][TagBank::RANGE_BTN_COUNT][16];
static char g_stackLongNames [CMyPlugin8::BANK_COUNT][TagBank::STACK_BTN_COUNT][16];
static char g_stackShortNames[CMyPlugin8::BANK_COUNT][TagBank::STACK_BTN_COUNT][16];

static std::string formatTagValue(double n, int width, bool sourceHasDecimal)
{
	char buf[32];
	if (!sourceHasDecimal && n == (int)n)
		snprintf(buf, sizeof buf, "%0*d", width, (int)n);
	else
		snprintf(buf, sizeof buf, "%g", n);
	return std::string(buf);
}

static std::string commentTagClause(const std::string& name, const std::string& value)
{
	return "Comment has tag #" + value + name;
}

static std::string buildOrExpression(const std::vector<std::string>& clauses)
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

//=============================================================================
// TagBank
//=============================================================================

void TagBank::Init(const std::string& name)
{
	tagName = name;
	posPeakHs = 0;
	negPeakHs = 0;
	zeroRangeEngaged = false;
	for (int i = 0; i < RANGE_BTN_COUNT; ++i) rangeBtns[i] = 0;
	for (int i = 0; i < STACK_BTN_COUNT; ++i) { stackBtns[i] = 0; stackEngaged[i] = false; }
}

void TagBank::Clear()
{
	posPeakHs = 0;
	negPeakHs = 0;
	zeroRangeEngaged = false;
	for (int i = 0; i < STACK_BTN_COUNT; ++i) stackEngaged[i] = false;
}

bool TagBank::AnyRangeEngaged() const
{
	return posPeakHs > 0 || negPeakHs > 0 || zeroRangeEngaged;
}

bool TagBank::AnyStackEngaged() const
{
	for (int i = 0; i < STACK_BTN_COUNT; ++i)
		if (stackEngaged[i]) return true;
	return false;
}

void TagBank::HandleRangePress(int idx)
{
	if (AnyStackEngaged()) {
		for (int i = 0; i < STACK_BTN_COUNT; ++i) stackEngaged[i] = false;
	}

	if (idx == RANGE_ZERO_IDX) {
		// "0" range: if either side is wider than 0, shrink both to 0 (keeping master engaged).
		// If only 0 was engaged, pressing turns the bank off. Otherwise engage just 0.
		if (posPeakHs > 0 || negPeakHs > 0) {
			posPeakHs = 0;
			negPeakHs = 0;
			zeroRangeEngaged = true;
		} else {
			zeroRangeEngaged = !zeroRangeEngaged;
		}
		return;
	}

	float off = RANGE_OFFSETS[idx];
	int hs = (int)((off < 0 ? -off : off) * 2);

	if (off > 0) posPeakHs = (posPeakHs == hs) ? 0 : hs;
	else         negPeakHs = (negPeakHs == hs) ? 0 : hs;
}

void TagBank::HandleStackPress(int idx)
{
	if (AnyRangeEngaged()) {
		posPeakHs = 0;
		negPeakHs = 0;
		zeroRangeEngaged = false;
	}
	stackEngaged[idx] = !stackEngaged[idx];
}

std::set<int> TagBank::ComputeDeltas() const
{
	std::set<int> deltas;
	if (AnyRangeEngaged()) deltas.insert(0);
	for (int d = 1; d <= posPeakHs; ++d) deltas.insert(d);
	for (int d = 1; d <= negPeakHs; ++d) deltas.insert(-d);

	for (int i = 0; i < STACK_BTN_COUNT; ++i) {
		if (!stackEngaged[i]) continue;
		float off = STACK_OFFSETS[i];
		int hs = (int)((off < 0 ? -off : off) * 2);
		if (off > 0) deltas.insert(hs);
		else         deltas.insert(-hs);
	}
	return deltas;
}

//=============================================================================
// CMyPlugin8
//=============================================================================

HRESULT VDJ_API CMyPlugin8::OnLoad()
{
	m_Btn1State = 0;
	m_KillState = 0;
	DeclareParameterButton(&m_Btn1State, ID_BUTTON_1, "Refresh Quick Filters", "RFR");
	DeclareParameterButton(&m_KillState, ID_BUTTON_2, "Kill Quick Filter",     "Kill");

	for (int b = 0; b < BANK_COUNT; ++b) {
		m_banks[b].Init(BANK_TAG_NAMES[b]);
		char prefix = BANK_TAG_NAMES[b][0]; // 'E', 'H', 'D', 'P'
		int bankBase = FIRST_BANK_ID + b * TagBank::TOTAL_BTN_COUNT;

		for (int i = 0; i < TagBank::RANGE_BTN_COUNT; ++i) {
			snprintf(g_rangeLongNames [b][i], 16, "%c%s", prefix, RANGE_SUFFIX[i]);
			snprintf(g_rangeShortNames[b][i], 16, "%c%s", prefix, RANGE_SUFFIX[i]);
			DeclareParameterButton(&m_banks[b].rangeBtns[i], bankBase + i,
			                       g_rangeLongNames[b][i], g_rangeShortNames[b][i]);
		}
		for (int i = 0; i < TagBank::STACK_BTN_COUNT; ++i) {
			snprintf(g_stackLongNames [b][i], 16, "%c%s", prefix, STACK_SUFFIX[i]);
			snprintf(g_stackShortNames[b][i], 16, "%c%s", prefix, STACK_SUFFIX[i]);
			DeclareParameterButton(&m_banks[b].stackBtns[i],
			                       bankBase + TagBank::RANGE_BTN_COUNT + i,
			                       g_stackLongNames[b][i], g_stackShortNames[b][i]);
		}

		SyncBankVars(b);
	}

	return S_OK;
}

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

ULONG VDJ_API CMyPlugin8::Release()
{
	delete this;
	return 0;
}

HRESULT VDJ_API CMyPlugin8::OnGetUserInterface(TVdjPluginInterface8 *pluginInterface)
{
	pluginInterface->Type = VDJINTERFACE_DEFAULT;
	return S_OK;
}

HRESULT VDJ_API CMyPlugin8::OnParameter(int id)
{
	if (id == ID_BUTTON_1) {
		if (m_Btn1State == 1) {
			// TODO: disengage + re-engage active filter (call RebuildFilter() after re-reading master)
		}
		return S_OK;
	}

	if (id == ID_BUTTON_2) {
		if (m_KillState == 1) {
			SendCommand("quick_filter off");
			m_ActiveFilter.clear();
			for (int b = 0; b < BANK_COUNT; ++b) {
				m_banks[b].Clear();
				SyncBankVars(b);
			}
		}
		return S_OK;
	}

	// Bank-button dispatch: figure out which bank, which mode (range/stack), and the within-mode index.
	int rel = id - FIRST_BANK_ID;
	if (rel < 0 || rel >= BANK_COUNT * TagBank::TOTAL_BTN_COUNT) return S_OK;

	int bankIdx     = rel / TagBank::TOTAL_BTN_COUNT;
	int withinBank  = rel % TagBank::TOTAL_BTN_COUNT;
	bool isStack    = withinBank >= TagBank::RANGE_BTN_COUNT;
	int modeIdx     = isStack ? withinBank - TagBank::RANGE_BTN_COUNT : withinBank;
	TagBank& bank   = m_banks[bankIdx];

	int sdkState = isStack ? bank.stackBtns[modeIdx] : bank.rangeBtns[modeIdx];
	if (sdkState != 1) return S_OK; // ignore release edge

	if (isStack) bank.HandleStackPress(modeIdx);
	else         bank.HandleRangePress(modeIdx);

	SyncBankVars(bankIdx);
	RebuildFilter();
	return S_OK;
}

int CMyPlugin8::GetMasterDeck()
{
	double n = 0.0;
	GetInfo("get_activedeck", &n);
	return (int)n;
}

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

// Mirrors a bank's engaged state to global (non-persistent) VDJ vars so controller
// mappings can light LEDs without needing to query the plugin. Var names:
//   $qfpro_<x>_pos   signed positive-side peak, 0..3.0
//   $qfpro_<x>_neg   signed negative-side peak, -3.0..0
//   $qfpro_<x>_r_0   0/1, lit whenever any range side (including standalone 0) is engaged
//   $qfpro_<x>_s_<suffix>  one 0/1 per stack button (suffix = p30..p05, n05..n30)
// where <x> is the lowercased first char of the tag name (e=Energy, h=Happy, ...).
void CMyPlugin8::SyncBankVars(int bankIdx)
{
	TagBank& bank = m_banks[bankIdx];
	if (bank.tagName.empty()) return;

	char lower = (char)std::tolower((unsigned char)bank.tagName[0]);
	char cmd[64];

	float pos =  bank.posPeakHs * 0.5f;
	float neg = -bank.negPeakHs * 0.5f;

	snprintf(cmd, sizeof cmd, "set '$qfpro_%c_pos' %g", lower, pos);
	SendCommand(cmd);
	snprintf(cmd, sizeof cmd, "set '$qfpro_%c_neg' %g", lower, neg);
	SendCommand(cmd);
	snprintf(cmd, sizeof cmd, "set '$qfpro_%c_r_0' %d", lower, bank.AnyRangeEngaged() ? 1 : 0);
	SendCommand(cmd);

	for (int i = 0; i < TagBank::STACK_BTN_COUNT; ++i) {
		snprintf(cmd, sizeof cmd, "set '$qfpro_%c_s_%s' %d",
		         lower, STACK_VAR_SUFFIX[i], bank.stackEngaged[i] ? 1 : 0);
		SendCommand(cmd);
	}
}

void CMyPlugin8::SendFilterExpression(const std::string& expr)
{
	std::string cmd = "quick_filter '" + expr + "'";

	if (!m_ActiveFilter.empty()) SendCommand(m_ActiveFilter.c_str());

	if (cmd == m_ActiveFilter) {
		m_ActiveFilter.clear();
		return;
	}

	SendCommand(cmd.c_str());
	m_ActiveFilter = cmd;
}

void CMyPlugin8::RebuildFilter()
{
	std::vector<std::string> bankExprs;

	for (int b = 0; b < BANK_COUNT; ++b) {
		TagBank& bank = m_banks[b];
		if (!bank.AnyEngaged()) continue;

		std::set<int> deltas = bank.ComputeDeltas();
		if (deltas.empty()) continue;

		std::string master = GetNumericTag(bank.tagName);
		if (master.empty()) continue;

		double base = std::stod(master);
		int width = (int)master.size();
		bool hasDecimal = master.find('.') != std::string::npos;

		std::vector<std::string> clauses;
		for (int d : deltas) {
			double n = base + d * 0.5;
			std::string val = formatTagValue(n, width, hasDecimal);
			clauses.push_back(commentTagClause(bank.tagName, val));
		}
		bankExprs.push_back(buildOrExpression(clauses));
	}

	if (bankExprs.empty()) {
		if (!m_ActiveFilter.empty()) {
			SendCommand(m_ActiveFilter.c_str());
			m_ActiveFilter.clear();
		}
		return;
	}

	std::string expr;
	if (bankExprs.size() == 1) {
		expr = bankExprs[0];
	} else {
		for (size_t i = 0; i < bankExprs.size(); ++i) {
			if (i > 0) expr += " and ";
			expr += "(" + bankExprs[i] + ")";
		}
	}

	SendFilterExpression(expr);
}
