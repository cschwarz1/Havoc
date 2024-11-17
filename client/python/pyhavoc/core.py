import threading

##
## import havoc specific libs
##
from _pyhavoc import core


def HcPluginRegister( plugin_path: str ):
    return core.HcPluginRegister( plugin_path )


def HcListenerProtocolData(
    protocol: str
) -> dict:
    return core.HcListenerProtocolData( protocol )


def HcListenerAll() -> list[str]:
    return core.HcListenerAll()


def HcListenerQueryType( name: str ) -> str:
    return core.HcListenerQueryType( name )


def HcListenerRegisterMenuAction(
    type: str,
    name: str,
    icon: str = "",
):
    def _register( function ):
        core.HcListenerRegisterMenuAction( type, name, icon, function )

    return _register


def HcRegisterMenuAction(
    name: str,
    icon_path: str = "",
):
    def _register( function ):
        core.HcRegisterMenuAction( name, icon_path, function )

    return _register
