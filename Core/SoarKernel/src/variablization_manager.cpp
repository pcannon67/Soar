/*
 * Original_Variable_Manager.cpp
 *
 *  Created on: Jul 25, 2013
 *      Author: mazzin
 */

#include "variablization_manager.h"
#include "agent.h"
#include "instantiations.h"
#include "assert.h"
#include "test.h"
#include "debug.h"

extern uint32_t hash_string (const char *s);
extern uint32_t hash_variable_raw_info (const char *name, short num_bits);
extern uint32_t compress (uint32_t h, short num_bits);

inline uint32_t hash_unique_string (void *item, short num_bits) {
  original_symbol_data *var;
  var = static_cast<original_symbol_data *>(item);
  return compress (hash_string(var->name),num_bits);
}

inline variablization * copy_variablization(agent *thisAgent, variablization *v)
{
  variablization *new_variablization = new variablization;
  new_variablization->instantiated_symbol = v->instantiated_symbol;
  new_variablization->variablized_symbol = v->variablized_symbol;
  symbol_add_ref(thisAgent, new_variablization->instantiated_symbol);
  symbol_add_ref(thisAgent, new_variablization->variablized_symbol);
  new_variablization->grounded = v->grounded;
  new_variablization->grounding_id= v->grounding_id;
  return new_variablization;
}

Variablization_Manager::Variablization_Manager(agent *myAgent)
{
  thisAgent = myAgent;
  create_OS_hashtable();
  variablization_sym_table = new std::map< Symbol *, variablization * >();
  variablization_g_id_table = new std::map< uint64_t, variablization * >();
  variablization_ovar_table = new std::map< char *, uint64_t >();
  current_unique_vars = new std::set< Symbol *>();
  ground_id_counter = 0;
}

Variablization_Manager::~Variablization_Manager()
{
  clear_data();
  delete variablization_sym_table;
  delete variablization_g_id_table;
  delete variablization_ovar_table;
  delete current_unique_vars;
}

void Variablization_Manager::clear_data()
{
  dprint(DT_VARIABLIZATION_MANAGER, "Clearing hash table and variablization maps.\n");
  clear_variablization_table();
  if (original_symbol_ht)
    clear_OS_hashtable();
}

void Variablization_Manager::reinit()
{
  dprint(DT_VARIABLIZATION_MANAGER, "Original_Variable_Manager reinitializing...\n");
  clear_data();
  create_OS_hashtable();
  ground_id_counter = 0;
}

/* -- ----------------------------------
 *    Variablization functions
 *    ----------------------------------
 *    The following functions handle variablization of LHS items.  It replaces
 *    variablize_symbol.
 *
 *    -- */


void Variablization_Manager::clear_variablization_table() {

  dprint(DT_VARIABLIZATION_MANAGER, "Original_Variable_Manager clearing variablization data...\n");
  print_variablization_tables(DT_VARIABLIZATION_MANAGER);

  dprint(DT_VARIABLIZATION_MANAGER, "Original_Variable_Manager clearing ovar table...\n");
  /* -- Clear original variable map -- */
  for (std::map< char *, uint64_t >::iterator it=(*variablization_ovar_table).begin(); it!=(*variablization_ovar_table).end(); ++it)
  {
    dprint(DT_VARIABLIZATION_MANAGER, "Clearing %s -> %llu\n", it->first, it->second);
    free_memory_block_for_string(thisAgent, it->first);
  }
  variablization_ovar_table->clear();

  dprint(DT_VARIABLIZATION_MANAGER, "Original_Variable_Manager clearing symbol->variablization map...\n");
  /* -- Clear symbol->variablization map -- */
  for (std::map< Symbol *, variablization * >::iterator it=(*variablization_sym_table).begin(); it!=(*variablization_sym_table).end(); ++it)
  {
    dprint(DT_VARIABLIZATION_MANAGER, "Clearing %s -> %s(%lld)/%s(%lld)\n",
        it->first->to_string(thisAgent),
        it->second->instantiated_symbol->to_string(thisAgent), it->second->instantiated_symbol->reference_count,
        it->second->variablized_symbol->to_string(thisAgent),  it->second->variablized_symbol->reference_count);
    symbol_remove_ref(thisAgent, it->second->instantiated_symbol);
    symbol_remove_ref(thisAgent, it->second->variablized_symbol);
    delete it->second;
  }
  variablization_sym_table->clear();

  dprint(DT_VARIABLIZATION_MANAGER, "Original_Variable_Manager clearing grounding_id->variablization map...\n");
  /* -- Clear grounding_id->variablization map -- */
  for (std::map< uint64_t, variablization * >::iterator it=(*variablization_g_id_table).begin(); it!=(*variablization_g_id_table).end(); ++it)
  {
    dprint(DT_VARIABLIZATION_MANAGER, "Clearing %llu -> %s(%lld)/%s(%lld)\n",
        it->first,
        it->second->instantiated_symbol->to_string(thisAgent), it->second->instantiated_symbol->reference_count,
        it->second->variablized_symbol->to_string(thisAgent),  it->second->variablized_symbol->reference_count);
    symbol_remove_ref(thisAgent, it->second->instantiated_symbol);
    symbol_remove_ref(thisAgent, it->second->variablized_symbol);
    delete it->second;
  }
  variablization_g_id_table->clear();
  dprint(DT_VARIABLIZATION_MANAGER, "Original_Variable_Manager done clearing variablization data.\n");
}

variablization * Variablization_Manager::get_variablization(uint64_t index_id)
{
  std::map< uint64_t, variablization * >::iterator iter = (*variablization_g_id_table).find(index_id);
  if (iter != (*variablization_g_id_table).end())
  {
    dprint(DT_VARIABLIZATION_MANAGER, "...found %llu in g_id variablization table: %s/%s\n", index_id,
       iter->second->variablized_symbol->to_string(thisAgent), iter->second->instantiated_symbol->to_string(thisAgent));
      return iter->second;
  }
  else
  {
    dprint(DT_VARIABLIZATION_MANAGER, "...did not find %llu in g_id variablization table.\n", index_id);
    print_variablization_tables(DT_VARIABLIZATION_MANAGER, 2);
    return NULL;
  }
}

variablization * Variablization_Manager::get_variablization(Symbol *index_sym)
{
  std::map< Symbol *, variablization * >::iterator iter = (*variablization_sym_table).find(index_sym);
  if (iter != (*variablization_sym_table).end())
  {
    dprint(DT_VARIABLIZATION_MANAGER, "...found %s in variablization table: %s/%s\n", index_sym->to_string(thisAgent),
       iter->second->variablized_symbol->to_string(thisAgent), iter->second->instantiated_symbol->to_string(thisAgent));
      return iter->second;
  }
  else
  {
    dprint(DT_VARIABLIZATION_MANAGER, "...did not find %s in variablization table.\n", index_sym->to_string(thisAgent));
    print_variablization_tables(DT_VARIABLIZATION_MANAGER, 1);
    return NULL;
  }
}

uint64_t Variablization_Manager::get_gid_for_orig_var(char *index_var)
{
  for (std::map< char *, uint64_t >::iterator it=(*variablization_ovar_table).begin(); it!=(*variablization_ovar_table).end(); ++it)
  {
    if (!strcmp(it->first, index_var))
    {
      dprint(DT_VARIABLIZATION_MANAGER, "...found %llu in orig_var variablization table for %s\n",
         it->second, index_var);
        return it->second;
    }
  }
  dprint(DT_VARIABLIZATION_MANAGER, "...did not find %s in orig_var variablization table.\n", index_var);
  print_variablization_tables(DT_VARIABLIZATION_MANAGER, 3);

  return 0;

//  std::map< char *, uint64_t >::iterator iter = (*variablization_ovar_table).find(index_var);
//  if (iter != (*variablization_ovar_table).end())
//  {
//    dprint(DT_VARIABLIZATION_MANAGER, "...found %llu in g_id variablization table for %s\n",
//       iter->second, index_var);
//      return iter->second;
//  }
//  else
//  {
//    dprint(DT_VARIABLIZATION_MANAGER, "...did not find %s in g_id variablization table.\n", index_var);
//    thisAgent->variablizationManager->print_variablization_tables(DT_VARIABLIZATION_MANAGER, 3);
//
//    return 0;
//  }
}

void Variablization_Manager::add_orig_var_mappings_for_test(test t)
{
  cons *c;
  test check_test;

  switch (t->type)
  {
    case DISJUNCTION_TEST:
    case GOAL_ID_TEST:
    case IMPASSE_ID_TEST:
      break;
    case CONJUNCTIVE_TEST:
      dprint(DT_VARIABLIZATION_MANAGER, "Adding original variable mappings for conjunctive test\n");
      cons *c;
      test check_test;
      for (c=t->data.conjunct_list; c!=NIL; c=c->rest)
      {
        add_orig_var_mappings_for_test(static_cast<test>(c->first));
      }
      break;
    default:
      assert(t->data.referent);
//      dprint(DT_VARIABLIZATION_MANAGER, "Adding original variable mappings for test with referent.\n");
      if (t->identity && t->identity->original_var && (t->identity->grounding_id > 0))
      {
        dprint(DT_VARIABLIZATION_MANAGER, "Adding original variable mappings entry: %s -> %llu\n", t->identity->original_var, t->identity->grounding_id);
        (*variablization_ovar_table)[make_memory_block_for_string(thisAgent, t->identity->original_var)] = t->identity->grounding_id;
      } else {
//        dprint(DT_VARIABLIZATION_MANAGER, "Did not add b/c %s %s %llu.\n",
//            (t->identity ? "True" : "False"),
//            ((t->identity && t->identity->original_var) ? t->identity->original_var : "No orig var"),
//            ((t->identity && t->identity->grounding_id) ? t->identity->grounding_id : 0));
      }
  }
}

void Variablization_Manager::add_orig_var_mappings_for_cond(condition *cond)
{
  switch (cond->type) {
  case POSITIVE_CONDITION:
  case NEGATIVE_CONDITION:
    add_orig_var_mappings_for_test(cond->data.tests.id_test);
    add_orig_var_mappings_for_test(cond->data.tests.attr_test);
    add_orig_var_mappings_for_test(cond->data.tests.value_test);
    break;
  case CONJUNCTIVE_NEGATION_CONDITION:
    add_orig_var_mappings_for_cond_list (cond->data.ncc.top);
    break;
  }
}

void Variablization_Manager::add_orig_var_mappings_for_cond_list(condition *cond)
{
  dprint(DT_VARIABLIZATION_MANAGER, "=============================================\n");
  dprint(DT_VARIABLIZATION_MANAGER, "add_orig_var_mappings_for_cond_list called...\n");
  print_variablization_tables(DT_VARIABLIZATION_MANAGER, 3);
  while (cond) {
    dprint(DT_VARIABLIZATION_MANAGER, "Adding original variable mappings for cond ");
    dprint_condition(DT_VARIABLIZATION_MANAGER, cond, "", true, true, true);
    add_orig_var_mappings_for_cond(cond);
    cond = cond->next;
  }
  dprint(DT_VARIABLIZATION_MANAGER, "Done adding original var mappings.\n");
  print_variablization_tables(DT_VARIABLIZATION_MANAGER, 3);
  dprint(DT_VARIABLIZATION_MANAGER, "=============================================\n");
}

void Variablization_Manager::store_variablization(Symbol *instantiated_sym,
                                                  Symbol *variable,
                                                  char *orig_varname,
                                                  identity_info *identity,
                                                  bool is_equality_test)
{
  variablization *new_variablization;
  assert(instantiated_sym && variable);
  dprint(DT_VARIABLIZATION_MANAGER, "Storing variablization for %s(%llu, %s) -=> %s (grounded %s) in ",
          instantiated_sym->to_string(thisAgent),
          identity->grounding_id, orig_varname,
          variable->to_string(thisAgent),
          (is_equality_test ? "T" : "F"));

  new_variablization = new variablization;
  new_variablization->instantiated_symbol = instantiated_sym;
  new_variablization->variablized_symbol = variable;
  symbol_add_ref(thisAgent, instantiated_sym);
  symbol_add_ref(thisAgent, variable);
  new_variablization->grounded = is_equality_test;

  if (instantiated_sym->is_sti())
  {
    /* -- STI may have more than one original symbol (mostly due to the fact
     *    that placeholder variables still exist to handle dot notation).  So, we
     *    look them up using the identifier symbol instead of the original variable.
     *
     *    Note that we also store an entry using the new variable as an index. Later,
     *    when looking for ungrounded variables in relational tests, the
     *    identifier symbol will have already been replaced with a variable,
     *    so we must use the variable instead to look up variablization info.
     *    This may not be necessary after we resurrect the old NOT code. -- */

    (*variablization_sym_table)[instantiated_sym] = new_variablization;
    (*variablization_sym_table)[variable] = copy_variablization(thisAgent, new_variablization);
    dprint_noprefix(DT_VARIABLIZATION_MANAGER, "symbol ([%s][%s] variablization table.\n",
        instantiated_sym->to_string(thisAgent), variable->to_string(thisAgent));
  } else {

    /* -- A constant symbol is being variablized, so store variablization info
     *    indexed by the constant's grounding id. -- */
    (*variablization_g_id_table)[identity->grounding_id] = new_variablization;

    /* -- Store variablization indexed original variable string.  This is used by
     *    RHS constant variablization.
     *
     *    Note:  Old system also needed this to reverse ungrounded relational
     *    tests -- */
//    (*variablization_ovar_table)[make_memory_block_for_string(thisAgent, orig_varname)] = identity->grounding_id;

    dprint_noprefix(DT_VARIABLIZATION_MANAGER, "identity[%llu] and original_var[%s] variablization tables.\n",
        identity->grounding_id, orig_varname);
  }
//  print_variablization_table();
}

/* -- variablize_rl_symbol is a very limited version of variablization for templates
 *    - The symbol passed in is guaranteed to be a short-term identifier.
 */
void Variablization_Manager::variablize_rl_symbol (Symbol **sym, bool is_equality_test)
{
  char prefix[2];
  Symbol *var;
  variablization *var_info;

  if (!(*sym)->is_sti()) return;

  dprint(DT_VARIABLIZATION_MANAGER, "Variablization_Manager variablizing rl symbol %s.\n", (*sym)->to_string(thisAgent));

  var_info = get_variablization((*sym));
  if (var_info)
  {
    if (is_equality_test && !var_info->grounded)
    {
      var_info->grounded = true;
      /* -- Update secondary index for identifiers -- */
      variablization *var_info2;
      dprint(DT_VARIABLIZATION_MANAGER, "...updating grounded info for %s %s %s.\n", (*sym)->to_string(thisAgent),
          var_info->variablized_symbol->to_string(thisAgent), (is_equality_test ? "T" : "F"));
      var_info2 = get_variablization(var_info->variablized_symbol);
      var_info2->grounded = true;
    }
    /* -- Symbol being passed in is being replaced, so decrease -- */
    /* -- and increase refcount for new variable symbol being returned -- */
    symbol_remove_ref (thisAgent, (*sym));
    *sym = var_info->variablized_symbol;
    symbol_add_ref(thisAgent, var_info->variablized_symbol);
    return;
  }

  /* --- need to create a new variable.  If constant is being variablized
   *     just used 'c' instead of first letter of id name --- */
  if((*sym)->is_identifier())
    prefix[0] = static_cast<char>(tolower((*sym)->id->name_letter));
  else
    prefix[0] = 'c';
  prefix[1] = 0;
  var = generate_new_variable (thisAgent, prefix);
  var->var->was_identifier = (*sym)->is_identifier();

  store_variablization((*sym), var, NULL, NULL, is_equality_test);

  dprint(DT_VARIABLIZATION_MANAGER, "...created new variablization %s.\n", var->to_string(thisAgent));

  /* MToDoRefCnt | This remove ref was removed before, but it seems like we should have it, no? */
  symbol_remove_ref (thisAgent, *sym);
  *sym = var;
}

void Variablization_Manager::variablize_lhs_symbol (Symbol **sym, Symbol *original_symbol, identity_info *identity, bool is_equality_test)
{
  char prefix[2];
  Symbol *var;
  variablization *var_info;
  bool is_st_id = (*sym)->is_sti();

  dprint(DT_VARIABLIZATION_MANAGER, "Variablizing %s(%llu, %s) %s.\n",
      (*sym)->to_string(thisAgent),
      (identity ? identity->grounding_id : 0),
      (original_symbol ? original_symbol->to_string(thisAgent) : "NULL"),
      (is_equality_test ? "T" : "F"));

  if (!is_st_id)
  {
    assert(identity);
    var_info = get_variablization(identity->grounding_id);
  } else {
    var_info = get_variablization(*sym);
  }
  if (var_info)
  {
    if (is_equality_test && !var_info->grounded)
    {
      var_info->grounded = true;
      if (is_st_id)
      {
        /* -- Update secondary index for identifiers -- */
        variablization *var_info2;
        dprint(DT_VARIABLIZATION_MANAGER, "...updating grounded info for %s/%s\n",
            (*sym)->to_string(thisAgent),
            var_info->variablized_symbol->to_string(thisAgent));
        var_info2 = get_variablization(var_info->variablized_symbol);
        var_info2->grounded = true;
      }
    }
    /* -- Symbol being passed in is being replaced, so decrease -- */
    /* -- and increase refcount for new variable symbol being returned -- */
    symbol_remove_ref (thisAgent, (*sym));
    *sym = var_info->variablized_symbol;
    symbol_add_ref(thisAgent, var_info->variablized_symbol);
    return;
  }

  /* --- need to create a new variable.  If constant is being variablized
   *     just used 'c' instead of first letter of id name --- */
  if((*sym)->is_identifier())
    prefix[0] = static_cast<char>(tolower((*sym)->id->name_letter));
  else
    prefix[0] = 'c';
  prefix[1] = 0;
  var = generate_new_variable (thisAgent, prefix);
  var->var->was_identifier = is_st_id;

  store_variablization((*sym), var, (identity ? identity->original_var : original_symbol->var->name), identity, is_equality_test);

  dprint(DT_VARIABLIZATION_MANAGER, "...created new variablization %s.\n", var->to_string(thisAgent));

  /* MToDoRefCnt | This remove ref was removed before, but it seems like we should have it, no? */
  symbol_remove_ref (thisAgent, *sym);
  *sym = var;
}
/* ======================================================================================================
 *
 *                                          variablize_rhs_symbol
 *
 *      The logic for variablizing the rhs is slightly different than the lhs since we need to
 *      match constants on the rhs with the conditions they match up with on the lhs. So, we match
 *      them up using the original variable names.
 *
 * ====================================================================================================== */

uint64_t Variablization_Manager::variablize_rhs_symbol (Symbol **sym, char *original_var) {
  char prefix[2];
  Symbol *var;
  variablization *found_variablization=NIL;
  bool is_st_id;
  uint64_t g_id;

  dprint(DT_VARIABLIZATION_MANAGER, "variablize_rhs_symbol called for %s(%s).\n",
      (*sym)->to_string(thisAgent),
      (original_var ? original_var : "NULL"));

  /* -- identifiers and unbound vars (which are instantiated as identifiers) are indexed by their symbol
   *    instead of their original variable. --  */
  is_st_id = (*sym)->is_sti();

  if (is_st_id)
  {
    dprint(DT_VARIABLIZATION_MANAGER, "...searching for sti %s in variablization sym table...\n", (*sym)->to_string(thisAgent));
    found_variablization = get_variablization(*sym);
  }
  else
  {
    if (original_var)
    {
      dprint(DT_VARIABLIZATION_MANAGER, "...searching for original var %s in variablization orig var table...\n", original_var);
      g_id = get_gid_for_orig_var(original_var);
      if (g_id > 0)
      {
        found_variablization = get_variablization(g_id);
      }
      else
      {
        dprint(DT_VARIABLIZATION_MANAGER, "...did not find entry for g_id %llu!  Not variablizing!\n", g_id);
        this->print_variablization_tables(DT_VARIABLIZATION_MANAGER, 2);
      }
    }
    else
    {
      dprint(DT_VARIABLIZATION_MANAGER, "...is a literal constant.  Not variablizing!\n");
      return 0;
    }
  }


  if (found_variablization)
  {
    if (found_variablization->grounded)
    {
      /* --- Grounded symbol that has been variablized before--- */

      dprint(DT_VARIABLIZATION_MANAGER, "... found existing grounded variablization %s.\n", found_variablization->variablized_symbol->to_string(thisAgent));

      symbol_add_ref(thisAgent, found_variablization->variablized_symbol);
      //symbol_remove_ref (thisAgent, (*sym));
      *sym = found_variablization->variablized_symbol;
      return found_variablization->grounding_id;
    }
    else if (!is_st_id)
    {
      dprint(DT_VARIABLIZATION_MANAGER, "...is ungrounded constant.  Not variablizing!\n");
      return 0;
    }
    else
    {
      /* -- Ungrounded short-term identifier
       *    This will pass through this case and create an unbound var in next code block. -- */

      /* -- Delete the symbol references for both entries in the variablization table
       *    then delete the entries themselves. */
      dprint(DT_VARIABLIZATION_MANAGER, "...is ungrounded identifier.  Clearing variablization entry for %s/%s and generating unbound var.\n",
          (*sym)->to_string(thisAgent), found_variablization->variablized_symbol->to_string(thisAgent));

      print_variablization_tables(DT_VARIABLIZATION_MANAGER, 1);
      variablization_sym_table->erase(*sym);
      variablization_sym_table->erase(found_variablization->variablized_symbol);
      symbol_remove_ref(thisAgent, found_variablization->variablized_symbol);
      symbol_remove_ref(thisAgent, found_variablization->instantiated_symbol);
      delete found_variablization;
      print_variablization_tables(DT_VARIABLIZATION_MANAGER, 1);
    }
  }

  /* -- Either the variablization manager has never seen this symbol or symbol is ungrounded symbol or literal constant.
   *    Both cases return 0.  Grounding id will be generate if requested by another match. -- */

  if((*sym)->is_sti())
  {
    /* -- First instance of an unbound rhs var -- */
    dprint(DT_VARIABLIZATION_MANAGER, "...is unbound variable.\n");
    prefix[0] = static_cast<char>(tolower((*sym)->id->name_letter));
    prefix[1] = 0;
    var = generate_new_variable (thisAgent, prefix);

    dprint(DT_VARIABLIZATION_MANAGER, "...created new variable for unbound rhs %s.\n", var->to_string(thisAgent));
    store_variablization((*sym), var, var->var->name, NULL, true);

    *sym = var;
  }
  else
  {
    /* -- RHS constant that was not in LHS condition.  -- */

    /* MToDo | Is this even possible?  Won't this be caught by not having an original var above? */
    dprint(DT_VARIABLIZATION_MANAGER, "...is a variable that did not appear in the LHS.  Not variablizing!\n");
  }
  return 0;
}


/* -- ----------------------------------
 *    Unique original variable functions
 *    ----------------------------------
 *    The following code is used when creating instantiations.  When the rete re-creates
 *    the production from a p-node, this function is used to make sure that the original variable
 *    names (the one that are in the original production), which are stored in the tests and RHS
 *    symbols are unique across instantiations but consistent within a particular
 *    instantiation, a property needed by the chunker to avoid conflation and ungrounded constants
 *    when variablizing LHS symbols and to match rhs bindings to the proper lhs bindings.
 * -- */

void Variablization_Manager::create_OS_hashtable()
{
  dprint(DT_UNIQUE_VARIABLIZATION, "Original_Variable_Manager creating hash table.\n");
  original_symbol_ht = make_hash_table (thisAgent, 0, hash_unique_string);
  init_memory_pool (thisAgent, &original_symbol_mp, sizeof(original_symbol_data), "unique_string");

}

bool free_original_symbol_data (agent* thisAgent, void *item, void*) {

  original_symbol_data *varname = static_cast<original_symbol_data *>(item);
  if (varname->current_unique_var_symbol)
  {
    dprint(DT_UNIQUE_VARIABLIZATION, "...decreasing refcount on symbol %s\n", varname->current_unique_var_symbol->to_string(thisAgent));
    symbol_remove_ref(thisAgent, varname->current_unique_var_symbol);
    varname->current_unique_var_symbol = NULL;
  }
  varname->current_instantiation = NULL;
  dprint(DT_UNIQUE_VARIABLIZATION, "...freeing memory for string %s\n", varname->name);
  free_memory_block_for_string(thisAgent, varname->name);
  return false;
}

void Variablization_Manager::clear_OS_hashtable()
{
  dprint(DT_UNIQUE_VARIABLIZATION, "Original_Variable_Manager clearing hash table of original_vars...\n");
  do_for_all_items_in_hash_table( thisAgent, original_symbol_ht, free_original_symbol_data, 0);

  free_memory(thisAgent, original_symbol_ht->buckets, HASH_TABLE_MEM_USAGE);
  free_memory(thisAgent, original_symbol_ht, HASH_TABLE_MEM_USAGE);
}

void Variablization_Manager::clear_CUV_for_symbol(Symbol *var)
{
  uint32_t hash_value;
  original_symbol_data *varname;

  hash_value = hash_variable_raw_info (var->var->name,original_symbol_ht->log2size);
  varname = reinterpret_cast<original_symbol_data *>(*(original_symbol_ht->buckets + hash_value));
  for ( ; varname != NIL; varname = varname->next_in_hash_table)
  {
    if (!strcmp(varname->name,var->var->name))
    {
      if (varname->current_unique_var_symbol)
      {
        dprint(DT_UNIQUE_VARIABLIZATION, "Original_Variable_Manager decreasing refcount on symbol %s\n", varname->current_unique_var_symbol->to_string(thisAgent));
        symbol_remove_ref(thisAgent, varname->current_unique_var_symbol);
        varname->current_unique_var_symbol = NULL;
      }
      varname->current_instantiation = NULL;
    }
  }
}
void Variablization_Manager::clear_CUV_cache() {

  dprint(DT_UNIQUE_VARIABLIZATION, "Original_Variable_Manager clearing unique var cache...\n");
  for (std::set< Symbol *>::iterator it=(*current_unique_vars).begin(); it!=(*current_unique_vars).end(); ++it)
  {
    dprint(DT_UNIQUE_VARIABLIZATION, "Erasing current_unique_var %s\n", (*it)->to_string(thisAgent));
  }
  current_unique_vars->clear();
}

bool Variablization_Manager::already_unique(Symbol *original_var) {

  dprint(DT_UNIQUE_VARIABLIZATION, "...checking if %s is already unique...", original_var->to_string(thisAgent));

  std::set< Symbol * >::iterator it = current_unique_vars->find(original_var);
  if (it != current_unique_vars->end()) {
    dprint_noprefix(DT_UNIQUE_VARIABLIZATION, " = TRUE\n");
    return true;
  }
  dprint_noprefix(DT_UNIQUE_VARIABLIZATION, " = FALSE\n");
  return false;
}

/* -- make_name_unique takes a symbol and replaces it with a unique version if it hasn't already
 *    been made unique for the current instantiation (thisAgent->newly_created_instantiations) -- */

void Variablization_Manager::make_name_unique(Symbol **sym)
{
  return;
  uint32_t hash_value;
  original_symbol_data *varname, *new_varname;

  dprint(DT_UNIQUE_VARIABLIZATION, "...uniqueifying %s for instantiation %s...\n",
                                           (*sym)->var->name,
                                           thisAgent->newly_created_instantiations->prod->name->sc->name );

  if (already_unique(*sym))
  {
    dprint(DT_UNIQUE_VARIABLIZATION, "...already unique, so using existing original variable %s\n",
                                      (*sym)->var->name);
    return;
  }

  hash_value = hash_variable_raw_info ((*sym)->var->name,original_symbol_ht->log2size);
  varname = reinterpret_cast<original_symbol_data *>(*(original_symbol_ht->buckets + hash_value));
  for ( ; varname != NIL; varname = varname->next_in_hash_table)
  {
    if (!strcmp(varname->name,(*sym)->var->name))
    {
      /* -- Found unique string record that matches original var name -- */

      if (varname->current_instantiation == thisAgent->newly_created_instantiations)
      {

        /* -- We've already created and cached a unique version of this variable name for this
         *    instantiation. Note that we do not need to increase refcount, since caller will
         *    increase the refcount again when it uses the symbol in a test. -- */

        dprint(DT_UNIQUE_VARIABLIZATION, "...found existing mapping %s -> %s for this instantiation.\n",
                (*sym)->var->name, varname->current_unique_var_symbol->var->name);
        *sym = varname->current_unique_var_symbol;

        return;
      }
      else
      {
        /* -- We need to create and cache a new unique version of this string
         *    for this instantiation -- */

        std::string suffix, new_name = (*sym)->var->name;

        /* -- Create a unique name by appending a numbered suffix to original var name -- */

        to_string(varname->next_unique_suffix_number, suffix);
        new_name.erase(new_name.end()-1);
        new_name += "+" + suffix + ">";

        /* -- Update the original_varname struct with a new variable and the current instantiation == */

        varname->next_unique_suffix_number++;
        varname->current_instantiation = thisAgent->newly_created_instantiations;

        /* MToDoRefCnt | After we clean up current_unique_vars in p_node, the following should never be necessary */
        if (varname->current_unique_var_symbol)
        {
          dprint(DT_UNIQUE_VARIABLIZATION, "...cleaning up current unique var still in variablization manager OSD table: %s\n", varname->current_unique_var_symbol->to_string(thisAgent));
          symbol_remove_ref(thisAgent, varname->current_unique_var_symbol);
        }
        varname->current_unique_var_symbol = make_variable(thisAgent, new_name.c_str());

        dprint(DT_UNIQUE_VARIABLIZATION, "...creating new unique version of %s: %s\n",
                                          (*sym)->var->name, new_name.c_str());

        *sym = varname->current_unique_var_symbol;
        current_unique_vars->insert(*sym);

        return;
      }
    }
  }

  /* -- var name was not found in the hash table, so add to hash table and leave original_varsym untouched -- */

  allocate_with_pool (thisAgent, &original_symbol_mp, &new_varname);
  new_varname->current_instantiation = thisAgent->newly_created_instantiations;
  new_varname->current_unique_var_symbol = (*sym);
  new_varname->name = make_memory_block_for_string (thisAgent, (*sym)->var->name);
  new_varname->next_unique_suffix_number = 1;
  add_to_hash_table (thisAgent, original_symbol_ht, new_varname);
  /* -- Increase refcount for cached current_unique_var_symbol -- */
  symbol_add_ref(thisAgent, (*sym));
  current_unique_vars->insert(*sym);
  dprint(DT_UNIQUE_VARIABLIZATION, "...first use, so using original variable %s\n",
                                    (*sym)->var->name);
}

/* -- A utility function to print all data stored in the variablization manager.  Used only for debugging -- */

bool print_original_symbol_data (agent* thisAgent, void *item, void*) {

  original_symbol_data *varname = static_cast<original_symbol_data *>(item);

  dprint(DT_VARIABLIZATION_MANAGER, "%s, CurrUnqVarSym: %s(%lld) CurrInst: %d\n",
        varname->name,
        (varname->current_unique_var_symbol ? varname->current_unique_var_symbol->to_string(thisAgent) : "None"),
        (varname->current_unique_var_symbol ? varname->current_unique_var_symbol->reference_count : 0),
        (varname->current_instantiation ? varname->current_instantiation : NULL));
  return false;
}

void Variablization_Manager::print_OSD_table()
{
  dprint(DT_VARIABLIZATION_MANAGER, "------------------------------------\n");
  dprint(DT_VARIABLIZATION_MANAGER, "   Variablization OSD Hash Table\n");
  dprint(DT_VARIABLIZATION_MANAGER, "------------------------------------\n");
  do_for_all_items_in_hash_table( thisAgent, original_symbol_ht, print_original_symbol_data, 0);
}

void Variablization_Manager::print_variablization_tables(TraceMode mode, int whichTable)
{
  dprint(mode, "------------------------------------\n");
  if (whichTable == 0)
  {
    dprint(mode, "       Variablization Tables\n");
    dprint(mode, "------------------------------------\n");
  }
  if ((whichTable == 0) || (whichTable == 1))
  {
    dprint(mode, "------------ Symbol -> v_info table ----------\n");
    if (whichTable != 0)
      dprint(mode, "------------------------------------\n");
    for (std::map< Symbol *, variablization * >::iterator it=(*variablization_sym_table).begin(); it!=(*variablization_sym_table).end(); ++it)
    {
      dprint(mode, "%s -> %s/%s (grounded %d)\n", it->first->to_string(thisAgent),
          it->second->variablized_symbol->to_string(thisAgent), it->second->instantiated_symbol->to_string(thisAgent), it->second->grounded);
    }
  }
  if ((whichTable == 0) || (whichTable == 2))
  {
    dprint(mode, "--------- G_ID -> v_info table -------\n");
    if (whichTable != 0)
      dprint(mode, "------------------------------------\n");
    for (std::map< uint64_t, variablization * >::iterator it=(*variablization_g_id_table).begin(); it!=(*variablization_g_id_table).end(); ++it)
    {
      dprint(mode, "%llu -> %s/%s (grounded %d)\n", it->first,
          it->second->variablized_symbol->to_string(thisAgent), it->second->instantiated_symbol->to_string(thisAgent), it->second->grounded);
    }
  }
  if ((whichTable == 0) || (whichTable == 3))
  {
    dprint(mode, "----- Original Var -> G_ID Table -----\n");
    if (whichTable != 0)
      dprint(mode, "------------------------------------\n");
    for (std::map< char *, uint64_t >::iterator it=(*variablization_ovar_table).begin(); it!=(*variablization_ovar_table).end(); ++it)
    {
      dprint(mode, "%s -> %llu\n", it->first, it->second);
    }
  }
  dprint(mode, "------------------------------------\n");
}
void Variablization_Manager::print_CUV_table() {

  dprint(DT_VARIABLIZATION_MANAGER, "------------------------------------\n");
  dprint(DT_VARIABLIZATION_MANAGER, "   Current Unique Variable Table\n");
  dprint(DT_VARIABLIZATION_MANAGER, "------------------------------------\n");
  for (std::set< Symbol *>::iterator it=(*current_unique_vars).begin(); it!=(*current_unique_vars).end(); ++it)
  {
    dprint(DT_VARIABLIZATION_MANAGER, "%s\n", (*it)->to_string(thisAgent));
  }
}

void Variablization_Manager::print_tables()
{
  print_OSD_table();
  print_variablization_tables(DT_VARIABLIZATION_MANAGER);
  print_CUV_table();
}
