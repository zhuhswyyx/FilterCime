//----------//
///*
//Date :20180610/20240331
//Author by :zhuhswyyx@163.com
//Use for :cime row filter,with config.
//gcc:	g++ FilterCime.cpp -o FilterCime.exe
//*/
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <map>
#include <string>
#include <dirent.h>//opendir()
#include <time.h>
//
#define TIMEFORMATSTRLEN	(20)
#define TABLENAMESIZE	(64)
//
#define TAB_FLAG ((char*)"	")	//TAB
//
#define ARG_MAX_CHAR (64)
#define PATH_MAX_CHAR (256)
//
#define ATTRVALUEMAXSIZE (64)
#define PER_LINE_COLUMN_NUM	(5120)
//
#ifndef __MAXSIZE__
#define __MAXSIZE__
#define LINEMAXSIZE		(ATTRVALUEMAXSIZE * PER_LINE_COLUMN_NUM)
#define PATHANDNAMEMAXSIZE	( PATH_MAX_CHAR + ARG_MAX_CHAR )
#endif
//
//define function
/*
//按行读取配置文件,载入vector结构中,用来筛选cime格式文件。
*/
int init_conf(char* fun_config_path,char* fun_config_name);
/*
//按行读取cime格式文件,每一行匹配vector中的结构信息,完成模糊搜索中的确定信息确定,将特征定位信息载入到map中,用来确定cime格式中的行的打印位置。
*/
int init_cime_match_cime(char* cime_path,char * cime_name);
/*
//扫描cime文件,每一行完整精确匹配map中的结构信息,打印到相应位置。
*/
int deal_cime_match_cime_print_f(char* cime_path,char* cime_name,char* filter_path,char * new_path);
/*
//完成过滤过程中,一行需要的逻辑操作,执行打印任务。
*/
int do_oneline_match_cime_print_f(FILE * fpw_file,FILE *fpwd_file,char* line,int len);
/*
//从用tab分隔的一行数据中找到第几个列号对应的数据块。
//例如：“#	a XXX XXX	... XXX”中“a”在行中为第一个有效位置。
*/
char* getvaluefromcolnum(int attrcolnum,char* myline,int mylen,char**outvalue);
/*
//按照sPathName展示出来的路径,逐层创建文件夹。
*/
int CreateDirs(const char *sPathName);
/*
//对cime文件备份,便于程序处理静态文本,避免程序运行过程中,动态数据失真情况。
*/
int backupfile(const char *BackupPath,const char*BackupName,const char *DestPath);
/*
//处理以“--”为开头的参数指定的参数数据,紧跟在参数后面的参数数据将会用一维指针的地址来接收。
//如果出错,返回空字符串("")。
*/
int MatchStrGetNextArg(char** source_data, int source_str_num,char *match,char**outstr);
//
/*
//提取字符串p_head中,位于字符串p_start和字符串p_end中间（不包含两侧字符串）的部分,用out_part来接受。
//注意：定义的一维指针out_part需要使用free_cutPartFromStartAndEnd(...)函数来进行释放。
*/
char*     cutPartFromStartAndEnd(char* p_head,char* p_start,char* p_end,char** out_part);
/*
//特定函数,配合释放cutPartFromStartAndEnd(...)产生的内存的空间。
*/
int  free_cutPartFromStartAndEnd(char* out_part);
//
/*
//指定路径将产生一个固定名字的文件,用来展示config模版用法示例。
*/
int Print_filter_config_template(char* template_file_path);
/*
//指定路径将产生一个固定名字的文件,用来展示table存储结构模板示例。
*/
int Print_table_template(char* template_file_path);
/*
//打印提示告知使用参数序列。
*/
int PrintFilterCimeHelp(char* fun_name);
//
/*
//对cime操作文件进行特定位置参数lock_state状态惊醒校验,如果不一致则进行程序终止。
*/
int is_lock_need_run(char* cime_path,char* cime_file,char* lock_status);
/*
//将src_p2文件描述符指定的文件中的数据拷贝到des_p1文件描述符指定的文件中。
*/
void MyCopyFile(FILE *des_p1,FILE *src_p2);
/*
//直接调用返回一个调整好的特定的时间格式,用一维指针的地址二位指针s_time来接收。
*/
char* now_time(char** s_time);
//
typedef struct
{
	std::string AttrName;
	int AttrColnum;
	std::string AttrValue;
}SCHEMA_ATTR; //config_file schema attr struct
typedef struct
{
	std::string RelClaName;
	int RelColnum;
	int AttrColnum;
}SCHEMA_RELCLA; //config_file schema relcla struct
typedef struct
{
	std::string TableName;
	std::vector<SCHEMA_ATTR> SchemaAttr;
	std::vector<SCHEMA_RELCLA> SchemaRelCla;
}SCHEMA_TABLE; //config_file schema table struct
typedef struct
{
	int RelCol;
	std::string RelValue;
}REL_MAP_STRUCT; //save map_struct for delete data
typedef struct
{
	int AttrColnum;
	std::string AttrValue;
}ATTR_MAP_STRUCT; //SAVE attr struct
//
std::vector<SCHEMA_TABLE> g_schema_table; //schema vector,save config_file data
std::multimap<std::string,REL_MAP_STRUCT> g_rel_multimap; //relation map,save will delete data relation
std::multimap<std::string,ATTR_MAP_STRUCT> g_attr_multimap; //attr map,save will delete data dependent by attr value
//
int print_config_vector(std::vector<SCHEMA_TABLE> &config_vec);
int print_attr_map(std::multimap<std::string,ATTR_MAP_STRUCT> &attr_multimap);
int print_rel_map(std::multimap<std::string,REL_MAP_STRUCT> &rel_multimap);

//global define
int globcount =0; //count del num
std::string g_CurTableName; //current table name
//go to start
int main(int argc, char ** argv)
{
	int ret=0;
	if(argc == 1)
	{
		PrintFilterCimeHelp(argv[0]);
		exit(1);
	}
	char * tmp_arg1_cime_path=NULL;
	char * tmp_arg2_cime_file=NULL;
	char * tmp_arg3_config_path=NULL;
	char * tmp_arg4_config_file=NULL;
	char * tmp_arg5_backup_path=NULL;
	char * tmp_arg6_del_path=NULL;
	char * tmp_arg7_new_path=NULL;
	char * tmp_arg8_config_example_path=NULL;
	char * tmp_arg9_table_example_path=NULL;
	MatchStrGetNextArg(argv,argc,(char*)"--cime_path",&tmp_arg1_cime_path);
	MatchStrGetNextArg(argv,argc,(char*)"--cime_file",&tmp_arg2_cime_file);
	MatchStrGetNextArg(argv,argc,(char*)"--config_path",&tmp_arg3_config_path);
	MatchStrGetNextArg(argv,argc,(char*)"--config_file",&tmp_arg4_config_file);
	MatchStrGetNextArg(argv,argc,(char*)"--backup_path",&tmp_arg5_backup_path);
	MatchStrGetNextArg(argv,argc,(char*)"--del_path",&tmp_arg6_del_path);
	MatchStrGetNextArg(argv,argc,(char*)"--new_path",&tmp_arg7_new_path);
	if(MatchStrGetNextArg(argv,argc,(char*)"--config_example_path",&tmp_arg8_config_example_path)==0)
	{
		Print_filter_config_template(tmp_arg8_config_example_path);	
	}
	if(MatchStrGetNextArg(argv,argc,(char*)"--table_example_path",&tmp_arg9_table_example_path)==0)
	{
		Print_table_template(tmp_arg9_table_example_path);
	}
	CreateDirs(tmp_arg1_cime_path);
	CreateDirs(tmp_arg3_config_path);
	CreateDirs(tmp_arg5_backup_path);
	CreateDirs(tmp_arg6_del_path);
	CreateDirs(tmp_arg7_new_path);
	is_lock_need_run(tmp_arg1_cime_path,tmp_arg2_cime_file,"unlock");
	if(*tmp_arg1_cime_path!='\0' ||*tmp_arg2_cime_file!='\0' ||*tmp_arg5_backup_path!='\0')
	{
		backupfile(tmp_arg1_cime_path,tmp_arg2_cime_file,tmp_arg5_backup_path);
	}
	if(*tmp_arg3_config_path!='\0' ||*tmp_arg4_config_file!='\0')
	{
		init_conf(tmp_arg3_config_path,tmp_arg4_config_file);
	}
	char cime_path_and_name[PATHANDNAMEMAXSIZE];
	memset(cime_path_and_name,0x00,PATHANDNAMEMAXSIZE);
	if(*tmp_arg5_backup_path!='\0' ||*tmp_arg2_cime_file!='\0')
	{
		ret = init_cime_match_cime(tmp_arg5_backup_path,tmp_arg2_cime_file);
		if(ret == -1)
		{
			printf("function init_cime_match_cime error ,ret =[%d], \
			please check path:[%s],cime_name[%s] for later...\n",ret,tmp_arg1_cime_path,tmp_arg2_cime_file);
			return -1;
		}
	}
	if(*tmp_arg5_backup_path!='\0' ||*tmp_arg2_cime_file!='\0' ||*tmp_arg6_del_path!='\0' ||*tmp_arg7_new_path!='\0')
	{
		deal_cime_match_cime_print_f(tmp_arg5_backup_path,tmp_arg2_cime_file,tmp_arg6_del_path,tmp_arg7_new_path);
		//print_config_vector(g_schema_table);
		//print_attr_map(g_attr_multimap);
		//print_rel_map(g_rel_multimap);
		printf("\nWrite file at:[ %s/new_%s ]\n",tmp_arg7_new_path,tmp_arg2_cime_file);
		printf("\nWrite file at:[ %s/del_%s ]\n",tmp_arg6_del_path,tmp_arg2_cime_file);
		printf("\nBackup file at:[ %s/bak_%s ]\n",tmp_arg5_backup_path,tmp_arg2_cime_file);
		printf("\n=========Function do SUCCESS!!!,total delete num:[ %d ]==========\n\n",globcount);
	}
	if(tmp_arg1_cime_path!=NULL)
	{
		free(tmp_arg1_cime_path);
	}
	if(tmp_arg2_cime_file!=NULL)
	{
		free(tmp_arg2_cime_file);
	}
	if(tmp_arg3_config_path!=NULL)
	{
		free(tmp_arg3_config_path);
	}
	if(tmp_arg4_config_file!=NULL)
	{
		free(tmp_arg4_config_file);
	}
	if(tmp_arg5_backup_path!=NULL)
	{
		free(tmp_arg5_backup_path);
	}
	if(tmp_arg6_del_path!=NULL)
	{
		free(tmp_arg6_del_path);
	}
	if(tmp_arg7_new_path!=NULL)
	{
		free(tmp_arg7_new_path);
	}
	if(tmp_arg8_config_example_path!=NULL)
	{
		free(tmp_arg8_config_example_path);
	}
	if(tmp_arg9_table_example_path!=NULL)
	{
		free(tmp_arg9_table_example_path);
	}
	return 0;
}
//
int init_conf(char* fun_config_path,char* fun_config_name) //read xxx_config.txt init conf
{
	char *tmp=NULL;
	char conf_file[PATHANDNAMEMAXSIZE], buf[LINEMAXSIZE];
	FILE *fp_conf=NULL;
	memset(conf_file,0x00,PATHANDNAMEMAXSIZE);
	sprintf(conf_file,"%s/%s\0",fun_config_path,fun_config_name);
	fp_conf = fopen(conf_file,"r");
	if (fp_conf==NULL)
	{
		printf("Error: Open conf_file:%s\n",conf_file);
		exit(1);
	}
	SCHEMA_TABLE tmp_table;
	SCHEMA_ATTR tmp_attr;
	SCHEMA_RELCLA tmp_rel;
	while (!feof(fp_conf))
	{
		memset(buf,0x00,LINEMAXSIZE);
		//fflush(fp_conf)//缓冲区刷新数据问题待解决。
		tmp=fgets(buf,LINEMAXSIZE,fp_conf);
		if(tmp == NULL || *tmp =='#')
		{
			continue;
		}else if( strncmp(buf,"<TableName=\"",12) == 0 ) //is a table
		{
			char* tmpName = NULL;
			tmpName = cutPartFromStartAndEnd(buf,(char*)"<TableName=\"",(char*)"\">",&tmpName);
			//printf("cut config table name is [%s]\n",tmpName);//检测缓冲区刷新数据,读取<TableName="读取不到问题的打印。
			tmp_table.TableName=tmpName;
			free_cutPartFromStartAndEnd(tmpName);
		}else if( strncmp(buf,"</TableName=\"",13) == 0 )
		{
			g_schema_table.push_back(tmp_table);
			//printf("g_schema_table tmp_table TableName is [%s]\n",tmp_table.TableName.c_str());//检测缓冲区刷新数据,读取<TableName="读取不到问题的打印。
			tmp_table.TableName.clear();
			tmp_table.SchemaAttr.clear();
			tmp_table.SchemaRelCla.clear();
		}else if(strncmp(buf,"<attr=\"",7) ==0) //is a attr
		{
			char* tmpName1 = NULL;
			tmpName1 = cutPartFromStartAndEnd(buf,(char*)"<attr=\"",(char*)"\"",&tmpName1);
			if(tmpName1 != NULL)
			{
				tmp_attr.AttrName=tmpName1;
			}
			free_cutPartFromStartAndEnd(tmpName1);
			char* anum = NULL;
			anum = cutPartFromStartAndEnd(buf,(char*)"colnum=\"",(char*)"\"",&anum);
			tmp_attr.AttrColnum = atoi(anum);
			free_cutPartFromStartAndEnd(anum);
			char* tmpValue = NULL;
			tmpValue = cutPartFromStartAndEnd(buf,(char*)"value=\"",(char*)"\">",&tmpValue);
			tmp_attr.AttrValue = tmpValue;
			free_cutPartFromStartAndEnd(tmpValue);
			tmp_table.SchemaAttr.push_back(tmp_attr);
		}else if(strncmp(buf,"<relcla=\"",9) ==0)
		{
			char* tmpName = NULL;
			tmpName = cutPartFromStartAndEnd(buf,(char*)"<relcla=\"",(char*)"\"",&tmpName);
			tmp_rel.RelClaName=tmpName;
			free_cutPartFromStartAndEnd(tmpName);
			char* anum1 = NULL;
			anum1 = cutPartFromStartAndEnd(buf,(char*)"relcolnum=\"",(char*)"\"",&anum1);
			tmp_rel.RelColnum = atoi(anum1);
			free_cutPartFromStartAndEnd(anum1);
			char* anum2 = NULL;
			anum2 = cutPartFromStartAndEnd(buf,(char*)"attrcolnum=\"",(char*)"\">",&anum2);
			tmp_rel.AttrColnum = atoi(anum2);//AttrColnum
			free_cutPartFromStartAndEnd(anum2);
			tmp_table.SchemaRelCla.push_back(tmp_rel);
		}
	}
	fclose(fp_conf);
	return 0;
}
//
int init_cime_match_cime(char * path_cime,char* name_cime)//cime_path_and_name
{
	char* line = (char*)malloc(LINEMAXSIZE);//line malloc
	memset(line,0x00,LINEMAXSIZE);
	char* tmp = NULL;
	size_t len = 0;
	FILE *fpr_file=NULL;
	char new_path_and_name[PATHANDNAMEMAXSIZE];
	memset(new_path_and_name,0x00,PATHANDNAMEMAXSIZE);
	sprintf(new_path_and_name,"%s/bak_%s\0",path_cime,name_cime);
	fpr_file = fopen(new_path_and_name,"r");
	if (fpr_file==NULL)
	{
		printf("Error: Open file:%s...\n",new_path_and_name);
		return -1;
	}
	while(!feof(fpr_file))
	{
		memset(line,0x00,LINEMAXSIZE);
		tmp = fgets(line,LINEMAXSIZE,fpr_file);
		if(tmp == NULL|| *tmp=='\n')
		{
			continue;
		}
		len = strlen(line);
		if(strncmp(line,"<!Library=\"",11) ==0|| strncmp(line,"//",2) ==0)
		{
			continue;
		}else if( strncmp(line,"</TableName=\"",13) ==0 )
		{
			continue;
		}else if(strncmp(line,"<TableName=\"",12) ==0)
		{
			char* ftablename = NULL;
			ftablename = cutPartFromStartAndEnd(line,(char*)"<TableName=\"",(char*)"\"",&ftablename);
			g_CurTableName=ftablename;
			free_cutPartFromStartAndEnd(ftablename);
			continue;
		}else if( strncmp(line,"@",1) ==0)
		{
			continue;
		}else if(strncmp(line,"#",1) ==0)
		{
			int mt=0;
			for(mt=0;mt<g_schema_table.size();mt++) //scanf schema table
			{
				if(strncmp(g_schema_table[mt].TableName.c_str(),g_CurTableName.c_str(),strlen(g_CurTableName.c_str()))==0)
				{
					int ma;
					for(ma=0;ma<g_schema_table[mt].SchemaAttr.size();ma++) //scanf schema attr
					{
						char* tmpvalue=(char*)malloc(ATTRVALUEMAXSIZE); 
						memset(tmpvalue,0x00,sizeof(tmpvalue));
						getvaluefromcolnum(g_schema_table[mt].SchemaAttr[ma].AttrColnum,line,len,(char**)&tmpvalue);
						if(strcmp(tmpvalue,g_schema_table[mt].SchemaAttr[ma].AttrValue.c_str())==0)
						{
							ATTR_MAP_STRUCT tmp_attr_multimap_struct;
							tmp_attr_multimap_struct.AttrColnum =g_schema_table[mt].SchemaAttr[ma].AttrColnum;
							tmp_attr_multimap_struct.AttrValue = tmpvalue;
							//need to del ,add del map
						    g_attr_multimap.insert(std::pair<std::string,ATTR_MAP_STRUCT>(g_schema_table[mt].TableName,tmp_attr_multimap_struct));
							tmp_attr_multimap_struct.AttrColnum=0;
							tmp_attr_multimap_struct.AttrValue.clear();
							int mr;
							if(g_schema_table[mt].SchemaRelCla.size() > 0 ) //exist relation
							{
								for(mr=0;mr<g_schema_table[mt].SchemaRelCla.size();mr++)
								{
									REL_MAP_STRUCT tmp_rel_map_struct;
									char* Curvaluerelneed=(char*)malloc(ATTRVALUEMAXSIZE); 
									memset(Curvaluerelneed,0x00,sizeof(Curvaluerelneed));
									getvaluefromcolnum(g_schema_table[mt].SchemaRelCla[mr].AttrColnum,line,len,(char**)&Curvaluerelneed);
									//do relation,add rel_del map
									tmp_rel_map_struct.RelCol =g_schema_table[mt].SchemaRelCla[mr].RelColnum;
									tmp_rel_map_struct.RelValue = Curvaluerelneed;
									g_rel_multimap.insert(std::pair<std::string,REL_MAP_STRUCT>(g_schema_table[mt].SchemaRelCla[mr].RelClaName,tmp_rel_map_struct));
									tmp_rel_map_struct.RelCol=0;
									tmp_rel_map_struct.RelValue.clear();
									free(Curvaluerelneed);
								}
							}
						}
						else
						{
							;//cur line not need to del
						}
						free(tmpvalue);
					}
				}else
				{//g_schema_table[mt].TableName == g_CurTableName
					;//printf("no match :[%s]!=[%s]\n",g_schema_table[mt].TableName.c_str(),g_CurTableName.c_str());
				}
			}
		}//end else if
	} //end while
	fclose(fpr_file);
	free(line);
	return 0;
}
int deal_cime_match_cime_print_f(char* cime_path,char* cime_name,char* filter_del_path,char * filter_new_path)
{
	int read;
	char new_path_bak[PATH_MAX_CHAR];
	char new_path_new[PATH_MAX_CHAR];
	char new_path_del[PATH_MAX_CHAR];
	FILE *deal_cime_fpw_file=NULL;
	FILE *deal_cime_fpr_file=NULL;
	FILE *deal_cime_fpwd_file=NULL;
	memset(new_path_bak,0x00,PATH_MAX_CHAR);
	sprintf(new_path_bak,"%s/bak_%s\0",cime_path,cime_name);
	deal_cime_fpr_file = fopen(new_path_bak,"r");
	if (deal_cime_fpr_file==NULL)
	{
		printf("Error: Open deal_cime_fpr_file file...\n");
		return -1;
	}
	memset(new_path_new,0x00,PATH_MAX_CHAR);
	sprintf(new_path_new,"%s/new_%s\0",filter_new_path,cime_name);
	deal_cime_fpw_file = fopen(new_path_new,"w+");
	if (deal_cime_fpw_file == NULL)
	{
		printf("Error: Open deal_cime_fpw_file file...\n");
		return -1;
	}
	memset(new_path_del,0x00,PATH_MAX_CHAR);
	sprintf(new_path_del,"%s/del_%s\0",filter_del_path,cime_name);
	deal_cime_fpwd_file = fopen(new_path_del,"w+");
	if (deal_cime_fpwd_file == NULL)
	{
		printf("Error: Open deal_cime_fpwd_file file...\n");
		return -1;
	}
	int ret=0;
	char* line = (char*)malloc(LINEMAXSIZE);
	while(!feof(deal_cime_fpr_file))
	{
		memset(line,0x00,LINEMAXSIZE);
		fgets(line,LINEMAXSIZE,deal_cime_fpr_file);
		read = strlen(line);
		//do oneline things
		if((ret=do_oneline_match_cime_print_f(deal_cime_fpw_file,deal_cime_fpwd_file,line,read))==-1)
		{
			printf("function match cime print to file err,ret=%d ! please check it...\n ",ret);
			return -1;
		}
	}//end_while
	if(line != NULL)
	{
		free(line);
	}	
	fclose(deal_cime_fpr_file);
	fclose(deal_cime_fpw_file);
	fclose(deal_cime_fpwd_file);
	return 0;
}
//
int do_oneline_match_cime_print_f(FILE * fpw_file,FILE *fpwd_file,char* line,int len)
{
	int ret=0;
	if(strncmp(line,"#\n",2)==0 || strncmp(line,"\n",1)==0)
	{
		return 0;
	}else if(strncmp(line,"<!Library=\"",11) == 0 || strncmp(line,"//",2)  == 0)
	{
		ret = fprintf(fpw_file,"%s",line);
		if(ret != len)
		{
			printf("write err:%d != %d\n",ret ,len);
			return -1;
		}
		ret = fprintf(fpwd_file,"%s",line);
		if(ret != len)
		{
			printf("write del_file err:%d != %d\n",ret ,len);
			return -1;
		}
		return 0;
	}else if( strncmp(line,"</TableName=\"",13) == 0 )
	{
		ret = fprintf(fpw_file,"%s",line);
		if(ret != len)
		{
			printf("write err:%d != %d\n",ret ,len);
			return -1;
		}
		ret = fprintf(fpwd_file,"%s",line);
		if(ret != len)
		{
			printf("write del_file err:%d != %d\n",ret ,len);
			return -1;
		}
		return 0;
	}else if(strncmp(line,"<TableName=\"",12) == 0)//table head
	{
		char* ftablename = NULL;
		ftablename = cutPartFromStartAndEnd(line,(char*)"TableName=\"",(char*)"\"",&ftablename);
		g_CurTableName.clear();
		g_CurTableName=ftablename;
		free_cutPartFromStartAndEnd(ftablename);
		ret = fprintf(fpw_file,"%s",line);
		if(ret != len)
		{
			printf("write filter_file err:%d != %d\n",ret ,len);
			return -1;
		}
		ret = fprintf(fpwd_file,"%s",line);
		if(ret != len)
		{
			printf("write del_file err:%d != %d\n",ret ,len);
			return -1;
		}
		return 0;
	}else if( strncmp(line,"@",1) == 0 )
	{
		ret = fprintf(fpw_file,"%s",line);
		if(ret != len)
		{
			printf("write err:%d != %d\n",ret ,len);
			return -1;
		}
		ret = fprintf(fpwd_file,"%s",line);
		if(ret != len)
		{
			printf("write del_file err:%d != %d\n",ret ,len);
			return -1;
		}
		return 0;
	}else if(strncmp(line,"#",1) ==0)
	{
		//printf("now do line [%s]\n",line);
		//first scanf g_attr_map
		unsigned int rule_count = g_attr_multimap.count(g_CurTableName);
		if(rule_count == 0)
		{
			;//std::cout << g_CurTableName<< " no rule..."<<std::endl;
		}else
		{
			std::multimap<std::string, ATTR_MAP_STRUCT>::iterator find_iter;
			find_iter = g_attr_multimap.find(g_CurTableName);
			if(find_iter == g_attr_multimap.end())
			{
				;//no find
			}else
			{
				char* tmp_attr=NULL;
				for(int i=0;i<rule_count;i++)
				{
					tmp_attr = (char*)malloc(ATTRVALUEMAXSIZE);
					memset(tmp_attr,0x00,sizeof(tmp_attr));
					getvaluefromcolnum((*find_iter).second.AttrColnum,line,len,(char**)&tmp_attr);
					if( strncmp((*find_iter).second.AttrValue.c_str(),tmp_attr,strlen((*find_iter).second.AttrValue.c_str()))==0)
					{
						//need to del
						ret = fprintf(fpwd_file,"%s",line);
						if(ret != len)
						{
							printf("write del_file err:%d != %d\n",ret ,len);
							return -1;
						}
						globcount++;
						return 0;
					}
					find_iter++;
				}
				free(tmp_attr);
			}
		}//end if
		//second scanf g_rel_multimap
		unsigned int rule_rel_count = g_rel_multimap.count(g_CurTableName);
		if(rule_rel_count == 0)
		{
			;//std::cout << g_CurTableName<< "rel no rule..."<<std::endl;
		}else
		{
			std::multimap<std::string, REL_MAP_STRUCT>::iterator find_iter;
			find_iter = g_rel_multimap.find(g_CurTableName);
			if(find_iter == g_rel_multimap.end())
			{
				;//no find
			}else
			{
				char* tmp_attr = NULL;
				for(int i=0;i<rule_rel_count;i++)
				{
					tmp_attr = (char*)malloc(ATTRVALUEMAXSIZE);
					memset(tmp_attr,0x00,sizeof(tmp_attr));
					getvaluefromcolnum((*find_iter).second.RelCol,line,len,(char**)&tmp_attr);
					//printf("rel match table:[%s]relcol:[%d]==>[%s]\n",g_CurTableName.c_str(),(*find_iter).second.RelCol,tmp_attr);
					if( strncmp((*find_iter).second.RelValue.c_str(),tmp_attr,strlen((*find_iter).second.RelValue.c_str()))==0)
					{
						//need to del
						ret = fprintf(fpwd_file,"%s",line);
						if(ret != len)
						{
							printf("write del_file err:%d != %d\n",ret ,len);
							return -1;
						}
						globcount++;
						return 0;
					}
					find_iter++;
				}
				free(tmp_attr);
			}
		}//end if
		//need not to del
		ret = fprintf(fpw_file,"%s",line);
		if(ret != len)
		{
			printf("write write_file err:%d != %d\n",ret ,len);
			return -1;
		}
		return 0;
	}else{
		printf("this line in CIME have no match ,will no print to file!!!...\n");
		return -1;
	}//end if #
	return -1;
}
//
char* getvaluefromcolnum(int attrcolnum,char* myline,int mylen,char**outvalue)
{
	char *f= NULL;
	char tmp[LINEMAXSIZE]={0};
	memcpy(tmp,myline,mylen-1);
	//
	f=strtok(tmp,TAB_FLAG);
	for(int k=0; k<attrcolnum; k++)//scanf attr col num
	{
		f=strtok(NULL,TAB_FLAG);
		if(f == NULL)
		{
			printf("Error !! Please check line [%s] have no  attrcolnum:[%d] ...\n",myline,attrcolnum);
			break;
		}
	}
	if(f == NULL)
	{
		return NULL;
	}
	//here ,have a problem,get col if in middle,we get head of col,but no head to TAB,it will get data of tail,so compare will no use this ,and will use strncmp(...)to compare!!!
	memcpy(*outvalue,f,(int)strlen(f)+1);
	return *outvalue;
}
//create files
int CreateDirs(const char *sPathName)
{
	if(*sPathName == '\0')
	{
		return -1;
	}
	char DirName[PATH_MAX_CHAR];
	memset(DirName,0x00,PATH_MAX_CHAR);
	sprintf(DirName,"%s/",sPathName);
	int len = strlen(DirName);
	for(int i=0;i<len+1;i++)
	{
		if(DirName[i] == '/'&& i!= 0)
		{
			DirName[i]='\0';
			if(access(DirName,0)!=0)
			{
				int ret = mkdir(DirName);
				if(ret != 0)//==-1
				{
					perror("mkdir");
					return -1;
				}
			}
			DirName[i]='/';
		}
	}
	return 0;
}
//
int backupfile(const char *BackupPath,const char*BackupName,const char *DestPath)
{
	char backup_pathAndName[PATHANDNAMEMAXSIZE], dest_pathAndName[PATHANDNAMEMAXSIZE];
	//
	if(BackupPath == NULL||DestPath == NULL)
	{
		printf("[backupfile] sourcename or destname you input is NULL...\n");
		return -1;
	}
	CreateDirs(DestPath);
	memset(backup_pathAndName,0x00,PATHANDNAMEMAXSIZE);
	sprintf(backup_pathAndName,"%s/%s\0",BackupPath,BackupName);
	memset(dest_pathAndName,0x00,PATHANDNAMEMAXSIZE);
	sprintf(dest_pathAndName,"%s/bak_%s\0",DestPath,BackupName);
	//
	FILE* fp_w = NULL;
	FILE* fp_r = fopen(backup_pathAndName,"r");
	if(fp_r != NULL)
	{
		fp_w = fopen(dest_pathAndName,"w+");
		if(fp_w != NULL)
		{
			if( access(backup_pathAndName,F_OK)==0 || opendir(DestPath) != NULL)
			{
				MyCopyFile(fp_w,fp_r);
			}else
			{
				printf("[backupfile] no file [%s]...\n",DestPath);
				fclose(fp_w);
				fclose(fp_r);
				return -2;
			}
		}
	}
	fclose(fp_w);
	fclose(fp_r);
	return 0;
}

//return 0 is normal out and no match return -1 
int MatchStrGetNextArg(char** source_data, int source_str_num,char *match,char**outstr)
{
	if(source_data == NULL || match == NULL)
	{
		return -1;
	}
	if(*outstr == NULL)
	{
		*outstr = (char*)malloc(ARG_MAX_CHAR);
	}else
	{
		*outstr = (char*)realloc(*outstr,ARG_MAX_CHAR);	
	}
	//is in
	for(int i=0;i<source_str_num;i++)
	{
		if(strncmp(source_data[i],match,strlen(match))==0)
		{
			break;
		}else if(i == source_str_num -1)
		{
			strcpy(*outstr,"");
			return 0;
		}
	}
	memset(*outstr,0x00,ARG_MAX_CHAR);
	char mem_tmp[ARG_MAX_CHAR];
	for(int i=0;i<source_str_num;i++)//argv[0]=xxx.exe
	{
		memset(mem_tmp,0x00,ARG_MAX_CHAR);
		strcpy(mem_tmp,source_data[i]);
		if(strcmp(mem_tmp,match)==0 && i+1 < source_str_num)
		{
			memset(mem_tmp,0x00,ARG_MAX_CHAR);
			strcpy(mem_tmp,source_data[i+1]);
			if(strlen(mem_tmp) > 0 && strncmp(mem_tmp,"--",2) != 0) //next is  std::string and no "--" 
			{
				strcpy(*outstr,mem_tmp);
			}
			if(strlen(mem_tmp) > 0 && strncmp(mem_tmp,"--",2) == 0)//next is  "--"
			{
				strcpy(*outstr,"");
			}
			printf("Get arg:[%s] set value:[%s]\n",source_data[i],*outstr);
			return 0;
		}
		if(strcmp(source_data[i],match)!=0)
		{
			;//go on
		}
	}
	strcpy(*outstr,"");
	printf("No match value:[%s]\n","");
	return -3;//no match 
}
//
int Print_table_template(char* template_file_path)
{
	char* path_and_name= NULL;
	FILE* fp_w = NULL;
	if(template_file_path != NULL)
	{
		CreateDirs(template_file_path);
		//
		int will_malloc_size = strlen(template_file_path)+strlen("table_template.txt")+1;
		path_and_name = (char*)malloc(will_malloc_size);//path + table_template + \0
		memset(path_and_name,0x00,will_malloc_size);
		strcat(path_and_name,template_file_path);
		strcat(path_and_name,"/");
		strcat(path_and_name,"table_template.txt");
		fp_w = fopen(path_and_name,"w+");
		char* now_at_time = NULL;
		now_time(&now_at_time);
		if(fp_w != NULL)
		{
			fprintf(fp_w,"######################table_template.txt#####\n");
			fprintf(fp_w,"表中值之间TAB作为分隔符.,Y/N 行可用性标识,L/U 行锁定性标识,管理数据时可以借鉴使用.\n");
			fprintf(fp_w,"<!Library=\"xxx\" lock_state=\"xxx\" Time=\"%s\"!>\n",now_at_time);
			fprintf(fp_w,"<TableName=\"xxx1\">\n");
			fprintf(fp_w,"@	ID	NAME	DATA	...	CODE\n");
			fprintf(fp_w,"//	XXX	XXX	XXX	...	XXX\n");
			fprintf(fp_w,"#YU	1 XXX XXX	... XXX\n");
			fprintf(fp_w,"#YU	2 XXX XXX	... XXX\n");
			fprintf(fp_w,"#YU	3 XXX XXX	... XXX\n");
			fprintf(fp_w,"</TableName=\"xxx1\">\n");
			fprintf(fp_w,"<TableName=\"xxx2\">\n");
			fprintf(fp_w,"@	ID	NAME	DATA	...	CODE\n");
			fprintf(fp_w,"//	XXX	XXX	XXX	...	XXX\n");
			fprintf(fp_w,"#YU	1 XXX XXX	... XXX\n");
			fprintf(fp_w,"#YU	2 XXX XXX	... XXX\n");
			fprintf(fp_w,"#YU	3 XXX XXX	... XXX\n");
			fprintf(fp_w,"</TableName=\"xxx2\">\n");
			fprintf(fp_w,"#######################table_template.txt####\n\n");
			//
			free(path_and_name);
			fclose(fp_w);
			return 0;
		}
	}
	free(path_and_name);
	fclose(fp_w);
	return -1;
}
//
int Print_filter_config_template(char* template_config_path)
{
	char* path_and_name=NULL;
	FILE* fp_w = NULL;
	if(template_config_path != NULL)
	{
		CreateDirs(template_config_path);
		int will_malloc_size = strlen(template_config_path)+strlen("filter_conf_example.txt")+1;
		path_and_name = (char*)malloc(will_malloc_size);//path + filter_conf_example + \0
		memset(path_and_name,0x00,will_malloc_size);
		sprintf(path_and_name,"%s/%s\0",template_config_path,"filter_conf_example.txt");
		fp_w = fopen(path_and_name,"w+");
		if(fp_w != NULL)
		{
			fprintf(fp_w,"######################filter_conf_example.txt#####\n");
			fprintf(fp_w,"'#'为注释部分,配置文件中值的开头结尾都需要用双引号.\n");
			fprintf(fp_w,"#\n");
			fprintf(fp_w,"匹配搜索(match search):");
			fprintf(fp_w,"#example_1:\n");
			fprintf(fp_w,"#<TableName=\"TableName1\">\n");
			fprintf(fp_w,"#<attr=\"Attrtype1\" colnum=\"51\" value=\"8\">\n");
			fprintf(fp_w,"#<attr=\"Attrtype2\" colnum=\"51\" value=\"3\">\n");
			fprintf(fp_w,"#</TableName=\"TableName1\">\n");
			fprintf(fp_w,"#\n");
			fprintf(fp_w,"关联匹配搜索(Association matching search):");
			fprintf(fp_w,"#example_2:\n");
			fprintf(fp_w,"#<TableName=\"TableName13\">\n");
			fprintf(fp_w,"#<attr=\"Attrtype1\" colnum=\"10\" value=\"attrtypeName1\">\n");
			fprintf(fp_w,"#<attr=\"Attrtype2\" colnum=\"10\" value=\"attrtypeName2\">\n");
			fprintf(fp_w,"#<relcla=\"TableName_6\" relcolnum=\"7\" attrcolnum=\"2\">\n");
			fprintf(fp_w,"#</TableName=\"TableName13\">\n");
			fprintf(fp_w,"#\n");
			fprintf(fp_w,"#交集搜索方法(Intersection search):");
			fprintf(fp_w,"#example_3:\n");
			fprintf(fp_w,"#<TableName=\"TableName13\">\n");
			fprintf(fp_w,"#<attr=\"Attrtype1\" colnum=\"10\" value=\"attrtypeName1\">\n");
			fprintf(fp_w,"#<attr=\"Attrtype2\" colnum=\"10\" value=\"attrtypeName2\">\n");
			fprintf(fp_w,"#<attr=\"Attrtype3\" colnum=\"10\" value=\"attrtypeName3\">\n");
			fprintf(fp_w,"#<relcla=\"TableName_6\" relcolnum=\"7\" attrcolnum=\"2\">\n");
			fprintf(fp_w,"#</TableName=\"TableName13\">\n");
			fprintf(fp_w,"#\n");
			fprintf(fp_w,"#<TableName=\"del_TableName13\">\n");
			fprintf(fp_w,"#<attr=\"Attrtype2\" colnum=\"10\" value=\"attrtypeName2\">\n");
			fprintf(fp_w,"#<attr=\"Attrtype3\" colnum=\"10\" value=\"attrtypeName3\">\n");
			fprintf(fp_w,"#<relcla=\"TableName_6\" relcolnum=\"7\" attrcolnum=\"2\">\n");
			fprintf(fp_w,"#</TableName=\"del_TableName13\">\n");
			fprintf(fp_w,"#\n");
			fprintf(fp_w,"#<TableName=\"del_del_TableName13\">\n");
			fprintf(fp_w,"#<attr=\"Attrtype3\" colnum=\"10\" value=\"attrtypeName3\">\n");
			fprintf(fp_w,"#<relcla=\"TableName_6\" relcolnum=\"7\" attrcolnum=\"2\">\n");
			fprintf(fp_w,"#</TableName=\"del_del_TableName13\">\n");
			fprintf(fp_w,"#\n");
			fprintf(fp_w,"######################filter_conf_example.txt#####\n\n");
			//
			free(path_and_name);
			fclose(fp_w);
			return 0;
		}
	}
	free(path_and_name);
	fclose(fp_w);
	return -1;
}
//
char* cutPartFromStartAndEnd(char* p_head,char* p_start,char* p_end,char** out_part)
{
    if(p_head == NULL || p_start == NULL || p_end == NULL)
    {
		printf("cutPartFromStartAndEnd return err,with args include NULL\n");
        return NULL;
    }
    char* head_p_start = strstr(p_head,p_start);
	if(head_p_start == NULL)
	{
		printf("cutPartFromStartAndEnd return err,head_p_start strstr find nothing!\n");
		return NULL;
	}
	char* tmp_head_p_start = head_p_start;
	char* head_p_start_skip_str = tmp_head_p_start + strlen(p_start);
    char* head_p_end = strstr(head_p_start_skip_str,p_end);
	if(head_p_end == NULL)
	{
		printf("cutPartFromStartAndEnd return err,head_p_end strstr find nothing!\n");
		return NULL;
	}
    if(head_p_start != NULL && head_p_end != NULL && head_p_end-head_p_start>0)
    {
        int i_start = strlen(p_start);
        int part_len = head_p_end - head_p_start - i_start;
        if(part_len<=0)
        {
			printf("cutPartFromStartAndEnd return err,part_len < 0 !\n");
            return NULL;
        }
        if(*out_part == NULL)
        {
            *out_part = (char*)malloc(part_len+1);//add \0
        }else if(*out_part != NULL && strlen(*out_part) < part_len +1 )
        {
            *out_part = (char*)realloc(*out_part,part_len+1);//add \0
        }
        memset(*out_part,0x00,part_len+1);
        memcpy(*out_part,head_p_start+i_start,part_len);
        (*out_part)[part_len+1]='\0';
        return *out_part;
    }
	printf("cutPartFromStartAndEnd return err,head_p_start==NULL  or head_p_end == NULL! or head_p_end < head_p_start \n");
    return NULL;
}
//
int  free_cutPartFromStartAndEnd(char* out_part)
{
    if(out_part != NULL)
    {
        free(out_part);
    }
    return 0;
}
//
int PrintFilterCimeHelp(char* fun_name)
{
		printf("====================================================		filter_cime[select_cime]		====================================================\n");
		printf("[Filter and Backup]:\n<\t%s\t--cime_path\t./xxx_cime_path\t--cime_file\txxx.CIME\t--config_path\t./xxx_config_path\t--config_file\txxx_conf_name.txt\t--backup_path\t./xxx_backup_path\t--del_path\t./xxx_del_path\t--new_path\t./xxx_new_path>\n",fun_name);
		printf("\nIf you want more infmation ,please use :\n<\t%s\t--config_example_path\t./xxx_config_example_path\t--table_example_path\t./xxx_table_example_path>\n",fun_name);
		printf("====================================================		filter_cime[select_cime]		====================================================\n");
		printf("\n");
		return 0;
}
//
void MyCopyFile(FILE *des_p1,FILE *src_p2)
{
	fseek(des_p1,0L,SEEK_END);
	char ch=fgetc(src_p2);
	while(ch!=EOF)
	{
		fprintf(des_p1,"%c",ch);	
		ch=fgetc(src_p2);
	}
}
//
char* now_time(char** s_time)
{
	time_t tmstmp1;
	tm* tmstruct1;
	//
	tmstmp1 = time(NULL);  
	tmstruct1 = localtime(&tmstmp1); 
	//
	int day,mon,year,hour,min,sec;
	//
	year = tmstruct1->tm_year + 1900;
	mon = tmstruct1->tm_mon + 1;
	day = tmstruct1->tm_mday;
	hour = tmstruct1->tm_hour;
	min = tmstruct1->tm_min;
	sec = tmstruct1->tm_sec;
	//
	if(*s_time == NULL)
	{
		*s_time = (char*)malloc(20);
	}else
	{
		*s_time = (char*)realloc(*s_time,20);
	}
	memset(*s_time,0x00,20);
	sprintf(*s_time, "%04d/%02d/%02d_%02d:%02d:%02d",year,mon,day,hour,min,sec);
	return *s_time;
}
//
int is_lock_need_run(char* cime_path,char* cime_file,char* lock_status)
{
	if(cime_path == NULL || cime_file == NULL || lock_status == NULL)
	{
		return -1;
	}
	char* line = (char*)malloc(LINEMAXSIZE);
	char* pathAndName = (char*)malloc(PATHANDNAMEMAXSIZE);
	memset(pathAndName,0x00,PATHANDNAMEMAXSIZE);
	sprintf(pathAndName,"%s/%s\0",cime_path,cime_file);
	FILE* fp_file = fopen(pathAndName,"r");
	if(fp_file != NULL)
	{
		while (!feof(fp_file))
		{
			memset(line,0x00,LINEMAXSIZE);
			fgets(line,LINEMAXSIZE,fp_file);
			//table fot database
			if( strncmp(line,"<!Library=\"",11) == 0 )
			{
				char* tmpName = NULL;
				tmpName = cutPartFromStartAndEnd(line,(char*)"lock_state=\"",(char*)"\"",&tmpName);
				if(strncmp(tmpName,lock_status,strlen(tmpName))==0)
				{
					//printf("run lock status table:[%s]==arg:[%s] OK.\n",tmpName,lock_status);
					break;
				}else{
					printf("You have no power to run it,pay attention about lock state.\n");
					exit(0);
				}
				free_cutPartFromStartAndEnd(tmpName);
			}
		}
	}
	free(line);
	free(pathAndName);
	return 0;
}

int print_attr_map(std::multimap<std::string,ATTR_MAP_STRUCT> &attr_multimap)
{
	for(std::multimap<std::string,ATTR_MAP_STRUCT>::iterator it_attr_multimap=attr_multimap.begin();it_attr_multimap!=attr_multimap.end();it_attr_multimap++)
	{
		std::cout<<"attr_multimap ["<<it_attr_multimap->first<<" <"<<it_attr_multimap->second.AttrColnum<<" "<<it_attr_multimap->second.AttrValue<<">] "<<std::endl;
	}
	return 0;
}
int print_rel_map(std::multimap<std::string,REL_MAP_STRUCT> &rel_multimap)
{
	for(std::multimap<std::string,REL_MAP_STRUCT>::iterator it_rel_multimap=rel_multimap.begin();it_rel_multimap!=rel_multimap.end();it_rel_multimap++)
	{
		std::cout<<"rel_multimap ["<<it_rel_multimap->first<<" <"<<it_rel_multimap->second.RelCol<<" "<<it_rel_multimap->second.RelValue<<">] "<<std::endl;
	}
	return 0;
}
int print_config_vector(std::vector<SCHEMA_TABLE>& config_vec)
{
	for (std::vector<SCHEMA_TABLE>::iterator it = config_vec.begin(); it != config_vec.end(); it++) 
	{
		std::cout << "vector SCHEMA_TABLE["<< it->TableName <<"] "<<std::endl;
		for(std::vector<SCHEMA_ATTR>::iterator it_SchemaAttr=it->SchemaAttr.begin();it_SchemaAttr!=it->SchemaAttr.end();it_SchemaAttr++)
		{
			std::cout<<"SCHEMA_ATTR:["<<it_SchemaAttr->AttrName <<" "<<it_SchemaAttr->AttrColnum<<" "<<it_SchemaAttr->AttrValue<<"] " <<std::endl;
		}
		for(std::vector<SCHEMA_RELCLA>::iterator it_SchemaRelCla=it->SchemaRelCla.begin();it_SchemaRelCla!=it->SchemaRelCla.end();it_SchemaRelCla++)
		{
			std::cout<<"SCHEMA_RELCLA:["<<it_SchemaRelCla->RelClaName <<" "<<it_SchemaRelCla->RelColnum<<" "<<it_SchemaRelCla->AttrColnum<<"] " <<std::endl;
		}
	}
	return 0;
}


