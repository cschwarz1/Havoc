#ifndef HCINTERFACE_IHCPLUGINAGENTVIEW_H
#define HCINTERFACE_IHCPLUGINAGENTVIEW_H

#include <pybind11/pybind11.h>
#include <IHcPlugin.h>

#include <QObject>

class IHcPluginAgentView : public IHcPluginBase {
public:
    virtual ~IHcPluginAgentView() = default;

    /*!
     * @brief
     *  initialize plugin agent view
     *
     * @param agent_instance
     *  agent python object instance
     *
     * @return
     *  boolean status of function
     */
    virtual auto initialize(
        const pybind11::object& agent_instance
    ) -> bool = 0;

    /*!
     * @brief
     *  render the ui widget in the client
     *
     * @param parent
     *  parent qt widget
     */
    virtual auto view(
        QWidget* parent
    ) -> void = 0;
};

class IHcPluginAgentFactory {
public:
    virtual ~IHcPluginAgentFactory() = default;

    /*!
     * @brief
     *  create an instance of plugin agent view
     *
     * @return
     *  new instance of plugin agent view
     */
    virtual auto AgentViewInstance(
        void
    ) -> IHcPluginAgentView* = 0;
};

#define IID_IHcPluginAgentView "com.havocframework.IHcAgentPluginView"
Q_DECLARE_INTERFACE( IHcPluginAgentView, IID_IHcPluginAgentView )

#define IID_IHcPluginAgentFactory "com.havocframework.IHcPluginAgentFactory"
Q_DECLARE_INTERFACE( IHcPluginAgentFactory, IID_IHcPluginAgentFactory )

#endif // HCINTERFACE_IHCPLUGINAGENTVIEW_H
