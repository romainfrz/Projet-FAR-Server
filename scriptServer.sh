gcc -o server server.c lib/src/colors.c lib/src/list.c lib/src/stringFunc.c lib/src/commandServer.c lib/src/sema.c lib/src/fileServer.c lib/src/admin.c lib/src/channel.c lib/src/channelList.c -lpthread;
./server $1;