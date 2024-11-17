#ifndef HAVOCCLIENT_HCPLUGINMANAGER_H
#define HAVOCCLIENT_HCPLUGINMANAGER_H

#include <Common.h>
#include <IHcPluginAgentView.h>

class HcPluginManager final : public QObject {
    Q_OBJECT

    std::vector<IHcPlugin*> plugins  = {};
    IHcApplication*         core_app = {};
public:
    explicit HcPluginManager();

    auto loadPlugin(
        const std::string& path
    ) -> void;
};

#endif // HAVOCCLIENT_HCPLUGINMANAGER_H
