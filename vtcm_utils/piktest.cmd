# this cmdlist is for tcm_emulator test 

in: readpubek 

in: createsm2key -pubkey capub.key -prikey capri.key 
#创建一个CA公私钥对，非可信根操作                        
out:

in: loadcakey -pubkey capub.key
#载入CA公钥  

in: apcreate -it 02
#建立owner会话  记录会话句柄 
out: 1:$ownerHandle

in: apcreate -it 04
#建立smk会话，记录会话句柄
out: 1:$smkHandle
								     
in: makeidentity -ioh $ownerHandle -ish $smkHandle -if user_info.list -of request.req -kf pik.key
#生成鉴别密钥和密钥认证申请包,密钥文件导 

in: apterminate -ih $ownerHandle
out: 

in: loadkey -ih $smkHandle -kf pik.key  
out: 1:$keyHandle

in: apcreate -it 01 -iv $keyHandle 
#创建pik会话，返回pik会话句柄
out: 1:$authHandle

in: apterminate -ih $authHandle
out: 

in: apterminate -ih $smkHandle
out: 
