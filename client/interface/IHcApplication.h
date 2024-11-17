#ifndef HCINTERFACE_IHCAPPLICATION_H
#define HCINTERFACE_IHCAPPLICATION_H

#include <QWidget>
#include <string>

template <typename T>
using HcFnCallbackCtx = auto ( * ) ( const T& context ) -> void;
using HcFnCallback    = auto ( * ) ( void ) -> void;

class IHcApplication {

public:
    virtual ~IHcApplication() = default;

    //
    // agent page apis
    //

    virtual auto PageAgentAddTab(
        const std::string& name,
        QWidget*           widget
    ) -> void = 0;

    //
    // register actions callbacks
    //

    /*!
     * @brief
     *  register agent action
     *
     * @param action_name
     *  action name
     *
     * @param action_icon
     *  action icon
     *
     * @param action_func
     *  agent function
     *
     * @param agent_type
     *  agent type to only
     *  specify the action to
     */
    virtual auto RegisterAgentAction(
        const std::string&           action_name,
        const std::string&           action_icon,
        HcFnCallbackCtx<std::string> action_func,
        const std::string&           agent_type
    ) -> void = 0;

    /*!
     * @brief
     *  register agent action for
     *  all available agents
     *
     * @param action_name
     *  action name
     *
     * @param action_icon
     *  action icon
     *
     * @param action_func
     *  agent function
     */
    virtual auto RegisterAgentAction(
        const std::string&           action_name,
        const std::string&           action_icon,
        HcFnCallbackCtx<std::string> action_func
    ) -> void = 0;

    /*!
     * @brief
     *  register agent action for
     *  all available agents
     *
     * @param action_name
     *  action name
     *
     * @param action_func
     *  agent function
     *
     * @param multi_select
     *  add the action to
     *  the multi select
     */
    virtual auto RegisterAgentAction(
        const std::string&           action_name,
        HcFnCallbackCtx<std::string> action_func,
        bool                         multi_select
    ) -> void = 0;

    /*!
     * @brief
     *  register an action under
     *  the action menu
     *
     * @param action_name
     *  action name
     *
     * @param action_icon
     *  action icon
     *
     * @param action_func
     *  action callback
     */
    virtual auto RegisterMenuAction(
        const std::string& action_name,
        const std::string& action_icon,
        HcFnCallback       action_func
    ) -> void = 0;
};

#endif //HCINTERFACE_IHCAPPLICATION_H
