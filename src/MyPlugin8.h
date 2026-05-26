#ifndef MYPLUGIN8_H
#define MYPLUGIN8_H

#include "vdjPlugin8.h"
#include <set>
#include <string>

// One bank of 26 buttons (13 range + 13 stack) tied to a single numeric comment tag (e.g. "Energy").
// Both range and stack have a "0" button in the middle. They are independent — stack `0` adds master
// to an additive selection; range `0` is the implicit start of both range sides (or stands alone).
struct TagBank
{
	static const int RANGE_BTN_COUNT = 13;
	static const int STACK_BTN_COUNT = 13;
	static const int TOTAL_BTN_COUNT = RANGE_BTN_COUNT + STACK_BTN_COUNT;
	static const int RANGE_ZERO_IDX  = 6; // index of the "0" range button

	std::string tagName;
	int  rangeBtns[RANGE_BTN_COUNT];   // SDK-owned momentary state
	int  stackBtns[STACK_BTN_COUNT];

	// Range mode: per-side peak overwrite, plus a standalone-zero latch.
	int  posPeakHs;          // 0..6 half-steps (0 = side off)
	int  negPeakHs;
	bool zeroRangeEngaged;   // standalone "0" engaged with no pos/neg side. Implicitly true whenever any side is non-zero (AnyRangeEngaged covers both).

	// Stack mode: per-button latch; each contributes its single half-step value.
	bool stackEngaged[STACK_BTN_COUNT];

	void Init(const std::string& name);
	void Clear();
	void HandleRangePress(int idx);  // idx 0..12
	void HandleStackPress(int idx);  // idx 0..11
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

		// -- Bank 0: Energy (buttons 3-28) --
		// range
		ID_BUTTON_3,   // E>=3.0
		ID_BUTTON_4,   // E>=2.5
		ID_BUTTON_5,   // E>=2.0
		ID_BUTTON_6,   // E>=1.5
		ID_BUTTON_7,   // E>=1.0
		ID_BUTTON_8,   // E>=0.5
		ID_BUTTON_9,   // E0  (range)
		ID_BUTTON_10,  // E=<0.5
		ID_BUTTON_11,  // E=<1.0
		ID_BUTTON_12,  // E=<1.5
		ID_BUTTON_13,  // E=<2.0
		ID_BUTTON_14,  // E=<2.5
		ID_BUTTON_15,  // E=<3.0
		// stack
		ID_BUTTON_16,  // E+3.0
		ID_BUTTON_17,  // E+2.5
		ID_BUTTON_18,  // E+2.0
		ID_BUTTON_19,  // E+1.5
		ID_BUTTON_20,  // E+1.0
		ID_BUTTON_21,  // E+0.5
		ID_BUTTON_22,  // E0  (stack)
		ID_BUTTON_23,  // E-0.5
		ID_BUTTON_24,  // E-1.0
		ID_BUTTON_25,  // E-1.5
		ID_BUTTON_26,  // E-2.0
		ID_BUTTON_27,  // E-2.5
		ID_BUTTON_28,  // E-3.0

		// -- Bank 1: Happy (buttons 29-54) --
		// range
		ID_BUTTON_29,  // H>=3.0
		ID_BUTTON_30,  // H>=2.5
		ID_BUTTON_31,  // H>=2.0
		ID_BUTTON_32,  // H>=1.5
		ID_BUTTON_33,  // H>=1.0
		ID_BUTTON_34,  // H>=0.5
		ID_BUTTON_35,  // H0  (range)
		ID_BUTTON_36,  // H=<0.5
		ID_BUTTON_37,  // H=<1.0
		ID_BUTTON_38,  // H=<1.5
		ID_BUTTON_39,  // H=<2.0
		ID_BUTTON_40,  // H=<2.5
		ID_BUTTON_41,  // H=<3.0
		// stack
		ID_BUTTON_42,  // H+3.0
		ID_BUTTON_43,  // H+2.5
		ID_BUTTON_44,  // H+2.0
		ID_BUTTON_45,  // H+1.5
		ID_BUTTON_46,  // H+1.0
		ID_BUTTON_47,  // H+0.5
		ID_BUTTON_48,  // H0  (stack)
		ID_BUTTON_49,  // H-0.5
		ID_BUTTON_50,  // H-1.0
		ID_BUTTON_51,  // H-1.5
		ID_BUTTON_52,  // H-2.0
		ID_BUTTON_53,  // H-2.5
		ID_BUTTON_54,  // H-3.0

		// -- Bank 2: Dance (buttons 55-80) --
		// range
		ID_BUTTON_55,  // D>=3.0
		ID_BUTTON_56,  // D>=2.5
		ID_BUTTON_57,  // D>=2.0
		ID_BUTTON_58,  // D>=1.5
		ID_BUTTON_59,  // D>=1.0
		ID_BUTTON_60,  // D>=0.5
		ID_BUTTON_61,  // D0  (range)
		ID_BUTTON_62,  // D=<0.5
		ID_BUTTON_63,  // D=<1.0
		ID_BUTTON_64,  // D=<1.5
		ID_BUTTON_65,  // D=<2.0
		ID_BUTTON_66,  // D=<2.5
		ID_BUTTON_67,  // D=<3.0
		// stack
		ID_BUTTON_68,  // D+3.0
		ID_BUTTON_69,  // D+2.5
		ID_BUTTON_70,  // D+2.0
		ID_BUTTON_71,  // D+1.5
		ID_BUTTON_72,  // D+1.0
		ID_BUTTON_73,  // D+0.5
		ID_BUTTON_74,  // D0  (stack)
		ID_BUTTON_75,  // D-0.5
		ID_BUTTON_76,  // D-1.0
		ID_BUTTON_77,  // D-1.5
		ID_BUTTON_78,  // D-2.0
		ID_BUTTON_79,  // D-2.5
		ID_BUTTON_80,  // D-3.0

		// -- Bank 3: Pop (buttons 81-106) --
		// range
		ID_BUTTON_81,  // P>=3.0
		ID_BUTTON_82,  // P>=2.5
		ID_BUTTON_83,  // P>=2.0
		ID_BUTTON_84,  // P>=1.5
		ID_BUTTON_85,  // P>=1.0
		ID_BUTTON_86,  // P>=0.5
		ID_BUTTON_87,  // P0  (range)
		ID_BUTTON_88,  // P=<0.5
		ID_BUTTON_89,  // P=<1.0
		ID_BUTTON_90,  // P=<1.5
		ID_BUTTON_91,  // P=<2.0
		ID_BUTTON_92,  // P=<2.5
		ID_BUTTON_93,  // P=<3.0
		// stack
		ID_BUTTON_94,  // P+3.0
		ID_BUTTON_95,  // P+2.5
		ID_BUTTON_96,  // P+2.0
		ID_BUTTON_97,  // P+1.5
		ID_BUTTON_98,  // P+1.0
		ID_BUTTON_99,  // P+0.5
		ID_BUTTON_100, // P0  (stack)
		ID_BUTTON_101, // P-0.5
		ID_BUTTON_102, // P-1.0
		ID_BUTTON_103, // P-1.5
		ID_BUTTON_104, // P-2.0
		ID_BUTTON_105, // P-2.5
		ID_BUTTON_106  // P-3.0
	} ID_Interface;
};

#endif
