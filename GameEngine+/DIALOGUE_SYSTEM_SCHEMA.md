# Dialogue System JSON Schema

## Overview
Node-based dialogue system with branching paths, conditions, and actions.

## File Structure
```
%APPDATA%\GameEnginePlus\dialogs\<npcId>.json
```

## JSON Schema

```json
{
  "version": 1,
  "npcId": "elder_sage",
  "npcName": "Elder Sage",
  "nodes": [
	{
	  "id": "greeting_001",
	  "type": "text",
	  "text": [
		"Greetings, adventurer. What brings you to my humble abode?",
		"Welcome! I've been expecting someone..."
	  ],
	  "speaker": "elder_sage",
	  "choices": [
		{
		  "id": "greeting_001_choice_1",
		  "text": "Tell me about the ancient artifact.",
		  "nextNode": "artifact_001",
		  "condition": null,
		  "action": null
		},
		{
		  "id": "greeting_001_choice_2",
		  "text": "I'm just passing through.",
		  "nextNode": "dismissal_001",
		  "condition": null,
		  "action": null
		}
	  ]
	},
	{
	  "id": "artifact_001",
	  "type": "text",
	  "text": [
		"Ah yes, the Crystal of Eternity. A powerful relic from ages past.",
		"It holds the key to preventing the darkness from spreading."
	  ],
	  "speaker": "elder_sage",
	  "choices": [
		{
		  "id": "artifact_001_choice_1",
		  "text": "Where can I find it?",
		  "nextNode": "artifact_location_001",
		  "condition": null,
		  "action": null
		},
		{
		  "id": "artifact_001_choice_2",
		  "text": "Tell me more.",
		  "nextNode": "artifact_lore_001",
		  "condition": null,
		  "action": null
		},
		{
		  "id": "artifact_001_choice_3",
		  "text": "I'm not interested.",
		  "nextNode": "end_001",
		  "condition": null,
		  "action": null
		}
	  ]
	},
	{
	  "id": "artifact_location_001",
	  "type": "text",
	  "text": [
		"The artifact rests in the Caverns of Shadow, to the east.",
		"Be warned - many dangers lurk there."
	  ],
	  "speaker": "elder_sage",
	  "choices": [
		{
		  "id": "artifact_location_001_choice_1",
		  "text": "I'll retrieve it.",
		  "nextNode": "quest_start_001",
		  "condition": null,
		  "action": {
			"type": "quest_start",
			"questId": "retrieve_crystal"
		  }
		},
		{
		  "id": "artifact_location_001_choice_2",
		  "text": "Is there anything else?",
		  "nextNode": "greeting_001",
		  "condition": null,
		  "action": null
		}
	  ]
	},
	{
	  "id": "quest_start_001",
	  "type": "text",
	  "text": ["Safe travels, adventurer. May fortune favor you."],
	  "speaker": "elder_sage",
	  "choices": [
		{
		  "id": "quest_start_001_end",
		  "text": "Farewell.",
		  "nextNode": "end_001",
		  "condition": null,
		  "action": null
		}
	  ]
	},
	{
	  "id": "dismissal_001",
	  "type": "text",
	  "text": ["Very well. Perhaps our paths will cross again."],
	  "speaker": "elder_sage",
	  "choices": [
		{
		  "id": "dismissal_001_end",
		  "text": "Goodbye.",
		  "nextNode": "end_001",
		  "condition": null,
		  "action": null
		}
	  ]
	},
	{
	  "id": "artifact_lore_001",
	  "type": "text",
	  "text": [
		"The Crystal was forged in the Age of Stars.",
		"It has the power to seal away darkness itself."
	  ],
	  "speaker": "elder_sage",
	  "choices": [
		{
		  "id": "artifact_lore_001_choice_1",
		  "text": "How can I use it?",
		  "nextNode": "artifact_usage_001",
		  "condition": null,
		  "action": null
		},
		{
		  "id": "artifact_lore_001_choice_2",
		  "text": "That's enough.",
		  "nextNode": "end_001",
		  "condition": null,
		  "action": null
		}
	  ]
	},
	{
	  "id": "artifact_usage_001",
	  "type": "text",
	  "text": ["Only a chosen one can wield it. You must prove your worth."],
	  "speaker": "elder_sage",
	  "choices": [
		{
		  "id": "artifact_usage_001_choice_1",
		  "text": "I'm ready to prove myself.",
		  "nextNode": "quest_start_001",
		  "condition": null,
		  "action": {
			"type": "quest_start",
			"questId": "retrieve_crystal"
		  }
		}
	  ]
	},
	{
	  "id": "end_001",
	  "type": "end",
	  "text": null,
	  "speaker": null,
	  "choices": []
	}
  ]
}
```

## Field Definitions

### Root Level
- `version` (int): Schema version for future compatibility
- `npcId` (string): Unique identifier for the NPC
- `npcName` (string): Display name of the NPC
- `nodes` (array): Array of dialogue nodes

### Node Object
- `id` (string): Unique node identifier within this dialogue
- `type` (string): "text" | "end" | "branch" (for complex conditions)
- `text` (array of strings): Dialogue text. If multiple, randomly pick one
- `speaker` (string): Who's speaking (NPC ID or "player")
- `choices` (array): Available player responses

### Choice Object
- `id` (string): Unique choice identifier
- `text` (string): Text displayed to player
- `nextNode` (string): ID of node to transition to
- `condition` (object or null): Condition that must be met to show this choice
- `action` (object or null): Action to execute when this choice is selected

### Condition Object (Future)
```json
{
  "type": "quest_completed | quest_active | has_item | level_minimum | has_flag",
  "value": "quest_id_or_item_id_or_level_num_or_flag_name"
}
```

### Action Object (Future)
```json
{
  "type": "quest_start | quest_complete | give_item | set_flag | add_experience",
  "questId": "quest_id",  // for quest_start/complete
  "itemId": "item_id",     // for give_item
  "flagName": "flag_name", // for set_flag
  "experience": 100        // for add_experience
}
```

## Notes
- `text` as array allows random variations of same dialogue line
- Multiple paths from one node enable branching
- `condition: null` means always show choice
- `action: null` means no side effects
- `type: "end"` terminates dialogue (text array can be null)
- All `id` values should be unique within their scope (globally or per-node)
