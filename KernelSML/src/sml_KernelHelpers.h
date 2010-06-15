/*************************************************************************
 * PLEASE SEE THE FILE "license.txt" (INCLUDED WITH THIS SOFTWARE PACKAGE)
 * FOR LICENSE AND COPYRIGHT INFORMATION. 
 *************************************************************************/

/********************************************************************
* @file gski_donottouch.h
*********************************************************************
* created:	   6/27/2002   10:44
*
* purpose: 
*********************************************************************/
#ifndef GSKI_DONOTTOUCH_H
#define GSKI_DONOTTOUCH_H

typedef struct production_struct production;
typedef unsigned char wme_trace_type;
typedef struct rete_node_struct rete_node;
typedef unsigned char ms_trace_type;
typedef struct agent_struct agent;

#include <string>

// Included because we need XMLCallbackData defined in KernelSML/CLI
#include "xml.h"
namespace soarxml
{
  class XMLTrace ;
}

namespace sml
{
  class AgentSML ;
  class KernelSML ;

  class KernelHelpers
  {
  public:

     /**
     * @brief
     */
     virtual ~KernelHelpers(){}

     /**
     * @brief
     */
     KernelHelpers(){}

	 void SetVerbosity(AgentSML* pIAgent, bool setting);
	 bool GetVerbosity(AgentSML* pIAgent);

	 bool BeginTracingProduction(AgentSML* pIAgent, const char* pProductionName);
	 bool StopTracingProduction(AgentSML* pIAgent, const char* pProductionName);

	 int AddWMEFilter(AgentSML* pIAgent, const char *pIdString, const char *pAttrString, const char *pValueString, bool adds, bool removes);
	 int RemoveWMEFilter(AgentSML* pIAgent, const char *pIdString, const char *pAttrString, const char *pValueString, bool adds, bool removes);
	 bool ResetWMEFilters(AgentSML* pIAgent, bool adds, bool removes);
	 void ListWMEFilters(AgentSML* pIAgent, bool adds, bool removes);

	 void ExplainListChunks(AgentSML* pIAgent);
	 bool ExplainChunks(AgentSML* pIAgent, const char* pProduction, int mode);
	 void print_rl_rules( agent* thisAgent, char *arg,bool internal, bool print_filename, bool full_prod);

  };
}
#endif

