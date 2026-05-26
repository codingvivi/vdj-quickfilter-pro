#ifndef MYPLUGIN8_H
#define MYPLUGIN8_H

#include "vdjPlugin8.h"
#include <set>
#include <string>

// One bank of 25 buttons (12 range + 13 stack) tied to a single numeric comment tag (e.g. "Energy").
// Range has no 0 button — stack's 0 button is the sole way to engage just the master value.
struct TagBank
{
	static const int RANGE_BTN_COUNT = 12;
	static const int STACK_BTN_COUNT = 13;
	static const int TOTAL_BTN_COUNT = RANGE_BTN_COUNT + STACK_BTN_COUNT;

	std::string tagName;
	int  rangeBtns[RANGE_BTN_COUNT];   // SDK-owned momentary state
	int  stackBtns[STACK_BTN_COUNT];

	// Range mode: per-side peak overwrite. No zero button on this side.
	int  posPeakHs;     // 0..6 half-steps (0 = side off)
	int  negPeakHs;

	// Stack mode: per-button latch; each contributes its single half-step value.
	bool stackEngaged[STACK_BTN_COUNT];

	void Init(const std::string& name);
	void Clear();
	void HandleRangePress(int idx);  // idx 0..11
	void HandleStackPress(int idx);  // idx 0..12
	bool AnyRangeEngaged() const;
	bool AnyStackEngaged() const;
	bool AnyEngaged() const { return AnyRangeEngaged() || AnyStackEngaged(); }
	std::set<int> ComputeDeltas() const; // half-step offsets relative to master
};

class CMyPlugin8 : public IVdjPlugin8
{
public:
	static const int BANK_COUNT     = 4;
	static const int CONTROL_BTNS   = 2;   // refresh + kill
	static const int FIRST_BANK_ID  = CONTROL_BTNS; // ID_BUTTON_3 (0-indexed enum value 2)

	HRESULT VDJ_API OnLoad();
	HRESULT VDJ_API OnGetPluginInfo(TVdjPluginInfo8 *infos);
	ULONG VDJ_API Release();
	HRESULT VDJ_API OnGetUserInterface(TVdjPluginInterface8 *pluginInterface);
	HRESULT VDJ_API OnParameter(int id);

private:
	int m_Btn1State;
	int m_KillState;
	TagBank m_banks[BANK_COUNT];
	std::string m_ActiveFilter; // last quick_filter expression we sent, for the toggle-off-then-replace dance

	int GetMasterDeck();
	std::string GetMasterDeckComment();
	std::string GetNumericTag(const std::string& name);
	void SendFilterExpression(const std::string& expr);
	void RebuildFilter(); // gathers each bank's clauses, AND-joins across banks, sends
	void SyncBankVars(int bankIdx); // mirrors bank state to global VDJ vars for LED feedback

protected:
	typedef enum _ID_Interface
	{
		ID_BUTTON_1,   // refresh
		ID_BUTTON_2,   // kill

		// -- Bank 0: Energy (buttons 3-27) --
		// range
		ID_BUTTON_3,   // E>=3.0
		ID_BUTTON_4,   // E>=2.5
		ID_BUTTON_5,   // E>=2.0
		ID_BUTTON_6,   // E>=1.5
		ID_BUTTON_7,   // E>=1.0
		ID_BUTTON_8,   // E>=0.5
		ID_BUTTON_9,   // E=<0.5
		ID_BUTTON_10,  // E=<1.0
		ID_BUTTON_11,  // E=<1.5
		ID_BUTTON_12,  // E=<2.0
		ID_BUTTON_13,  // E=<2.5
		ID_BUTTON_14,  // E=<3.0
		// stack
		ID_BUTTON_15,  // E+3.0
		ID_BUTTON_16,  // E+2.5
		ID_BUTTON_17,  // E+2.0
		ID_BUTTON_18,  // E+1.5
		ID_BUTTON_19,  // E+1.0
		ID_BUTTON_20,  // E+0.5
		ID_BUTTON_21,  // E0
		ID_BUTTON_22,  // E-0.5
		ID_BUTTON_23,  // E-1.0
		ID_BUTTON_24,  // E-1.5
		ID_BUTTON_25,  // E-2.0
		ID_BUTTON_26,  // E-2.5
		ID_BUTTON_27,  // E-3.0

		// -- Bank 1: Happy (buttons 28-52) --
		// range
		ID_BUTTON_28,  // H>=3.0
		ID_BUTTON_29,  // H>=2.5
		ID_BUTTON_30,  // H>=2.0
		ID_BUTTON_31,  // H>=1.5
		ID_BUTTON_32,  // H>=1.0
		ID_BUTTON_33,  // H>=0.5
		ID_BUTTON_34,  // H=<0.5
		ID_BUTTON_35,  // H=<1.0
		ID_BUTTON_36,  // H=<1.5
		ID_BUTTON_37,  // H=<2.0
		ID_BUTTON_38,  // H=<2.5
		ID_BUTTON_39,  // H=<3.0
		// stack
		ID_BUTTON_40,  // H+3.0
		ID_BUTTON_41,  // H+2.5
		ID_BUTTON_42,  // H+2.0
		ID_BUTTON_43,  // H+1.5
		ID_BUTTON_44,  // H+1.0
		ID_BUTTON_45,  // H+0.5
		ID_BUTTON_46,  // H0
		ID_BUTTON_47,  // H-0.5
		ID_BUTTON_48,  // H-1.0
		ID_BUTTON_49,  // H-1.5
		ID_BUTTON_50,  // H-2.0
		ID_BUTTON_51,  // H-2.5
		ID_BUTTON_52,  // H-3.0

		// -- Bank 2: Dance (buttons 53-77) --
		// range
		ID_BUTTON_53,  // D>=3.0
		ID_BUTTON_54,  // D>=2.5
		ID_BUTTON_55,  // D>=2.0
		ID_BUTTON_56,  // D>=1.5
		ID_BUTTON_57,  // D>=1.0
		ID_BUTTON_58,  // D>=0.5
		ID_BUTTON_59,  // D=<0.5
		ID_BUTTON_60,  // D=<1.0
		ID_BUTTON_61,  // D=<1.5
		ID_BUTTON_62,  // D=<2.0
		ID_BUTTON_63,  // D=<2.5
		ID_BUTTON_64,  // D=<3.0
		// stack
		ID_BUTTON_65,  // D+3.0
		ID_BUTTON_66,  // D+2.5
		ID_BUTTON_67,  // D+2.0
		ID_BUTTON_68,  // D+1.5
		ID_BUTTON_69,  // D+1.0
		ID_BUTTON_70,  // D+0.5
		ID_BUTTON_71,  // D0
		ID_BUTTON_72,  // D-0.5
		ID_BUTTON_73,  // D-1.0
		ID_BUTTON_74,  // D-1.5
		ID_BUTTON_75,  // D-2.0
		ID_BUTTON_76,  // D-2.5
		ID_BUTTON_77,  // D-3.0

		// -- Bank 3: Pop (buttons 78-102) --
		// range
		ID_BUTTON_78,  // P>=3.0
		ID_BUTTON_79,  // P>=2.5
		ID_BUTTON_80,  // P>=2.0
		ID_BUTTON_81,  // P>=1.5
		ID_BUTTON_82,  // P>=1.0
		ID_BUTTON_83,  // P>=0.5
		ID_BUTTON_84,  // P=<0.5
		ID_BUTTON_85,  // P=<1.0
		ID_BUTTON_86,  // P=<1.5
		ID_BUTTON_87,  // P=<2.0
		ID_BUTTON_88,  // P=<2.5
		ID_BUTTON_89,  // P=<3.0
		// stack
		ID_BUTTON_90,  // P+3.0
		ID_BUTTON_91,  // P+2.5
		ID_BUTTON_92,  // P+2.0
		ID_BUTTON_93,  // P+1.5
		ID_BUTTON_94,  // P+1.0
		ID_BUTTON_95,  // P+0.5
		ID_BUTTON_96,  // P0
		ID_BUTTON_97,  // P-0.5
		ID_BUTTON_98,  // P-1.0
		ID_BUTTON_99,  // P-1.5
		ID_BUTTON_100, // P-2.0
		ID_BUTTON_101, // P-2.5
		ID_BUTTON_102  // P-3.0
	} ID_Interface;
};

#endif
