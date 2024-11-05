import pyhavoc

##
## test profile listing and query
##

print( f"List : { pyhavoc.agent.HcAgentProfileList() }" )
print( f"Query: { pyhavoc.agent.HcAgentProfileQuery( 'Profile' ) }" )