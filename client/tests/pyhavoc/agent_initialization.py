from pyhavoc.agent import HcAgentRegisterInitialize, HcAgent


##
## capture Kaine agent initialization
##
@HcAgentRegisterInitialize( 'Kaine' )
def KaineInitializeCallback( agent: HcAgent ):
    print( f'agent kaine initialized: {agent.agent_meta()}\n' )


##
## capture every agent initialization
##
@HcAgentRegisterInitialize()
def AgentInitializeCallback( agent: HcAgent ):
    print( f'agent [type: {agent.agent_type()}] initialized: {agent.agent_meta()}\n' )


##
## most likely won't be called
##
@HcAgentRegisterInitialize( 'AgentType' )
def AgentInitializeCallback( agent: HcAgent ):
    print( f'agent AgentType initialized: {agent.agent_meta()}\n' )

