#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <dlfcn.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>


#include "data_type.h"
#include "alloc.h"
#include "memfunc.h"
#include "basefunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "channel.h"
#include "memdb.h"
#include "message.h"
#include "connector.h"
#include "ex_module.h"
#include "sys_func.h"
#include "tcm_constants.h"
#include "app_struct.h"
#include "pik_struct.h"
//#include "sm3.h"
//#include "sm4.h"

#include "tsm_typedef.h"
#include "tsm_structs.h"
#include "tspi.h"
#include "tspi_context.h"
#include "tspi_internal.h"

TSMD_CONTEXT this_context;
static char Buf[1024];

TSM_UUID TSM_UUID_SMK = {0,0,0,0,0,{0,0,0,0,0,1}}; 

static char main_config_file[DIGEST_SIZE*2]="./main_config.cfg";
static char sys_config_file[DIGEST_SIZE*2]="./sys_config.cfg";

int lib_read(int fd,int type,int subtype,void ** record);
int lib_write(int fd, int type,int subtype, void * record);

int lib_gettype(char * libname, int * typeno,int * subtypeno)
{
		char buffer[DIGEST_SIZE*3];
		char typename[DIGEST_SIZE];
		char subtypename[DIGEST_SIZE];
		int ret;
		int offset=0;

		Strncpy(buffer,libname,DIGEST_SIZE*3);
		ret=Getfiledfromstr(typename,buffer+offset,'-',DIGEST_SIZE*2);
		if(ret<=0)
			return -EINVAL;
		*typeno=memdb_get_typeno(typename);
		if(*typeno<=0)
			return -EINVAL;
		offset+=ret;
		offset++;
		ret=Getfiledfromstr(subtypename,buffer+offset,'.',DIGEST_SIZE*2);
		*subtypeno=memdb_get_subtypeno(*typeno,subtypename);
		if(*subtypeno<0)
			return -EINVAL;
		return 0;
};

int _TSMD_Init()
{
    int ret;
    int retval;
    int i,j;
    int argv_offset;	
    char namebuffer[DIGEST_SIZE*4];
    void * main_proc; // point to the main proc's subject struct
    char * sys_plugin;		
    char * app_plugin;		
    char * base_define;

    int readlen;
    int json_offset;
    void * memdb_template ;
    BYTE uuid[DIGEST_SIZE];
    char local_uuid[DIGEST_SIZE*2];

    FILE * fp;
    char audit_text[4096];
    char buffer[4096];
    void * root_node;
    void * temp_node;
    PROC_INIT plugin_proc; 
    int fd;	

    char * baseconfig[] =
    {
	"namelist.json",
	"dispatchnamelist.json",
	"typelist.json",
	"subtypelist.json",
	"memdb.json",
	"msghead.json",
	"msgrecord.json",
	"expandrecord.json",
	"base_msg.json",
	"dispatchrecord.json",
	"exmoduledefine.json",
	 NULL
    };

    sys_plugin=getenv("CUBE_SYS_PLUGIN");
    // process the command argument

    int ifmerge=0;

//	alloc_init(alloc_buffer);
	struct_deal_init();
	memdb_init();

    	base_define=getenv("CUBE_BASE_DEFINE");
	for(i=0;baseconfig[i]!=NULL;i++)
	{
		Strcpy(namebuffer,base_define);
		Strcat(namebuffer,"/");
		Strcat(namebuffer,baseconfig[i]);
		ret=read_json_file(namebuffer);
		if(ret<0)
			return ret;
		printf("read %d elem from file %s!\n",ret,namebuffer);
	}


	msgfunc_init();


    struct lib_para_struct * lib_para=NULL;
    fd=open(sys_config_file,O_RDONLY);
    if(fd>0)
    {

   	 ret=read_json_node(fd,&root_node);
  	 if(ret<0)
		return ret;	
    	 close(fd);
	

    	 ret=read_sys_cfg(&lib_para,root_node,NULL);
    	 if(ret<0)
		return ret;
    }	 		
    fd=open(main_config_file,O_RDONLY);
    if(fd<0)
	return -EINVAL;

    ret=read_json_node(fd,&root_node);
    if(ret<0)
	return ret;	
    close(fd);
	
    ret=read_main_cfg(lib_para,root_node);
    if(ret<0)
	return ret; 		

    ret=get_local_uuid(local_uuid);
    digest_to_uuid(local_uuid,Buf);
    Buf[64]=0;

    printf("this machine's local uuid is %s\n",Buf);
    return 0;	

}
// main proc 

UINT32 Tspi_Context_Create(TSM_HCONTEXT * phContext)
{

    int    i;	
    int    ret;
    static key_t sem_key;
    static int semid;

    static key_t shm_share_key;
    static int shm_share_id;
    static int shm_size;
    static void * shm_share_addr;
    struct context_init * share_init_context;

    static CHANNEL * shm_channel;
    char * pathname="/tmp";

    if(this_context.count==0)
    {
	ret=_TSMD_Init();
	if(ret<0)
		return -EINVAL;
    }	


    sem_key = ftok(pathname,0x01);

    if(sem_key==-1)

    {
        printf("ftok sem_key error");
        return -1;
    }

    printf("sem_key=%d\n",sem_key) ;
    semid=semget(sem_key,1,IPC_EXCL|0666);
    if(semid<0)
    {
	printf("open share semaphore failed!\n");
	return -EINVAL;
    }

    shm_share_key = ftok(pathname,0x02);
    if(shm_share_key==-1)

    {
        printf("ftok shm_share_key error");
        return -1;
    }
    printf("shm_share_key=%d\n",shm_share_key) ;

    shm_size=sizeof(* share_init_context);
    shm_share_id=shmget(shm_share_key,shm_size,0666);
    if(shm_share_id<0)
    {
        printf("open share memory failed!\n");
        return -EINVAL;
    }
    share_init_context=(struct context_init *)shmat(shm_share_id,NULL,0);


    // get the sem_key

    ret=semaphore_p(semid,2);
    	
    if(ret<=0)
	return -EINVAL;

    this_context.count=share_init_context->count;	

    		
    RAND_bytes(&share_init_context->handle,sizeof(share_init_context->handle));
  
    ret=semaphore_p(semid,1);
    if(ret<=0)
	return -EINVAL;
    this_context.handle=share_init_context->handle;

   static key_t shm_key;
   shm_key=ftok(pathname,this_context.count+0x02);
   if(shm_key == -1)
   {
         printf("ftok shm_share_key error");
         return -1;
   }
   printf("shm_key=%d\n",shm_key) ;
   this_context.shm_size=4096;

   for(i=0;i<10;i++)
   {	
 	  this_context.shmid=shmget(shm_key,this_context.shm_size,0666);
   	if(this_context.shmid>0)
	{
		break;
	}
	usleep(time_val.tv_usec);
   }	
 
   if(this_context.shmid<0)
   {
          printf("open context share memory failed!\n");
          return -EINVAL;
   }
   void * share_addr;
   share_addr=shmat(this_context.shmid,NULL,0);

   this_context.tsmd_API_channel = channel_register_fixmem("channel",CHANNEL_RDWR|CHANNEL_FIXMEM,NULL,
                this_context.shm_size/2,share_addr,share_addr+this_context.shm_size/2);

   share_init_context->count++;	

   for(i=0;i<10;i++)
   {	
 	ret=channel_read(this_context.tsmd_API_channel,Buf,5);
	if(ret==5)
	{
		if(Strcmp(Buf,"TSMD")!=0)
			return -EINVAL;
		ret=channel_write(this_context.tsmd_API_channel,"TSPI",5);
		*phContext = this_context.handle;
		return TSM_SUCCESS;
	}
	usleep(time_val.tv_usec);
   }	
	
   return TSM_E_CONNECTION_BROKEN;
}

UINT32 Tspi_Context_GetTcmObject(TSM_HCONTEXT hContext, TSM_HTCM * phTCM)
{
	int ret;

	RECORD(TSPI_IN,GETTCMOBJECT) tspi_in;
	RECORD(TSPI_OUT,GETTCMOBJECT) tspi_out;

	tspi_in.apino=SUBTYPE(TSPI_OUT,GETTCMOBJECT);
	tspi_in.paramSize=sizeof(tspi_in);
	tspi_in.hContext=hContext;

	void * tspi_in_template = memdb_get_template(TYPE_PAIR(TSPI_IN,GETTCMOBJECT));
	if(tspi_in_template == NULL)
	{
		return -EINVAL;
	}
	void * tspi_out_template = memdb_get_template(TYPE_PAIR(TSPI_OUT,GETTCMOBJECT));
	if(tspi_out_template == NULL)
	{
		return -EINVAL;
	}			
	
	ret=struct_2_blob(&tspi_in,Buf,tspi_in_template);
	if(ret<0)
		return -EINVAL;

	ret=channel_write(this_context.tsmd_API_channel,Buf,ret);
	if(ret<0)
		return TSM_E_CONNECTION_BROKEN;
	while(1)
	{
		ret=channel_read(this_context.tsmd_API_channel,Buf,512);
		if(ret<0)
			return TSM_E_CONNECTION_BROKEN;
		if(ret>0)
			break;
		usleep(time_val.tv_usec);
	}

	ret=blob_2_struct(Buf,&tspi_out,tspi_out_template);
	if(ret<0)
		return TSM_E_INVALID_ATTRIB_DATA;
	*phTCM=tspi_out.hTCM;	
	
	return TSM_SUCCESS;		
}

UINT32 Tspi_TCM_GetRandom(TSM_HTCM hTCM,UINT32 ulRandomDataLength, BYTE ** prgbRandomData)
{
	int ret;

	RECORD(TSPI_IN,GETRANDOM) tspi_in;
	RECORD(TSPI_OUT,GETRANDOM) tspi_out;

	tspi_in.apino=SUBTYPE(TSPI_IN,GETRANDOM);
	tspi_in.paramSize=sizeof(tspi_in);
	tspi_in.hTCM=hTCM;
	tspi_in.ulRandomDataLength=ulRandomDataLength;
	

	void * tspi_in_template = memdb_get_template(TYPE_PAIR(TSPI_IN,GETRANDOM));
	if(tspi_in_template == NULL)
	{
		return -EINVAL;
	}
	void * tspi_out_template = memdb_get_template(TYPE_PAIR(TSPI_OUT,GETRANDOM));
	if(tspi_out_template == NULL)
	{
		return -EINVAL;
	}			
	
	ret=struct_2_blob(&tspi_in,Buf,tspi_in_template);
	if(ret<0)
		return -EINVAL;

	ret=channel_write(this_context.tsmd_API_channel,Buf,ret);
	if(ret<0)
		return TSM_E_CONNECTION_BROKEN;
	while(1)
	{
		ret=channel_read(this_context.tsmd_API_channel,Buf,512);
		if(ret<0)
			return TSM_E_CONNECTION_BROKEN;
		if(ret>0)
			break;
		usleep(time_val.tv_usec);
	}

	ret=blob_2_struct(Buf,&tspi_out,tspi_out_template);
	if(ret<0)
		return TSM_E_INVALID_ATTRIB_DATA;
	*prgbRandomData=malloc(ulRandomDataLength);
	if(*prgbRandomData == NULL)
		return -ENOMEM;

	Memcpy(*prgbRandomData,tspi_out.rgbRandomData,ulRandomDataLength);

	Free(tspi_out.rgbRandomData);
	return TSM_SUCCESS;		
}

UINT32 Tspi_TCM_PcrRead(TSM_HTCM hTCM,UINT32 ulPcrIndex, UINT32 * pulPcrValueLength, BYTE ** prgbPcrValue)
{
	int ret;

	RECORD(TSPI_IN,PCRREAD) tspi_in;
	RECORD(TSPI_OUT,PCRREAD) tspi_out;

	tspi_in.apino=SUBTYPE(TSPI_IN,PCRREAD);
	tspi_in.paramSize=sizeof(tspi_in);
	tspi_in.hTCM=hTCM;
	tspi_in.ulPcrIndex=ulPcrIndex;
	
	void * tspi_in_template = memdb_get_template(TYPE_PAIR(TSPI_IN,PCRREAD));
	if(tspi_in_template == NULL)
	{
		return -EINVAL;
	}
	void * tspi_out_template = memdb_get_template(TYPE_PAIR(TSPI_OUT,PCRREAD));
	if(tspi_out_template == NULL)
	{
		return -EINVAL;
	}			
	
	ret=struct_2_blob(&tspi_in,Buf,tspi_in_template);
	if(ret<0)
		return -EINVAL;

	ret=channel_write(this_context.tsmd_API_channel,Buf,ret);
	if(ret<0)
		return TSM_E_CONNECTION_BROKEN;
	while(1)
	{
		ret=channel_read(this_context.tsmd_API_channel,Buf,512);
		if(ret<0)
                        //printf("read ret <0 ");
			return TSM_E_CONNECTION_BROKEN;
		if(ret>0)
                        //printf("read ret >0 ");
			break;
		usleep(time_val.tv_usec);
	}

	ret=blob_2_struct(Buf,&tspi_out,tspi_out_template);
	if(ret<0)
		return TSM_E_INVALID_ATTRIB_DATA;
	*pulPcrValueLength=tspi_out.ulPcrValueLength;
	*prgbPcrValue=malloc(*pulPcrValueLength);
	if(*prgbPcrValue == NULL)
		return -ENOMEM;

	Memcpy(*prgbPcrValue,tspi_out.rgbPcrValue,*pulPcrValueLength);

	Free(tspi_out.rgbPcrValue);
	return TSM_SUCCESS;		
}

UINT32 Tspi_TCM_PcrExtend(TSM_HTCM hTCM,UINT32 ulPcrIndex, UINT32 ulPcrDataLength,BYTE * pbPcrData,
	TSM_PCR_EVENT * pPcrEvent, UINT32 * pulPcrValueLength, BYTE ** prgbPcrValue)
{
	int ret;

	RECORD(TSPI_IN,PCREXTEND) tspi_in;
	RECORD(TSPI_OUT,PCREXTEND) tspi_out;

	if(pPcrEvent!=NULL)
		return -TSM_E_INVALID_OBJECT_TYPE;

	tspi_in.apino=SUBTYPE(TSPI_IN,PCREXTEND);
	tspi_in.paramSize=sizeof(tspi_in)-sizeof(BYTE *)+ulPcrDataLength;
	tspi_in.hTCM=hTCM;
	tspi_in.ulPcrIndex=ulPcrIndex;
	tspi_in.ulPcrDataLength=ulPcrDataLength;
	tspi_in.pbPcrData=pbPcrData;
	
	void * tspi_in_template = memdb_get_template(TYPE_PAIR(TSPI_IN,PCREXTEND));
	if(tspi_in_template == NULL)
	{
		return -EINVAL;
	}
	void * tspi_out_template = memdb_get_template(TYPE_PAIR(TSPI_OUT,PCREXTEND));
	if(tspi_out_template == NULL)
	{
		return -EINVAL;
	}			
	
	ret=struct_2_blob(&tspi_in,Buf,tspi_in_template);
	if(ret<0)
		return -EINVAL;

	ret=channel_write(this_context.tsmd_API_channel,Buf,ret);
	if(ret<0)
		return TSM_E_CONNECTION_BROKEN;
	while(1)
	{
		ret=channel_read(this_context.tsmd_API_channel,Buf,512);
		if(ret<0)
			return TSM_E_CONNECTION_BROKEN;
		if(ret>0)
			break;
		usleep(time_val.tv_usec);
	}

	ret=blob_2_struct(Buf,&tspi_out,tspi_out_template);
	if(ret<0)
		return TSM_E_INVALID_ATTRIB_DATA;
	*pulPcrValueLength=tspi_out.ulPcrValueLength;
	*prgbPcrValue=malloc(*pulPcrValueLength);
	if(*prgbPcrValue == NULL)
		return -ENOMEM;

	Memcpy(*prgbPcrValue,tspi_out.rgbPcrValue,*pulPcrValueLength);

	Free(tspi_out.rgbPcrValue);
	return TSM_SUCCESS;		
}

UINT32 Tspi_Context_CreateObject(TSM_HCONTEXT hContext, TSM_FLAG objectType, TSM_FLAG initFlags, TSM_HOBJECT * phObject)
{
        int ret;
	
        RECORD(TSPI_IN,CREATEOBJECT) tspi_in;
	RECORD(TSPI_OUT,CREATEOBJECT) tspi_out;

	tspi_in.apino=SUBTYPE(TSPI_OUT,CREATEOBJECT);
	tspi_in.paramSize=sizeof(tspi_in); 
	tspi_in.hContext=hContext;
        tspi_in.objectType=objectType;
	tspi_in.initFlags=initFlags;
	/* 
	tspi_in.objectType=objectType;
	tspi_in.initFlags=initFlags;
	NEED MORE
	*/

	void * tspi_in_template = memdb_get_template(TYPE_PAIR(TSPI_IN,CREATEOBJECT));
	if(tspi_in_template == NULL)
	{
		return -EINVAL;
	}
	void * tspi_out_template = memdb_get_template(TYPE_PAIR(TSPI_OUT,CREATEOBJECT));
	if(tspi_out_template == NULL)
	{
		return -EINVAL;
	}			
	
	ret=struct_2_blob(&tspi_in,Buf,tspi_in_template);
	if(ret<0)
		return -EINVAL;

	ret=channel_write(this_context.tsmd_API_channel,Buf,ret);
	if(ret<0)
		return TSM_E_CONNECTION_BROKEN;
	while(1)
	{
		ret=channel_read(this_context.tsmd_API_channel,Buf,512);
		if(ret<0)
			return TSM_E_CONNECTION_BROKEN;
		if(ret>0)
			break;
		usleep(time_val.tv_usec);
	}

	ret=blob_2_struct(Buf,&tspi_out,tspi_out_template);
	if(ret<0)
		return TSM_E_INVALID_ATTRIB_DATA;
	//*hPcrComposite=tspi_out.phObject;
        *phObject=tspi_out.phObject;
        //objectLength=
        //Memcpy(*hPcrComposite,*phObject,);	       
        return TSM_SUCCESS;
}


UINT32 Tspi_PcrComposite_SelectPcrIndex(TSM_HPCRS hPcrComposite, UINT32 ulPcrIndex, UINT32 direction)
{
        int ret;
	RECORD(TSPI_IN,SELECTPCRINDEX) tspi_in;
	RECORD(TSPI_OUT,SELECTPCRINDEX) tspi_out;

	tspi_in.apino=SUBTYPE(TSPI_IN,SELECTPCRINDEX);
	tspi_in.paramSize=sizeof(tspi_in);
	tspi_in.hPcrComposite=hPcrComposite;
        tspi_in.ulPcrIndex=ulPcrIndex;
	tspi_in.direction=direction;
	

	void * tspi_in_template = memdb_get_template(TYPE_PAIR(TSPI_IN,SELECTPCRINDEX));
	if(tspi_in_template == NULL)
	{
		return -EINVAL;
	}
	void * tspi_out_template = memdb_get_template(TYPE_PAIR(TSPI_OUT,SELECTPCRINDEX));
	if(tspi_out_template == NULL)
	{
		return -EINVAL;
	}			
	
	ret=struct_2_blob(&tspi_in,Buf,tspi_in_template);
	if(ret<0)
		return -EINVAL;

	ret=channel_write(this_context.tsmd_API_channel,Buf,ret);
	if(ret<0)
		return TSM_E_CONNECTION_BROKEN;
	while(1)
	{
		ret=channel_read(this_context.tsmd_API_channel,Buf,512);
		if(ret<0)
			return TSM_E_CONNECTION_BROKEN;
		if(ret>0)
			break;
		usleep(time_val.tv_usec);
	}

	ret=blob_2_struct(Buf,&tspi_out,tspi_out_template);
	if(ret<0)
		return TSM_E_INVALID_ATTRIB_DATA;
        
        return TSM_SUCCESS;
}


UINT32 Tspi_TCM_PcrReset(TSM_HTCM hTCM,TSM_HPCRS hPcrComposite)
{
	int ret;

	RECORD(TSPI_IN,PCRRESET) tspi_in;
	RECORD(TSPI_OUT,PCRRESET) tspi_out;

	tspi_in.apino=SUBTYPE(TSPI_IN,PCRRESET);
	tspi_in.hTCM=hTCM;
	tspi_in.hPcrComposite=hPcrComposite;
	
	void * tspi_in_template = memdb_get_template(TYPE_PAIR(TSPI_IN,PCRRESET));
	if(tspi_in_template == NULL)
	{
		return -EINVAL;
	}
	void * tspi_out_template = memdb_get_template(TYPE_PAIR(TSPI_OUT,PCRRESET));
	if(tspi_out_template == NULL)
	{
		return -EINVAL;
	}			
	
	ret=struct_2_blob(&tspi_in,Buf,tspi_in_template);
	if(ret<0)
		return -EINVAL;

	ret=channel_write(this_context.tsmd_API_channel,Buf,ret);
	if(ret<0)
		return TSM_E_CONNECTION_BROKEN;
	while(1)
	{
		ret=channel_read(this_context.tsmd_API_channel,Buf,512);
		if(ret<0)
			return TSM_E_CONNECTION_BROKEN;
		if(ret>0)
			break;
		usleep(time_val.tv_usec);
	}

	ret=blob_2_struct(Buf,&tspi_out,tspi_out_template);
	if(ret<0)
		return TSM_E_INVALID_ATTRIB_DATA;
	//*pulPcrValueLength=tspi_out.ulPcrValueLength;
	//*prgbPcrValue=malloc(*pulPcrValueLength);
	//if(*prgbPcrValue == NULL)
		//return -ENOMEM;

	//Memcpy(*prgbPcrValue,tspi_out.rgbPcrValue,*pulPcrValueLength);

	//Free(tspi_out.rgbPcrValue);
	return TSM_SUCCESS;		
}

UINT32 Tspi_GetPolicyObject(TSM_HOBJECT hObject, TSM_FLAG policyType, TSM_HPOLICY * phPolicy)
{
        int ret;

        RECORD(TSPI_IN,GETPOLICYOBJECT) tspi_in;
        RECORD(TSPI_OUT,GETPOLICYOBJECT) tspi_out;

        tspi_in.apino=SUBTYPE(TSPI_OUT,GETPOLICYOBJECT);
        tspi_in.paramSize=sizeof(tspi_in);
        tspi_in.hObject=hObject;
        tspi_in.policyType=policyType;

        void * tspi_in_template = memdb_get_template(TYPE_PAIR(TSPI_IN,GETPOLICYOBJECT));
        if(tspi_in_template == NULL)
        {
                return -EINVAL;
        }
        void * tspi_out_template = memdb_get_template(TYPE_PAIR(TSPI_OUT,GETPOLICYOBJECT));
        if(tspi_out_template == NULL)
        {
                return -EINVAL;
        }

        ret=struct_2_blob(&tspi_in,Buf,tspi_in_template);
        if(ret<0)
                return -EINVAL;

        ret=channel_write(this_context.tsmd_API_channel,Buf,ret);
        if(ret<0)
                return TSM_E_CONNECTION_BROKEN;
        while(1)
        {
                ret=channel_read(this_context.tsmd_API_channel,Buf,512);
                if(ret<0)
                        return TSM_E_CONNECTION_BROKEN;
                if(ret>0)
                        break;
                usleep(time_val.tv_usec);
        }

        ret=blob_2_struct(Buf,&tspi_out,tspi_out_template);
        if(ret<0)
                return TSM_E_INVALID_ATTRIB_DATA;
          
        *phPolicy=tspi_out.phPolicy;
        return TSM_SUCCESS;
}

UINT32 Tspi_Policy_SetSecret(TSM_HPOLICY hPolicy, TSM_FLAG secretMode, UINT32 ulSecretLength, BYTE * rgbSecret)
{
        int ret;

        RECORD(TSPI_IN,SETSECRET) tspi_in;
        RECORD(TSPI_OUT,SETSECRET) tspi_out;

        tspi_in.apino=SUBTYPE(TSPI_OUT,SETSECRET);
        tspi_in.paramSize=sizeof(tspi_in);
        tspi_in.hPolicy=hPolicy;
        tspi_in.secretMode=secretMode;
        tspi_in.ulSecretLength=ulSecretLength;
        tspi_in.rgbSecret=rgbSecret;

        void * tspi_in_template = memdb_get_template(TYPE_PAIR(TSPI_IN,SETSECRET));
        if(tspi_in_template == NULL)
        {
                return -EINVAL;
        }
        void * tspi_out_template = memdb_get_template(TYPE_PAIR(TSPI_OUT,SETSECRET));
        if(tspi_out_template == NULL)
        {
                return -EINVAL;
        }

        ret=struct_2_blob(&tspi_in,Buf,tspi_in_template);
        if(ret<0)
                return -EINVAL;

        ret=channel_write(this_context.tsmd_API_channel,Buf,ret);
        if(ret<0)
                return TSM_E_CONNECTION_BROKEN;
        while(1)
        {
                ret=channel_read(this_context.tsmd_API_channel,Buf,512);
                if(ret<0)
                        return TSM_E_CONNECTION_BROKEN;
                if(ret>0)
                        break;
                usleep(time_val.tv_usec);
        }

        ret=blob_2_struct(Buf,&tspi_out,tspi_out_template);
        if(ret<0)
                return TSM_E_INVALID_ATTRIB_DATA;

        return TSM_SUCCESS;
}

UINT32 Tspi_Context_LoadKeyByUUID(TSM_HCONTEXT hContext, TSM_FLAG persistentStorageType, TSM_UUID uuidData, TSM_HKEY * phKey)
{
	int ret;

	RECORD(TSPI_IN,LOADKEYBYUUID) tspi_in;
	RECORD(TSPI_OUT,LOADKEYBYUUID) tspi_out;

	tspi_in.apino=SUBTYPE(TSPI_IN,LOADKEYBYUUID);
        tspi_in.paramSize=sizeof(tspi_in);
        tspi_in.hContext=hContext;
	tspi_in.persistentStorageType=persistentStorageType;
	//tspi_in.uuidData=uuidData;

	void * tspi_in_template = memdb_get_template(TYPE_PAIR(TSPI_IN,LOADKEYBYUUID));
	if(tspi_in_template == NULL)
	{
		return -EINVAL;
	}
	void * tspi_out_template = memdb_get_template(TYPE_PAIR(TSPI_OUT,LOADKEYBYUUID));
	if(tspi_out_template == NULL)
	{
		return -EINVAL;
	}			
	
	ret=struct_2_blob(&tspi_in,Buf,tspi_in_template);
	if(ret<0)
		return -EINVAL;

	ret=channel_write(this_context.tsmd_API_channel,Buf,ret);
	if(ret<0)
		return TSM_E_CONNECTION_BROKEN;
	while(1)
	{
		ret=channel_read(this_context.tsmd_API_channel,Buf,512);
		if(ret<0)
			return TSM_E_CONNECTION_BROKEN;
		if(ret>0)
			break;
		usleep(time_val.tv_usec);
	}

	ret=blob_2_struct(Buf,&tspi_out,tspi_out_template);
	if(ret<0)
		return TSM_E_INVALID_ATTRIB_DATA;

        *phKey=tspi_out.phKey;

	return TSM_SUCCESS;		
}

UINT32 Tspi_Policy_AssignToObject(TSM_HPOLICY hPolicy, TSM_HOBJECT hObject)
{
	int ret;

	RECORD(TSPI_IN,ASSIGNTOOBJECT) tspi_in;
	RECORD(TSPI_OUT,ASSIGNTOOBJECT) tspi_out;

	tspi_in.apino=SUBTYPE(TSPI_IN,ASSIGNTOOBJECT);
        tspi_in.paramSize=sizeof(tspi_in);
        tspi_in.hPolicy=hPolicy;
	tspi_in.hObject=hObject;

	void * tspi_in_template = memdb_get_template(TYPE_PAIR(TSPI_IN,ASSIGNTOOBJECT));
	if(tspi_in_template == NULL)
	{
		return -EINVAL;
	}
	void * tspi_out_template = memdb_get_template(TYPE_PAIR(TSPI_OUT,ASSIGNTOOBJECT));
	if(tspi_out_template == NULL)
	{
		return -EINVAL;
	}			
	
	ret=struct_2_blob(&tspi_in,Buf,tspi_in_template);
	if(ret<0)
		return -EINVAL;

	ret=channel_write(this_context.tsmd_API_channel,Buf,ret);
	if(ret<0)
		return TSM_E_CONNECTION_BROKEN;
	while(1)
	{
		ret=channel_read(this_context.tsmd_API_channel,Buf,512);
		if(ret<0)
			return TSM_E_CONNECTION_BROKEN;
		if(ret>0)
			break;
		usleep(time_val.tv_usec);
	}

	ret=blob_2_struct(Buf,&tspi_out,tspi_out_template);
	if(ret<0)
		return TSM_E_INVALID_ATTRIB_DATA;

	return TSM_SUCCESS;		
}

UINT32 Tspi_Key_CreateKey(TSM_HKEY hKey, TSM_HKEY hWrappingKey, TSM_HPCRS hPcrComposite)
{
	int ret;

	RECORD(TSPI_IN,CREATEKEY) tspi_in;
	RECORD(TSPI_OUT,CREATEKEY) tspi_out;

	tspi_in.apino=SUBTYPE(TSPI_IN,CREATEKEY);
        tspi_in.paramSize=sizeof(tspi_in);
        tspi_in.hKey=hKey;
	tspi_in.hWrappingKey=hWrappingKey;
        tspi_in.hPcrComposite=hPcrComposite;

	void * tspi_in_template = memdb_get_template(TYPE_PAIR(TSPI_IN,CREATEKEY));
	if(tspi_in_template == NULL)
	{
		return -EINVAL;
	}
	void * tspi_out_template = memdb_get_template(TYPE_PAIR(TSPI_OUT,CREATEKEY));
	if(tspi_out_template == NULL)
	{
		return -EINVAL;
	}			
	
	ret=struct_2_blob(&tspi_in,Buf,tspi_in_template);
	if(ret<0)
		return -EINVAL;

	ret=channel_write(this_context.tsmd_API_channel,Buf,ret);
	if(ret<0)
		return TSM_E_CONNECTION_BROKEN;
	while(1)
	{
		ret=channel_read(this_context.tsmd_API_channel,Buf,512);
		if(ret<0)
			return TSM_E_CONNECTION_BROKEN;
		if(ret>0)
			break;
		usleep(time_val.tv_usec);
	}

	ret=blob_2_struct(Buf,&tspi_out,tspi_out_template);
	if(ret<0)
		return TSM_E_INVALID_ATTRIB_DATA;

	return TSM_SUCCESS;		
}
