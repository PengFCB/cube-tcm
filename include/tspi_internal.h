enum enum_cube_manage {
	TYPE(TSPI_GENERAL)=0x2110,
	TYPE(TSPI_IN)=0x2111,
	TYPE(TSPI_OUT)=0x2112
};

enum subtype_tspi_in {
	SUBTYPE(TSPI_IN,GETTCMOBJECT)=0x1,
	SUBTYPE(TSPI_IN,GETRANDOM),
	SUBTYPE(TSPI_IN,PCREXTEND),
	SUBTYPE(TSPI_IN,PCRREAD),
        SUBTYPE(TSPI_IN,CREATEOBJECT),
        SUBTYPE(TSPI_IN,SELECTPCRINDEX),
	SUBTYPE(TSPI_IN,PCRRESET),
        SUBTYPE(TSPI_IN,GETPOLICYOBJECT),
        SUBTYPE(TSPI_IN,SETSECRET),
        SUBTYPE(TSPI_IN,LOADKEYBYUUID),
        SUBTYPE(TSPI_IN,ASSIGNTOOBJECT),
        SUBTYPE(TSPI_IN,CREATEKEY)
};
enum subtype_tspi_out {
	SUBTYPE(TSPI_OUT,GETTCMOBJECT)=0x1,
	SUBTYPE(TSPI_OUT,GETRANDOM),
	SUBTYPE(TSPI_OUT,PCREXTEND),
	SUBTYPE(TSPI_OUT,PCRREAD),
        SUBTYPE(TSPI_OUT,CREATEOBJECT),
        SUBTYPE(TSPI_OUT,SELECTPCRINDEX),
	SUBTYPE(TSPI_OUT,PCRRESET),
        SUBTYPE(TSPI_OUT,GETPOLICYOBJECT),
        SUBTYPE(TSPI_OUT,SETSECRET),
        SUBTYPE(TSPI_OUT,LOADKEYBYUUID),
        SUBTYPE(TSPI_OUT,ASSIGNTOOBJECT),
        SUBTYPE(TSPI_OUT,CREATEKEY)
};

typedef struct tspi_in_GetTcmObject{
	int apino;
	int paramSize;
	int hContext;
}__attribute__((packed)) RECORD(TSPI_IN,GETTCMOBJECT);

typedef struct tspi_in_GetRandom{
	int apino;
	int paramSize;
	int hTCM;
	int ulRandomDataLength;
}__attribute__((packed)) RECORD(TSPI_IN,GETRANDOM);

typedef struct tspi_in_PcrExtend{
	int apino;
	int paramSize;
	int hTCM;
	int ulPcrIndex;
	int ulPcrDataLength;
	BYTE * pbPcrData;
}__attribute__((packed)) RECORD(TSPI_IN,PCREXTEND);

typedef struct tspi_in_PcrRead{
	int apino;
	int paramSize;
	int hTCM;
	int ulPcrIndex;
}__attribute__((packed)) RECORD(TSPI_IN,PCRREAD);

typedef struct tspi_in_CreateObject{
        int apino;
        int paramSize;
        int hContext;
        int objectType;
        int initFlags;
}__attribute__((packed)) RECORD(TSPI_IN,CREATEOBJECT);

typedef struct tspi_in_SelectPcrIndex{
        int apino;
        int paramSize;
        int hPcrComposite;
        int ulPcrIndex;
        int direction;
}__attribute__((packed)) RECORD(TSPI_IN,SELECTPCRINDEX);

typedef struct tspi_in_PcrReset{
	int apino;
	int hTCM;
	int hPcrComposite;
}__attribute__((packed)) RECORD(TSPI_IN,PCRRESET);

typedef struct tspi_in_GetPolicyObject{
	int apino;
        int paramSize;
        int hObject;
        int policyType;
}__attribute__((packed)) RECORD(TSPI_IN,GETPOLICYOBJECT);

typedef struct tspi_in_SetSecret{
	int apino;
        int paramSize;
        int hPolicy;
        int secretMode;
        int ulSecretLength;
        BYTE * rgbSecret;
}__attribute__((packed)) RECORD(TSPI_IN,SETSECRET);

typedef struct tspi_in_LoadKeyByUUID{
	int apino;
        int paramSize;
        int hContext;
        int persistentStorageType;
        int uuidData;
}__attribute__((packed)) RECORD(TSPI_IN,LOADKEYBYUUID);

typedef struct tspi_in_AssignToObject{
	int apino;
        int paramSize;
        int hPolicy;
        int hObject;
}__attribute__((packed)) RECORD(TSPI_IN,ASSIGNTOOBJECT);

typedef struct tspi_in_CreateKey{
	int apino;
        int paramSize;
        int hKey;
        int hWrappingKey;
        int hPcrComposite;
}__attribute__((packed)) RECORD(TSPI_IN,CREATEKEY);

typedef struct tspi_out_GetTcmObject{
	int returncode;
	int paramSize;
	int hTCM;
}__attribute__((packed)) RECORD(TSPI_OUT,GETTCMOBJECT);

typedef struct tspi_out_GetRandom{
	int returncode;
	int paramSize;
	int ulRandomDataLength;
	BYTE * rgbRandomData;
}__attribute__((packed)) RECORD(TSPI_OUT,GETRANDOM);

typedef struct tspi_out_PcrExtend{
	int returncode;
	int paramSize;
	int ulPcrValueLength;
	BYTE * rgbPcrValue;
}__attribute__((packed)) RECORD(TSPI_OUT,PCREXTEND);

typedef struct tspi_out_PcrRead{
	int returncode;
	int paramSize;
	int ulPcrValueLength;
	BYTE * rgbPcrValue;
}__attribute__((packed)) RECORD(TSPI_OUT,PCRREAD);

typedef struct tspi_out_CreateObject{
        int returncode;
	int paramSize;
        int phObject;
}__attribute__((packed)) RECORD(TSPI_OUT,CREATEOBJECT);

typedef struct tspi_out_SelectPcrIndex{
        int returncode;
	int paramSize;
}__attribute__((packed)) RECORD(TSPI_OUT,SELECTPCRINDEX);

typedef struct tspi_out_PcrReset{
	int returncode;
}__attribute__((packed)) RECORD(TSPI_OUT,PCRRESET);

typedef struct tspi_out_GetPolicyObject{
        int returncode;
	int paramSize;
        int phPolicy;
}__attribute__((packed)) RECORD(TSPI_OUT,GETPOLICYOBJECT);

typedef struct tspi_out_SetSecret{
        int returncode;
	int paramSize;
}__attribute__((packed)) RECORD(TSPI_OUT,SETSECRET);

typedef struct tspi_out_LoadKeyByUUID{
	int returncode;
        int paramSize;
        int phKey;
}__attribute__((packed)) RECORD(TSPI_OUT,LOADKEYBYUUID);

typedef struct tspi_out_AssignToObject{
	int returncode;
}__attribute__((packed)) RECORD(TSPI_OUT,ASSIGNTOOBJECT);

typedef struct tspi_out_CreateKey{
	int returncode;
}__attribute__((packed)) RECORD(TSPI_OUT,CREATEKEY);




