/*
 * debug_inventories.cpp
 *
 *  Created on: Feb 1, 2017
 *      Author: mazzin
 */
#include "debug_inventories.h"

#include "agent.h"
#include "dprint.h"
#include "instantiation.h"
#include "misc.h"
#include "preference.h"
#include "symbol_manager.h"
#include "output_manager.h"
#include "working_memory.h"

#include <assert.h>
#include <string>

#ifdef DEBUG_TRACE_REFCOUNT_FOR

    static int64_t debug_last_refcount = 0;
    static int64_t debug_last_refcount2 = 0;

    void debug_refcount_change_start(agent* thisAgent, bool twoPart)
    {
        int lSymNum;
        std::string numString;
        numString.push_back(DEBUG_TRACE_REFCOUNT_FOR[1]);
        if (!from_string(lSymNum, std::string(numString)) || (lSymNum < 1) ) assert(false);
        Symbol *sym = thisAgent->symbolManager->find_identifier(DEBUG_TRACE_REFCOUNT_FOR[0], 1);
        if (sym)
        {
            int64_t* last_count = twoPart ? &(debug_last_refcount2) : &(debug_last_refcount);
            (*last_count) = sym->reference_count;
        };
    }
    void debug_refcount_change_end(agent* thisAgent, const char* callerString, bool twoPart)
    {
        int lSymNum;
        std::string numString;
        numString.push_back(DEBUG_TRACE_REFCOUNT_FOR[1]);
        if (!from_string(lSymNum, std::string(numString)) || (lSymNum < 1) ) assert(false);
        Symbol *sym = thisAgent->symbolManager->find_identifier(DEBUG_TRACE_REFCOUNT_FOR[0], 1);
        if (sym)
        {
            int64_t new_count = static_cast<int64_t>(sym->reference_count);
            int64_t* last_count = twoPart ? &(debug_last_refcount2) : &(debug_last_refcount);
            if (new_count != (*last_count))
            {
                dprint_noprefix(DT_ID_LEAKING, "%s Reference count of %s changed (%d -> %d) by %d\n", callerString, DEBUG_TRACE_REFCOUNT_FOR,
                    (*last_count), new_count, (new_count - (*last_count)));
                if (std::string(callerString) == std::string("DEALLOCATED INST preference deallocation")) break_if_id_matches(1, 1);
            }
            (*last_count) = 0;
            if (twoPart) debug_last_refcount2 = debug_last_refcount2 + (new_count - debug_last_refcount);
        };
    }
    void debug_refcount_reset()
    {
        debug_last_refcount = 0;
        debug_last_refcount2 = 0;
    }

#else
    void debug_refcount_change_start(agent* thisAgent, const char* symString, bool twoPart) {}
    void debug_refcount_change_end(agent* thisAgent, const char* symString, const char* callerString, bool twoPart) {}
    void debug_refcount_reset() {}
#endif

#ifdef DEBUG_INSTANTIATION_INVENTORY
    id_to_sym_map inst_deallocation_map;

    void IDI_add(agent* thisAgent, instantiation* pInst)
    {
        inst_deallocation_map[pInst->i_id] = pInst->prod_name;
        thisAgent->symbolManager->symbol_add_ref(pInst->prod_name);
    }
    void IDI_remove(agent* thisAgent, uint64_t pID)
    {
        auto it = inst_deallocation_map.find(pID);
        assert (it != inst_deallocation_map.end());
        Symbol* lSym = it->second;
        if (lSym)
        {
            thisAgent->symbolManager->symbol_remove_ref(&lSym);
        } else {
            thisAgent->outputManager->printa_sf(thisAgent, "Instantiation %u was deallocated twice!\n", it->first);
        }
        inst_deallocation_map[pID] = NULL;
    }
    void IDI_print_and_cleanup(agent* thisAgent)
    {
        Symbol* lSym;
        thisAgent->outputManager->printa_sf(thisAgent, "Looking for instantiations that were not deallocated...\n");
        for (auto it = inst_deallocation_map.begin(); it != inst_deallocation_map.end(); ++it)
        {
            lSym = it->second;
            if (lSym != NULL)
            {
                thisAgent->outputManager->printa_sf(thisAgent, "Instantiation %u (%y) was not deallocated!\n", it->first, lSym);
                thisAgent->symbolManager->symbol_remove_ref(&lSym);
            }
        }
        inst_deallocation_map.clear();
    }
#else
    void IDI_add(agent* thisAgent, instantiation* pInst) {}
    void IDI_remove(agent* thisAgent, uint64_t pID) {}
    void IDI_print_and_cleanup(agent* thisAgent) {}
#endif

#ifdef DEBUG_PREFERENCE_INVENTORY
    id_to_string_map pref_deallocation_map;

    uint64_t PDI_id_counter = 0;

    void PDI_add(agent* thisAgent, preference* pPref, bool isShallow)
    {
        std::string lPrefString;
        pPref->p_id = ++PDI_id_counter;
        thisAgent->outputManager->sprinta_sf(thisAgent, lPrefString, "%u: %p", pPref->p_id, pPref);
//        dprint(DT_DEBUG, "%u:%s%p\n", pPref->p_id, isShallow ? " shallow " : " ", pPref);
        pref_deallocation_map[pPref->p_id] = lPrefString;
    }
    void PDI_remove(agent* thisAgent, preference* pPref)
    {
        auto it = pref_deallocation_map.find(pPref->p_id);
        assert (it != pref_deallocation_map.end());
//        if (it == pref_deallocation_map.end())
//        {
//            dprint(DT_DEBUG, "Did not find preference to remove!  %p\nRemaining preference deallocation map:\n", pPref);
//            std::string lPrefString;
//            for (auto it = pref_deallocation_map.begin(); it != pref_deallocation_map.end(); ++it)
//            {
//                lPrefString = it->second;
//                if (!lPrefString.empty()) dprint(DT_DEBUG, "%u: %s\n", it->first, lPrefString.c_str());
//            }
//            return;
//        }
        std::string lPrefString = it->second;
        if (!lPrefString.empty())
        {
            pref_deallocation_map[pPref->p_id].clear();
        } else {
            thisAgent->outputManager->printa_sf(thisAgent, "Preferences %u was deallocated twice!\n", it->first);
        }
    }

    void PDI_print_and_cleanup(agent* thisAgent)
    {
        std::string lPrefString;
        uint64_t bugCount = 0;
        thisAgent->outputManager->printa_sf(thisAgent, "Looking for preferences that were not deallocated...\n");
        for (auto it = pref_deallocation_map.begin(); it != pref_deallocation_map.end(); ++it)
        {
            lPrefString = it->second;
            if (!lPrefString.empty())
            {
                bugCount++;
            }
        }
        if (bugCount <= 23)
        {
            for (auto it = pref_deallocation_map.begin(); it != pref_deallocation_map.end(); ++it)
            {
                lPrefString = it->second;
                if (!lPrefString.empty()) thisAgent->outputManager->printa_sf(thisAgent, "Preference %u was not deallocated: %s!\n", it->first, lPrefString.c_str());
            }
        }
        thisAgent->outputManager->printa_sf(thisAgent, "Preference inventory result:  %u/%u were not deallocated.\n", bugCount, PDI_id_counter);
        pref_deallocation_map.clear();
    }
#else
    void PDI_add(agent* thisAgent, preference* pPref, bool isShallow) {}
    void PDI_remove(agent* thisAgent, preference* pPref) {}
    void PDI_print_and_cleanup(agent* thisAgent) {}
#endif

#ifdef DEBUG_WME_INVENTORY
    id_to_string_map wme_deallocation_map;

    uint64_t WDI_id_counter = 0;

    void WDI_add(agent* thisAgent, wme* pWME)
    {
        std::string lWMEString;
        pWME->w_id = ++WDI_id_counter;
        thisAgent->outputManager->sprinta_sf(thisAgent, lWMEString, "%u: %w", pWME->w_id, pWME);
//        dprint(DT_DEBUG, "%u: %w\n", pWME->w_id, pWME);
        wme_deallocation_map[pWME->w_id] = lWMEString;
    }
    void WDI_remove(agent* thisAgent, wme* pWME)
    {
        auto it = wme_deallocation_map.find(pWME->w_id);
        assert (it != wme_deallocation_map.end());
//        if (it == wme_deallocation_map.end())
//        {
//            dprint(DT_DEBUG, "Did not find preference to remove!  %p\nRemaining preference deallocation map:\n", pWME);
//            std::string lPrefString;
//            for (auto it = wme_deallocation_map.begin(); it != wme_deallocation_map.end(); ++it)
//            {
//                lPrefString = it->second;
//                if (!lPrefString.empty()) dprint(DT_DEBUG, "%u: %s\n", it->first, lPrefString.c_str());
//            }
//            return;
//        }
        std::string lPrefString = it->second;
        if (!lPrefString.empty())
        {
            wme_deallocation_map[pWME->w_id].clear();
        } else {
            thisAgent->outputManager->printa_sf(thisAgent, "WME %u was deallocated twice!\n", it->first);
        }
    }

    void WDI_print_and_cleanup(agent* thisAgent)
    {
        std::string lWMEString;
        uint64_t bugCount = 0;
        thisAgent->outputManager->printa_sf(thisAgent, "Looking for WMEs that were not deallocated...\n");
        for (auto it = wme_deallocation_map.begin(); it != wme_deallocation_map.end(); ++it)
        {
            lWMEString = it->second;
            if (!lWMEString.empty())
            {
                bugCount++;
            }
        }
        if (bugCount <= 23)
        {
            for (auto it = wme_deallocation_map.begin(); it != wme_deallocation_map.end(); ++it)
            {
                lWMEString = it->second;
                if (!lWMEString.empty()) thisAgent->outputManager->printa_sf(thisAgent, "WME %u was not deallocated: %s!\n", it->first, lWMEString.c_str());
            }
        }
        thisAgent->outputManager->printa_sf(thisAgent, "WME inventory result:  %u/%u were not deallocated.\n", bugCount, WDI_id_counter);
        wme_deallocation_map.clear();
    }
#else
    void WDI_add(agent* thisAgent, wme* pWME) {}
    void WDI_remove(agent* thisAgent, wme* pWME) {}
    void WDI_print_and_cleanup(agent* thisAgent) {}
#endif

