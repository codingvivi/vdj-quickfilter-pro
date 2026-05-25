#ifndef MYPLUGIN8_H
#define MYPLUGIN8_H

#include "vdjPlugin8.h"
#include <string>

class CMyPlugin8 : public IVdjPlugin8
{
public:
	HRESULT VDJ_API OnLoad();
	HRESULT VDJ_API OnGetPluginInfo(TVdjPluginInfo8 *infos);
	ULONG VDJ_API Release();
    HRESULT VDJ_API OnGetUserInterface(TVdjPluginInterface8 *pluginInterface);
    HRESULT VDJ_API OnParameter(int id);

private:
	int m_Btn1State;
	int m_Btn2State;

	int GetMasterDeck(); // returns 1-4 for the current master deck, or 0 if none
	std::string GetMasterDeckComment(); // comment of the track on the master deck
	std::string GetNumericTag(const std::string& name); // extracts the NN from a #NN<name> tag in the master comment; empty if none
	void SendCommentTagFilter(const std::string& name, const std::string& value); // sends quick_filter 'Comment has tag #<value><name>'

	bool isMasterFX(); // an example of additional function for the use of GetInfo()

protected:
	typedef enum _ID_Interface
	{
		ID_BUTTON_1,
		ID_BUTTON_2
	} ID_Interface;
};

#endif
