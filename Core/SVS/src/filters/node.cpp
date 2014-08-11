#include <iostream>
#include <assert.h>
#include <string>
#include <map>
#include "filter.h"
#include "sgnode.h"
#include "scene.h"
#include "filter_table.h"

using namespace std;

typedef map<sgnode*, const filter_params*> node_param_map;

/*
 This filter takes a "name" parameter and outputs a pointer to the node
 with that name in the scene graph.
*/
class node_filter : public select_filter<sgnode*>
{
    public:
        node_filter(Symbol* root, soar_interface* si, scene* scn, filter_input* input)
            : select_filter<sgnode*>(root, si, input), scn(scn)
        {
        }
        
        bool compute(const filter_params* params, sgnode*& out, bool& select)
        {
            //out = NULL;
            //changed = false;
            //select = false;
            //return true;
            string id;
            if (!get_filter_param(this, params, "id", id))
            {
                set_status("expecting parameter id");
                return false;
            }
            
            
            out = scn->get_node(id);
            if(out == NULL){
              select = false;
            } else {
              select = true;
            }

            return true;
        }
    private:
        scene* scn;
};

/* Return all nodes from the scene */
class all_nodes_filter : public filter, public sgnode_listener
{
    public:
        all_nodes_filter(Symbol* root, soar_interface* si, scene* scn)
            : filter(root, si, NULL), scn(scn), first(true) {}
            
        ~all_nodes_filter()
        {
            map<sgnode*, filter_val*>::iterator i;
            for (i = outputs.begin(); i != outputs.end(); ++i)
            {
                i->first->unlisten(this);
            }
            
            vector<sgnode*> nodes;
            scn->get_all_nodes(nodes);
            nodes[0]->unlisten(this);
        }
        
        bool update_outputs()
        {
            vector<sgnode*> nodes;
            
            if (!first)
            {
                return true;
            }
            
            scn->get_all_nodes(nodes);
            nodes[0]->listen(this);
            
            for (int i = 1, iend = nodes.size(); i < iend; ++i) // don't add world node
            {
                add_node(nodes[i]);
            }
            first = false;
            return true;
        }
        
        void node_update(sgnode* n, sgnode::change_type t, const std::string& update_info)
        {
            filter_val* r;
            group_node* g;
            int added_child = 0;
            switch (t)
            {
                case sgnode::CHILD_ADDED:
                    if (parse_int(update_info, added_child))
                    {
                        g = n->as_group();
                        add_node(g->get_child(added_child));
                    }
                    break;
                case sgnode::DELETED:
                    if (map_get(outputs, n, r))
                    {
                        remove_output(r);
                        outputs.erase(n);
                    }
                    else
                    {
                        assert(n->get_name() == "world");
                    }
                    break;
                case sgnode::TRANSFORM_CHANGED:
                case sgnode::SHAPE_CHANGED:
                case sgnode::TAG_CHANGED:
                case sgnode::TAG_DELETED:
                    if (map_get(outputs, n, r))
                    {
                        change_output(r);
                    }
                    else
                    {
                        assert(n->get_name() == "world");
                    }
                    break;
            }
        }
        
    private:
        filter_val* add_node(sgnode* n)
        {
            n->listen(this);
            filter_val* r = new filter_val_c<sgnode*>(n);
            outputs[n] = r;
            add_output(r);
            return r;
        }
        
        scene* scn;
        bool first;
        
        map<sgnode*, filter_val*> outputs;
};

class remove_node_filter : public select_filter<sgnode*>
{
    public:
        remove_node_filter(Symbol* root, soar_interface* si, filter_input* input)
            : select_filter<sgnode*>(root, si, input), scn(scn)
        {}
        
        bool compute(const filter_params* p, sgnode*& out, bool& select)
        {
            sgnode* a;
            sgnode* b;
            
            if (!get_filter_param(this, p, "a", a))
            {
                set_status("expecting parameter a");
                return false;
            }
            if (!get_filter_param(this, p, "b", b))
            {
                set_status("expecting parameter b");
                return false;
            }
            
            out = a;
            select = (a != b);
            return true;
        }
        
    private:
        scene* scn;
};

class node_bbox_filter : public map_filter<bbox>
{
    public:
        node_bbox_filter(Symbol* root, soar_interface* si, filter_input* input)
            : map_filter<bbox>(root, si, input)
        {}
        
        bool compute(const filter_params* params, bbox& out){
            sgnode* n;
            if (!get_filter_param(this, params, "a", n)){
                return false;
            }
            
            out = n->get_bounds();
            return true;
        }
};

class node_trans_filter : public map_filter<vec3>
{
    public:
        node_trans_filter(Symbol* root, soar_interface* si, filter_input* input, char trans_type)
            : map_filter<vec3>(root, si, input), trans_type(trans_type)
        {}
        
        bool compute(const filter_params* params, vec3& out){
            sgnode* n;
            if (!get_filter_param(this, params, "a", n)){
                return false;
            }
            
            out = n->get_trans(trans_type);
            return true;
        }
    private:
        char trans_type;
};

class combine_nodes_filter : public passthru_filter<sgnode*>
{
    public:
        combine_nodes_filter(Symbol* root, soar_interface* si, filter_input* input)
            : passthru_filter<sgnode*>(root, si, input)
        {}
};

filter* make_node_filter(Symbol* root, soar_interface* si, scene* scn, filter_input* input)
{
    return new node_filter(root, si, scn, input);
}

filter* make_all_nodes_filter(Symbol* root, soar_interface* si, scene* scn, filter_input* input)
{
    return new all_nodes_filter(root, si, scn);
}

filter* make_node_position_filter(Symbol* root, soar_interface* si, scene* scn, filter_input* input)
{
    return new node_trans_filter(root, si, input, 'p');
}

filter* make_node_rotation_filter(Symbol* root, soar_interface* si, scene* scn, filter_input* input)
{
    return new node_trans_filter(root, si, input, 'r');
}

filter* make_node_scale_filter(Symbol* root, soar_interface* si, scene* scn, filter_input* input)
{
    return new node_trans_filter(root, si, input, 's');
}

filter* make_node_bbox_filter(Symbol* root, soar_interface* si, scene* scn, filter_input* input)
{
    return new node_bbox_filter(root, si, input);
}

filter* make_remove_node_filter(Symbol* root, soar_interface* si, scene* scn, filter_input* input)
{
    return new remove_node_filter(root, si, input);
}

filter* make_combine_nodes_filter(Symbol* root, soar_interface* si, scene* scn, filter_input* input)
{
    return new combine_nodes_filter(root, si, input);
}

filter_table_entry* node_filter_entry()
{
    filter_table_entry* e = new filter_table_entry;
    e->name = "node";
    e->create = &make_node_filter;
    return e;
}

filter_table_entry* all_nodes_filter_entry()
{
    filter_table_entry* e = new filter_table_entry;
    e->name = "all_nodes";
    e->create = &make_all_nodes_filter;
    return e;
}

filter_table_entry* node_position_filter_entry()
{
    filter_table_entry* e = new filter_table_entry;
    e->name = "node_position";
    e->create = &make_node_position_filter;
    return e;
}

filter_table_entry* node_rotation_filter_entry()
{
    filter_table_entry* e = new filter_table_entry;
    e->name = "node_rotation";
    e->create = &make_node_rotation_filter;
    return e;
}

filter_table_entry* node_scale_filter_entry()
{
    filter_table_entry* e = new filter_table_entry;
    e->name = "node_scale";
    e->create = &make_node_scale_filter;
    return e;
}

filter_table_entry* node_bbox_filter_entry()
{
    filter_table_entry* e = new filter_table_entry;
    e->name = "node_bbox";
    e->create = &make_node_bbox_filter;
    return e;
}

filter_table_entry* remove_node_filter_entry()
{
    filter_table_entry* e = new filter_table_entry;
    e->name = "remove_node";
    e->create = &make_remove_node_filter;
    return e;
}

filter_table_entry* combine_nodes_filter_entry()
{
    filter_table_entry* e = new filter_table_entry;
    e->name = "combine_nodes";
    e->create = &make_combine_nodes_filter;
    return e;
}
