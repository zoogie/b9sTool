import os,sys,glob,binascii,hashlib

def generateHeader(buff, arrayName, firmwareNames, filename, isnew):
	f=open("source/"+arrayName+".h","w")
	if firmwareNames != "":
		f.write('const char firmwareNames[] = '+firmwareNames+';\n')
	if filename != "":
		model="OLD"
		if isnew:
			model="NEW"
		f.write('char FIRM_%s[] = "%s";\n' % (model,filename))
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
	
def hash(buf,ifnull):
	if(ifnull):
		return "\x00"*20
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
	
payload=glob.glob("payload/*")
firm_old3ds=glob.glob("firm_old3ds/*")
firm_new3ds=glob.glob("firm_new3ds/*")

fp=open(payload[0],"rb")
fo=open(firm_old3ds[0],"rb")
fn=open(firm_new3ds[0],"rb")

buff_pay=fp.read()
payload_len=len(buff_pay)

buff_old=fo.read(payload_len)
buff_new=fn.read(payload_len)

fp.close()
fo.close()
fn.close()

print("payload length: %08X" % payload_len)
print("old firm  sha1: %s" % binascii.hexlify(hash(buff_old,False)))
print("new firm  sha1: %s" % binascii.hexlify(hash(buff_new,False)))
print("payload   sha1: %s" % binascii.hexlify(hash(buff_pay,False)))
print("(all files hashed to payload length)\n")

null_old=False
null_new=False
if("FIRM" not in buff_old[:4]):
	null_old=True
	print("Non-firm detected, SHA1 will be nulled for old3ds")
	print("Old3ds native firm must be loaded from SD!\n")
if("FIRM" not in buff_new[:4]):
	null_new=True
	print("Non-firm detected, SHA1 will be nulled for new3ds")
	print("New3ds native firm must be loaded from SD!\n")

with open("source/hash_stash.h","w") as f:
	f.write("const u8 SHA1OLD[20]={" + buff2array(hash(buff_old,null_old))+"}; //native firm 2.55-0 old3ds\n")
	f.write("const u8 SHA1NEW[20]={" + buff2array(hash(buff_new,null_new))+"}; //native firm 2.55-0 new3ds\n")
	f.write("const u8 SHA1B9S[20]={" + buff2array(hash(buff_pay,False))+"}; //payload")

generateHeader(buff_old, "firm_old","","2.55-0_11.8_OLD.firm", False)
generateHeader(buff_new, "firm_new","", "2.55-0_11.8_NEW.firm", True)
generateHeader(buff_pay, "payload", '"'+firmwareNames+'"',"",False)