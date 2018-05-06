import os,sys,glob,binascii

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
try:
	firmwareNames=sys.argv[1]
except:
	print('python build.py <string to describe compatible firm versions to users>')
	exit()
	
payload="payload/boot9strap.firm"
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

generateHeader(buff_old, "firm_old","")
generateHeader(buff_new, "firm_new","")
generateHeader(buff_pay, "payload", '"'+firmwareNames+'"')

#os.system("make")
