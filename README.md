# shimrit-ex2

# EX2 - implementation of http server request 
<u>Author details</u> - name : asaf jospeh , id : 203819065
<u>Files</u> - client.c

<u>compile</u> with : gcc -Wall client.c -o client -g

<u>valgrind with</u> :  valgrind --track-origins=yes --tool=memcheck --leak-check=full -v

<u>Description</u> : in this exercise we have implemented an HTTP header request using sockets.

we parsed the command line command :

./client -p blabla -r 2 addr=jecrusalem tel=02-6655443 http://www.google.co.il

make the HTTP header and then send it to the server.

In case of command error the output will be : 

Usage: client -p  -r n <pr1=value1 pr2=value2 ... prN=valueN>  <URL>
