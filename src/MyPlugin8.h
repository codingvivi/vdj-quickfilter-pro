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

protected:
	typedef enum _ID_Interface
	{
		ID_BUTTON_1,  // refresh
		ID_BUTTON_2,  // kill

		// -- Bank 0: Energy (buttons 3-27) --
		// range
		ID_BUTTON_3,  ID_BUTTON_4,  ID_BUTTON_5,  ID_BUTTON_6,  ID_BUTTON_7,
		ID_BUTTON_8,  ID_BUTTON_9,  ID_BUTTON_10, ID_BUTTON_11, ID_BUTTON_12,
		ID_BUTTON_13, ID_BUTTON_14,
		// stack
		ID_BUTTON_15, ID_BUTTON_16, ID_BUTTON_17, ID_BUTTON_18, ID_BUTTON_19,
		ID_BUTTON_20, ID_BUTTON_21, ID_BUTTON_22, ID_BUTTON_23, ID_BUTTON_24,
		ID_BUTTON_25, ID_BUTTON_26, ID_BUTTON_27,

		// -- Bank 1: Happy (buttons 28-52) --
		// range
		ID_BUTTON_28, ID_BUTTON_29, ID_BUTTON_30, ID_BUTTON_31, ID_BUTTON_32,
		ID_BUTTON_33, ID_BUTTON_34, ID_BUTTON_35, ID_BUTTON_36, ID_BUTTON_37,
		ID_BUTTON_38, ID_BUTTON_39,
		// stack
		ID_BUTTON_40, ID_BUTTON_41, ID_BUTTON_42, ID_BUTTON_43, ID_BUTTON_44,
		ID_BUTTON_45, ID_BUTTON_46, ID_BUTTON_47, ID_BUTTON_48, ID_BUTTON_49,
		ID_BUTTON_50, ID_BUTTON_51, ID_BUTTON_52,

		// -- Bank 2: Dance (buttons 53-77) --
		// range
		ID_BUTTON_53, ID_BUTTON_54, ID_BUTTON_55, ID_BUTTON_56, ID_BUTTON_57,
		ID_BUTTON_58, ID_BUTTON_59, ID_BUTTON_60, ID_BUTTON_61, ID_BUTTON_62,
		ID_BUTTON_63, ID_BUTTON_64,
		// stack
		ID_BUTTON_65, ID_BUTTON_66, ID_BUTTON_67, ID_BUTTON_68, ID_BUTTON_69,
		ID_BUTTON_70, ID_BUTTON_71, ID_BUTTON_72, ID_BUTTON_73, ID_BUTTON_74,
		ID_BUTTON_75, ID_BUTTON_76, ID_BUTTON_77,

		// -- Bank 3: Pop (buttons 78-102) --
		// range
		ID_BUTTON_78, ID_BUTTON_79, ID_BUTTON_80, ID_BUTTON_81, ID_BUTTON_82,
		ID_BUTTON_83, ID_BUTTON_84, ID_BUTTON_85, ID_BUTTON_86, ID_BUTTON_87,
		ID_BUTTON_88, ID_BUTTON_89,
		// stack
		ID_BUTTON_90, ID_BUTTON_91, ID_BUTTON_92, ID_BUTTON_93, ID_BUTTON_94,
		ID_BUTTON_95, ID_BUTTON_96, ID_BUTTON_97, ID_BUTTON_98, ID_BUTTON_99,
		ID_BUTTON_100, ID_BUTTON_101, ID_BUTTON_102
	} ID_Interface;
};

#endif
