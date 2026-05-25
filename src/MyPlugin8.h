#ifndef MYPLUGIN8_H
#define MYPLUGIN8_H

#include "vdjPlugin8.h"
#include <string>
#include <vector>

class CMyPlugin8 : public IVdjPlugin8
{
public:
	static const int ENERGY_BTN_COUNT = 13; // +3 to -3 in 0.5 steps

	HRESULT VDJ_API OnLoad();
	HRESULT VDJ_API OnGetPluginInfo(TVdjPluginInfo8 *infos);
	ULONG VDJ_API Release();
    HRESULT VDJ_API OnGetUserInterface(TVdjPluginInterface8 *pluginInterface);
    HRESULT VDJ_API OnParameter(int id);

private:
	int m_Btn1State;
	int m_KillState;
	int m_RangeBtns[ENERGY_BTN_COUNT]; // momentary SDK state for range buttons
	int m_StackBtns[ENERGY_BTN_COUNT]; // momentary SDK state for stack buttons

	// Range mode: each side has a single "peak" half-step magnitude. Pressing raises (overwrite) or clears (if same).
	int  m_PosPeakHs;     // 0..6 (0 = positive side off)
	int  m_NegPeakHs;     // 0..6 (0 = negative side off)
	bool m_ZeroEngaged;   // master value, toggled by the range 0 button only

	// Stack mode: each button independently latches; contributes only its single half-step value.
	bool m_StackEngaged[ENERGY_BTN_COUNT];

	void ClearRangeState();
	void ClearStackState();
	bool AnyRangeEngaged() const;
	bool AnyStackEngaged() const;
	std::string m_ActiveFilter; // last quick_filter expression we sent that's still engaged in VDJ

	int GetMasterDeck(); // returns 1-4 for the current master deck, or 0 if none
	std::string GetMasterDeckComment(); // comment of the track on the master deck
	std::string GetNumericTag(const std::string& name); // extracts the NN from a #NN<name> tag in the master comment; empty if none
	void SendFilterExpression(const std::string& expr); // wraps expr in `quick_filter '...'`, handles toggle-off of previous via m_ActiveFilter
	std::string BuildOrExpression(const std::vector<std::string>& clauses); // joins clauses as `(c1) or (c2) or ...`
	std::string CommentTagClause(const std::string& name, const std::string& value); // builds `Comment has tag #<value><name>`
	void SendCommentTagFilter(const std::string& name, const std::string& value); // convenience: single-clause comment-tag filter
	void RebuildEnergyFilter(); // unions every engaged energy button's value-span and sends the resulting OR expression

	bool isMasterFX(); // an example of additional function for the use of GetInfo()

protected:
	typedef enum _ID_Interface
	{
		ID_BUTTON_1,  // refresh
		ID_BUTTON_2,  // kill
		ID_BUTTON_3,  // Range +3.0
		ID_BUTTON_4,  // Range +2.5
		ID_BUTTON_5,  // Range +2.0
		ID_BUTTON_6,  // Range +1.5
		ID_BUTTON_7,  // Range +1.0
		ID_BUTTON_8,  // Range +0.5
		ID_BUTTON_9,  // Range  0.0
		ID_BUTTON_10, // Range -0.5
		ID_BUTTON_11, // Range -1.0
		ID_BUTTON_12, // Range -1.5
		ID_BUTTON_13, // Range -2.0
		ID_BUTTON_14, // Range -2.5
		ID_BUTTON_15, // Range -3.0
		ID_BUTTON_16, // Stack +3.0
		ID_BUTTON_17, // Stack +2.5
		ID_BUTTON_18, // Stack +2.0
		ID_BUTTON_19, // Stack +1.5
		ID_BUTTON_20, // Stack +1.0
		ID_BUTTON_21, // Stack +0.5
		ID_BUTTON_22, // Stack  0.0
		ID_BUTTON_23, // Stack -0.5
		ID_BUTTON_24, // Stack -1.0
		ID_BUTTON_25, // Stack -1.5
		ID_BUTTON_26, // Stack -2.0
		ID_BUTTON_27, // Stack -2.5
		ID_BUTTON_28  // Stack -3.0
	} ID_Interface;
};

#endif
