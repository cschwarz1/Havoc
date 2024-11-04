import pyhavoc
import hexdump

##
## test profile selecting
##

print( f"Random: { pyhavoc.agent.HcAgentProfileSelect() }" )

print( f'Kaine: { pyhavoc.agent.HcAgentProfileSelect( "Kaine" ) }' )

print( f'NoneExisting: { pyhavoc.agent.HcAgentProfileSelect( "NoneExisting" ) }' )
