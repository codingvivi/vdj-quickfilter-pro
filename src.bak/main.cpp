#include "vdjPlugin8.h"
#include <stdio.h>
#include <string.h>

// Toggles a saved Quick Filter on/off via the VDJScript `quick_filter <index>`
// verb. The index is configurable from the plugin's parameter panel so the
// user can pick which saved Quick Filter slot to control.
class QuickFilterRefresh : public IVdjPluginStartStop8
{
public:
    int  toggle_btn;
    char index_str[16];

    HRESULT VDJ_API OnLoad()
    {
        memset(index_str, 0, sizeof(index_str));
        strcpy(index_str, "1");

        DeclareParameterButton(&toggle_btn, 1, "Toggle Quick Filter", "Toggle");
        DeclareParameterString(index_str,   2, "Quick Filter Index",  "Index",
                               sizeof(index_str));
        return S_OK;
    }

    HRESULT VDJ_API OnGetPluginInfo(TVdjPluginInfo8 *info)
    {
        info->PluginName  = "Quick Filter Refresh";
        info->Author      = "Me";
        info->Description = "Toggle a saved Quick Filter on and off.";
        info->Version     = "0.1";
        info->Flags       = 0;
        return S_OK;
    }

    HRESULT VDJ_API OnParameter(int id)
    {
        switch (id) {
        case 1:
            if (toggle_btn) {
                const char *idx = index_str[0] ? index_str : "1";

                // Bail out if no Quick Filter is saved at that slot — saves
                // VDJ from logging a no-op and gives the user a hint that
                // their index is wrong.
                double exists = 0.0;
                char query[64];
                snprintf(query, sizeof(query), "has_quick_filter %s", idx);
                GetInfo(query, &exists);
                if (exists < 0.5)
                    break;

                char cmd[64];
                snprintf(cmd, sizeof(cmd), "quick_filter %s", idx);
                SendCommand(cmd);
            }
            break;
        }
        return S_OK;
    }

    HRESULT VDJ_API OnStart() { return S_OK; }
    HRESULT VDJ_API OnStop()  { return S_OK; }
};

extern "C" __attribute__((visibility("default")))
int DllGetClassObject(const GUID &rclsid, const GUID &riid, void **ppObject)
{
    *ppObject = new QuickFilterRefresh();
    return 0; // S_OK
}
