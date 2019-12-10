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
	for i in bytearray(buff):
		f.write("0x%02X," % i)
	f.write("\n")
	f.write("};")
	f.close()
	
def hash(buf,ifnull,ifsha256=False):
	if(ifnull):
		return b"\x00"*20
	obj=hashlib.sha1(buf)
	if(ifsha256):
		obj=hashlib.sha256(buf)
	return obj.digest()
	
def buff2array(s):
	final=""
	for i in bytearray(s):
		final+=",0x%02X" % i
	return final[1:]
	
try:
	firmwareNames=sys.argv[1]
except:
	print('python build.py <string to describe compatible firm versions to users>')
	exit()
	
payload=glob.glob("payload/*")
firm_old3ds=glob.glob("firm_old3ds/*")
firm_new3ds=glob.glob("firm_new3ds/*")
stage2_payload=glob.glob("a9lh_files/stage2_payload/*")
stage3_payload=glob.glob("a9lh_files/stage3_payload/*")
a9lh_bin_installer=glob.glob("a9lh_files/bin_installer/*")
secret_sector=glob.glob("a9lh_files/secret_sector/*")

fp=open(payload[0],"rb")
fo=open(firm_old3ds[0],"rb")
fn=open(firm_new3ds[0],"rb")
f2=open(stage2_payload[0],"rb")
f3=open(stage3_payload[0],"rb")
fb=open(a9lh_bin_installer[0],"rb")
fs=open(secret_sector[0],"rb")

buff_pay=fp.read()
payload_len=len(buff_pay)
	
buff_old=fo.read(payload_len)
buff_new=fn.read(payload_len)
buff_s2p=f2.read(-1)
buff_s3p=f3.read(-1)
buff_abi=fb.read(-1)
buff_sec=fs.read(-1)

fp.close()
fo.close()
fn.close()
f2.close()
f3.close()
fb.close()
fs.close()

if(payload_len > len(buff_old) or payload_len > len(buff_new)):
	print("Payload is too large!")
	sys.exit(1)

print("payload length: %08X" % payload_len)
print("old firm  sha1: %s" % binascii.hexlify(hash(buff_old,False)))
print("new firm  sha1: %s" % binascii.hexlify(hash(buff_new,False)))
print("payload   sha1: %s" % binascii.hexlify(hash(buff_pay,False)))
print("(all firms hashed to payload length)")
print("stage2    sha1: %s" % binascii.hexlify(hash(buff_s2p,False)))
print("stage3    sha1: %s" % binascii.hexlify(hash(buff_s3p,False)))
print("installer sha1: %s" % binascii.hexlify(hash(buff_abi,False)))
print("ssector   sha1: %s" % binascii.hexlify(hash(buff_sec,False)))
print("\n")

null_old=False
null_new=False
if(b"FIRM" not in buff_old[:4]):
	null_old=True
	print("Non-firm detected, SHA1 will be nulled for old3ds")
	print("Old3ds native firm must be loaded from SD!\n")
if(b"FIRM" not in buff_new[:4]):
	null_new=True
	print("Non-firm detected, SHA1 will be nulled for new3ds")
	print("New3ds native firm must be loaded from SD!\n")
if(hash(buff_sec,False) != b'\xb9\xcf\xa8\x49\x16\xa9\x30\xd2\x72\x25\x01\x30\xd6\x7c\xeb\x82\x21\x41\x17\x7d'):
	print("Wrong file for secret sector detected")
	print("Secret sector must be loaded from SD if A9LH is present on n3ds")

with open("source/hash_stash.h","w") as f:
	f.write("const u8 SHA1OLD[20]={" + buff2array(hash(buff_old,null_old)) + "}; // native firm 2.56-0 old3ds\n")
	f.write("const u8 SHA1NEW[20]={" + buff2array(hash(buff_new,null_new)) + "}; // native firm 2.56-0 new3ds\n")
	f.write("const u8 SHA1B9S[20]={" + buff2array(hash(buff_pay,False)) + "}; // payload\n")
	f.write("// Below here is only needed if dealing with A9LH\n")
	f.write("const u8 SHA2B9S[32]={" + buff2array(hash(buff_pay,False,True)) + "}; // payload SHA256\n")
	f.write("const u8 SHA1S2P[20]={" + buff2array(hash(buff_s2p,False)) + "}; // A9LH stage2 payload\n")
	f.write("const u8 SHA1S3P[20]={" + buff2array(hash(buff_s3p,False)) + "}; // A9LH stage3 payload\n")
	f.write("const u8 SHA1ABI[20]={" + buff2array(hash(buff_abi,False)) + "}; // arm9loaderhax.bin b9s installer\n")
	f.write("const u8 SHA1SEC[20]={" + buff2array(b'\xb9\xcf\xa8\x49\x16\xa9\x30\xd2\x72\x25\x01\x30\xd6\x7c\xeb\x82\x21\x41\x17\x7d') + "}; // clean secret sector\n")

generateHeader(buff_old, "firm_old","","2.56-0_11.12_OLD.firm", False)
generateHeader(buff_new, "firm_new","", "2.56-0_11.12_NEW.firm", True)
generateHeader(buff_pay, "payload", '"'+firmwareNames+'"',"",False)
generateHeader(buff_s2p, "stage2_payload","","",False)
generateHeader(buff_s3p, "stage3_payload","","",False)
generateHeader(buff_abi, "a9lh_bin_installer","","",False)
generateHeader(buff_sec, "secret_sector","","",False)
