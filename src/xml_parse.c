#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/parser.h>  
#include <libxml/xmlmemory.h>

void parse_children(xmlDocPtr doc, xmlNodePtr cur)
{
// move to root element's children.  
    cur = cur->xmlChildrenNode; 
    char *nodeName;  
    char *content;  
  
    while (cur != NULL)  
    {  
        nodeName = (char *) cur->name;   
        content = (char *) xmlNodeGetContent(cur);  
        printf ("Current node name:%s,\t the content is:%s.\n\n", nodeName, content); 
        cur = cur->next;
    }
   
    return;
}

//void parseDoc(char *docname)
int main()
{
    xmlDocPtr doc;
    xmlNodePtr curNode;

// open a xml doc. 
    doc = xmlReadFile("osc_widget.xml", "UTF-8", XML_PARSE_RECOVER);  
    //doc = xmlReadFile(docname, "UTF-8", XML_PARSE_RECOVER); // open a xml doc.  

    if (doc == NULL ) {
        fprintf(stderr,"xml not parsed successfully. \n");
        return -1;
    }

// get root element.
    curNode = xmlDocGetRootElement(doc); 
    if (curNode == NULL) {
        fprintf(stderr,"empty document\n");
        xmlFreeDoc(doc);
        return -1;
    }

    // if the same,xmlStrcmp return 0, else return 1 
    if (xmlStrcmp(curNode->name, (const xmlChar *) "dsp_parameter")) {
        fprintf(stderr,"xml of the wrong type, root node != dsp_parameter");
        xmlFreeDoc(doc);
        return -1;
    }

    curNode = curNode->xmlChildrenNode;
    while (curNode != NULL) {
        if ((!xmlStrcmp(curNode->name, (const xmlChar *)"index"))){
            parse_children(doc, curNode);
        }

        curNode = curNode->next;
    }

    xmlFreeDoc(doc);
    return 0;
}
