Scriptname PathsOfEmpyrean_DLLFunctions Hidden

;-----------------------------------------------------------------------
;Returns the SKSE plugin's version as an array of 3 ints. Use to verify the plugin is installed and working.
;Version 1.0.0 becomes [1,0,0]
int[] function GetVersion() global native

;-----------------------------------------------------------------------
function UpdateRacesAllowPickpocket() global native

;-----------------------------------------------------------------------
armor[] function GetAllEquippedArmor(Actor a_actor) global native

bool function CreateStasisCubeFromAutomaton(Actor automaton) global native

