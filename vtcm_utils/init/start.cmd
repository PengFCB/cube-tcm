# this cmdlist is for tcm_emulator init

in: createek -wf ekpub.key  
info: create endorsement pair 

in: readpubek
info: read endorsement key's public key

in:  apcreate -it 12
out: 1:$ownerHandle

in: takeownership -pwdo ooo -pwds sss

in: apterminate -ih $ownerHandle

in:  apcreate -it 04 -pwd sss
out: 1:$smkHandle

in: apterminate -ih $smkHandle
