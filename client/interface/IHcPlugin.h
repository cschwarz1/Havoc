#ifndef HCINTERFACE_IHCPLUGINBASE_H
#define HCINTERFACE_IHCPLUGINBASE_H

#include <IHcApplication.h>
#include <QObject>

enum HcPluginType {
    PluginWidget,   // plugin is designed and written as a ui interface widget
    PluginWorker    // plugin is written to be run in the background
};

class IHcPluginBase : public QObject {
    Q_OBJECT

public:
    virtual ~IHcPluginBase() = default;

    virtual auto PluginName() -> std::string  = 0;
    virtual auto PluginType() -> HcPluginType = 0;
};

class IHcPlugin {
public:
    virtual ~IHcPlugin() = default;

    virtual auto main(
        IHcApplication* HcApplication
    ) -> void = 0;
};

#define IID_IHcPlugin "com.havocframework.IHcPlugin"
Q_DECLARE_INTERFACE( IHcPlugin, IID_IHcPlugin );

#endif //HCINTERFACE_IHCPLUGINBASE_H
