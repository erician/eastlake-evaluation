# /usr/python
def Prepare():
	old = open("data", "r")
	new = open("ponames.txt", "w")
	cnt = 0
	line = old.readline()
	while line:
		pairs = line.split(' ')
		new.write(pairs[0] + '\n')
		line = old.readline()
		cnt = cnt + 1
		if cnt >= 10000:
			break
	old.close()
	new.close()

if __name__ == "__main__":
	Prepare()
