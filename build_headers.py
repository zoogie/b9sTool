import os,sys,glob,binascii,hashlib

def generateHeader(buff, arrayName, firmwareNames):
	f=open("source/"+arrayName+".h","w")
	if firmwareNames != "":
		f.write('const char firmwareNames[] = '+firmwareNames+';\n')
	f.write("const unsigned int "+arrayName+"_len = "+str(len(buff))+";\n")
	f.write("extern unsigned char "+arrayName+"[];\n")
	f.close()
	
	f=open("source/"+arrayName+".c","w")
	f.write("unsigned char "+arrayName+"[] = {\n")
	for i in buff:
		f.write("0x"+binascii.hexlify(i)+",")
	f.write("\n")
	f.write("};")
	f.close()
	
def hash(buf):
	obj=hashlib.sha1(buf)
	return obj.digest()
	
def buff2array(s):
	final=""
	for i in s:
		final+=",0x%02X" % ord(i)
	return final[1:]
	
try:
	firmwareNames=sys.argv[1]
except:
	print('python build.py <string to describe compatible firm versions to users>')
	exit()
	
payload="payload/fastboot3DS.firm"
firm_old3ds=glob.glob("firm_old3ds/*")
firm_new3ds=glob.glob("firm_new3ds/*")

fp=open(payload,"rb")
fo=open(firm_old3ds[0],"rb")
fn=open(firm_new3ds[0],"rb")

buff_pay=fp.read()
payload_len=len(buff_pay)

print(payload_len)

buff_old=fo.read(payload_len)
buff_new=fn.read(payload_len)

fp.close()
fo.close()
fn.close()

#print(hash(buff_old))
#print(hash(buff_new))
#print(hash(buff_pay))
with open("source/hash_stash.h","w") as f:
	f.write("const u8 SHA1OLD[20]={" + buff2array(hash(buff_old))+"}; //native firm 2.55-0 old3ds\n")
	f.write("const u8 SHA1NEW[20]={" + buff2array(hash(buff_new))+"}; //native firm 2.55-0 new3ds\n")
	f.write("const u8 SHA1B9S[20]={" + buff2array(hash(buff_pay))+"}; //payload")

generateHeader(buff_old, "firm_old","")
generateHeader(buff_new, "firm_new","")
generateHeader(buff_pay, "payload", '"'+firmwareNames+'"')

#os.system("make")
