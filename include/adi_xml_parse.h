/*
 * =====================================================================================
 *
 *       Filename:  xml_parse.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2019年07月26日 14时32分06秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef XML_PARSE_H
#define XML_PARSE_H

struct adi_osc{
	int32_t index;
	const char *path;
	const char *type;
	const char *desc;
	unsigned int base;
	unsigned int width;
};

int xml_parse(char *filename, char *root_name, struct adi_osc *para);
int get_xml_node_count(char *filename, char *root_name);

#endif
