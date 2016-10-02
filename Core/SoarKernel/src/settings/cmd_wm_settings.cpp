#include "agent.h"
#include "output_manager.h"
#include "cmd_settings.h"

//#include "sml_KernelSML.h"
//#include "sml_Events.h"

wm_param_container::wm_param_container(agent* new_agent): soar_module::param_container(new_agent)
{
    add_cmd = new soar_module::boolean_param("add", on, new soar_module::f_predicate<boolean>());
    add(add_cmd);
    remove_cmd = new soar_module::boolean_param("remove", on, new soar_module::f_predicate<boolean>());
    add(remove_cmd);
    watch_cmd = new soar_module::boolean_param("watch", on, new soar_module::f_predicate<boolean>());
    add(watch_cmd);
    wma_cmd = new soar_module::boolean_param("activation", on, new soar_module::f_predicate<boolean>());
    add(wma_cmd);

    help_cmd = new soar_module::boolean_param("help", on, new soar_module::f_predicate<boolean>());
    add(help_cmd);
    qhelp_cmd = new soar_module::boolean_param("?", on, new soar_module::f_predicate<boolean>());
    add(qhelp_cmd);
}

void wm_param_container::print_settings(agent* thisAgent)
{
    std::string tempString;
    Output_Manager* outputManager = &Output_Manager::Get_OM();

    outputManager->reset_column_indents();
    outputManager->set_column_indent(1, 17);
    outputManager->printa(thisAgent,    "=========================================================\n");
    outputManager->printa(thisAgent,    "-               WM Sub-Commands and Options             -\n");
    outputManager->printa(thisAgent,    "=========================================================\n");
    outputManager->printa_sf(thisAgent, "wm %-[? | help]\n");
    outputManager->printa(thisAgent,    "---------------------------------------------------------\n");
    outputManager->printa_sf(thisAgent, "wm add    %-<id> [^]<attribute> <value> [+]\n");
    outputManager->printa_sf(thisAgent, "wm remove %-<timetag>\n");
    outputManager->printa(thisAgent,    "---------------------------------------------------------\n");
    outputManager->printa_sf(thisAgent, "wm activation %---get <parameter\n");
    outputManager->printa_sf(thisAgent, "              %---set <parameter> <value>\n");
    outputManager->printa_sf(thisAgent, "              %---stats [<statistic>]\n");
    outputManager->printa_sf(thisAgent, "              %---timers [<timer>]\n");
    outputManager->printa_sf(thisAgent, "              %---history <timetag>\n");
    outputManager->printa(thisAgent,    "---------------------------------------------------------\n");
    outputManager->printa_sf(thisAgent, "wm watch %-[--add-filter   ]  --type <type>  pattern\n");
    outputManager->printa_sf(thisAgent, "         %-[--remove-filter]  --type <type>  pattern\n");
    outputManager->printa_sf(thisAgent, "wm watch %-[--list-filter] [--type <type>]\n");
    outputManager->printa_sf(thisAgent, "         %-[--reset-filter ] [--type <type>]\n");
    outputManager->printa(thisAgent,    "---------------------------------------------------------\n");
    outputManager->printa_sf(thisAgent, "For a detailed explanation of sub-commands:       help wm\n");
}
void wm_param_container::print_summary(agent* thisAgent)
{
    print_settings(thisAgent);
}
