#Author: Lachlan Sinclair
#Date: 7/15/2019
#Description: generates 3 files with 10 random characters
#then genereates two random numbers and multiplies them

import random
import sys
import array

#seed the random generator
random.seed()

#create the three files
f1 = open("file1","w+")
f2 = open("file2","w+")
f3 = open("file3","w+")

#generate 3 arrays of random numbers in the ascii value range
#for lower case numbers
x1=[]
x2=[]
x3=[]
for x in range(0,10):
	x1.append(random.randint(97,122))
	x2.append(random.randint(97,122))
	x3.append(random.randint(97,122))

#generate random numbers from 1 to 42
num1 = random.randint(1,42)
num2 = random.randint(1,42)
#multiply the numbers
num3 = num1*num2

#create strings using the ascii arrays
s1= ''.join(map(chr,x1))
s2= ''.join(map(chr,x2))
s3= ''.join(map(chr,x3))

#append newlines to the strings
s1=s1+'\n'
s2=s2+'\n'
s3=s3+'\n'

#write the strings to the file
f1.write(s1)
f2.write(s2)
f3.write(s3)

#print the strings to the console
sys.stdout.write(s1)
sys.stdout.write(s2)
sys.stdout.write(s3)

#print the three numbers
print num1
print num2
print num3




