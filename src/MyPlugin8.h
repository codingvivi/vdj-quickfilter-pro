#ifndef MYPLUGIN8_H
#define MYPLUGIN8_H

#include "vdjPlugin8.h"
#include <string>

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
	int m_EnergyBtns[ENERGY_BTN_COUNT];

	int GetMasterDeck(); // returns 1-4 for the current master deck, or 0 if none
	std::string GetMasterDeckComment(); // comment of the track on the master deck
	std::string GetNumericTag(const std::string& name); // extracts the NN from a #NN<name> tag in the master comment; empty if none
	void SendCommentTagFilter(const std::string& name, const std::string& value); // sends quick_filter 'Comment has tag #<value><name>'
	void MatchNumericTag(const std::string& name, float offset); // reads #NN<name> from master comment, adds offset, filters by the new tag

	bool isMasterFX(); // an example of additional function for the use of GetInfo()

protected:
	typedef enum _ID_Interface
	{
		ID_BUTTON_1,  // refresh
		ID_BUTTON_2,  // Energy +3.0
		ID_BUTTON_3,  // Energy +2.5
		ID_BUTTON_4,  // Energy +2.0
		ID_BUTTON_5,  // Energy +1.5
		ID_BUTTON_6,  // Energy +1.0
		ID_BUTTON_7,  // Energy +0.5
		ID_BUTTON_8,  // Energy  0.0
		ID_BUTTON_9,  // Energy -0.5
		ID_BUTTON_10, // Energy -1.0
		ID_BUTTON_11, // Energy -1.5
		ID_BUTTON_12, // Energy -2.0
		ID_BUTTON_13, // Energy -2.5
		ID_BUTTON_14  // Energy -3.0
	} ID_Interface;
};

#endif
