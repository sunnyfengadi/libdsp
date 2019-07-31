#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/parser.h>  
#include <libxml/xmlmemory.h>

#include "adi_xml_parse.h"

int get_xml_node_count(char *filename, char *root_name)
{
    xmlDocPtr doc;
    xmlNodePtr cur_node;
	unsigned long node_num = 0;

	/* open a xml doc */
    doc = xmlReadFile(filename, "UTF-8", XML_PARSE_RECOVER);  
    if (doc == NULL ) {
        fprintf(stderr,"xml not parsed successfully. \n");
        return -1;
    }

	/* get root element. */
    cur_node = xmlDocGetRootElement(doc);
    if (cur_node == NULL) {
        fprintf(stderr,"empty document\n");
        xmlFreeDoc(doc);
        return -1;
	}

	/* if the same,xmlStrcmp return 0, else return 1 */
    if (xmlStrcmp(cur_node->name, (const xmlChar *) root_name)) {
        fprintf(stderr,"xml of the wrong type, root node != %s",root_name);
        xmlFreeDoc(doc);
        return -1;
   	}
	
	node_num = xmlChildElementCount(cur_node);
    xmlFreeDoc(doc);

	return node_num;
}

int xml_parse(char *filename, char *root_name, struct adi_osc *para)
{
    xmlDocPtr doc;
    xmlNodePtr cur, root;
	int id = 0;

	// open a xml doc 
    doc = xmlReadFile(filename, "UTF-8", XML_PARSE_RECOVER);  
    if (doc == NULL ) {
        fprintf(stderr,"xml not parsed successfully. \n");
        return -1;
    }

	/* get root element */
    root = xmlDocGetRootElement(doc);
    if (root == NULL) {
        fprintf(stderr,"empty document\n");
        xmlFreeDoc(doc);
        return -1;
    }

	/* if the same,xmlStrcmp return 0, else return 1 */
    if (xmlStrcmp(root->name, (const xmlChar *) root_name)) {
        fprintf(stderr,"xml of the wrong type, root node != %s",root_name);
        xmlFreeDoc(doc);
        return -1;
   	}

	/* Get the children node: index */
    cur = root->xmlChildrenNode;
    while (cur != NULL) {
        if ((!xmlStrcmp(cur->name, (const xmlChar *)"index"))) {
			/* If children node is: index */
			para[id].index = atoi(xmlGetProp(cur, (const xmlChar *) "ID"));
	
			/* Move to index's sub node: path/type/desc */
			xmlNodePtr cur_sub = cur;
		    cur_sub = cur->xmlChildrenNode; 
  
		    while (cur_sub != NULL) {  
				/* If sub node is path */
		        if ((!xmlStrcmp(cur_sub->name, (const xmlChar *)"path")))
					para[id].path = (char *) xmlNodeGetContent(cur_sub);  

				/* If sub node is type */
				if ((!xmlStrcmp(cur_sub->name, (const xmlChar *)"type")))
				{
					para[id].type = (char *) xmlNodeGetContent(cur_sub);

					/* Calculate each offset */
					if (strcmp(para[id].type, "i"))
						para[id].width = sizeof(int32_t);
					else if (strcmp(para[id].type, "f"))
						para[id].width = sizeof(float);

					if (id == 0)
						para[id].base = 0;
					else 
						para[id].base = para[id-1].base + para[id-1].width;
				}

				/* If sub node is desc */
				if ((!xmlStrcmp(cur_sub->name, (const xmlChar *)"desc")))
					para[id].desc = (char *) xmlNodeGetContent(cur_sub);  

		        cur_sub = cur_sub->next;
			}
   
			id++;
		}

        cur = cur->next;
    }

    xmlFreeDoc(doc);
    return id;
}
