import pyhavoc
import hexdump

##
## test profile selecting
##
agent_type    = "Kaine"
kaine_profile = pyhavoc.agent.HcAgentProfileSelect( agent_type )
print( f'{agent_type}: { kaine_profile }' )

##
## testing agent profile generation
##

print( f"Generating {agent_type} Profile:" )
hexdump.hexdump( pyhavoc.agent.HcAgentProfileBuild( agent_type, kaine_profile ) )

print( "Generating from faulty profile:" )
hexdump.hexdump( pyhavoc.agent.HcAgentProfileBuild( agent_type, {
    "test value": "",
    "this is an example": 99
} ) )