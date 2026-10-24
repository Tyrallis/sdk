---@meta wxl
-- The Lua API of wxl.lua, for lua-language-server. Generated from the SDK's event
-- table (reflect.hpp) and object table (object_reads.def) by wxl-lua-types; do not
-- edit by hand.

---A guid, as "0x" and sixteen hex digits: nothing is lost and it keys a table.
---@alias wxl.Guid string

---An event's bytes, copied into a string.
---@alias wxl.Bytes string

---@class wxl.Vec3
---@field x number
---@field y number
---@field z number

---An element of an update field: a number, a guid's string for a guid or another
---64-bit value, a table of numbers for the bytes or the two halves of a dword.
---@alias wxl.FieldValue number|string|number[]

---A field of update_fields.def: the client's name, the kind's and the field's, or
---the field's alone where the object has one of that name.
---@alias wxl.FieldName
---| "OBJECT_FIELD_GUID"
---| "Object.Guid"
---| "OBJECT_FIELD_TYPE"
---| "Object.Type"
---| "OBJECT_FIELD_ENTRY"
---| "Object.Entry"
---| "OBJECT_FIELD_SCALE_X"
---| "Object.ScaleX"
---| "OBJECT_FIELD_PADDING"
---| "Object.Padding"
---| "ITEM_FIELD_OWNER"
---| "Item.Owner"
---| "ITEM_FIELD_CONTAINED"
---| "Item.Contained"
---| "ITEM_FIELD_CREATOR"
---| "Item.Creator"
---| "ITEM_FIELD_GIFTCREATOR"
---| "Item.Giftcreator"
---| "ITEM_FIELD_STACK_COUNT"
---| "Item.StackCount"
---| "ITEM_FIELD_DURATION"
---| "Item.Duration"
---| "ITEM_FIELD_SPELL_CHARGES"
---| "Item.SpellCharges"
---| "ITEM_FIELD_FLAGS"
---| "Item.Flags"
---| "ITEM_FIELD_ENCHANTMENT_1_1"
---| "Item.Enchantment1_1"
---| "ITEM_FIELD_ENCHANTMENT_1_3"
---| "Item.Enchantment1_3"
---| "ITEM_FIELD_ENCHANTMENT_2_1"
---| "Item.Enchantment2_1"
---| "ITEM_FIELD_ENCHANTMENT_2_3"
---| "Item.Enchantment2_3"
---| "ITEM_FIELD_ENCHANTMENT_3_1"
---| "Item.Enchantment3_1"
---| "ITEM_FIELD_ENCHANTMENT_3_3"
---| "Item.Enchantment3_3"
---| "ITEM_FIELD_ENCHANTMENT_4_1"
---| "Item.Enchantment4_1"
---| "ITEM_FIELD_ENCHANTMENT_4_3"
---| "Item.Enchantment4_3"
---| "ITEM_FIELD_ENCHANTMENT_5_1"
---| "Item.Enchantment5_1"
---| "ITEM_FIELD_ENCHANTMENT_5_3"
---| "Item.Enchantment5_3"
---| "ITEM_FIELD_ENCHANTMENT_6_1"
---| "Item.Enchantment6_1"
---| "ITEM_FIELD_ENCHANTMENT_6_3"
---| "Item.Enchantment6_3"
---| "ITEM_FIELD_ENCHANTMENT_7_1"
---| "Item.Enchantment7_1"
---| "ITEM_FIELD_ENCHANTMENT_7_3"
---| "Item.Enchantment7_3"
---| "ITEM_FIELD_ENCHANTMENT_8_1"
---| "Item.Enchantment8_1"
---| "ITEM_FIELD_ENCHANTMENT_8_3"
---| "Item.Enchantment8_3"
---| "ITEM_FIELD_ENCHANTMENT_9_1"
---| "Item.Enchantment9_1"
---| "ITEM_FIELD_ENCHANTMENT_9_3"
---| "Item.Enchantment9_3"
---| "ITEM_FIELD_ENCHANTMENT_10_1"
---| "Item.Enchantment10_1"
---| "ITEM_FIELD_ENCHANTMENT_10_3"
---| "Item.Enchantment10_3"
---| "ITEM_FIELD_ENCHANTMENT_11_1"
---| "Item.Enchantment11_1"
---| "ITEM_FIELD_ENCHANTMENT_11_3"
---| "Item.Enchantment11_3"
---| "ITEM_FIELD_ENCHANTMENT_12_1"
---| "Item.Enchantment12_1"
---| "ITEM_FIELD_ENCHANTMENT_12_3"
---| "Item.Enchantment12_3"
---| "ITEM_FIELD_PROPERTY_SEED"
---| "Item.PropertySeed"
---| "ITEM_FIELD_RANDOM_PROPERTIES_ID"
---| "Item.RandomPropertiesId"
---| "ITEM_FIELD_DURABILITY"
---| "Item.Durability"
---| "ITEM_FIELD_MAXDURABILITY"
---| "Item.Maxdurability"
---| "ITEM_FIELD_CREATE_PLAYED_TIME"
---| "Item.CreatePlayedTime"
---| "ITEM_FIELD_PAD"
---| "Item.Pad"
---| "CONTAINER_FIELD_NUM_SLOTS"
---| "Container.NumSlots"
---| "CONTAINER_ALIGN_PAD"
---| "Container.AlignPad"
---| "CONTAINER_FIELD_SLOT_1"
---| "Container.Slot1"
---| "UNIT_FIELD_CHARM"
---| "Unit.Charm"
---| "UNIT_FIELD_SUMMON"
---| "Unit.Summon"
---| "UNIT_FIELD_CRITTER"
---| "Unit.Critter"
---| "UNIT_FIELD_CHARMEDBY"
---| "Unit.Charmedby"
---| "UNIT_FIELD_SUMMONEDBY"
---| "Unit.Summonedby"
---| "UNIT_FIELD_CREATEDBY"
---| "Unit.Createdby"
---| "UNIT_FIELD_TARGET"
---| "Unit.Target"
---| "UNIT_FIELD_CHANNEL_OBJECT"
---| "Unit.ChannelObject"
---| "UNIT_CHANNEL_SPELL"
---| "Unit.ChannelSpell"
---| "UNIT_FIELD_BYTES_0"
---| "Unit.Bytes0"
---| "UNIT_FIELD_HEALTH"
---| "Unit.Health"
---| "UNIT_FIELD_POWER1"
---| "Unit.Power1"
---| "UNIT_FIELD_POWER2"
---| "Unit.Power2"
---| "UNIT_FIELD_POWER3"
---| "Unit.Power3"
---| "UNIT_FIELD_POWER4"
---| "Unit.Power4"
---| "UNIT_FIELD_POWER5"
---| "Unit.Power5"
---| "UNIT_FIELD_POWER6"
---| "Unit.Power6"
---| "UNIT_FIELD_POWER7"
---| "Unit.Power7"
---| "UNIT_FIELD_MAXHEALTH"
---| "Unit.Maxhealth"
---| "UNIT_FIELD_MAXPOWER1"
---| "Unit.Maxpower1"
---| "UNIT_FIELD_MAXPOWER2"
---| "Unit.Maxpower2"
---| "UNIT_FIELD_MAXPOWER3"
---| "Unit.Maxpower3"
---| "UNIT_FIELD_MAXPOWER4"
---| "Unit.Maxpower4"
---| "UNIT_FIELD_MAXPOWER5"
---| "Unit.Maxpower5"
---| "UNIT_FIELD_MAXPOWER6"
---| "Unit.Maxpower6"
---| "UNIT_FIELD_MAXPOWER7"
---| "Unit.Maxpower7"
---| "UNIT_FIELD_POWER_REGEN_FLAT_MODIFIER"
---| "Unit.PowerRegenFlatModifier"
---| "UNIT_FIELD_POWER_REGEN_INTERRUPTED_FLAT_MODIFIER"
---| "Unit.PowerRegenInterruptedFlatModifier"
---| "UNIT_FIELD_LEVEL"
---| "Unit.Level"
---| "UNIT_FIELD_FACTIONTEMPLATE"
---| "Unit.Factiontemplate"
---| "UNIT_VIRTUAL_ITEM_SLOT_ID"
---| "Unit.VirtualItemSlotId"
---| "UNIT_FIELD_FLAGS"
---| "Unit.Flags"
---| "UNIT_FIELD_FLAGS_2"
---| "Unit.Flags2"
---| "UNIT_FIELD_AURASTATE"
---| "Unit.Aurastate"
---| "UNIT_FIELD_BASEATTACKTIME"
---| "Unit.Baseattacktime"
---| "UNIT_FIELD_RANGEDATTACKTIME"
---| "Unit.Rangedattacktime"
---| "UNIT_FIELD_BOUNDINGRADIUS"
---| "Unit.Boundingradius"
---| "UNIT_FIELD_COMBATREACH"
---| "Unit.Combatreach"
---| "UNIT_FIELD_DISPLAYID"
---| "Unit.Displayid"
---| "UNIT_FIELD_NATIVEDISPLAYID"
---| "Unit.Nativedisplayid"
---| "UNIT_FIELD_MOUNTDISPLAYID"
---| "Unit.Mountdisplayid"
---| "UNIT_FIELD_MINDAMAGE"
---| "Unit.Mindamage"
---| "UNIT_FIELD_MAXDAMAGE"
---| "Unit.Maxdamage"
---| "UNIT_FIELD_MINOFFHANDDAMAGE"
---| "Unit.Minoffhanddamage"
---| "UNIT_FIELD_MAXOFFHANDDAMAGE"
---| "Unit.Maxoffhanddamage"
---| "UNIT_FIELD_BYTES_1"
---| "Unit.Bytes1"
---| "UNIT_FIELD_PETNUMBER"
---| "Unit.Petnumber"
---| "UNIT_FIELD_PET_NAME_TIMESTAMP"
---| "Unit.PetNameTimestamp"
---| "UNIT_FIELD_PETEXPERIENCE"
---| "Unit.Petexperience"
---| "UNIT_FIELD_PETNEXTLEVELEXP"
---| "Unit.Petnextlevelexp"
---| "UNIT_DYNAMIC_FLAGS"
---| "Unit.DynamicFlags"
---| "UNIT_MOD_CAST_SPEED"
---| "Unit.ModCastSpeed"
---| "UNIT_CREATED_BY_SPELL"
---| "Unit.CreatedBySpell"
---| "UNIT_NPC_FLAGS"
---| "Unit.NpcFlags"
---| "UNIT_NPC_EMOTESTATE"
---| "Unit.NpcEmotestate"
---| "UNIT_FIELD_STAT0"
---| "Unit.Stat0"
---| "UNIT_FIELD_STAT1"
---| "Unit.Stat1"
---| "UNIT_FIELD_STAT2"
---| "Unit.Stat2"
---| "UNIT_FIELD_STAT3"
---| "Unit.Stat3"
---| "UNIT_FIELD_STAT4"
---| "Unit.Stat4"
---| "UNIT_FIELD_POSSTAT0"
---| "Unit.Posstat0"
---| "UNIT_FIELD_POSSTAT1"
---| "Unit.Posstat1"
---| "UNIT_FIELD_POSSTAT2"
---| "Unit.Posstat2"
---| "UNIT_FIELD_POSSTAT3"
---| "Unit.Posstat3"
---| "UNIT_FIELD_POSSTAT4"
---| "Unit.Posstat4"
---| "UNIT_FIELD_NEGSTAT0"
---| "Unit.Negstat0"
---| "UNIT_FIELD_NEGSTAT1"
---| "Unit.Negstat1"
---| "UNIT_FIELD_NEGSTAT2"
---| "Unit.Negstat2"
---| "UNIT_FIELD_NEGSTAT3"
---| "Unit.Negstat3"
---| "UNIT_FIELD_NEGSTAT4"
---| "Unit.Negstat4"
---| "UNIT_FIELD_RESISTANCES"
---| "Unit.Resistances"
---| "UNIT_FIELD_RESISTANCEBUFFMODSPOSITIVE"
---| "Unit.Resistancebuffmodspositive"
---| "UNIT_FIELD_RESISTANCEBUFFMODSNEGATIVE"
---| "Unit.Resistancebuffmodsnegative"
---| "UNIT_FIELD_BASE_MANA"
---| "Unit.BaseMana"
---| "UNIT_FIELD_BASE_HEALTH"
---| "Unit.BaseHealth"
---| "UNIT_FIELD_BYTES_2"
---| "Unit.Bytes2"
---| "UNIT_FIELD_ATTACK_POWER"
---| "Unit.AttackPower"
---| "UNIT_FIELD_ATTACK_POWER_MODS"
---| "Unit.AttackPowerMods"
---| "UNIT_FIELD_ATTACK_POWER_MULTIPLIER"
---| "Unit.AttackPowerMultiplier"
---| "UNIT_FIELD_RANGED_ATTACK_POWER"
---| "Unit.RangedAttackPower"
---| "UNIT_FIELD_RANGED_ATTACK_POWER_MODS"
---| "Unit.RangedAttackPowerMods"
---| "UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER"
---| "Unit.RangedAttackPowerMultiplier"
---| "UNIT_FIELD_MINRANGEDDAMAGE"
---| "Unit.Minrangeddamage"
---| "UNIT_FIELD_MAXRANGEDDAMAGE"
---| "Unit.Maxrangeddamage"
---| "UNIT_FIELD_POWER_COST_MODIFIER"
---| "Unit.PowerCostModifier"
---| "UNIT_FIELD_POWER_COST_MULTIPLIER"
---| "Unit.PowerCostMultiplier"
---| "UNIT_FIELD_MAXHEALTHMODIFIER"
---| "Unit.Maxhealthmodifier"
---| "UNIT_FIELD_HOVERHEIGHT"
---| "Unit.Hoverheight"
---| "UNIT_FIELD_PADDING"
---| "Unit.Padding"
---| "PLAYER_DUEL_ARBITER"
---| "Player.DuelArbiter"
---| "PLAYER_FLAGS"
---| "Player.Flags"
---| "PLAYER_GUILDID"
---| "Player.Guildid"
---| "PLAYER_GUILDRANK"
---| "Player.Guildrank"
---| "PLAYER_BYTES"
---| "Player.Bytes"
---| "PLAYER_BYTES_2"
---| "Player.Bytes2"
---| "PLAYER_BYTES_3"
---| "Player.Bytes3"
---| "PLAYER_DUEL_TEAM"
---| "Player.DuelTeam"
---| "PLAYER_GUILD_TIMESTAMP"
---| "Player.GuildTimestamp"
---| "PLAYER_QUEST_LOG_1_1"
---| "Player.QuestLog1_1"
---| "PLAYER_QUEST_LOG_1_2"
---| "Player.QuestLog1_2"
---| "PLAYER_QUEST_LOG_1_3"
---| "Player.QuestLog1_3"
---| "PLAYER_QUEST_LOG_1_4"
---| "Player.QuestLog1_4"
---| "PLAYER_QUEST_LOG_2_1"
---| "Player.QuestLog2_1"
---| "PLAYER_QUEST_LOG_2_2"
---| "Player.QuestLog2_2"
---| "PLAYER_QUEST_LOG_2_3"
---| "Player.QuestLog2_3"
---| "PLAYER_QUEST_LOG_2_5"
---| "Player.QuestLog2_5"
---| "PLAYER_QUEST_LOG_3_1"
---| "Player.QuestLog3_1"
---| "PLAYER_QUEST_LOG_3_2"
---| "Player.QuestLog3_2"
---| "PLAYER_QUEST_LOG_3_3"
---| "Player.QuestLog3_3"
---| "PLAYER_QUEST_LOG_3_5"
---| "Player.QuestLog3_5"
---| "PLAYER_QUEST_LOG_4_1"
---| "Player.QuestLog4_1"
---| "PLAYER_QUEST_LOG_4_2"
---| "Player.QuestLog4_2"
---| "PLAYER_QUEST_LOG_4_3"
---| "Player.QuestLog4_3"
---| "PLAYER_QUEST_LOG_4_5"
---| "Player.QuestLog4_5"
---| "PLAYER_QUEST_LOG_5_1"
---| "Player.QuestLog5_1"
---| "PLAYER_QUEST_LOG_5_2"
---| "Player.QuestLog5_2"
---| "PLAYER_QUEST_LOG_5_3"
---| "Player.QuestLog5_3"
---| "PLAYER_QUEST_LOG_5_5"
---| "Player.QuestLog5_5"
---| "PLAYER_QUEST_LOG_6_1"
---| "Player.QuestLog6_1"
---| "PLAYER_QUEST_LOG_6_2"
---| "Player.QuestLog6_2"
---| "PLAYER_QUEST_LOG_6_3"
---| "Player.QuestLog6_3"
---| "PLAYER_QUEST_LOG_6_5"
---| "Player.QuestLog6_5"
---| "PLAYER_QUEST_LOG_7_1"
---| "Player.QuestLog7_1"
---| "PLAYER_QUEST_LOG_7_2"
---| "Player.QuestLog7_2"
---| "PLAYER_QUEST_LOG_7_3"
---| "Player.QuestLog7_3"
---| "PLAYER_QUEST_LOG_7_5"
---| "Player.QuestLog7_5"
---| "PLAYER_QUEST_LOG_8_1"
---| "Player.QuestLog8_1"
---| "PLAYER_QUEST_LOG_8_2"
---| "Player.QuestLog8_2"
---| "PLAYER_QUEST_LOG_8_3"
---| "Player.QuestLog8_3"
---| "PLAYER_QUEST_LOG_8_5"
---| "Player.QuestLog8_5"
---| "PLAYER_QUEST_LOG_9_1"
---| "Player.QuestLog9_1"
---| "PLAYER_QUEST_LOG_9_2"
---| "Player.QuestLog9_2"
---| "PLAYER_QUEST_LOG_9_3"
---| "Player.QuestLog9_3"
---| "PLAYER_QUEST_LOG_9_5"
---| "Player.QuestLog9_5"
---| "PLAYER_QUEST_LOG_10_1"
---| "Player.QuestLog10_1"
---| "PLAYER_QUEST_LOG_10_2"
---| "Player.QuestLog10_2"
---| "PLAYER_QUEST_LOG_10_3"
---| "Player.QuestLog10_3"
---| "PLAYER_QUEST_LOG_10_5"
---| "Player.QuestLog10_5"
---| "PLAYER_QUEST_LOG_11_1"
---| "Player.QuestLog11_1"
---| "PLAYER_QUEST_LOG_11_2"
---| "Player.QuestLog11_2"
---| "PLAYER_QUEST_LOG_11_3"
---| "Player.QuestLog11_3"
---| "PLAYER_QUEST_LOG_11_5"
---| "Player.QuestLog11_5"
---| "PLAYER_QUEST_LOG_12_1"
---| "Player.QuestLog12_1"
---| "PLAYER_QUEST_LOG_12_2"
---| "Player.QuestLog12_2"
---| "PLAYER_QUEST_LOG_12_3"
---| "Player.QuestLog12_3"
---| "PLAYER_QUEST_LOG_12_5"
---| "Player.QuestLog12_5"
---| "PLAYER_QUEST_LOG_13_1"
---| "Player.QuestLog13_1"
---| "PLAYER_QUEST_LOG_13_2"
---| "Player.QuestLog13_2"
---| "PLAYER_QUEST_LOG_13_3"
---| "Player.QuestLog13_3"
---| "PLAYER_QUEST_LOG_13_5"
---| "Player.QuestLog13_5"
---| "PLAYER_QUEST_LOG_14_1"
---| "Player.QuestLog14_1"
---| "PLAYER_QUEST_LOG_14_2"
---| "Player.QuestLog14_2"
---| "PLAYER_QUEST_LOG_14_3"
---| "Player.QuestLog14_3"
---| "PLAYER_QUEST_LOG_14_5"
---| "Player.QuestLog14_5"
---| "PLAYER_QUEST_LOG_15_1"
---| "Player.QuestLog15_1"
---| "PLAYER_QUEST_LOG_15_2"
---| "Player.QuestLog15_2"
---| "PLAYER_QUEST_LOG_15_3"
---| "Player.QuestLog15_3"
---| "PLAYER_QUEST_LOG_15_5"
---| "Player.QuestLog15_5"
---| "PLAYER_QUEST_LOG_16_1"
---| "Player.QuestLog16_1"
---| "PLAYER_QUEST_LOG_16_2"
---| "Player.QuestLog16_2"
---| "PLAYER_QUEST_LOG_16_3"
---| "Player.QuestLog16_3"
---| "PLAYER_QUEST_LOG_16_5"
---| "Player.QuestLog16_5"
---| "PLAYER_QUEST_LOG_17_1"
---| "Player.QuestLog17_1"
---| "PLAYER_QUEST_LOG_17_2"
---| "Player.QuestLog17_2"
---| "PLAYER_QUEST_LOG_17_3"
---| "Player.QuestLog17_3"
---| "PLAYER_QUEST_LOG_17_5"
---| "Player.QuestLog17_5"
---| "PLAYER_QUEST_LOG_18_1"
---| "Player.QuestLog18_1"
---| "PLAYER_QUEST_LOG_18_2"
---| "Player.QuestLog18_2"
---| "PLAYER_QUEST_LOG_18_3"
---| "Player.QuestLog18_3"
---| "PLAYER_QUEST_LOG_18_5"
---| "Player.QuestLog18_5"
---| "PLAYER_QUEST_LOG_19_1"
---| "Player.QuestLog19_1"
---| "PLAYER_QUEST_LOG_19_2"
---| "Player.QuestLog19_2"
---| "PLAYER_QUEST_LOG_19_3"
---| "Player.QuestLog19_3"
---| "PLAYER_QUEST_LOG_19_5"
---| "Player.QuestLog19_5"
---| "PLAYER_QUEST_LOG_20_1"
---| "Player.QuestLog20_1"
---| "PLAYER_QUEST_LOG_20_2"
---| "Player.QuestLog20_2"
---| "PLAYER_QUEST_LOG_20_3"
---| "Player.QuestLog20_3"
---| "PLAYER_QUEST_LOG_20_5"
---| "Player.QuestLog20_5"
---| "PLAYER_QUEST_LOG_21_1"
---| "Player.QuestLog21_1"
---| "PLAYER_QUEST_LOG_21_2"
---| "Player.QuestLog21_2"
---| "PLAYER_QUEST_LOG_21_3"
---| "Player.QuestLog21_3"
---| "PLAYER_QUEST_LOG_21_5"
---| "Player.QuestLog21_5"
---| "PLAYER_QUEST_LOG_22_1"
---| "Player.QuestLog22_1"
---| "PLAYER_QUEST_LOG_22_2"
---| "Player.QuestLog22_2"
---| "PLAYER_QUEST_LOG_22_3"
---| "Player.QuestLog22_3"
---| "PLAYER_QUEST_LOG_22_5"
---| "Player.QuestLog22_5"
---| "PLAYER_QUEST_LOG_23_1"
---| "Player.QuestLog23_1"
---| "PLAYER_QUEST_LOG_23_2"
---| "Player.QuestLog23_2"
---| "PLAYER_QUEST_LOG_23_3"
---| "Player.QuestLog23_3"
---| "PLAYER_QUEST_LOG_23_5"
---| "Player.QuestLog23_5"
---| "PLAYER_QUEST_LOG_24_1"
---| "Player.QuestLog24_1"
---| "PLAYER_QUEST_LOG_24_2"
---| "Player.QuestLog24_2"
---| "PLAYER_QUEST_LOG_24_3"
---| "Player.QuestLog24_3"
---| "PLAYER_QUEST_LOG_24_5"
---| "Player.QuestLog24_5"
---| "PLAYER_QUEST_LOG_25_1"
---| "Player.QuestLog25_1"
---| "PLAYER_QUEST_LOG_25_2"
---| "Player.QuestLog25_2"
---| "PLAYER_QUEST_LOG_25_3"
---| "Player.QuestLog25_3"
---| "PLAYER_QUEST_LOG_25_5"
---| "Player.QuestLog25_5"
---| "PLAYER_VISIBLE_ITEM_1_ENTRYID"
---| "Player.VisibleItem1Entryid"
---| "PLAYER_VISIBLE_ITEM_1_ENCHANTMENT"
---| "Player.VisibleItem1Enchantment"
---| "PLAYER_VISIBLE_ITEM_2_ENTRYID"
---| "Player.VisibleItem2Entryid"
---| "PLAYER_VISIBLE_ITEM_2_ENCHANTMENT"
---| "Player.VisibleItem2Enchantment"
---| "PLAYER_VISIBLE_ITEM_3_ENTRYID"
---| "Player.VisibleItem3Entryid"
---| "PLAYER_VISIBLE_ITEM_3_ENCHANTMENT"
---| "Player.VisibleItem3Enchantment"
---| "PLAYER_VISIBLE_ITEM_4_ENTRYID"
---| "Player.VisibleItem4Entryid"
---| "PLAYER_VISIBLE_ITEM_4_ENCHANTMENT"
---| "Player.VisibleItem4Enchantment"
---| "PLAYER_VISIBLE_ITEM_5_ENTRYID"
---| "Player.VisibleItem5Entryid"
---| "PLAYER_VISIBLE_ITEM_5_ENCHANTMENT"
---| "Player.VisibleItem5Enchantment"
---| "PLAYER_VISIBLE_ITEM_6_ENTRYID"
---| "Player.VisibleItem6Entryid"
---| "PLAYER_VISIBLE_ITEM_6_ENCHANTMENT"
---| "Player.VisibleItem6Enchantment"
---| "PLAYER_VISIBLE_ITEM_7_ENTRYID"
---| "Player.VisibleItem7Entryid"
---| "PLAYER_VISIBLE_ITEM_7_ENCHANTMENT"
---| "Player.VisibleItem7Enchantment"
---| "PLAYER_VISIBLE_ITEM_8_ENTRYID"
---| "Player.VisibleItem8Entryid"
---| "PLAYER_VISIBLE_ITEM_8_ENCHANTMENT"
---| "Player.VisibleItem8Enchantment"
---| "PLAYER_VISIBLE_ITEM_9_ENTRYID"
---| "Player.VisibleItem9Entryid"
---| "PLAYER_VISIBLE_ITEM_9_ENCHANTMENT"
---| "Player.VisibleItem9Enchantment"
---| "PLAYER_VISIBLE_ITEM_10_ENTRYID"
---| "Player.VisibleItem10Entryid"
---| "PLAYER_VISIBLE_ITEM_10_ENCHANTMENT"
---| "Player.VisibleItem10Enchantment"
---| "PLAYER_VISIBLE_ITEM_11_ENTRYID"
---| "Player.VisibleItem11Entryid"
---| "PLAYER_VISIBLE_ITEM_11_ENCHANTMENT"
---| "Player.VisibleItem11Enchantment"
---| "PLAYER_VISIBLE_ITEM_12_ENTRYID"
---| "Player.VisibleItem12Entryid"
---| "PLAYER_VISIBLE_ITEM_12_ENCHANTMENT"
---| "Player.VisibleItem12Enchantment"
---| "PLAYER_VISIBLE_ITEM_13_ENTRYID"
---| "Player.VisibleItem13Entryid"
---| "PLAYER_VISIBLE_ITEM_13_ENCHANTMENT"
---| "Player.VisibleItem13Enchantment"
---| "PLAYER_VISIBLE_ITEM_14_ENTRYID"
---| "Player.VisibleItem14Entryid"
---| "PLAYER_VISIBLE_ITEM_14_ENCHANTMENT"
---| "Player.VisibleItem14Enchantment"
---| "PLAYER_VISIBLE_ITEM_15_ENTRYID"
---| "Player.VisibleItem15Entryid"
---| "PLAYER_VISIBLE_ITEM_15_ENCHANTMENT"
---| "Player.VisibleItem15Enchantment"
---| "PLAYER_VISIBLE_ITEM_16_ENTRYID"
---| "Player.VisibleItem16Entryid"
---| "PLAYER_VISIBLE_ITEM_16_ENCHANTMENT"
---| "Player.VisibleItem16Enchantment"
---| "PLAYER_VISIBLE_ITEM_17_ENTRYID"
---| "Player.VisibleItem17Entryid"
---| "PLAYER_VISIBLE_ITEM_17_ENCHANTMENT"
---| "Player.VisibleItem17Enchantment"
---| "PLAYER_VISIBLE_ITEM_18_ENTRYID"
---| "Player.VisibleItem18Entryid"
---| "PLAYER_VISIBLE_ITEM_18_ENCHANTMENT"
---| "Player.VisibleItem18Enchantment"
---| "PLAYER_VISIBLE_ITEM_19_ENTRYID"
---| "Player.VisibleItem19Entryid"
---| "PLAYER_VISIBLE_ITEM_19_ENCHANTMENT"
---| "Player.VisibleItem19Enchantment"
---| "PLAYER_CHOSEN_TITLE"
---| "Player.ChosenTitle"
---| "PLAYER_FAKE_INEBRIATION"
---| "Player.FakeInebriation"
---| "PLAYER_FIELD_PAD_0"
---| "Player.Pad0"
---| "PLAYER_FIELD_INV_SLOT_HEAD"
---| "Player.InvSlotHead"
---| "PLAYER_FIELD_PACK_SLOT_1"
---| "Player.PackSlot1"
---| "PLAYER_FIELD_BANK_SLOT_1"
---| "Player.BankSlot1"
---| "PLAYER_FIELD_BANKBAG_SLOT_1"
---| "Player.BankbagSlot1"
---| "PLAYER_FIELD_VENDORBUYBACK_SLOT_1"
---| "Player.VendorbuybackSlot1"
---| "PLAYER_FIELD_KEYRING_SLOT_1"
---| "Player.KeyringSlot1"
---| "PLAYER_FIELD_CURRENCYTOKEN_SLOT_1"
---| "Player.CurrencytokenSlot1"
---| "PLAYER_FARSIGHT"
---| "Player.Farsight"
---| "PLAYER__FIELD_KNOWN_TITLES"
---| "Player.KnownTitles"
---| "PLAYER__FIELD_KNOWN_TITLES1"
---| "Player.KnownTitles1"
---| "PLAYER__FIELD_KNOWN_TITLES2"
---| "Player.KnownTitles2"
---| "PLAYER_FIELD_KNOWN_CURRENCIES"
---| "Player.KnownCurrencies"
---| "PLAYER_XP"
---| "Player.Xp"
---| "PLAYER_NEXT_LEVEL_XP"
---| "Player.NextLevelXp"
---| "PLAYER_SKILL_INFO_1_1"
---| "Player.SkillInfo1_1"
---| "PLAYER_CHARACTER_POINTS1"
---| "Player.CharacterPoints1"
---| "PLAYER_CHARACTER_POINTS2"
---| "Player.CharacterPoints2"
---| "PLAYER_TRACK_CREATURES"
---| "Player.TrackCreatures"
---| "PLAYER_TRACK_RESOURCES"
---| "Player.TrackResources"
---| "PLAYER_BLOCK_PERCENTAGE"
---| "Player.BlockPercentage"
---| "PLAYER_DODGE_PERCENTAGE"
---| "Player.DodgePercentage"
---| "PLAYER_PARRY_PERCENTAGE"
---| "Player.ParryPercentage"
---| "PLAYER_EXPERTISE"
---| "Player.Expertise"
---| "PLAYER_OFFHAND_EXPERTISE"
---| "Player.OffhandExpertise"
---| "PLAYER_CRIT_PERCENTAGE"
---| "Player.CritPercentage"
---| "PLAYER_RANGED_CRIT_PERCENTAGE"
---| "Player.RangedCritPercentage"
---| "PLAYER_OFFHAND_CRIT_PERCENTAGE"
---| "Player.OffhandCritPercentage"
---| "PLAYER_SPELL_CRIT_PERCENTAGE1"
---| "Player.SpellCritPercentage1"
---| "PLAYER_SHIELD_BLOCK"
---| "Player.ShieldBlock"
---| "PLAYER_SHIELD_BLOCK_CRIT_PERCENTAGE"
---| "Player.ShieldBlockCritPercentage"
---| "PLAYER_EXPLORED_ZONES_1"
---| "Player.ExploredZones1"
---| "PLAYER_REST_STATE_EXPERIENCE"
---| "Player.RestStateExperience"
---| "PLAYER_FIELD_COINAGE"
---| "Player.Coinage"
---| "PLAYER_FIELD_MOD_DAMAGE_DONE_POS"
---| "Player.ModDamageDonePos"
---| "PLAYER_FIELD_MOD_DAMAGE_DONE_NEG"
---| "Player.ModDamageDoneNeg"
---| "PLAYER_FIELD_MOD_DAMAGE_DONE_PCT"
---| "Player.ModDamageDonePct"
---| "PLAYER_FIELD_MOD_HEALING_DONE_POS"
---| "Player.ModHealingDonePos"
---| "PLAYER_FIELD_MOD_HEALING_PCT"
---| "Player.ModHealingPct"
---| "PLAYER_FIELD_MOD_HEALING_DONE_PCT"
---| "Player.ModHealingDonePct"
---| "PLAYER_FIELD_MOD_TARGET_RESISTANCE"
---| "Player.ModTargetResistance"
---| "PLAYER_FIELD_MOD_TARGET_PHYSICAL_RESISTANCE"
---| "Player.ModTargetPhysicalResistance"
---| "PLAYER_FIELD_BYTES"
---| "Player.FieldBytes"
---| "PLAYER_AMMO_ID"
---| "Player.AmmoId"
---| "PLAYER_SELF_RES_SPELL"
---| "Player.SelfResSpell"
---| "PLAYER_FIELD_PVP_MEDALS"
---| "Player.PvpMedals"
---| "PLAYER_FIELD_BUYBACK_PRICE_1"
---| "Player.BuybackPrice1"
---| "PLAYER_FIELD_BUYBACK_TIMESTAMP_1"
---| "Player.BuybackTimestamp1"
---| "PLAYER_FIELD_KILLS"
---| "Player.Kills"
---| "PLAYER_FIELD_TODAY_CONTRIBUTION"
---| "Player.TodayContribution"
---| "PLAYER_FIELD_YESTERDAY_CONTRIBUTION"
---| "Player.YesterdayContribution"
---| "PLAYER_FIELD_LIFETIME_HONORBALE_KILLS"
---| "Player.LifetimeHonorbaleKills"
---| "PLAYER_FIELD_BYTES2"
---| "Player.FieldBytes2"
---| "PLAYER_FIELD_WATCHED_FACTION_INDEX"
---| "Player.WatchedFactionIndex"
---| "PLAYER_FIELD_COMBAT_RATING_1"
---| "Player.CombatRating1"
---| "PLAYER_FIELD_ARENA_TEAM_INFO_1_1"
---| "Player.ArenaTeamInfo1_1"
---| "PLAYER_FIELD_HONOR_CURRENCY"
---| "Player.HonorCurrency"
---| "PLAYER_FIELD_ARENA_CURRENCY"
---| "Player.ArenaCurrency"
---| "PLAYER_FIELD_MAX_LEVEL"
---| "Player.MaxLevel"
---| "PLAYER_FIELD_DAILY_QUESTS_1"
---| "Player.DailyQuests1"
---| "PLAYER_RUNE_REGEN_1"
---| "Player.RuneRegen1"
---| "PLAYER_NO_REAGENT_COST_1"
---| "Player.NoReagentCost1"
---| "PLAYER_FIELD_GLYPH_SLOTS_1"
---| "Player.GlyphSlots1"
---| "PLAYER_FIELD_GLYPHS_1"
---| "Player.Glyphs1"
---| "PLAYER_GLYPHS_ENABLED"
---| "Player.GlyphsEnabled"
---| "PLAYER_PET_SPELL_POWER"
---| "Player.PetSpellPower"
---| "OBJECT_FIELD_CREATED_BY"
---| "GameObject.CreatedBy"
---| "GAMEOBJECT_DISPLAYID"
---| "GameObject.Displayid"
---| "GAMEOBJECT_FLAGS"
---| "GameObject.Flags"
---| "GAMEOBJECT_PARENTROTATION"
---| "GameObject.Parentrotation"
---| "GAMEOBJECT_DYNAMIC"
---| "GameObject.Dynamic"
---| "GAMEOBJECT_FACTION"
---| "GameObject.Faction"
---| "GAMEOBJECT_LEVEL"
---| "GameObject.Level"
---| "GAMEOBJECT_BYTES_1"
---| "GameObject.Bytes1"
---| "DYNAMICOBJECT_CASTER"
---| "DynamicObject.Caster"
---| "DYNAMICOBJECT_BYTES"
---| "DynamicObject.Bytes"
---| "DYNAMICOBJECT_SPELLID"
---| "DynamicObject.Spellid"
---| "DYNAMICOBJECT_RADIUS"
---| "DynamicObject.Radius"
---| "DYNAMICOBJECT_CASTTIME"
---| "DynamicObject.Casttime"
---| "CORPSE_FIELD_OWNER"
---| "Corpse.Owner"
---| "CORPSE_FIELD_PARTY"
---| "Corpse.Party"
---| "CORPSE_FIELD_DISPLAY_ID"
---| "Corpse.DisplayId"
---| "CORPSE_FIELD_ITEM"
---| "Corpse.Item"
---| "CORPSE_FIELD_BYTES_1"
---| "Corpse.Bytes1"
---| "CORPSE_FIELD_BYTES_2"
---| "Corpse.Bytes2"
---| "CORPSE_FIELD_GUILD"
---| "Corpse.Guild"
---| "CORPSE_FIELD_FLAGS"
---| "Corpse.Flags"
---| "CORPSE_FIELD_DYNAMIC_FLAGS"
---| "Corpse.DynamicFlags"
---| "CORPSE_FIELD_PAD"
---| "Corpse.Pad"

---The dword index of every field of a Object.
---@class wxl.field.Object
---@field Guid integer OBJECT_FIELD_GUID
---@field Type integer OBJECT_FIELD_TYPE
---@field Entry integer OBJECT_FIELD_ENTRY
---@field ScaleX integer OBJECT_FIELD_SCALE_X
---@field Padding integer OBJECT_FIELD_PADDING

---The dword index of every field of a Item.
---@class wxl.field.Item
---@field Owner integer ITEM_FIELD_OWNER
---@field Contained integer ITEM_FIELD_CONTAINED
---@field Creator integer ITEM_FIELD_CREATOR
---@field Giftcreator integer ITEM_FIELD_GIFTCREATOR
---@field StackCount integer ITEM_FIELD_STACK_COUNT
---@field Duration integer ITEM_FIELD_DURATION
---@field SpellCharges integer ITEM_FIELD_SPELL_CHARGES
---@field Flags integer ITEM_FIELD_FLAGS
---@field Enchantment1_1 integer ITEM_FIELD_ENCHANTMENT_1_1
---@field Enchantment1_3 integer ITEM_FIELD_ENCHANTMENT_1_3
---@field Enchantment2_1 integer ITEM_FIELD_ENCHANTMENT_2_1
---@field Enchantment2_3 integer ITEM_FIELD_ENCHANTMENT_2_3
---@field Enchantment3_1 integer ITEM_FIELD_ENCHANTMENT_3_1
---@field Enchantment3_3 integer ITEM_FIELD_ENCHANTMENT_3_3
---@field Enchantment4_1 integer ITEM_FIELD_ENCHANTMENT_4_1
---@field Enchantment4_3 integer ITEM_FIELD_ENCHANTMENT_4_3
---@field Enchantment5_1 integer ITEM_FIELD_ENCHANTMENT_5_1
---@field Enchantment5_3 integer ITEM_FIELD_ENCHANTMENT_5_3
---@field Enchantment6_1 integer ITEM_FIELD_ENCHANTMENT_6_1
---@field Enchantment6_3 integer ITEM_FIELD_ENCHANTMENT_6_3
---@field Enchantment7_1 integer ITEM_FIELD_ENCHANTMENT_7_1
---@field Enchantment7_3 integer ITEM_FIELD_ENCHANTMENT_7_3
---@field Enchantment8_1 integer ITEM_FIELD_ENCHANTMENT_8_1
---@field Enchantment8_3 integer ITEM_FIELD_ENCHANTMENT_8_3
---@field Enchantment9_1 integer ITEM_FIELD_ENCHANTMENT_9_1
---@field Enchantment9_3 integer ITEM_FIELD_ENCHANTMENT_9_3
---@field Enchantment10_1 integer ITEM_FIELD_ENCHANTMENT_10_1
---@field Enchantment10_3 integer ITEM_FIELD_ENCHANTMENT_10_3
---@field Enchantment11_1 integer ITEM_FIELD_ENCHANTMENT_11_1
---@field Enchantment11_3 integer ITEM_FIELD_ENCHANTMENT_11_3
---@field Enchantment12_1 integer ITEM_FIELD_ENCHANTMENT_12_1
---@field Enchantment12_3 integer ITEM_FIELD_ENCHANTMENT_12_3
---@field PropertySeed integer ITEM_FIELD_PROPERTY_SEED
---@field RandomPropertiesId integer ITEM_FIELD_RANDOM_PROPERTIES_ID
---@field Durability integer ITEM_FIELD_DURABILITY
---@field Maxdurability integer ITEM_FIELD_MAXDURABILITY
---@field CreatePlayedTime integer ITEM_FIELD_CREATE_PLAYED_TIME
---@field Pad integer ITEM_FIELD_PAD

---The dword index of every field of a Container.
---@class wxl.field.Container
---@field NumSlots integer CONTAINER_FIELD_NUM_SLOTS
---@field AlignPad integer CONTAINER_ALIGN_PAD
---@field Slot1 integer CONTAINER_FIELD_SLOT_1

---The dword index of every field of a Unit.
---@class wxl.field.Unit
---@field Charm integer UNIT_FIELD_CHARM
---@field Summon integer UNIT_FIELD_SUMMON
---@field Critter integer UNIT_FIELD_CRITTER
---@field Charmedby integer UNIT_FIELD_CHARMEDBY
---@field Summonedby integer UNIT_FIELD_SUMMONEDBY
---@field Createdby integer UNIT_FIELD_CREATEDBY
---@field Target integer UNIT_FIELD_TARGET
---@field ChannelObject integer UNIT_FIELD_CHANNEL_OBJECT
---@field ChannelSpell integer UNIT_CHANNEL_SPELL
---@field Bytes0 integer UNIT_FIELD_BYTES_0
---@field Health integer UNIT_FIELD_HEALTH
---@field Power1 integer UNIT_FIELD_POWER1
---@field Power2 integer UNIT_FIELD_POWER2
---@field Power3 integer UNIT_FIELD_POWER3
---@field Power4 integer UNIT_FIELD_POWER4
---@field Power5 integer UNIT_FIELD_POWER5
---@field Power6 integer UNIT_FIELD_POWER6
---@field Power7 integer UNIT_FIELD_POWER7
---@field Maxhealth integer UNIT_FIELD_MAXHEALTH
---@field Maxpower1 integer UNIT_FIELD_MAXPOWER1
---@field Maxpower2 integer UNIT_FIELD_MAXPOWER2
---@field Maxpower3 integer UNIT_FIELD_MAXPOWER3
---@field Maxpower4 integer UNIT_FIELD_MAXPOWER4
---@field Maxpower5 integer UNIT_FIELD_MAXPOWER5
---@field Maxpower6 integer UNIT_FIELD_MAXPOWER6
---@field Maxpower7 integer UNIT_FIELD_MAXPOWER7
---@field PowerRegenFlatModifier integer UNIT_FIELD_POWER_REGEN_FLAT_MODIFIER
---@field PowerRegenInterruptedFlatModifier integer UNIT_FIELD_POWER_REGEN_INTERRUPTED_FLAT_MODIFIER
---@field Level integer UNIT_FIELD_LEVEL
---@field Factiontemplate integer UNIT_FIELD_FACTIONTEMPLATE
---@field VirtualItemSlotId integer UNIT_VIRTUAL_ITEM_SLOT_ID
---@field Flags integer UNIT_FIELD_FLAGS
---@field Flags2 integer UNIT_FIELD_FLAGS_2
---@field Aurastate integer UNIT_FIELD_AURASTATE
---@field Baseattacktime integer UNIT_FIELD_BASEATTACKTIME
---@field Rangedattacktime integer UNIT_FIELD_RANGEDATTACKTIME
---@field Boundingradius integer UNIT_FIELD_BOUNDINGRADIUS
---@field Combatreach integer UNIT_FIELD_COMBATREACH
---@field Displayid integer UNIT_FIELD_DISPLAYID
---@field Nativedisplayid integer UNIT_FIELD_NATIVEDISPLAYID
---@field Mountdisplayid integer UNIT_FIELD_MOUNTDISPLAYID
---@field Mindamage integer UNIT_FIELD_MINDAMAGE
---@field Maxdamage integer UNIT_FIELD_MAXDAMAGE
---@field Minoffhanddamage integer UNIT_FIELD_MINOFFHANDDAMAGE
---@field Maxoffhanddamage integer UNIT_FIELD_MAXOFFHANDDAMAGE
---@field Bytes1 integer UNIT_FIELD_BYTES_1
---@field Petnumber integer UNIT_FIELD_PETNUMBER
---@field PetNameTimestamp integer UNIT_FIELD_PET_NAME_TIMESTAMP
---@field Petexperience integer UNIT_FIELD_PETEXPERIENCE
---@field Petnextlevelexp integer UNIT_FIELD_PETNEXTLEVELEXP
---@field DynamicFlags integer UNIT_DYNAMIC_FLAGS
---@field ModCastSpeed integer UNIT_MOD_CAST_SPEED
---@field CreatedBySpell integer UNIT_CREATED_BY_SPELL
---@field NpcFlags integer UNIT_NPC_FLAGS
---@field NpcEmotestate integer UNIT_NPC_EMOTESTATE
---@field Stat0 integer UNIT_FIELD_STAT0
---@field Stat1 integer UNIT_FIELD_STAT1
---@field Stat2 integer UNIT_FIELD_STAT2
---@field Stat3 integer UNIT_FIELD_STAT3
---@field Stat4 integer UNIT_FIELD_STAT4
---@field Posstat0 integer UNIT_FIELD_POSSTAT0
---@field Posstat1 integer UNIT_FIELD_POSSTAT1
---@field Posstat2 integer UNIT_FIELD_POSSTAT2
---@field Posstat3 integer UNIT_FIELD_POSSTAT3
---@field Posstat4 integer UNIT_FIELD_POSSTAT4
---@field Negstat0 integer UNIT_FIELD_NEGSTAT0
---@field Negstat1 integer UNIT_FIELD_NEGSTAT1
---@field Negstat2 integer UNIT_FIELD_NEGSTAT2
---@field Negstat3 integer UNIT_FIELD_NEGSTAT3
---@field Negstat4 integer UNIT_FIELD_NEGSTAT4
---@field Resistances integer UNIT_FIELD_RESISTANCES
---@field Resistancebuffmodspositive integer UNIT_FIELD_RESISTANCEBUFFMODSPOSITIVE
---@field Resistancebuffmodsnegative integer UNIT_FIELD_RESISTANCEBUFFMODSNEGATIVE
---@field BaseMana integer UNIT_FIELD_BASE_MANA
---@field BaseHealth integer UNIT_FIELD_BASE_HEALTH
---@field Bytes2 integer UNIT_FIELD_BYTES_2
---@field AttackPower integer UNIT_FIELD_ATTACK_POWER
---@field AttackPowerMods integer UNIT_FIELD_ATTACK_POWER_MODS
---@field AttackPowerMultiplier integer UNIT_FIELD_ATTACK_POWER_MULTIPLIER
---@field RangedAttackPower integer UNIT_FIELD_RANGED_ATTACK_POWER
---@field RangedAttackPowerMods integer UNIT_FIELD_RANGED_ATTACK_POWER_MODS
---@field RangedAttackPowerMultiplier integer UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER
---@field Minrangeddamage integer UNIT_FIELD_MINRANGEDDAMAGE
---@field Maxrangeddamage integer UNIT_FIELD_MAXRANGEDDAMAGE
---@field PowerCostModifier integer UNIT_FIELD_POWER_COST_MODIFIER
---@field PowerCostMultiplier integer UNIT_FIELD_POWER_COST_MULTIPLIER
---@field Maxhealthmodifier integer UNIT_FIELD_MAXHEALTHMODIFIER
---@field Hoverheight integer UNIT_FIELD_HOVERHEIGHT
---@field Padding integer UNIT_FIELD_PADDING

---The dword index of every field of a Player.
---@class wxl.field.Player
---@field DuelArbiter integer PLAYER_DUEL_ARBITER
---@field Flags integer PLAYER_FLAGS
---@field Guildid integer PLAYER_GUILDID
---@field Guildrank integer PLAYER_GUILDRANK
---@field Bytes integer PLAYER_BYTES
---@field Bytes2 integer PLAYER_BYTES_2
---@field Bytes3 integer PLAYER_BYTES_3
---@field DuelTeam integer PLAYER_DUEL_TEAM
---@field GuildTimestamp integer PLAYER_GUILD_TIMESTAMP
---@field QuestLog1_1 integer PLAYER_QUEST_LOG_1_1
---@field QuestLog1_2 integer PLAYER_QUEST_LOG_1_2
---@field QuestLog1_3 integer PLAYER_QUEST_LOG_1_3
---@field QuestLog1_4 integer PLAYER_QUEST_LOG_1_4
---@field QuestLog2_1 integer PLAYER_QUEST_LOG_2_1
---@field QuestLog2_2 integer PLAYER_QUEST_LOG_2_2
---@field QuestLog2_3 integer PLAYER_QUEST_LOG_2_3
---@field QuestLog2_5 integer PLAYER_QUEST_LOG_2_5
---@field QuestLog3_1 integer PLAYER_QUEST_LOG_3_1
---@field QuestLog3_2 integer PLAYER_QUEST_LOG_3_2
---@field QuestLog3_3 integer PLAYER_QUEST_LOG_3_3
---@field QuestLog3_5 integer PLAYER_QUEST_LOG_3_5
---@field QuestLog4_1 integer PLAYER_QUEST_LOG_4_1
---@field QuestLog4_2 integer PLAYER_QUEST_LOG_4_2
---@field QuestLog4_3 integer PLAYER_QUEST_LOG_4_3
---@field QuestLog4_5 integer PLAYER_QUEST_LOG_4_5
---@field QuestLog5_1 integer PLAYER_QUEST_LOG_5_1
---@field QuestLog5_2 integer PLAYER_QUEST_LOG_5_2
---@field QuestLog5_3 integer PLAYER_QUEST_LOG_5_3
---@field QuestLog5_5 integer PLAYER_QUEST_LOG_5_5
---@field QuestLog6_1 integer PLAYER_QUEST_LOG_6_1
---@field QuestLog6_2 integer PLAYER_QUEST_LOG_6_2
---@field QuestLog6_3 integer PLAYER_QUEST_LOG_6_3
---@field QuestLog6_5 integer PLAYER_QUEST_LOG_6_5
---@field QuestLog7_1 integer PLAYER_QUEST_LOG_7_1
---@field QuestLog7_2 integer PLAYER_QUEST_LOG_7_2
---@field QuestLog7_3 integer PLAYER_QUEST_LOG_7_3
---@field QuestLog7_5 integer PLAYER_QUEST_LOG_7_5
---@field QuestLog8_1 integer PLAYER_QUEST_LOG_8_1
---@field QuestLog8_2 integer PLAYER_QUEST_LOG_8_2
---@field QuestLog8_3 integer PLAYER_QUEST_LOG_8_3
---@field QuestLog8_5 integer PLAYER_QUEST_LOG_8_5
---@field QuestLog9_1 integer PLAYER_QUEST_LOG_9_1
---@field QuestLog9_2 integer PLAYER_QUEST_LOG_9_2
---@field QuestLog9_3 integer PLAYER_QUEST_LOG_9_3
---@field QuestLog9_5 integer PLAYER_QUEST_LOG_9_5
---@field QuestLog10_1 integer PLAYER_QUEST_LOG_10_1
---@field QuestLog10_2 integer PLAYER_QUEST_LOG_10_2
---@field QuestLog10_3 integer PLAYER_QUEST_LOG_10_3
---@field QuestLog10_5 integer PLAYER_QUEST_LOG_10_5
---@field QuestLog11_1 integer PLAYER_QUEST_LOG_11_1
---@field QuestLog11_2 integer PLAYER_QUEST_LOG_11_2
---@field QuestLog11_3 integer PLAYER_QUEST_LOG_11_3
---@field QuestLog11_5 integer PLAYER_QUEST_LOG_11_5
---@field QuestLog12_1 integer PLAYER_QUEST_LOG_12_1
---@field QuestLog12_2 integer PLAYER_QUEST_LOG_12_2
---@field QuestLog12_3 integer PLAYER_QUEST_LOG_12_3
---@field QuestLog12_5 integer PLAYER_QUEST_LOG_12_5
---@field QuestLog13_1 integer PLAYER_QUEST_LOG_13_1
---@field QuestLog13_2 integer PLAYER_QUEST_LOG_13_2
---@field QuestLog13_3 integer PLAYER_QUEST_LOG_13_3
---@field QuestLog13_5 integer PLAYER_QUEST_LOG_13_5
---@field QuestLog14_1 integer PLAYER_QUEST_LOG_14_1
---@field QuestLog14_2 integer PLAYER_QUEST_LOG_14_2
---@field QuestLog14_3 integer PLAYER_QUEST_LOG_14_3
---@field QuestLog14_5 integer PLAYER_QUEST_LOG_14_5
---@field QuestLog15_1 integer PLAYER_QUEST_LOG_15_1
---@field QuestLog15_2 integer PLAYER_QUEST_LOG_15_2
---@field QuestLog15_3 integer PLAYER_QUEST_LOG_15_3
---@field QuestLog15_5 integer PLAYER_QUEST_LOG_15_5
---@field QuestLog16_1 integer PLAYER_QUEST_LOG_16_1
---@field QuestLog16_2 integer PLAYER_QUEST_LOG_16_2
---@field QuestLog16_3 integer PLAYER_QUEST_LOG_16_3
---@field QuestLog16_5 integer PLAYER_QUEST_LOG_16_5
---@field QuestLog17_1 integer PLAYER_QUEST_LOG_17_1
---@field QuestLog17_2 integer PLAYER_QUEST_LOG_17_2
---@field QuestLog17_3 integer PLAYER_QUEST_LOG_17_3
---@field QuestLog17_5 integer PLAYER_QUEST_LOG_17_5
---@field QuestLog18_1 integer PLAYER_QUEST_LOG_18_1
---@field QuestLog18_2 integer PLAYER_QUEST_LOG_18_2
---@field QuestLog18_3 integer PLAYER_QUEST_LOG_18_3
---@field QuestLog18_5 integer PLAYER_QUEST_LOG_18_5
---@field QuestLog19_1 integer PLAYER_QUEST_LOG_19_1
---@field QuestLog19_2 integer PLAYER_QUEST_LOG_19_2
---@field QuestLog19_3 integer PLAYER_QUEST_LOG_19_3
---@field QuestLog19_5 integer PLAYER_QUEST_LOG_19_5
---@field QuestLog20_1 integer PLAYER_QUEST_LOG_20_1
---@field QuestLog20_2 integer PLAYER_QUEST_LOG_20_2
---@field QuestLog20_3 integer PLAYER_QUEST_LOG_20_3
---@field QuestLog20_5 integer PLAYER_QUEST_LOG_20_5
---@field QuestLog21_1 integer PLAYER_QUEST_LOG_21_1
---@field QuestLog21_2 integer PLAYER_QUEST_LOG_21_2
---@field QuestLog21_3 integer PLAYER_QUEST_LOG_21_3
---@field QuestLog21_5 integer PLAYER_QUEST_LOG_21_5
---@field QuestLog22_1 integer PLAYER_QUEST_LOG_22_1
---@field QuestLog22_2 integer PLAYER_QUEST_LOG_22_2
---@field QuestLog22_3 integer PLAYER_QUEST_LOG_22_3
---@field QuestLog22_5 integer PLAYER_QUEST_LOG_22_5
---@field QuestLog23_1 integer PLAYER_QUEST_LOG_23_1
---@field QuestLog23_2 integer PLAYER_QUEST_LOG_23_2
---@field QuestLog23_3 integer PLAYER_QUEST_LOG_23_3
---@field QuestLog23_5 integer PLAYER_QUEST_LOG_23_5
---@field QuestLog24_1 integer PLAYER_QUEST_LOG_24_1
---@field QuestLog24_2 integer PLAYER_QUEST_LOG_24_2
---@field QuestLog24_3 integer PLAYER_QUEST_LOG_24_3
---@field QuestLog24_5 integer PLAYER_QUEST_LOG_24_5
---@field QuestLog25_1 integer PLAYER_QUEST_LOG_25_1
---@field QuestLog25_2 integer PLAYER_QUEST_LOG_25_2
---@field QuestLog25_3 integer PLAYER_QUEST_LOG_25_3
---@field QuestLog25_5 integer PLAYER_QUEST_LOG_25_5
---@field VisibleItem1Entryid integer PLAYER_VISIBLE_ITEM_1_ENTRYID
---@field VisibleItem1Enchantment integer PLAYER_VISIBLE_ITEM_1_ENCHANTMENT
---@field VisibleItem2Entryid integer PLAYER_VISIBLE_ITEM_2_ENTRYID
---@field VisibleItem2Enchantment integer PLAYER_VISIBLE_ITEM_2_ENCHANTMENT
---@field VisibleItem3Entryid integer PLAYER_VISIBLE_ITEM_3_ENTRYID
---@field VisibleItem3Enchantment integer PLAYER_VISIBLE_ITEM_3_ENCHANTMENT
---@field VisibleItem4Entryid integer PLAYER_VISIBLE_ITEM_4_ENTRYID
---@field VisibleItem4Enchantment integer PLAYER_VISIBLE_ITEM_4_ENCHANTMENT
---@field VisibleItem5Entryid integer PLAYER_VISIBLE_ITEM_5_ENTRYID
---@field VisibleItem5Enchantment integer PLAYER_VISIBLE_ITEM_5_ENCHANTMENT
---@field VisibleItem6Entryid integer PLAYER_VISIBLE_ITEM_6_ENTRYID
---@field VisibleItem6Enchantment integer PLAYER_VISIBLE_ITEM_6_ENCHANTMENT
---@field VisibleItem7Entryid integer PLAYER_VISIBLE_ITEM_7_ENTRYID
---@field VisibleItem7Enchantment integer PLAYER_VISIBLE_ITEM_7_ENCHANTMENT
---@field VisibleItem8Entryid integer PLAYER_VISIBLE_ITEM_8_ENTRYID
---@field VisibleItem8Enchantment integer PLAYER_VISIBLE_ITEM_8_ENCHANTMENT
---@field VisibleItem9Entryid integer PLAYER_VISIBLE_ITEM_9_ENTRYID
---@field VisibleItem9Enchantment integer PLAYER_VISIBLE_ITEM_9_ENCHANTMENT
---@field VisibleItem10Entryid integer PLAYER_VISIBLE_ITEM_10_ENTRYID
---@field VisibleItem10Enchantment integer PLAYER_VISIBLE_ITEM_10_ENCHANTMENT
---@field VisibleItem11Entryid integer PLAYER_VISIBLE_ITEM_11_ENTRYID
---@field VisibleItem11Enchantment integer PLAYER_VISIBLE_ITEM_11_ENCHANTMENT
---@field VisibleItem12Entryid integer PLAYER_VISIBLE_ITEM_12_ENTRYID
---@field VisibleItem12Enchantment integer PLAYER_VISIBLE_ITEM_12_ENCHANTMENT
---@field VisibleItem13Entryid integer PLAYER_VISIBLE_ITEM_13_ENTRYID
---@field VisibleItem13Enchantment integer PLAYER_VISIBLE_ITEM_13_ENCHANTMENT
---@field VisibleItem14Entryid integer PLAYER_VISIBLE_ITEM_14_ENTRYID
---@field VisibleItem14Enchantment integer PLAYER_VISIBLE_ITEM_14_ENCHANTMENT
---@field VisibleItem15Entryid integer PLAYER_VISIBLE_ITEM_15_ENTRYID
---@field VisibleItem15Enchantment integer PLAYER_VISIBLE_ITEM_15_ENCHANTMENT
---@field VisibleItem16Entryid integer PLAYER_VISIBLE_ITEM_16_ENTRYID
---@field VisibleItem16Enchantment integer PLAYER_VISIBLE_ITEM_16_ENCHANTMENT
---@field VisibleItem17Entryid integer PLAYER_VISIBLE_ITEM_17_ENTRYID
---@field VisibleItem17Enchantment integer PLAYER_VISIBLE_ITEM_17_ENCHANTMENT
---@field VisibleItem18Entryid integer PLAYER_VISIBLE_ITEM_18_ENTRYID
---@field VisibleItem18Enchantment integer PLAYER_VISIBLE_ITEM_18_ENCHANTMENT
---@field VisibleItem19Entryid integer PLAYER_VISIBLE_ITEM_19_ENTRYID
---@field VisibleItem19Enchantment integer PLAYER_VISIBLE_ITEM_19_ENCHANTMENT
---@field ChosenTitle integer PLAYER_CHOSEN_TITLE
---@field FakeInebriation integer PLAYER_FAKE_INEBRIATION
---@field Pad0 integer PLAYER_FIELD_PAD_0
---@field InvSlotHead integer PLAYER_FIELD_INV_SLOT_HEAD
---@field PackSlot1 integer PLAYER_FIELD_PACK_SLOT_1
---@field BankSlot1 integer PLAYER_FIELD_BANK_SLOT_1
---@field BankbagSlot1 integer PLAYER_FIELD_BANKBAG_SLOT_1
---@field VendorbuybackSlot1 integer PLAYER_FIELD_VENDORBUYBACK_SLOT_1
---@field KeyringSlot1 integer PLAYER_FIELD_KEYRING_SLOT_1
---@field CurrencytokenSlot1 integer PLAYER_FIELD_CURRENCYTOKEN_SLOT_1
---@field Farsight integer PLAYER_FARSIGHT
---@field KnownTitles integer PLAYER__FIELD_KNOWN_TITLES
---@field KnownTitles1 integer PLAYER__FIELD_KNOWN_TITLES1
---@field KnownTitles2 integer PLAYER__FIELD_KNOWN_TITLES2
---@field KnownCurrencies integer PLAYER_FIELD_KNOWN_CURRENCIES
---@field Xp integer PLAYER_XP
---@field NextLevelXp integer PLAYER_NEXT_LEVEL_XP
---@field SkillInfo1_1 integer PLAYER_SKILL_INFO_1_1
---@field CharacterPoints1 integer PLAYER_CHARACTER_POINTS1
---@field CharacterPoints2 integer PLAYER_CHARACTER_POINTS2
---@field TrackCreatures integer PLAYER_TRACK_CREATURES
---@field TrackResources integer PLAYER_TRACK_RESOURCES
---@field BlockPercentage integer PLAYER_BLOCK_PERCENTAGE
---@field DodgePercentage integer PLAYER_DODGE_PERCENTAGE
---@field ParryPercentage integer PLAYER_PARRY_PERCENTAGE
---@field Expertise integer PLAYER_EXPERTISE
---@field OffhandExpertise integer PLAYER_OFFHAND_EXPERTISE
---@field CritPercentage integer PLAYER_CRIT_PERCENTAGE
---@field RangedCritPercentage integer PLAYER_RANGED_CRIT_PERCENTAGE
---@field OffhandCritPercentage integer PLAYER_OFFHAND_CRIT_PERCENTAGE
---@field SpellCritPercentage1 integer PLAYER_SPELL_CRIT_PERCENTAGE1
---@field ShieldBlock integer PLAYER_SHIELD_BLOCK
---@field ShieldBlockCritPercentage integer PLAYER_SHIELD_BLOCK_CRIT_PERCENTAGE
---@field ExploredZones1 integer PLAYER_EXPLORED_ZONES_1
---@field RestStateExperience integer PLAYER_REST_STATE_EXPERIENCE
---@field Coinage integer PLAYER_FIELD_COINAGE
---@field ModDamageDonePos integer PLAYER_FIELD_MOD_DAMAGE_DONE_POS
---@field ModDamageDoneNeg integer PLAYER_FIELD_MOD_DAMAGE_DONE_NEG
---@field ModDamageDonePct integer PLAYER_FIELD_MOD_DAMAGE_DONE_PCT
---@field ModHealingDonePos integer PLAYER_FIELD_MOD_HEALING_DONE_POS
---@field ModHealingPct integer PLAYER_FIELD_MOD_HEALING_PCT
---@field ModHealingDonePct integer PLAYER_FIELD_MOD_HEALING_DONE_PCT
---@field ModTargetResistance integer PLAYER_FIELD_MOD_TARGET_RESISTANCE
---@field ModTargetPhysicalResistance integer PLAYER_FIELD_MOD_TARGET_PHYSICAL_RESISTANCE
---@field FieldBytes integer PLAYER_FIELD_BYTES
---@field AmmoId integer PLAYER_AMMO_ID
---@field SelfResSpell integer PLAYER_SELF_RES_SPELL
---@field PvpMedals integer PLAYER_FIELD_PVP_MEDALS
---@field BuybackPrice1 integer PLAYER_FIELD_BUYBACK_PRICE_1
---@field BuybackTimestamp1 integer PLAYER_FIELD_BUYBACK_TIMESTAMP_1
---@field Kills integer PLAYER_FIELD_KILLS
---@field TodayContribution integer PLAYER_FIELD_TODAY_CONTRIBUTION
---@field YesterdayContribution integer PLAYER_FIELD_YESTERDAY_CONTRIBUTION
---@field LifetimeHonorbaleKills integer PLAYER_FIELD_LIFETIME_HONORBALE_KILLS
---@field FieldBytes2 integer PLAYER_FIELD_BYTES2
---@field WatchedFactionIndex integer PLAYER_FIELD_WATCHED_FACTION_INDEX
---@field CombatRating1 integer PLAYER_FIELD_COMBAT_RATING_1
---@field ArenaTeamInfo1_1 integer PLAYER_FIELD_ARENA_TEAM_INFO_1_1
---@field HonorCurrency integer PLAYER_FIELD_HONOR_CURRENCY
---@field ArenaCurrency integer PLAYER_FIELD_ARENA_CURRENCY
---@field MaxLevel integer PLAYER_FIELD_MAX_LEVEL
---@field DailyQuests1 integer PLAYER_FIELD_DAILY_QUESTS_1
---@field RuneRegen1 integer PLAYER_RUNE_REGEN_1
---@field NoReagentCost1 integer PLAYER_NO_REAGENT_COST_1
---@field GlyphSlots1 integer PLAYER_FIELD_GLYPH_SLOTS_1
---@field Glyphs1 integer PLAYER_FIELD_GLYPHS_1
---@field GlyphsEnabled integer PLAYER_GLYPHS_ENABLED
---@field PetSpellPower integer PLAYER_PET_SPELL_POWER

---The dword index of every field of a GameObject.
---@class wxl.field.GameObject
---@field CreatedBy integer OBJECT_FIELD_CREATED_BY
---@field Displayid integer GAMEOBJECT_DISPLAYID
---@field Flags integer GAMEOBJECT_FLAGS
---@field Parentrotation integer GAMEOBJECT_PARENTROTATION
---@field Dynamic integer GAMEOBJECT_DYNAMIC
---@field Faction integer GAMEOBJECT_FACTION
---@field Level integer GAMEOBJECT_LEVEL
---@field Bytes1 integer GAMEOBJECT_BYTES_1

---The dword index of every field of a DynamicObject.
---@class wxl.field.DynamicObject
---@field Caster integer DYNAMICOBJECT_CASTER
---@field Bytes integer DYNAMICOBJECT_BYTES
---@field Spellid integer DYNAMICOBJECT_SPELLID
---@field Radius integer DYNAMICOBJECT_RADIUS
---@field Casttime integer DYNAMICOBJECT_CASTTIME

---The dword index of every field of a Corpse.
---@class wxl.field.Corpse
---@field Owner integer CORPSE_FIELD_OWNER
---@field Party integer CORPSE_FIELD_PARTY
---@field DisplayId integer CORPSE_FIELD_DISPLAY_ID
---@field Item integer CORPSE_FIELD_ITEM
---@field Bytes1 integer CORPSE_FIELD_BYTES_1
---@field Bytes2 integer CORPSE_FIELD_BYTES_2
---@field Guild integer CORPSE_FIELD_GUILD
---@field Flags integer CORPSE_FIELD_FLAGS
---@field DynamicFlags integer CORPSE_FIELD_DYNAMIC_FLAGS
---@field Pad integer CORPSE_FIELD_PAD

---Every field's dword index, by kind: wxl.field.Unit.Health is UNIT_FIELD_HEALTH's.
---@class wxl.field
---@field Object wxl.field.Object
---@field Item wxl.field.Item
---@field Container wxl.field.Container
---@field Unit wxl.field.Unit
---@field Player wxl.field.Player
---@field GameObject wxl.field.GameObject
---@field DynamicObject wxl.field.DynamicObject
---@field Corpse wxl.field.Corpse

---A wxl_object_type value, by name; a newer value than these headers know
---arrives as its number.
---@alias wxl.ObjectType "Unknown"|"Item"|"Container"|"Unit"|"Player"|"GameObject"|"DynamicObject"|"Corpse"

---A wxl_script_context value, by name; a newer value than these headers know
---arrives as its number.
---@alias wxl.ScriptContext "glue"|"game"

---A wxl_mouse_button value, by name; a newer value than these headers know
---arrives as its number.
---@alias wxl.MouseButton "left"|"right"|"middle"

---A wxl_cooldown_change value, by name; a newer value than these headers know
---arrives as its number.
---@alias wxl.CooldownChange "started"|"cleared"|"shifted"

---A wxl_damage_kind value, by name; a newer value than these headers know
---arrives as its number.
---@alias wxl.DamageKind "melee"|"spell"|"periodic"|"environment"

---A wxl_environment value, by name; a newer value than these headers know
---arrives as its number.
---@alias wxl.Environment "exhaustion"|"drowning"|"fall"|"lava"|"slime"|"fire"

---A wxl_xp_source value, by name; a newer value than these headers know
---arrives as its number.
---@alias wxl.XpSource "kill"|"other"

-- The script families. A function defined on one of these tables as On<Event> is
-- subscribed when the entry file has run and called with the event's parameters;
-- on an event that can be answered its return values are taken, as Override
-- takes them.

---@class WorldScript
---@field OnUpdate fun(self: WorldScript, delta: number)
---@field OnWorldEnter fun(self: WorldScript, map_id: integer, position: wxl.Vec3)
---@field OnWorldLeave fun(self: WorldScript, map_id: integer, position: wxl.Vec3)
---@field OnWorldClick fun(self: WorldScript, point: wxl.Vec3, object: wxl.Object|nil, button: wxl.MouseButton|integer)
---@field OnInput fun(self: WorldScript, message: integer, wparam: integer, lparam: integer, handled: boolean|nil): boolean|nil answers through handled
---@field OnSoundPlay fun(self: WorldScript, sound_id: integer, path: string, suppressed: boolean|nil): boolean|nil answers through suppressed
---@field OnZoneChanged fun(self: WorldScript, area_id: integer, subzone_id: integer, wmo_group_id: integer, zone_text: string, subzone_text: string, real_zone_text: string, indoors: boolean, new_area: boolean)
---@field OnMouseoverChanged fun(self: WorldScript, previous: wxl.Object|nil, previous_guid: wxl.Guid, current: wxl.Object|nil, current_guid: wxl.Guid)
---@field OnCVarChanged fun(self: WorldScript, name: string, previous: string, value: string)
WorldScript = {}

---@class RenderScript
---@field OnFrameBegin fun(self: RenderScript, index: integer, delta: number)
---@field OnFrameEnd fun(self: RenderScript, index: integer, delta: number)
---@field OnWorldRenderBegin fun(self: RenderScript, index: integer, delta: number)
---@field OnWorldRenderEnd fun(self: RenderScript, index: integer, delta: number)
---@field OnUiRenderEnd fun(self: RenderScript, index: integer, delta: number)
---@field OnSwapchainLost fun(self: RenderScript, width: integer, height: integer)
---@field OnSwapchainRestored fun(self: RenderScript, width: integer, height: integer)
RenderScript = {}

---@class ObjectScript
---@field OnObjectCreate fun(self: ObjectScript, object: wxl.Object|nil, guid: wxl.Guid, type: wxl.ObjectType|integer)
---@field OnObjectUpdate fun(self: ObjectScript, object: wxl.Object|nil, guid: wxl.Guid, type: wxl.ObjectType|integer)
---@field OnObjectDestroy fun(self: ObjectScript, object: wxl.Object|nil, guid: wxl.Guid, type: wxl.ObjectType|integer)
---@field OnTargetChanged fun(self: ObjectScript, previous: wxl.Object|nil, current: wxl.Object|nil)
---@field OnDoodadSpawn fun(self: ObjectScript, doodad: wxl.Object|nil, path: string, position: wxl.Vec3)
---@field OnEquipmentChanged fun(self: ObjectScript, unit: wxl.Object|nil, slot: integer, item_id: integer)
---@field OnUnitDeath fun(self: ObjectScript, unit: wxl.Object|nil)
---@field OnUnitAuraChanged fun(self: ObjectScript, unit: wxl.Object|nil, spell_id: integer, removed: boolean)
---@field OnSpellCastStart fun(self: ObjectScript, caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, cast_item: wxl.Guid, spell_id: integer, cast_count: integer, cast_flags: integer, target_flags: integer, cast_time: integer, hit_count: integer, hits: wxl.Bytes, misses: wxl.Bytes, miss_reasons: wxl.Bytes)
---@field OnSpellCastGo fun(self: ObjectScript, caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, cast_item: wxl.Guid, spell_id: integer, cast_count: integer, cast_flags: integer, target_flags: integer, cast_time: integer, hit_count: integer, hits: wxl.Bytes, misses: wxl.Bytes, miss_reasons: wxl.Bytes)
---@field OnSpellCastFailed fun(self: ObjectScript, caster: wxl.Object|nil, caster_guid: wxl.Guid, spell_id: integer, cast_count: integer, reason: integer, custom_error: integer, custom_arg: integer)
---@field OnSpellCastInterrupted fun(self: ObjectScript, caster: wxl.Object|nil, caster_guid: wxl.Guid, spell_id: integer, cast_count: integer, reason: integer, custom_error: integer, custom_arg: integer)
---@field OnSpellCastRequest fun(self: ObjectScript, caster: wxl.Object|nil, spell_id: integer, item: wxl.Object|nil, target_guid: wxl.Guid, confirmed: boolean, refused: boolean|nil): boolean|nil answers through refused
---@field OnItemUseRequest fun(self: ObjectScript, item: wxl.Object|nil, item_guid: wxl.Guid, target_guid: wxl.Guid, confirmed: boolean, refused: boolean|nil): boolean|nil answers through refused
---@field OnSpellChannelStart fun(self: ObjectScript, caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, duration: integer, remaining: integer)
---@field OnSpellChannelUpdate fun(self: ObjectScript, caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, duration: integer, remaining: integer)
---@field OnSpellChannelStop fun(self: ObjectScript, caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, duration: integer, remaining: integer)
---@field OnSpellCastResume fun(self: ObjectScript, caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, duration: integer, remaining: integer)
---@field OnSpellCastDelayed fun(self: ObjectScript, caster: wxl.Object|nil, caster_guid: wxl.Guid, spell_id: integer, delay: integer)
---@field OnSpellCooldown fun(self: ObjectScript, unit: wxl.Object|nil, unit_guid: wxl.Guid, item: wxl.Object|nil, item_guid: wxl.Guid, spell_id: integer, duration: integer, flags: integer)
---@field OnSpellCooldownChanged fun(self: ObjectScript, unit: wxl.Object|nil, unit_guid: wxl.Guid, spell_id: integer, change: wxl.CooldownChange|integer, delay: integer)
---@field OnSpellCastCancel fun(self: ObjectScript, caster: wxl.Object|nil, caster_guid: wxl.Guid, spell_id: integer, cast_count: integer, reason: integer, notify_server: boolean)
---@field OnObjectFieldChanged fun(self: ObjectScript, object: wxl.Object|nil, type: wxl.ObjectType|integer, field: integer, count: integer, old_value: wxl.FieldValue, new_value: wxl.FieldValue)
ObjectScript = {}

---@class AssetScript
---@field OnModelLoad fun(self: AssetScript, path: string, data: wxl.Bytes, replacement: lightuserdata|nil, replacement_size: lightuserdata|nil)
---@field OnTextureLoad fun(self: AssetScript, path: string)
---@field OnTerrainChunkBuild fun(self: AssetScript, map_id: integer, tile_x: integer, tile_y: integer, chunk_x: integer, chunk_y: integer)
---@field OnMapObjectLoad fun(self: AssetScript, path: string, data: wxl.Bytes)
---@field OnFileOpen fun(self: AssetScript, path: string, redirect: string|nil): string|nil answers through redirect
---@field OnFileOpened fun(self: AssetScript, path: string, found: boolean, size: integer)
---@field OnClientDbLoaded fun(self: AssetScript, name: string, records: lightuserdata|nil, record_count: integer, record_size: integer)
AssetScript = {}

---@class NetScript
---@field OnMessageReceived fun(self: NetScript, opcode: integer, data: wxl.Bytes, suppressed: boolean|nil): boolean|nil answers through suppressed
---@field OnMessageSent fun(self: NetScript, opcode: integer, data: wxl.Bytes, suppressed: boolean|nil): boolean|nil answers through suppressed
---@field OnConnected fun(self: NetScript, address: string)
---@field OnDisconnected fun(self: NetScript, address: string)
NetScript = {}

---@class LuaScript
---@field OnScriptVmReady fun(self: LuaScript, context: wxl.ScriptContext|integer, native_state: lightuserdata|nil)
---@field OnScriptVmClosing fun(self: LuaScript, context: wxl.ScriptContext|integer, native_state: lightuserdata|nil)
---@field OnUiEvent fun(self: LuaScript, index: integer, name: string, format: string, values: wxl.Bytes, value_count: integer)
---@field OnScriptError fun(self: LuaScript, context: wxl.ScriptContext|integer, message: string)
LuaScript = {}

---@class CombatScript
---@field OnCombatAttackStart fun(self: CombatScript, attacker: wxl.Object|nil, attacker_guid: wxl.Guid, victim: wxl.Object|nil, victim_guid: wxl.Guid)
---@field OnCombatAttackStop fun(self: CombatScript, attacker: wxl.Object|nil, attacker_guid: wxl.Guid, victim: wxl.Object|nil, victim_guid: wxl.Guid)
---@field OnCombatDamage fun(self: CombatScript, source: wxl.Object|nil, source_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, kind: wxl.DamageKind|integer, environment: wxl.Environment|integer, amount: integer, overkill: integer, school: integer, absorbed: integer, resisted: integer, blocked: integer, crit: boolean, hit_info: integer)
---@field OnCombatHeal fun(self: CombatScript, source: wxl.Object|nil, source_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, amount: integer, overheal: integer, absorbed: integer, crit: boolean, periodic: boolean)
---@field OnCombatMiss fun(self: CombatScript, source: wxl.Object|nil, source_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, melee: boolean, reason: integer, hit_info: integer)
---@field OnCombatEnergize fun(self: CombatScript, source: wxl.Object|nil, source_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, power_type: integer, amount: integer, periodic: boolean, drain: boolean, multiplier: number)
CombatScript = {}

---@class PlayerScript
---@field OnPlayerMoveStart fun(self: PlayerScript, unit: wxl.Object|nil, unit_guid: wxl.Guid, opcode: integer, flags: integer, position: wxl.Vec3, facing: number)
---@field OnPlayerMoveStop fun(self: PlayerScript, unit: wxl.Object|nil, unit_guid: wxl.Guid, opcode: integer, flags: integer, position: wxl.Vec3, facing: number)
---@field OnPlayerLevelUp fun(self: PlayerScript, level: integer, health: integer, mana: integer, rage: integer, focus: integer, energy: integer, happiness: integer, runes: integer, runic_power: integer, strength: integer, agility: integer, stamina: integer, intellect: integer, spirit: integer)
---@field OnPlayerXpGain fun(self: PlayerScript, source: wxl.Object|nil, source_guid: wxl.Guid, amount: integer, kind: wxl.XpSource|integer, bonus: integer, group_multiplier: number, refer_a_friend: boolean)
---@field OnPlayerMoneyChanged fun(self: PlayerScript, player: wxl.Object|nil, previous: integer, current: integer)
PlayerScript = {}

---@class GlueScript
---@field OnGlueScreenChanged fun(self: GlueScript, screen: string, previous: string)
---@field OnGlueCharacterList fun(self: GlueScript, count: integer, valid: boolean)
---@field OnGlueCharacter fun(self: GlueScript, index: integer, count: integer, guid: wxl.Guid, name: string, race: integer, class_id: integer, sex: integer, level: integer, zone_id: integer, map_id: integer, position: wxl.Vec3, guild_id: integer, flags: integer, customize_flags: integer, first_login: boolean)
---@field OnGlueRealmList fun(self: GlueScript, count: integer)
---@field OnGlueRealm fun(self: GlueScript, index: integer, count: integer, name: string, address: string, type: integer, flags: integer, locked: boolean, population: number, characters: integer, category: integer, realm_id: integer, version_major: integer, version_minor: integer, version_revision: integer, version_build: integer, region_id: integer, battlegroup_id: integer, build: integer)
---@field OnGlueAuthResult fun(self: GlueScript, result: integer, queue_position: integer, billing_time_remaining: integer, billing_flags: integer, billing_time_rested: integer, expansion: integer, free_character_migration: boolean)
---@field OnGlueEnterWorld fun(self: GlueScript, character_guid: wxl.Guid)
GlueScript = {}

---@class ChatScript
---@field OnChatMessage fun(self: ChatScript, type: integer, language: integer, sender: wxl.Object|nil, sender_guid: wxl.Guid, sender_name: string, target_guid: wxl.Guid, target_name: string, channel: string, text: string, tag: integer, achievement_id: integer, gm: boolean, line_id: integer, shown_text: string)
---@field OnChatSend fun(self: ChatScript, text: string, type: string, language: string, target: string, prefix: string, addon: boolean, suppressed: boolean|nil): boolean|nil answers through suppressed
---@field OnChatAddonMessage fun(self: ChatScript, type: integer, sender: wxl.Object|nil, sender_guid: wxl.Guid, sender_name: string, prefix: string, text: string, channel: string, gm: boolean)
ChatScript = {}

---@alias wxl.Family "WorldScript"|"RenderScript"|"ObjectScript"|"AssetScript"|"NetScript"|"LuaScript"|"CombatScript"|"PlayerScript"|"GlueScript"|"ChatScript"

---Calls `handler` with the event's parameters whenever it is raised. Returns the
---handler's id.
---@overload fun(family: "WorldScript", method: "OnUpdate", handler: fun(delta: number)): integer
---@overload fun(family: "WorldScript", method: "OnWorldEnter", handler: fun(map_id: integer, position: wxl.Vec3)): integer
---@overload fun(family: "WorldScript", method: "OnWorldLeave", handler: fun(map_id: integer, position: wxl.Vec3)): integer
---@overload fun(family: "WorldScript", method: "OnWorldClick", handler: fun(point: wxl.Vec3, object: wxl.Object|nil, button: wxl.MouseButton|integer)): integer
---@overload fun(family: "WorldScript", method: "OnInput", handler: fun(message: integer, wparam: integer, lparam: integer, handled: boolean|nil)): integer
---@overload fun(family: "WorldScript", method: "OnSoundPlay", handler: fun(sound_id: integer, path: string, suppressed: boolean|nil)): integer
---@overload fun(family: "RenderScript", method: "OnFrameBegin", handler: fun(index: integer, delta: number)): integer
---@overload fun(family: "RenderScript", method: "OnFrameEnd", handler: fun(index: integer, delta: number)): integer
---@overload fun(family: "RenderScript", method: "OnWorldRenderBegin", handler: fun(index: integer, delta: number)): integer
---@overload fun(family: "RenderScript", method: "OnWorldRenderEnd", handler: fun(index: integer, delta: number)): integer
---@overload fun(family: "RenderScript", method: "OnUiRenderEnd", handler: fun(index: integer, delta: number)): integer
---@overload fun(family: "RenderScript", method: "OnSwapchainLost", handler: fun(width: integer, height: integer)): integer
---@overload fun(family: "RenderScript", method: "OnSwapchainRestored", handler: fun(width: integer, height: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnObjectCreate", handler: fun(object: wxl.Object|nil, guid: wxl.Guid, type: wxl.ObjectType|integer)): integer
---@overload fun(family: "ObjectScript", method: "OnObjectUpdate", handler: fun(object: wxl.Object|nil, guid: wxl.Guid, type: wxl.ObjectType|integer)): integer
---@overload fun(family: "ObjectScript", method: "OnObjectDestroy", handler: fun(object: wxl.Object|nil, guid: wxl.Guid, type: wxl.ObjectType|integer)): integer
---@overload fun(family: "ObjectScript", method: "OnTargetChanged", handler: fun(previous: wxl.Object|nil, current: wxl.Object|nil)): integer
---@overload fun(family: "ObjectScript", method: "OnDoodadSpawn", handler: fun(doodad: wxl.Object|nil, path: string, position: wxl.Vec3)): integer
---@overload fun(family: "ObjectScript", method: "OnEquipmentChanged", handler: fun(unit: wxl.Object|nil, slot: integer, item_id: integer)): integer
---@overload fun(family: "AssetScript", method: "OnModelLoad", handler: fun(path: string, data: wxl.Bytes, replacement: lightuserdata|nil, replacement_size: lightuserdata|nil)): integer
---@overload fun(family: "AssetScript", method: "OnTextureLoad", handler: fun(path: string)): integer
---@overload fun(family: "AssetScript", method: "OnTerrainChunkBuild", handler: fun(map_id: integer, tile_x: integer, tile_y: integer, chunk_x: integer, chunk_y: integer)): integer
---@overload fun(family: "AssetScript", method: "OnMapObjectLoad", handler: fun(path: string, data: wxl.Bytes)): integer
---@overload fun(family: "AssetScript", method: "OnFileOpen", handler: fun(path: string, redirect: string|nil)): integer
---@overload fun(family: "AssetScript", method: "OnFileOpened", handler: fun(path: string, found: boolean, size: integer)): integer
---@overload fun(family: "AssetScript", method: "OnClientDbLoaded", handler: fun(name: string, records: lightuserdata|nil, record_count: integer, record_size: integer)): integer
---@overload fun(family: "NetScript", method: "OnMessageReceived", handler: fun(opcode: integer, data: wxl.Bytes, suppressed: boolean|nil)): integer
---@overload fun(family: "NetScript", method: "OnMessageSent", handler: fun(opcode: integer, data: wxl.Bytes, suppressed: boolean|nil)): integer
---@overload fun(family: "NetScript", method: "OnConnected", handler: fun(address: string)): integer
---@overload fun(family: "NetScript", method: "OnDisconnected", handler: fun(address: string)): integer
---@overload fun(family: "LuaScript", method: "OnScriptVmReady", handler: fun(context: wxl.ScriptContext|integer, native_state: lightuserdata|nil)): integer
---@overload fun(family: "LuaScript", method: "OnScriptVmClosing", handler: fun(context: wxl.ScriptContext|integer, native_state: lightuserdata|nil)): integer
---@overload fun(family: "ObjectScript", method: "OnUnitDeath", handler: fun(unit: wxl.Object|nil)): integer
---@overload fun(family: "ObjectScript", method: "OnUnitAuraChanged", handler: fun(unit: wxl.Object|nil, spell_id: integer, removed: boolean)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastStart", handler: fun(caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, cast_item: wxl.Guid, spell_id: integer, cast_count: integer, cast_flags: integer, target_flags: integer, cast_time: integer, hit_count: integer, hits: wxl.Bytes, misses: wxl.Bytes, miss_reasons: wxl.Bytes)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastGo", handler: fun(caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, cast_item: wxl.Guid, spell_id: integer, cast_count: integer, cast_flags: integer, target_flags: integer, cast_time: integer, hit_count: integer, hits: wxl.Bytes, misses: wxl.Bytes, miss_reasons: wxl.Bytes)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastFailed", handler: fun(caster: wxl.Object|nil, caster_guid: wxl.Guid, spell_id: integer, cast_count: integer, reason: integer, custom_error: integer, custom_arg: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastInterrupted", handler: fun(caster: wxl.Object|nil, caster_guid: wxl.Guid, spell_id: integer, cast_count: integer, reason: integer, custom_error: integer, custom_arg: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastRequest", handler: fun(caster: wxl.Object|nil, spell_id: integer, item: wxl.Object|nil, target_guid: wxl.Guid, confirmed: boolean, refused: boolean|nil)): integer
---@overload fun(family: "ObjectScript", method: "OnItemUseRequest", handler: fun(item: wxl.Object|nil, item_guid: wxl.Guid, target_guid: wxl.Guid, confirmed: boolean, refused: boolean|nil)): integer
---@overload fun(family: "LuaScript", method: "OnUiEvent", handler: fun(index: integer, name: string, format: string, values: wxl.Bytes, value_count: integer)): integer
---@overload fun(family: "LuaScript", method: "OnScriptError", handler: fun(context: wxl.ScriptContext|integer, message: string)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellChannelStart", handler: fun(caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, duration: integer, remaining: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellChannelUpdate", handler: fun(caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, duration: integer, remaining: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellChannelStop", handler: fun(caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, duration: integer, remaining: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastResume", handler: fun(caster: wxl.Object|nil, caster_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, duration: integer, remaining: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastDelayed", handler: fun(caster: wxl.Object|nil, caster_guid: wxl.Guid, spell_id: integer, delay: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCooldown", handler: fun(unit: wxl.Object|nil, unit_guid: wxl.Guid, item: wxl.Object|nil, item_guid: wxl.Guid, spell_id: integer, duration: integer, flags: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCooldownChanged", handler: fun(unit: wxl.Object|nil, unit_guid: wxl.Guid, spell_id: integer, change: wxl.CooldownChange|integer, delay: integer)): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastCancel", handler: fun(caster: wxl.Object|nil, caster_guid: wxl.Guid, spell_id: integer, cast_count: integer, reason: integer, notify_server: boolean)): integer
---@overload fun(family: "CombatScript", method: "OnCombatAttackStart", handler: fun(attacker: wxl.Object|nil, attacker_guid: wxl.Guid, victim: wxl.Object|nil, victim_guid: wxl.Guid)): integer
---@overload fun(family: "CombatScript", method: "OnCombatAttackStop", handler: fun(attacker: wxl.Object|nil, attacker_guid: wxl.Guid, victim: wxl.Object|nil, victim_guid: wxl.Guid)): integer
---@overload fun(family: "CombatScript", method: "OnCombatDamage", handler: fun(source: wxl.Object|nil, source_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, kind: wxl.DamageKind|integer, environment: wxl.Environment|integer, amount: integer, overkill: integer, school: integer, absorbed: integer, resisted: integer, blocked: integer, crit: boolean, hit_info: integer)): integer
---@overload fun(family: "CombatScript", method: "OnCombatHeal", handler: fun(source: wxl.Object|nil, source_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, amount: integer, overheal: integer, absorbed: integer, crit: boolean, periodic: boolean)): integer
---@overload fun(family: "CombatScript", method: "OnCombatMiss", handler: fun(source: wxl.Object|nil, source_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, melee: boolean, reason: integer, hit_info: integer)): integer
---@overload fun(family: "WorldScript", method: "OnZoneChanged", handler: fun(area_id: integer, subzone_id: integer, wmo_group_id: integer, zone_text: string, subzone_text: string, real_zone_text: string, indoors: boolean, new_area: boolean)): integer
---@overload fun(family: "WorldScript", method: "OnMouseoverChanged", handler: fun(previous: wxl.Object|nil, previous_guid: wxl.Guid, current: wxl.Object|nil, current_guid: wxl.Guid)): integer
---@overload fun(family: "WorldScript", method: "OnCVarChanged", handler: fun(name: string, previous: string, value: string)): integer
---@overload fun(family: "PlayerScript", method: "OnPlayerMoveStart", handler: fun(unit: wxl.Object|nil, unit_guid: wxl.Guid, opcode: integer, flags: integer, position: wxl.Vec3, facing: number)): integer
---@overload fun(family: "PlayerScript", method: "OnPlayerMoveStop", handler: fun(unit: wxl.Object|nil, unit_guid: wxl.Guid, opcode: integer, flags: integer, position: wxl.Vec3, facing: number)): integer
---@overload fun(family: "PlayerScript", method: "OnPlayerLevelUp", handler: fun(level: integer, health: integer, mana: integer, rage: integer, focus: integer, energy: integer, happiness: integer, runes: integer, runic_power: integer, strength: integer, agility: integer, stamina: integer, intellect: integer, spirit: integer)): integer
---@overload fun(family: "PlayerScript", method: "OnPlayerXpGain", handler: fun(source: wxl.Object|nil, source_guid: wxl.Guid, amount: integer, kind: wxl.XpSource|integer, bonus: integer, group_multiplier: number, refer_a_friend: boolean)): integer
---@overload fun(family: "PlayerScript", method: "OnPlayerMoneyChanged", handler: fun(player: wxl.Object|nil, previous: integer, current: integer)): integer
---@overload fun(family: "GlueScript", method: "OnGlueScreenChanged", handler: fun(screen: string, previous: string)): integer
---@overload fun(family: "GlueScript", method: "OnGlueCharacterList", handler: fun(count: integer, valid: boolean)): integer
---@overload fun(family: "GlueScript", method: "OnGlueCharacter", handler: fun(index: integer, count: integer, guid: wxl.Guid, name: string, race: integer, class_id: integer, sex: integer, level: integer, zone_id: integer, map_id: integer, position: wxl.Vec3, guild_id: integer, flags: integer, customize_flags: integer, first_login: boolean)): integer
---@overload fun(family: "GlueScript", method: "OnGlueRealmList", handler: fun(count: integer)): integer
---@overload fun(family: "GlueScript", method: "OnGlueRealm", handler: fun(index: integer, count: integer, name: string, address: string, type: integer, flags: integer, locked: boolean, population: number, characters: integer, category: integer, realm_id: integer, version_major: integer, version_minor: integer, version_revision: integer, version_build: integer, region_id: integer, battlegroup_id: integer, build: integer)): integer
---@overload fun(family: "GlueScript", method: "OnGlueAuthResult", handler: fun(result: integer, queue_position: integer, billing_time_remaining: integer, billing_flags: integer, billing_time_rested: integer, expansion: integer, free_character_migration: boolean)): integer
---@overload fun(family: "GlueScript", method: "OnGlueEnterWorld", handler: fun(character_guid: wxl.Guid)): integer
---@overload fun(family: "ChatScript", method: "OnChatMessage", handler: fun(type: integer, language: integer, sender: wxl.Object|nil, sender_guid: wxl.Guid, sender_name: string, target_guid: wxl.Guid, target_name: string, channel: string, text: string, tag: integer, achievement_id: integer, gm: boolean, line_id: integer, shown_text: string)): integer
---@overload fun(family: "ChatScript", method: "OnChatSend", handler: fun(text: string, type: string, language: string, target: string, prefix: string, addon: boolean, suppressed: boolean|nil)): integer
---@overload fun(family: "CombatScript", method: "OnCombatEnergize", handler: fun(source: wxl.Object|nil, source_guid: wxl.Guid, target: wxl.Object|nil, target_guid: wxl.Guid, spell_id: integer, power_type: integer, amount: integer, periodic: boolean, drain: boolean, multiplier: number)): integer
---@overload fun(family: "ChatScript", method: "OnChatAddonMessage", handler: fun(type: integer, sender: wxl.Object|nil, sender_guid: wxl.Guid, sender_name: string, prefix: string, text: string, channel: string, gm: boolean)): integer
---@overload fun(family: "ObjectScript", method: "OnObjectFieldChanged", handler: fun(object: wxl.Object|nil, type: wxl.ObjectType|integer, field: integer, count: integer, old_value: wxl.FieldValue, new_value: wxl.FieldValue)): integer
---@param family wxl.Family
---@param method string
---@param handler function
---@return integer id
function Observe(family, method, handler) end

---Like Observe, on an event that can be answered. The handler returns one value per
---member it answers through, in member order: true sets an answer, a string fills a
---buffer, nil leaves the member as it is. Taken on the main thread only.
---@overload fun(family: "WorldScript", method: "OnInput", handler: fun(message: integer, wparam: integer, lparam: integer, handled: boolean|nil): boolean|nil): integer
---@overload fun(family: "WorldScript", method: "OnSoundPlay", handler: fun(sound_id: integer, path: string, suppressed: boolean|nil): boolean|nil): integer
---@overload fun(family: "AssetScript", method: "OnFileOpen", handler: fun(path: string, redirect: string|nil): string|nil): integer
---@overload fun(family: "NetScript", method: "OnMessageReceived", handler: fun(opcode: integer, data: wxl.Bytes, suppressed: boolean|nil): boolean|nil): integer
---@overload fun(family: "NetScript", method: "OnMessageSent", handler: fun(opcode: integer, data: wxl.Bytes, suppressed: boolean|nil): boolean|nil): integer
---@overload fun(family: "ObjectScript", method: "OnSpellCastRequest", handler: fun(caster: wxl.Object|nil, spell_id: integer, item: wxl.Object|nil, target_guid: wxl.Guid, confirmed: boolean, refused: boolean|nil): boolean|nil): integer
---@overload fun(family: "ObjectScript", method: "OnItemUseRequest", handler: fun(item: wxl.Object|nil, item_guid: wxl.Guid, target_guid: wxl.Guid, confirmed: boolean, refused: boolean|nil): boolean|nil): integer
---@overload fun(family: "ChatScript", method: "OnChatSend", handler: fun(text: string, type: string, language: string, target: string, prefix: string, addon: boolean, suppressed: boolean|nil): boolean|nil): integer
---@param family wxl.Family
---@param method string
---@param handler function
---@return integer id
function Override(family, method, handler) end

---Drops a handler by the id Observe or Override returned.
---@param id integer
---@return boolean dropped
function Unobserve(id) end

---@alias wxl.CallbackName "onInit"|"onUpdate"|"onDraw"|"onShutdown"

---Registers a callback: onInit at WXL_STAGE_READY (or right after a reload),
---onUpdate(delta) at every World.Update, onDraw at every Render.UiRenderEnd,
---onShutdown at WXL_STAGE_SHUTDOWN and before a reload.
---@overload fun(event: "onInit", fn: fun())
---@overload fun(event: "onUpdate", fn: fun(delta: number))
---@overload fun(event: "onDraw", fn: fun())
---@overload fun(event: "onShutdown", fn: fun())
---@param event wxl.CallbackName
---@param fn function
function registerForEvent(event, fn) end

---A world object, by handle. Every getter answers the value, or nil and why:
---"gone" for an object that left, "unsupported" for a runtime without the read, or
---what the object is not ("a GameObject has no Health").
---
---One object is one Lua value for as long as it stays in view: the same at every event
---and every lookup, a table key, equal only to itself. A field set on it that is not a
---method's name (`obj.seen = true`) is the mod's own and is kept until the object leaves.
---@class wxl.Object
---@field [string] any what the mod stored on the object
local Object = {}

---Whether the object is still in view.
---@return boolean|nil valid, string|nil reason
function Object:Valid() end

---The server's id for the object, as "0x" and sixteen hex digits.
---@return string|nil guid, string|nil reason
function Object:Guid() end

---The kind's name, "Unit", "GameObject"...
---@return wxl.ObjectType|nil type, string|nil reason
function Object:Type() end

---The kind as a number.
---@return integer|nil type, string|nil reason
function Object:TypeId() end

---@return string|nil name, string|nil reason
function Object:Name() end

---What a unit targets; nil when nothing in view.
---@return wxl.Object|nil target, string|nil reason
function Object:Target() end

---The handle itself, as "0x" and sixteen hex digits: for logs.
---@return string
function Object:Handle() end

---The size of the object's descriptor block, in dwords.
---@return integer|nil count, string|nil reason
function Object:FieldCount() end

---One element of a descriptor field: a number, a guid's string, or the four bytes or two
---halves of a dword as that many numbers.
---@param name wxl.FieldName|string
---@param element? integer 0 for the first
---@return wxl.FieldValue|nil ..., string|nil reason
function Object:Field(name, element) end

---Every element of a descriptor field, in order; the bytes and halves as tables.
---@param name wxl.FieldName|string
---@return wxl.FieldValue[]|nil values, string|nil reason
function Object:Fields(name) end

---Writes one element of a descriptor field in the client, until the server's next update
---of it replaces it: a number, a guid, or a table of the bytes or the halves.
---@param name wxl.FieldName|string
---@param value wxl.FieldValue
---@param element? integer
---@return boolean|nil written, string|nil reason
function Object:SetField(name, value, element) end

---For any object (position).
---@return wxl.Vec3|nil value, string|nil reason
function Object:Position() end

---For any object (facing).
---@return number|nil value, string|nil reason
function Object:Facing() end

---For any object (entry).
---@return integer|nil value, string|nil reason
function Object:Entry() end

---For any object (scale).
---@return number|nil value, string|nil reason
function Object:Scale() end

---For Unit, Player (unit_health).
---@return integer|nil value, string|nil reason
function Object:Health() end

---For Unit, Player (unit_max_health).
---@return integer|nil value, string|nil reason
function Object:MaxHealth() end

---For Unit, Player; GameObject (unit_level, gameobject_level).
---@return integer|nil value, string|nil reason
function Object:Level() end

---For Unit, Player (unit_power).
---@param index integer
---@return integer|nil value, string|nil reason
function Object:Power(index) end

---For Unit, Player (unit_max_power).
---@param index integer
---@return integer|nil value, string|nil reason
function Object:MaxPower(index) end

---For Unit, Player (unit_power_type).
---@return integer|nil value, string|nil reason
function Object:PowerType() end

---For Unit, Player (unit_race).
---@return integer|nil value, string|nil reason
function Object:Race() end

---For Unit, Player (unit_class).
---@return integer|nil value, string|nil reason
function Object:Class() end

---For Unit, Player (unit_sex).
---@return integer|nil value, string|nil reason
function Object:Sex() end

---For Unit, Player; GameObject; Corpse (unit_display_id, gameobject_display_id, corpse_display_id).
---@return integer|nil value, string|nil reason
function Object:DisplayId() end

---For Unit, Player (unit_native_display_id).
---@return integer|nil value, string|nil reason
function Object:NativeDisplayId() end

---For Unit, Player (unit_mount_display_id).
---@return integer|nil value, string|nil reason
function Object:MountDisplayId() end

---For Unit, Player (unit_faction_template).
---@return integer|nil value, string|nil reason
function Object:FactionTemplate() end

---For Unit, Player; Item, Container; GameObject; Corpse (unit_flags, item_flags, gameobject_flags, corpse_flags).
---@return integer|nil value, string|nil reason
function Object:Flags() end

---For Unit, Player (unit_flags2).
---@return integer|nil value, string|nil reason
function Object:Flags2() end

---For Unit, Player; GameObject; Corpse (unit_dynamic_flags, gameobject_dynamic_flags, corpse_dynamic_flags).
---@return integer|nil value, string|nil reason
function Object:DynamicFlags() end

---For Unit, Player (unit_npc_flags).
---@return integer|nil value, string|nil reason
function Object:NpcFlags() end

---For Unit, Player (unit_bounding_radius).
---@return number|nil value, string|nil reason
function Object:BoundingRadius() end

---For Unit, Player (unit_combat_reach).
---@return number|nil value, string|nil reason
function Object:CombatReach() end

---For Unit, Player (unit_charm).
---@return string|nil value, string|nil reason
function Object:Charm() end

---For Unit, Player (unit_summon).
---@return string|nil value, string|nil reason
function Object:Summon() end

---For Unit, Player (unit_charmed_by).
---@return string|nil value, string|nil reason
function Object:CharmedBy() end

---For Unit, Player (unit_summoned_by).
---@return string|nil value, string|nil reason
function Object:SummonedBy() end

---For Unit, Player; GameObject (unit_created_by, gameobject_created_by).
---@return string|nil value, string|nil reason
function Object:CreatedBy() end

---For Player (player_flags).
---@return integer|nil value, string|nil reason
function Object:PlayerFlags() end

---For Player (player_guild_id).
---@return integer|nil value, string|nil reason
function Object:GuildId() end

---For Player (player_guild_rank).
---@return integer|nil value, string|nil reason
function Object:GuildRank() end

---For Player (player_chosen_title).
---@return integer|nil value, string|nil reason
function Object:ChosenTitle() end

---For Player (player_visible_item).
---@param index integer
---@return integer|nil value, string|nil reason
function Object:VisibleItem(index) end

---For Item, Container; Corpse (item_owner, corpse_owner).
---@return string|nil value, string|nil reason
function Object:Owner() end

---For Item, Container (item_contained_in).
---@return string|nil value, string|nil reason
function Object:ContainedIn() end

---For Item, Container (item_creator).
---@return string|nil value, string|nil reason
function Object:Creator() end

---For Item, Container (item_stack_count).
---@return integer|nil value, string|nil reason
function Object:StackCount() end

---For Item, Container (item_duration).
---@return integer|nil value, string|nil reason
function Object:Duration() end

---For Item, Container (item_durability).
---@return integer|nil value, string|nil reason
function Object:Durability() end

---For Item, Container (item_max_durability).
---@return integer|nil value, string|nil reason
function Object:MaxDurability() end

---For Container (container_num_slots).
---@return integer|nil value, string|nil reason
function Object:NumSlots() end

---For Container (container_slot).
---@param index integer
---@return string|nil value, string|nil reason
function Object:Slot(index) end

---For GameObject (gameobject_faction).
---@return integer|nil value, string|nil reason
function Object:Faction() end

---For GameObject (gameobject_state).
---@return integer|nil value, string|nil reason
function Object:State() end

---For GameObject (gameobject_type).
---@return integer|nil value, string|nil reason
function Object:GameObjectType() end

---For GameObject (gameobject_art_kit).
---@return integer|nil value, string|nil reason
function Object:ArtKit() end

---For DynamicObject (dynamic_caster).
---@return string|nil value, string|nil reason
function Object:Caster() end

---For DynamicObject (dynamic_spell_id).
---@return integer|nil value, string|nil reason
function Object:SpellId() end

---For DynamicObject (dynamic_radius).
---@return number|nil value, string|nil reason
function Object:Radius() end

---For DynamicObject (dynamic_cast_time).
---@return integer|nil value, string|nil reason
function Object:CastTime() end

---For Corpse (corpse_party).
---@return string|nil value, string|nil reason
function Object:Party() end

---For Corpse (corpse_guild).
---@return integer|nil value, string|nil reason
function Object:Guild() end

---@class wxl.ModInfo
---@field id string
---@field name string
---@field version string
---@field directory string

---@class wxl.RuntimeInfo
---@field id string
---@field name string
---@field version integer
---@field abi string the contract this mod is served, "major.minor.feature"
---@field abi_major integer
---@field abi_minor integer
---@field abi_feature integer

---@class wxl.object
local object = {}

---The object the local player controls, or nil while out of the world.
---@return wxl.Object|nil
function object.LocalPlayer() end

---The object carrying a guid, or nil when it is not in view.
---@param guid string|integer "0x" and up to sixteen hex digits, or a whole number
---@return wxl.Object|nil
function object.FromGuid(guid) end

---The objects of a type in view, or every object.
---@param type? wxl.ObjectType|integer
---@return wxl.Object[]
function object.InView(type) end

---One element of a field of every object of a list, in its order, in one call; false
---for an object that has none.
---@param objects wxl.Object[]
---@param name wxl.FieldName|string
---@param element? integer
---@return (wxl.FieldValue|false)[]
function object.Fields(objects, name, element) end

---Asks for ObjectScript's OnObjectFieldChanged whenever the field changes on an object of
---the type (nil for every object), from now on, for every mod observing it.
---@param type wxl.ObjectType|"Object"|nil
---@param name wxl.FieldName|string
---@return boolean|nil watched, string|nil reason
function object.Watch(type, name) end

---@class wxl
---@field mod wxl.ModInfo this mod, from its manifest or its folder
---@field runtime wxl.RuntimeInfo the runtime that loaded wxl.lua
---@field object wxl.object
---@field field wxl.field every update field's dword index, by kind
---@field events table<wxl.Family, string[]> every family's events, as On<Event>
wxl = {}

---Writes a line to the runtime's log, tagged with the mod's id.
---@param level "trace"|"debug"|"info"|"warn"|"error"
---@param ... any
function wxl.log(level, ...) end
