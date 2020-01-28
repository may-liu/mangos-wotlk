/* This file is part of the ScriptDev2 Project. See AUTHORS file for Copyright information
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Battleground
SD%Complete: 100
SDComment: Spirit guides in battlegrounds will revive all players every 30 sec
SDCategory: Battlegrounds
EndScriptData */

#include "AI/ScriptDevAI/include/sc_common.h"
#include "Spells/Scripts/SpellScript.h"

// **** Script Info ****
// Spiritguides in battlegrounds resurrecting many players at once
// every 30 seconds - through a channeled spell, which gets autocasted
// the whole time
// if spiritguide despawns all players around him will get teleported
// to the next spiritguide
// here i'm not sure, if a dummyspell exist for it

// **** Quick Info ****
// battleground spiritguides - this script handles gossipHello
// and JustDied also it let autocast the channel-spell

enum
{
    SPELL_SPIRIT_HEAL_CHANNEL       = 22011,                // Spirit Heal Channel

    SPELL_SPIRIT_HEAL               = 22012,                // Spirit Heal
    SPELL_SPIRIT_HEAL_MANA          = 44535,                // in battlegrounds player get this no-mana-cost-buff

    SPELL_WAITING_TO_RESURRECT      = 2584                  // players who cancel this aura don't want a resurrection
};

struct npc_spirit_guideAI : public ScriptedAI
{
    npc_spirit_guideAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        pCreature->SetActiveObjectState(true);
        Reset();
    }

    void Reset() override {}

    void UpdateAI(const uint32 /*uiDiff*/) override
    {
        // auto cast the whole time this spell
        if (!m_creature->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
            m_creature->CastSpell(m_creature, SPELL_SPIRIT_HEAL_CHANNEL, TRIGGERED_NONE);
    }

    void CorpseRemoved(uint32&) override
    {
        // TODO: would be better to cast a dummy spell
        Map* pMap = m_creature->GetMap();

        if (!pMap || !pMap->IsBattleGround())
            return;

        Map::PlayerList const& PlayerList = pMap->GetPlayers();

        for (const auto& itr : PlayerList)
        {
            Player* pPlayer = itr.getSource();
            if (!pPlayer || !pPlayer->IsWithinDistInMap(m_creature, 20.0f) || !pPlayer->HasAura(SPELL_WAITING_TO_RESURRECT))
                continue;

            // repop player again - now this node won't be counted and another node is searched
            pPlayer->RepopAtGraveyard();
        }
    }

    void SpellHitTarget(Unit* pUnit, const SpellEntry* pSpellEntry) override
    {
        if (pSpellEntry->Id == SPELL_SPIRIT_HEAL && pUnit->GetTypeId() == TYPEID_PLAYER
                && pUnit->HasAura(SPELL_WAITING_TO_RESURRECT))
            pUnit->CastSpell(pUnit, SPELL_SPIRIT_HEAL_MANA, TRIGGERED_OLD_TRIGGERED);
    }
};

bool GossipHello_npc_spirit_guide(Player* pPlayer, Creature* /*pCreature*/)
{
    pPlayer->CastSpell(pPlayer, SPELL_WAITING_TO_RESURRECT, TRIGGERED_OLD_TRIGGERED);
    return true;
}

struct GYMidTrigger : public SpellScript
{
    void OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const
    {
        // TODO: Fix when go casting is fixed
        WorldObject* obj = spell->GetAffectiveCasterObject();
        if (obj->IsGameObject() && spell->GetUnitTarget()->IsPlayer())
        {
            Player* player = static_cast<Player*>(spell->GetUnitTarget());
            if (BattleGround* bg = player->GetBattleGround())
            {
                // check if it's correct bg
                if (bg->GetTypeID() == BATTLEGROUND_AV)
                    bg->EventPlayerClickedOnFlag(player, static_cast<GameObject*>(obj));
                return;
            }
        }
    }
};

void AddSC_battleground()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "npc_spirit_guide";
    pNewScript->GetAI = &GetNewAIInstance<npc_spirit_guideAI>;
    pNewScript->pGossipHello = &GossipHello_npc_spirit_guide;
    pNewScript->RegisterSelf();

    RegisterSpellScript<GYMidTrigger>("spell_gy_mid_trigger");
}
